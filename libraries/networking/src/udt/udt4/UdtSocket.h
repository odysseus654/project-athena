//
//  UdtSocket.h
//  libraries/networking/src/udt/udt4
//
//  Created by Heather Anderson on 2020-05-28.
//  Copyright 2020 Vircadia contributors
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_udt4_UdtSocket_h
#define hifi_udt4_UdtSocket_h

#include "ByteSlice.h"
#include "Packet.h"
#include <QtCore/QIODevice>
#include <QtCore/QAtomicInteger>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtNetwork/QHostAddress>
#include <QtCore/QMutex>
#include <QtCore/QReadWriteLock>
#include <QtCore/QSharedPointer>
#include <QtNetwork/QUdpSocket>

namespace udt4 {

class HandshakePacket;
class UdtMultiplexer;
class UdtSocket;
typedef QSharedPointer<UdtMultiplexer> UdtMultiplexerPointer;
typedef QSharedPointer<UdtSocket> UdtSocketPointer;

/*
udtSocket encapsulates a UDT socket between a local and remote address pair, as
defined by the UDT specification.  udtSocket implements the net.Conn interface
so that it can be used anywhere that a stream-oriented network connection
(like TCP) would be used.
*/
class UdtSocket : public QIODevice {
    Q_OBJECT
public:
    enum class SocketState
    {
        Init,        // object is being constructed
        Rendezvous,  // attempting to create a rendezvous connection
        Connecting,  // attempting to connect to a server
        Connected,   // connection is established
        Closed,      // connection has been closed (by either end)
        Refused,     // connection rejected by remote host
        Corrupted,   // peer behaved in an improper manner
        Timeout,     // connection failed due to peer timeout
    };
    Q_ENUM(SocketState)

public:
    explicit UdtSocket(QObject* parent = nullptr);
    virtual ~UdtSocket();

public: // from QUdpSocket
    bool isInDatagramMode() const;
    bool hasPendingDatagrams() const;
    qint64 pendingDatagramSize() const;
    ByteSlice receiveDatagram(qint64 maxSize = -1);
    qint64 readDatagram(char* data, qint64 maxlen);
    qint64 writeDatagram(const char* data, qint64 len, const QDeadlineTimer& timeout = QDeadlineTimer(QDeadlineTimer::Forever));
    inline qint64 writeDatagram(const QByteArray& datagram);
    inline qint64 writeDatagram(const ByteSlice& datagram);

public:  // from QAbstractSocket
    void abort();
    virtual void connectToHost(const QString& hostName, quint16 port, QIODevice::OpenMode openMode = ReadWrite, bool datagramMode = true);
    virtual void connectToHost(const QHostAddress& address, quint16 port, QIODevice::OpenMode openMode = ReadWrite, bool datagramMode = true);
    virtual void rendezvousToHost(const QString& hostName, quint16 port, QIODevice::OpenMode openMode = ReadWrite, bool datagramMode = true);
    virtual void rendezvousToHost(const QHostAddress& address, quint16 port, QIODevice::OpenMode openMode = ReadWrite, bool datagramMode = true);
    virtual void disconnectFromHost();
    QAbstractSocket::SocketError error() const;
    bool flush();
    bool isValid() const;
    QHostAddress localAddress() const;
    quint16 localPort() const;
    inline const QHostAddress& peerAddress() const;
    inline quint16 peerPort() const;
    qint64 readBufferSize() const;
    virtual void setReadBufferSize(qint64 size);
    virtual void setSocketOption(QAbstractSocket::SocketOption option, const QVariant& value);
    virtual QVariant socketOption(QAbstractSocket::SocketOption option);
    inline SocketState state() const;
    virtual bool waitForConnected(int msecs = 30000);
    virtual bool waitForDisconnected(int msecs = 30000);

    virtual bool atEnd() const override;
    virtual qint64 bytesAvailable() const override;
    virtual qint64 bytesToWrite() const override;
    virtual bool canReadLine() const override;
    virtual void close() override;
    virtual bool isSequential() const override;
    virtual bool waitForBytesWritten(int msecs = 30000) override;
    virtual bool waitForReadyRead(int msecs = 30000) override;

signals: // from QAbstractSocket
    void connected();
    void disconnected();
    void errorOccurred(QAbstractSocket::SocketError socketError);
    void hostFound();
    void stateChanged(SocketState socketState);

protected: // from QAbstractSocket
    virtual qint64 readData(char* data, qint64 maxSize) override;
    virtual qint64 readLineData(char* data, qint64 maxlen) override;
    virtual qint64 writeData(const char* data, qint64 size) override;

public: // internal implementation
    bool readHandshake(const HandshakePacket& hsPacket, const QHostAddress& peerAddress, uint peerPort);
    bool checkValidHandshake(const HandshakePacket& hsPacket, const QHostAddress& peerAddress, uint peerPort);
    static UdtSocketPointer newSocket(UdtMultiplexerPointer multiplexer, quint32 socketID, bool isServer, bool isDatagram,
        const QHostAddress& peerAddress, uint peerPort);
    void readPacket(const Packet& udtPacket, const QHostAddress& peerAddress, uint peerPort);

private:
    void sendHandshake(quint32 synCookie, HandshakePacket::RequestType requestType);

private:
	// this data not changed after the socket is initialized and/or handshaked
    QMutex _connectWait;                 // released when connection is complete (or failed)
    QElapsedTimer _createTime;           // the time this socket was created
	quint32 _farSocketID;                // the remote's socketID
    quint32 _initialPacketSequence;      // initial packet sequence to start the connection with
    bool _isDatagram;                    // if true then we're sending and receiving datagrams, otherwise we're a streaming socket
	bool _isServer;                      // if true then we are behaving like a server, otherwise client (or rendezvous). Only useful during handshake
    QUdpSocket _offAxisUdpSocket;        // a "connected" udp socket we only use for detecting MTU path (as otherwise the system won't tell us about ICMP packets)
    UdtMultiplexerPointer _multiplexer;  // the multiplexer that handles this socket
    QHostAddress _remoteAddr;            // the remote address
    quint16 _remotePort;                 // the remote port number
    quint32 _socketID;                   // our socketID identifying this stream to the multiplexer

	SocketState _sockState;        // socket state - used mostly during handshakes
    QAtomicInteger<quint32> _mtu;  // the negotiated maximum packet size
	uint _maxFlowWinSize;          // receiver: maximum unacknowledged packet count
	ByteSlice _currPartialRead;    // stream connections: currently reading message (for partial reads). Owned by client caller (Read)
	QDeadlineTimer _readDeadline;  // if set, then calls to Read() will return "timeout" after this time
	bool _readDeadlinePassed;      // if set, then calls to Read() will return "timeout"
	QDeadlineTimer _writeDeadline; // if set, then calls to Write() will return "timeout" after this time
	bool _writeDeadlinePassed;     // if set, then calls to Write() will return "timeout"

	QReadWriteLock _rttProt;  // lock must be held before referencing rtt/rttVar
	uint _rtt;                // receiver: estimated roundtrip time. (in microseconds)
	uint _rttVar;             // receiver: roundtrip variance. (in microseconds)

	QReadWriteLock _receiveRateProt; // lock must be held before referencing deliveryRate/bandwidth
	uint _deliveryRate;              // delivery rate reported from peer (packets/sec)
	uint _bandwidth;                 // bandwidth reported from peer (packets/sec)
/*
	// channels
	messageIn     chan []byte          // inbound messages. Sender is goReceiveEvent->ingestData, Receiver is client caller (Read)
	messageOut    chan sendMessage     // outbound messages. Sender is client caller (Write), Receiver is goSendEvent. Closed when socket is closed
	recvEvent     chan recvPktEvent    // receiver: ingest the specified packet. Sender is readPacket, receiver is goReceiveEvent
	sendEvent     chan recvPktEvent    // sender: ingest the specified packet. Sender is readPacket, receiver is goSendEvent
	sendPacket    chan packet.Packet   // packets to send out on the wire (once goManageConnection is running)
	shutdownEvent chan shutdownMessage // channel signals the connection to be shutdown
	sockShutdown  chan struct{}        // closed when socket is shutdown
	sockClosed    chan struct{}        // closed when socket is closed

	// timers
	connTimeout <-chan time.Time // connecting: fires when connection attempt times out
	connRetry   <-chan time.Time // connecting: fires when connection attempt to be retried
	lingerTimer <-chan time.Time // after disconnection, fires once our linger timer runs out

	send *udtSocketSend // reference to sending side of this socket
	recv *udtSocketRecv // reference to receiving side of this socket
	cong *udtSocketCc   // reference to contestion control
*/
	// performance metrics
	//quint64 PktSent        // number of sent data packets, including retransmissions
	//quint64 PktRecv        // number of received packets
	//uint    PktSndLoss     // number of lost packets (sender side)
	//uint    PktRcvLoss     // number of lost packets (receiver side)
	//uint    PktRetrans     // number of retransmitted packets
	//uint    PktSentACK     // number of sent ACK packets
	//uint    PktRecvACK     // number of received ACK packets
	//uint    PktSentNAK     // number of sent NAK packets
	//uint    PktRecvNAK     // number of received NAK packets
	//double  MbpsSendRate   // sending rate in Mb/s
	//double  MbpsRecvRate   // receiving rate in Mb/s
	//time.Duration SndDuration // busy sending time (i.e., idle time exclusive)

	// instant measurements
	//time.Duration PktSndPeriod        // packet sending period
	//uint          PktFlowWindow       // flow window size, in number of packets
	//uint          PktCongestionWindow // congestion window size, in number of packets
	//uint          PktFlightSize       // number of packets on flight
	//time.Duration MsRTT               // RTT
	//double        MbpsBandwidth       // estimated bandwidth, in Mb/s
	//uint          ByteAvailSndBuf     // available UDT sender buffer size
	//uint          ByteAvailRcvBuf     // available UDT receiver buffer size

private:
    Q_DISABLE_COPY(UdtSocket)
};

} // namespace udt4
#include "UdtSocket.inl"
#endif /* hifi_udt4_UdtSocket_h */
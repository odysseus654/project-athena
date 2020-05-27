//
//  Multiplexer.cpp
//  libraries/networking/src/udt/udt4
//
//  Created by Heather Anderson on 2020-05-25.
//  Copyright 2020 Vircadia contributors
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "multiplexer.h"

#include "ByteSlice.h"
#include "../../NetworkLogging.h"
#include <QtCore/QException>
#include <QtCore/QLoggingCategory>
#include <QtNetwork/QNetworkDatagram>
#include <QtCore/QRandomGenerator>
#include "UdtServer.h"

#ifdef WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#include <netinet/in.h>
#endif

using namespace udt4;

QMutex UdtMultiplexer::gl_multiplexerMapProtect;
UdtMultiplexer::TMultiplexerMap UdtMultiplexer::gl_multiplexerMap;

// getInstance gets or creates a multiplexer for the given local address.
UdtMultiplexerPointer UdtMultiplexer::getInstance(quint16 port,
                                                           const QHostAddress& localAddress /* = QHostAddress::Any */,
                                                           QAbstractSocket::SocketError* serverError /* = nullptr */,
                                                           QString* errorString /* = nullptr */) {
    TLocalPortPair localPort(port, localAddress);
    UdtMultiplexerPointer multiplexer;

    gl_multiplexerMapProtect.lock();
    TMultiplexerMap::const_iterator lookup = gl_multiplexerMap.find(localPort);
    if (lookup != gl_multiplexerMap.end()) {
        multiplexer = lookup.value().lock();
    }
    gl_multiplexerMapProtect.unlock();
    if (!multiplexer.isNull() && multiplexer->isLive()) {
        return multiplexer;
    }

	// No multiplexer, need to create connection
    multiplexer = UdtMultiplexerPointer::create();
    if (!multiplexer->create(port, localAddress)) {
        // if we hit an exception trying to construct a multiplexer, check to see if we haven't hit a race condition
        gl_multiplexerMapProtect.lock();
        TMultiplexerMap::const_iterator lookup = gl_multiplexerMap.find(localPort);
        if (lookup != gl_multiplexerMap.end()) {
            multiplexer = lookup.value().lock();
        }
        gl_multiplexerMapProtect.unlock();
        if (!multiplexer.isNull() && multiplexer->isLive()) {
            return multiplexer;
        }

        // we haven't, throw our exception
        if (serverError != nullptr) {
            *serverError = multiplexer->serverError();
        }
        if (errorString != nullptr) {
            *errorString = multiplexer->errorString();
        }
        return UdtMultiplexerPointer();
    }

    gl_multiplexerMapProtect.lock();
    gl_multiplexerMap.insert(localPort, multiplexer);
    gl_multiplexerMapProtect.unlock();
    return multiplexer;
}

UdtMultiplexer::UdtMultiplexer(quint16 port, const QHostAddress& localAddress) {
    QRandomGenerator random;
    _nextSid = random.generate();

    connect(&_udpSocket, SIGNAL(readyRead), this, SLOT(onPacketReadReady), Qt::DirectConnection);
    connect(this, SIGNAL(sendPacket), this, SLOT(onPacketWriteReady));

    create(port, localAddress);
}

UdtMultiplexer::~UdtMultiplexer() {
    // deregister this multiplexer
    TLocalPortPair localPort(_serverPort, _serverAddress);
    gl_multiplexerMapProtect.lock();
    TMultiplexerMap::iterator lookup = gl_multiplexerMap.find(localPort);
    if (lookup != gl_multiplexerMap.end()) {
        gl_multiplexerMap.erase(lookup);
    }
    gl_multiplexerMapProtect.unlock();

    // tear everything down
    _udpSocket.close();

    _readThread.quit();
    _writeThread.quit();
}

bool UdtMultiplexer::create(quint16 port, const QHostAddress& localAddress) {
    _readThread.start();
    _writeThread.start();
    _udpSocket.moveToThread(&_readThread);
    moveToThread(&_writeThread);

    if (!_udpSocket.bind(localAddress, port)) {
        return false;
    }
    _serverAddress = localAddress;
    _serverPort = port;

    // try to avoid fragmentation (and hopefully be notified if we exceed path MTU)
#if defined(Q_OS_LINUX)
    auto sd = _udpSocket.socketDescriptor();
    if (localAddress.protocol() == QAbstractSocket::IPv4Protocol) {  // DF bit is only relevant for IPv4
        int val = IP_PMTUDISC_DONT;
        setsockopt(sd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));
    }
    //int on = 1;
    //setsockopt(sd, SOL_IP, IP_RECVERR, &on, sizeof(on)); // Let us know of any network errors
#elif defined(Q_OS_WIN)
    if (localAddress.protocol() == QAbstractSocket::IPv4Protocol) {  // DF bit is only relevant for IPv4
        auto sd = _udpSocket.socketDescriptor();
        int val = 0;  // false
        if (setsockopt(sd, IPPROTO_IP, IP_DONTFRAGMENT, reinterpret_cast<const char*>(&val), sizeof(val))) {
            auto wsaErr = WSAGetLastError();
            qCWarning(networking) << "Socket::bind Cannot setsockopt IP_DONTFRAGMENT" << wsaErr;
        }
    }
#endif
    return true;
}

QHostAddress UdtMultiplexer::serverAddress() const {
    return _serverAddress;
}

QAbstractSocket::SocketError UdtMultiplexer::serverError() const {
    return _udpSocket.error();
}

quint16 UdtMultiplexer::serverPort() const {
    return _serverPort;
}

QString UdtMultiplexer::errorString() const {
    return _udpSocket.errorString();
}

bool UdtMultiplexer::startListenUdt(UdtServer* server) {
    _serverSocketProtect.lock();
    if (_serverSocket != nullptr) {
        _serverSocketProtect.unlock();
        return false;
	}
	_serverSocket = server;
    _serverSocketProtect.unlock();
	return true;
}

bool UdtMultiplexer::stopListenUdt(UdtServer* server) {
    _serverSocketProtect.lock();
    if (_serverSocket != server) {
        _serverSocketProtect.unlock();
		return false;
	}
    _serverSocket = nullptr;
	_serverSocketProtect.unlock();
	return true;
}
/*
// Adapted from https://github.com/hlandau/degoutils/blob/master/net/mtu.go
const absMaxDatagramSize = 2147483646 // 2**31-2
func discoverMTU(ourIP net.IP) (uint, error) {

	ifaces, err := net.Interfaces()
	if err != nil {
		return 65535, err
	}

	var filtered []net.Interface
	for _, iface := range ifaces {
		addrs, err := iface.Addrs()
		if err != nil {
			log.Printf("cannot retrieve iface addresses for %s: %s", iface.Name, err.Error())
			continue
		}
		for _, a := range addrs {
			var ipnet *net.IPNet
			switch v := a.(type) {
			case *net.IPAddr:
				ipnet = &net.IPNet{IP: v.IP, Mask: v.IP.DefaultMask()}
			case *net.IPNet:
				ipnet = v
			}
			if ipnet == nil {
				log.Printf("cannot retrieve IPNet from address %s on interface %s", a.String(), iface.Name)
				continue
			}
			if ipnet.Contains(ourIP) {
				filtered = append(filtered, iface)
			}
		}
	}
	if len(filtered) == 0 {
		log.Printf("cannot identify interface(s) associated with %s, doing blind search", ourIP.String())
		filtered = ifaces
	}

	var mtu int = 65535
	for _, iface := range filtered {
		if iface.Flags&(net.FlagUp|net.FlagLoopback) == net.FlagUp && iface.MTU > mtu {
			mtu = iface.MTU
		}
	}
	if mtu > absMaxDatagramSize {
		mtu = absMaxDatagramSize
	}
	return uint(mtu), nil
}
*/
UdtSocketPointer UdtMultiplexer::newSocket(const QHostAddress& peerAddress, quint16 peerPort, bool isServer, bool isDatagram) {
    quint32 sid = _nextSid.fetchAndSubRelaxed(1);

	UdtSocketPointer socket = UdtSocket::newSocket(this, sid, isServer, isDatagram, peerAddress, peerPort);

    _connectedSocketsProtect.lock();
    _connectedSockets.insert(sid, socket.get());
    _connectedSocketsProtect.unlock();

	return socket;
}

bool UdtMultiplexer::closeSocket(quint32 sockID) {
    _connectedSocketsProtect.lock();
    TSocketMap::iterator lookup = _connectedSockets.find(sockID);
    if (lookup != _connectedSockets.end()) {
        _connectedSockets.erase(lookup);
    }
    _connectedSocketsProtect.unlock();
    return lookup != _connectedSockets.end();
}

bool UdtMultiplexer::isLive() const {
    if (!_udpSocket.isOpen()) {
        return false;
	}
    bool isEmpty = true;
    
	_serverSocketProtect.lock();
    if (_serverSocket != nullptr) {
		_serverSocketProtect.unlock();
		return true;
	}
    _serverSocketProtect.unlock();
    
    _rendezvousSocketsProtect.lock();
    isEmpty = _rendezvousSockets.empty();
    _rendezvousSocketsProtect.unlock();
	if(!isEmpty) {
		return true;
	}

    _connectedSocketsProtect.lock();
    isEmpty = _connectedSockets.empty();
    _connectedSocketsProtect.unlock();

	return !isEmpty;
}

bool UdtMultiplexer::startRendezvous(UdtSocket* udtSocket) {
    _rendezvousSocketsProtect.lock();
    int indexOf = _rendezvousSockets.indexOf(udtSocket);
    if (indexOf == -1) {
        _rendezvousSockets.append(udtSocket);
    }
    _rendezvousSocketsProtect.unlock();
    return indexOf == -1;
}

bool UdtMultiplexer::endRendezvous(UdtSocket* udtSocket) {
    _rendezvousSocketsProtect.lock();
    int indexOf = _rendezvousSockets.indexOf(udtSocket);
    if (indexOf != -1) {
        _rendezvousSockets.removeAt(indexOf);
    }
    _rendezvousSocketsProtect.unlock();
    return indexOf != -1;
}

void UdtMultiplexer::onPacketReadReady() {  // executes from goRead thread
    ByteSlice packetData;
    qint64 packetSize = _udpSocket.pendingDatagramSize();
    void* packetBuffer = packetData.create(packetSize);
    QHostAddress peerAddress;
    quint16 peerPort;
    if (-1 == _udpSocket.readDatagram(reinterpret_cast<char*>(packetBuffer), packetSize, &peerAddress, &peerPort)) {
        qCWarning(networking) << "Received an invalid UDP packet (error likely logged nearby)";
        return;
	}

    Packet udtPacket(packetData);

	// attempt to route the packet
	if(udtPacket._socketID == 0) {
        if (udtPacket._type != PacketType::Handshake) {
            qCWarning(networking) << "Received non-handshake packet with destination socket = 0";
            return;
        }

		bool isHandled = false;
        _rendezvousSocketsProtect.lock();
        for (TSocketList::const_iterator trans = _rendezvousSockets.begin(); !isHandled && trans != _rendezvousSockets.end();
             ++trans) {
            if ((*trans)->readHandshake(this, udtPacket, peerAddress, peerPort)) {
                isHandled = true;
                break;
			}
		}
        _rendezvousSocketsProtect.unlock();
		if(isHandled) {
            return;
		}
        
		_serverSocketProtect.lock();
		if(_serverSocket != nullptr) {
			_serverSocket->readHandshake(this, udtPacket, peerAddress, peerPort);
		}
		_serverSocketProtect.unlock();
	}

    UdtSocket* destSocket = nullptr;
    _connectedSocketsProtect.lock();
    TSocketMap::const_iterator lookup = _connectedSockets.find(udtPacket._socketID);
    if (lookup != _connectedSockets.end()) {
        destSocket = lookup.value();
    }
    _connectedSocketsProtect.unlock();
    if (destSocket) {
        destSocket->readPacket(this, udtPacket, peerAddress, peerPort);
	}
}

/*
write runs in a goroutine and writes packets to conn using a buffer from the
writeBufferPool, or a new buffer.
*/
void UdtMultiplexer::onPacketWriteReady(Packet packet, QHostAddress destAddr, quint32 destPort) {
    ByteSlice networkPacket = packet.toNetworkPacket();
    _udpSocket.writeDatagram(reinterpret_cast<const char*>(&networkPacket[0]), networkPacket.length(), destAddr, destPort);
}

void UdtMultiplexer::sendPacket(const QHostAddress& destAddr,
                                quint32 destPort,
                                quint32 destSockID,
                                quint32 timestamp,
                                Packet packet) {
    packet._socketID = destSockID;
    packet._timestamp = timestamp;
	if(destSockID == 0 && packet._type != PacketType::Handshake) {
        qCCritical(networking) << "Attempt to send non-handshake packet with destination socket = 0";
        return;
	}
    emit sendPacket(packet, destAddr, destPort, QPrivateSignal());
}

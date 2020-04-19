//
//  MetaMesh.h
//  assignment-client/src/metamesh
//
//  Created by Heather Anderson on 4/19/2020.
//  Copyright 2020 Vircadia.
//
//  The metamesh node launches the external mesh node and establishes a connection to it
//  for logging and management
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MetaMesh_h
#define hifi_MetaMesh_h

#include <ThreadedAssignment.h>

/// Handles assignments of type MessagesMixer - management of external mesh node
class MetaMeshProxy : public ThreadedAssignment {
    Q_OBJECT
public:
    MetaMeshProxy(ReceivedMessage& message);

public slots:
    void run() override;
    void nodeKilled(SharedNodePointer killedNode);
//    void sendStatsPacket() override;

private slots:
    void childExited(int exitCode, QProcess::ExitStatus exitStatus);
    void childStarted();
    void childStdoutReady();
    void childStderrReady();
//    void handleMessages(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
//    void handleMessagesSubscribe(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
//    void handleMessagesUnsubscribe(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);

private:
    void createMeshNode();

private:
//    QHash<QString, QSet<QUuid>> _channelSubscribers;
    QProcess* _meshNode = nullptr;
};

#endif  // hifi_MetaMesh_h

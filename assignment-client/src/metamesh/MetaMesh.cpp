//
//  MetaMesh.cpp
//  assignment-client/src/metamesh
//
//  Created by Heather Anderson on 4/19/2020.
//  Copyright 2020 Vircadia.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MetaMesh.h"

const QString METAMESH_LOGGING_NAME = "metamesh-node";

MetaMeshProxy::MetaMeshProxy(ReceivedMessage& message) : ThreadedAssignment(message) {
    connect(DependencyManager::get<NodeList>().data(), &NodeList::nodeKilled, this, &MetaMeshProxy::nodeKilled);
    //auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    //packetReceiver.registerListener(PacketType::MessagesData, this, "handleMessages");
    //packetReceiver.registerListener(PacketType::MessagesSubscribe, this, "handleMessagesSubscribe");
    //packetReceiver.registerListener(PacketType::MessagesUnsubscribe, this, "handleMessagesUnsubscribe");
}

void MetaMeshProxy::nodeKilled(SharedNodePointer killedNode) {
    //for (auto& channel : _channelSubscribers) {
    //    channel.remove(killedNode->getUUID());
    //}
}

void MetaMeshProxy::createMeshNode() {
    if (_meshNode) {
        // huh? this assumes run() is being executed multiple times.  Let's not leak memory in any case, can we ASSERT this?
        delete _meshNode;
    }
    _meshNode = new QProcess(this);
    connect(_meshNode, &QProcess::readyReadStandardOutput, this, &MetaMeshProxy::childStdoutReady);
    connect(_meshNode, &QProcess::started, this, &MetaMeshProxy::childStarted);
    connect(_meshNode, &QProcess::readyReadStandardOutput, this, &MetaMeshProxy::childStdoutReady);
    connect(_meshNode, &QProcess::readyReadStandardError, this, &MetaMeshProxy::childStderrReady);
}

void MetaMeshProxy::run() {
    ThreadedAssignment::commonInit(METAMESH_LOGGING_NAME, NodeType::MetaMesh);
    createMeshNode();

    //    auto nodeList = DependencyManager::get<NodeList>();
//    nodeList->addSetOfNodeTypesToNodeInterestSet({ NodeType::Agent, NodeType::EntityScriptServer });
}

void MetaMeshProxy::childStarted() {
}

void MetaMeshProxy::childExited(int exitCode, QProcess::ExitStatus exitStatus) {
}

void MetaMeshProxy::childStdoutReady() {
}

void MetaMeshProxy::childStderrReady() {
}

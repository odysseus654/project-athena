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

#include <PathUtils.h>

Q_LOGGING_CATEGORY(metamesh_node, "vircadia.metamesh-node")
const static QString METAMESH_LOGGING_NAME = "metamesh-node";
const static QStringList METAMESH_ARGUMENTS = QStringList() << QString("node");

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
    connect(_meshNode, &QProcess::errorOccurred, this, &MetaMeshProxy::childErrorOccurred);
    connect(_meshNode, &QProcess::readyReadStandardOutput, this, &MetaMeshProxy::childStdoutReady);
    connect(_meshNode, &QProcess::readyReadStandardError, this, &MetaMeshProxy::childStderrReady);
}

void MetaMeshProxy::run() {
    ThreadedAssignment::commonInit(METAMESH_LOGGING_NAME, NodeType::MetaMesh);

    createMeshNode();

    auto meshDir = QDir(PathUtils::getAppDataFilePath("mesh/"));
    if (!meshDir.mkpath(".")) {
        qCCritical(metamesh_node) << "Unable to create file directory for mesh files. Stopping assignment.";
        setFinished(true);
        return;
    }
    _meshNode->setWorkingDirectory(meshDir.absolutePath());

    auto meshApp = QFileInfo(QCoreApplication::applicationDirPath() + "/metamesh.exe");
    if (!meshApp.exists() || !meshApp.isExecutable()) {
        qCCritical(metamesh_node) << "Cannot locate metamesh.exe as an execution target. Stopping assignment.";
        setFinished(true);
        return;
    }

    _meshNode->start(meshApp.absoluteFilePath(), METAMESH_ARGUMENTS);
    // returning waiting for either childStarted or childErrorOccurred to fire
}

void MetaMeshProxy::childErrorOccurred(QProcess::ProcessError error) {
    auto metaEnum = QMetaEnum::fromType<QProcess::ProcessError>();
    qCCritical(metamesh_node) << "Failed to launch metamesh.exe (got error " << metaEnum.valueToKey(error) << "). Stopping assignment.";
    setFinished(true);
}

void MetaMeshProxy::childStarted() {
}

void MetaMeshProxy::childExited(int exitCode, QProcess::ExitStatus exitStatus) {
}

void MetaMeshProxy::childStdoutReady() {
}

void MetaMeshProxy::childStderrReady() {
}

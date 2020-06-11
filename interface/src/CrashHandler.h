//
//  CrashHandler.h
//  interface/src
//
//  Created by Clement Brisset on 01/19/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CrashHandler_h
#define hifi_CrashHandler_h

class QMessageLogContext;
class QString;
enum QtMsgType;

bool startCrashHandler(const QString& appPath);
void setCrashAnnotation(const QString& name, const QString& value);
void logMessageForCrashes(QtMsgType type, const QMessageLogContext& context, const QString& message);

#endif // hifi_CrashHandler_h

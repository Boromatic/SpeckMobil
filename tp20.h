/*
SpeckMobil - Experimental TP2.0-KWP2000 Software

Copyright (C) 2014 Matthias Amberg

Derived from VAG Blocks, Copyright 2013 Jared Wiltshire

SpeckMobil is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef TP20_H
#define TP20_H

#include <QObject>
#include <QTimer>
#include "elm327.h"
#include "canframe.h"
#include "util.h"

#define T_wait 100  //wait for receiver ready in ms
#define MNTB 5      //Maximum acceptances of receiver not ready
#define T_CTa 600  //Connection Test Timeout in ms
#define MNCT 5      //Maximum Repeats of connection test

typedef struct {
    quint8 dest;
    quint8 opcode;
    quint8 rxID;
    quint8 rxV;
    quint8 rxPre;
    quint8 txID;
    quint8 txV;
    quint8 txPre;
    quint8 app;
} chanSetup;

typedef struct {
    quint8 opcode;
    quint8 bs;
    quint8 T1;
    quint8 T2;
    quint8 T3;
    quint8 T4;
} chanParam;

typedef struct {
    quint8 opcode;
    quint8 bs;
} chanParamShort;

typedef struct {
    quint8 opcode;
    quint8 seq;
} dataTrans;

typedef struct {
    quint8 opcode;
    quint8 seq;
    quint16 len;
} dataTransFirst;

class tp20 : public QObject
{
    Q_OBJECT
public:
    tp20(elm327* elm, QObject *parent = 0);
    int getChannelDest();
    bool getElmInitialised();
    void setKeepAliveInterval(int time);
public slots:
    void initialiseElm(bool open);
    void portClosed();
    void openChannel(int dest, int timeout);
    void closeChannel();
    void sendData(const QByteArray &data, int requestedTimeout);
    void stopKeepAliveTimer();
    void startKeepAliveTimer();
    void waitForData();
private slots:
    void sendKeepAlive();
    void passiveRxReady();
signals:
    void log(const QString &txt, int logLevel = stdLog);
    void elmInitDone(bool ok);
    void channelOpened(bool ok);
    void response(QByteArray* data);
    void passiveRxReadySignal();
private:
    elm327* elm;
    QList<canFrame*>* lastResponse;
    QList<canFrame*>* savedResponse;
    int channelDest;
    quint16 txID;
    quint16 rxID;
    quint8 bs;
    quint8 t1;
    quint8 t3;
    quint8 txSeq;
    quint8 rxSeq;
    QTimer keepAliveTimer;
    QMutex sendLock;
    bool elmInitilised;

    bool getResponseCAN(bool replyExpected = true, bool closingChannel = false);
    QString getResponseStr();
    bool getResponseStatus(int expectedResult);

    bool checkResponse(int len);
    chanSetup getAsCS(int i);
    chanParam getAsCP(int i);
    dataTrans getAsDT(int i);
    dataTransFirst getAsDTFirst(int i);

    bool checkSeq();
    bool checkACK();
    bool sendACK(bool dataFollowing = false);
    bool checkForCommands();

    void recvData();

    void writeToElmStr(const QString &str);
    void writeToElmBA(const QByteArray &str);
    void setSendCanID(int id);
    void setRecvCanID(int id);

    void setChannelClosed();
    void elmInitialisationFailed();
    QString decodeError(int status);

    bool applyRecvTimeout(int msecs);
    int recvTimeout;
    int keepAliveError;
    bool passiveRxReadyStatus;
    quint8 minimumSendTime;
    quint8 timerT_Wait;
    bool responsePending;
};

#endif // TP20_H

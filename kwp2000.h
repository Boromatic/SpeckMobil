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

#ifndef KWP2000_H
#define KWP2000_H

#include <QObject>
#include <QThread>
#include <QMap>
#include "elm327.h"
#include "tp20.h"
#include "util.h"
#include "serialsettings.h"
#include "qeventloop.h"

typedef struct {
    int number;
    int addr;
    QString name;
    bool isPresent;
    int status;
} moduleInfo_t;

typedef struct {
    QString swNum;
    QString swVers;
    QString systemId;
    QString coding;
    quint64 shopNum[3];
} ecuLongId;

typedef struct {
    QString sum;
    QString part;
} DTC;

class kwp2000 : public QObject
{
    Q_OBJECT
public:
    explicit kwp2000(QObject *parent = 0);
    ~kwp2000();
    void setSerialParams(const serialSettings &in);
    int getChannelDest() const;
    bool getPortOpen() const;
    bool getElmInitialised() const;
    int getDiagSession() const;
    const QMap<int, moduleInfo_t>& getModuleList() const;
    void setTimeouts(int norm);
    void setKeepAliveInterval(int time);
    void readErrors();
    void deleteErrors();
    void sendOwn(int parm1, int parm2);

signals:
    void log(const QString &txt, int logLevel = stdLog);
    void elmInitDone(bool ok);
    void channelOpened(bool status);
    void portOpened(bool open);
    void portClosed();
    void newModuleInfo(ecuLongId ecuInfo, QStringList hwNum , QString vin, QString serNum);
    void moduleListRefreshed();
    void newDTCs(DTC nDTCs);
public slots:
    void openPort();
    void closePort();
    void openChannel(int i);
    void closeChannel();
    void startDiag(int param = 0x89);
    void openGW_refresh(bool ok = true);
private slots:
    void recvKWP(QByteArray* data);
    void channelOpenedClosed(bool status);
private:
    QThread* elmThread;
    QThread* tpThread;
    elm327* elm;
    tp20* tp;


    void startDiagHandler(QByteArray* data, quint8 param);
    void startIdHandler(QByteArray* data, quint8 param);
    void DTCHandler(QByteArray *data, quint8 NumDTCs);
    void miscHandler(QByteArray* data, quint8 respCode, quint8 param);
    void longIdHandler(QByteArray* data);
    void shortIdHandler(QByteArray* data);
    void queryModulesHandler(QByteArray* data);
    void serialHandler(QByteArray* data);
    void vinHandler(QByteArray* data);
    void longCodingHandler(QByteArray* data);
    void pause();


    ecuLongId ecuInfo;
    QStringList hwNum;
    DTC nDTCs;
    QString vin;
    QString serNum;

    QList<QByteArray> interpretRawData(QByteArray* raw);
    QMap<int, moduleInfo_t> moduleList;

    void initModuleNames();
    QMap<int, QString> moduleNames;

    void initResponseCodes();
    QMap<int, QString> responseCode;

    int destModule;

    int normRecvTimeout;
    int diagSession;
    QEventLoop longloop;
};

#endif // KWP2000_H

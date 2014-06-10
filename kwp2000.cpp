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

#include "kwp2000.h"
#include "util.h"
#include "qeventloop.h"

/**
 * @brief KWP2000
 *
 * @param parent
 */
kwp2000::kwp2000(QObject *parent) :
    QObject(parent),
    destModule(-1),
    normRecvTimeout(100),
    diagSession(0)
{
    elmThread = new QThread(this);
    tpThread = new QThread(this);

    elm = new elm327();
    tp = new tp20(elm);

    elm->moveToThread(elmThread);
    tp->moveToThread(tpThread);

    elmThread->start();
    tpThread->start();

    connect(tp, SIGNAL(response(QByteArray*)), this, SLOT(recvKWP(QByteArray*)));

    connect(elm, SIGNAL(log(QString, int)), this, SIGNAL(log(QString, int)));
    connect(tp, SIGNAL(log(QString, int)), this, SIGNAL(log(QString, int)));
    connect(tp, SIGNAL(elmInitDone(bool)), this, SIGNAL(elmInitDone(bool)));
    connect(elm, SIGNAL(portClosed()), this, SIGNAL(portClosed()));
//    connect(elm, SIGNAL(receivedData()), &longloop, SLOT(quit()));

    connect(tp, SIGNAL(channelOpened(bool)), this, SLOT(channelOpenedClosed(bool)));
    connect(tp, SIGNAL(channelOpened(bool)), this, SIGNAL(channelOpened(bool)));

    initModuleNames();
    initResponseCodes();
}

/**
 * @brief
 *
 */
kwp2000::~kwp2000()
{
    QMetaObject::invokeMethod(tp, "closeChannel", Qt::BlockingQueuedConnection);
    closePort();

    tpThread->exit(0);
    elmThread->exit(0);

    tpThread->wait(10000);
    elmThread->wait(10000);

    if (elmThread->isRunning()) {
        elmThread->terminate();
    }
    if (tpThread->isRunning()) {
        tpThread->terminate();
    }
}

/**
 * @brief Open the COM-port.
 *
 */
void kwp2000::openPort()
{
    if (tp->getChannelDest() >= 0) {
        QMetaObject::invokeMethod(tp, "closeChannel", Qt::BlockingQueuedConnection);
    }
    QMetaObject::invokeMethod(elm, "openPort", Qt::BlockingQueuedConnection);
}

/**
 * @brief Close the COM-port.
 *
 */
void kwp2000::closePort() {
    if (tp->getChannelDest() >= 0) {
        QMetaObject::invokeMethod(tp, "closeChannel", Qt::BlockingQueuedConnection);
    }
    QMetaObject::invokeMethod(elm, "closePort", Qt::BlockingQueuedConnection);
}

/**
 * @brief Open a module with channel i.
 *
 * @param i numer of the channel
 */
void kwp2000::openChannel(int i) {
    if (getChannelDest() >= 0) {
        return;
    }

    if (i == 0) {
        return;
    }

    int addr;
    if (moduleList.contains(i)) {
        addr = moduleList.value(i).addr;
    }
    else {
        addr = i;
    }

    destModule = i;

    emit log(tr("Opening channel to module 0x") + toHex(destModule));
    QMetaObject::invokeMethod(tp, "openChannel", Qt::BlockingQueuedConnection,
                              Q_ARG(int, addr),
                              Q_ARG(int, normRecvTimeout));
}

/**
 * @brief Close the open module.
 *
 */
void kwp2000::closeChannel() {
    emit log(tr("Closing channel to module 0x") + toHex(destModule));
    QMetaObject::invokeMethod(tp, "closeChannel", Qt::BlockingQueuedConnection);
}

/**
 * @brief Set parameters for COM-port.
 *
 * @param in parameters for serial settings
 */
void kwp2000::setSerialParams(const serialSettings &in)
{
    if (tp->getChannelDest() >= 0) {
        QMetaObject::invokeMethod(tp, "closeChannel", Qt::BlockingQueuedConnection);
    }
    elm->setSerialParams(in);
}

/**
 * @brief Returns number of channel to opened module.
 *
 * @return int number of channer to opened module or -1 if no channel opened
 */
int kwp2000::getChannelDest() const
{
    return tp->getChannelDest();
}

/**
 * @brief Returns true if COM-port connected.
 *
 * @return bool true for COM-port connected
 */
bool kwp2000::getPortOpen() const
{
    return elm->getPortOpen();
}

/**
 * @brief Returns true if ELM327 is correctly initialized.
 *
 * @return bool true for correct ELM327 initializing.
 */
bool kwp2000::getElmInitialised() const
{
    return tp->getElmInitialised();
}

void kwp2000::pause()
{
    QEventLoop loop;
    QTimer::singleShot(900, &loop, SLOT(quit()));
    loop.exec();
}

/**
 * @brief Start a diagnostic session to opened module.
 *
 * @param param type of diagnostic session, default = 0x89, that is VW specific diagnosis
 */
void kwp2000::startDiag(int param)
{
    QByteArray packet;
    //pause();
//    packet.append(0x10);
//    packet.append(param);
//    QMetaObject::invokeMethod(tp, "sendData", Qt::BlockingQueuedConnection,
//                              Q_ARG(QByteArray, packet),
//                              Q_ARG(int, normRecvTimeout));
    // do request for long ID
   pause();
    packet.clear();
    packet.append(0x1A);
    packet.append(0x9B); //Read calibration data
    QMetaObject::invokeMethod(tp, "sendData", Qt::BlockingQueuedConnection,
                              Q_ARG(QByteArray, packet),
                              Q_ARG(int, normRecvTimeout));
    pause();
    //QMetaObject::invokeMethod(tp, "sendKeepAlive", Qt::BlockingQueuedConnection);

    // now send request for short ID
    packet.clear();
    packet.append(0x1A);
    packet.append(0x91); //Read HW Number
    QMetaObject::invokeMethod(tp, "sendData", Qt::BlockingQueuedConnection,
                              Q_ARG(QByteArray, packet),
                              Q_ARG(int, normRecvTimeout));
    pause();
    //QMetaObject::invokeMethod(tp, "sendKeepAlive", Qt::BlockingQueuedConnection);

    packet.clear();
    packet.append(0x1A);
    packet.append(0x97); //Read systemName
    QMetaObject::invokeMethod(tp, "sendData", Qt::BlockingQueuedConnection,
                              Q_ARG(QByteArray, packet),
                              Q_ARG(int, normRecvTimeout));
    pause();
    //QMetaObject::invokeMethod(tp, "sendKeepAlive", Qt::BlockingQueuedConnection);

    packet.clear();
    packet.append(0x1A);
    packet.append(0x86); //Read ManufacuterSepcific Data (Serial Number)
    QMetaObject::invokeMethod(tp, "sendData", Qt::BlockingQueuedConnection,
                              Q_ARG(QByteArray, packet),
                              Q_ARG(int, normRecvTimeout));
    pause();
    //QMetaObject::invokeMethod(tp, "sendKeepAlive", Qt::BlockingQueuedConnection);

    if (tp->getChannelDest() == 1)  // only Engine
    {
        packet.clear();
        packet.append(0x1A);
        packet.append(0x90); //Read VIN
        QMetaObject::invokeMethod(tp, "sendData", Qt::BlockingQueuedConnection,
                                  Q_ARG(QByteArray, packet),
                                  Q_ARG(int, normRecvTimeout));
       // QMetaObject::invokeMethod(tp, "sendKeepAlive", Qt::BlockingQueuedConnection);
    }
}

/**
 * @brief Prints text if channel opened or closed
 *
 * @param status
 */
void kwp2000::channelOpenedClosed(bool status)
{
    if (status == false) {
        if (destModule >= 0) {
            emit log(tr("Channel closed to module 0x") + toHex(destModule));
        }
        diagSession = 0;
        destModule = -1;
    }
    else {
        diagSession = 81;
        serNum = "";
        vin = "";
        hwNum.clear();
        ecuInfo.coding = "";
        ecuInfo.shopNum[0] = 0;
        ecuInfo.shopNum[1] = 0;
        ecuInfo.shopNum[2] = 0;
        ecuInfo.swNum = "";
        ecuInfo.swVers = "";
        ecuInfo.systemId = "";
        emit log(tr("Channel opened to module 0x") + toHex(destModule));
    }
}

/**
 * @brief
 *
 * @param ok
 */
void kwp2000::openGW_refresh(bool ok)
{
    if (ok) {
        destModule = 31;
        QMetaObject::invokeMethod(tp, "openChannel", Qt::BlockingQueuedConnection,
                                  Q_ARG(int, destModule),
                                  Q_ARG(int, normRecvTimeout));
        QByteArray packet;
        pause();
        packet.append(0x1A);
        packet.append(0x9F);
        QMetaObject::invokeMethod(tp, "sendData", Qt::BlockingQueuedConnection,
                                  Q_ARG(QByteArray, packet),
                                  Q_ARG(int, normRecvTimeout));
      //  QMetaObject::invokeMethod(tp, "sendKeepAlive", Qt::BlockingQueuedConnection);
    }
}

/**
 * @brief
 *
 */
void kwp2000::readErrors()
{
    QByteArray packet;
    pause();
    packet.append(0x18); //readDiagnosticTroubleCodesByStatus
    packet.append(0x02); //requestStoredDTCAndStatus
    packet.append(0xFF);
    packet.append((char)0x00);
    QMetaObject::invokeMethod(tp, "sendData", Qt::BlockingQueuedConnection,
                              Q_ARG(QByteArray, packet),
                              Q_ARG(int, normRecvTimeout));
   // QMetaObject::invokeMethod(tp, "sendKeepAlive", Qt::BlockingQueuedConnection);

    //debug
//    packet.clear();
//    packet.append(0x18); //readDiagnosticTroubleCodesByStatus
//    packet.append(0x03); //requestAllDTCAndStatus
//    packet.append(0xFF);
//    packet.append((char)0x00);
//    QMetaObject::invokeMethod(tp, "sendData", Qt::BlockingQueuedConnection,
//                              Q_ARG(QByteArray, packet),
//                              Q_ARG(int, normRecvTimeout));
//    QMetaObject::invokeMethod(tp, "sendKeepAlive", Qt::BlockingQueuedConnection);
    //\debug
}

/**
 * @brief
 *
 * @param parm1
 * @param parm2
 */
void kwp2000::sendOwn(int parm1, int parm2)
{
    if (((0 <= parm1) <= 255) && ((0 <= parm2) <= 255))
    {
        QByteArray packet;
        pause();
        packet.append(parm1);
        packet.append(parm2);
        emit log(tr("Send: ") + toHex(parm1) + toHex(parm2));
        QMetaObject::invokeMethod(tp, "sendData", Qt::BlockingQueuedConnection,
                                  Q_ARG(QByteArray, packet),
                                  Q_ARG(int, normRecvTimeout));
     //   QMetaObject::invokeMethod(tp, "sendKeepAlive", Qt::BlockingQueuedConnection);
    }
}

/**
 * @brief
 *
 */
void kwp2000::deleteErrors()
{
    QByteArray packet;
    pause();
    packet.append(0x14);
    packet.append(0xFF);
    packet.append((char)0x00);
    QMetaObject::invokeMethod(tp, "sendData", Qt::BlockingQueuedConnection,
                              Q_ARG(QByteArray, packet),
                              Q_ARG(int, normRecvTimeout));
   // QMetaObject::invokeMethod(tp, "sendKeepAlive", Qt::BlockingQueuedConnection);
}

/**
 * @brief
 *
 * @param data
 */
void kwp2000::recvKWP(QByteArray *data)
{
    if (!data) {
        emit log(tr("Error: Received empty KWP data"));
        QMetaObject::invokeMethod(tp, "closeChannel", Qt::BlockingQueuedConnection);
        return;
    }

    if (data->length() < 2) {
        emit log(tr("Error: Received malformed KWP data"));
        QMetaObject::invokeMethod(tp, "closeChannel", Qt::BlockingQueuedConnection);
        delete data;
        return;
    }

    quint8 respCode = static_cast<quint8>(data->at(0));
    quint8 param = static_cast<quint8>(data->at(1));
    data->remove(0,2);

    if (respCode == 0x7F) {
        //negative response
        emit log(tr("Warning: Received negative KWP response to ") + toHex(param) + tr(" command"));
        if (data->length() > 0) {
            quint8 reasonCode = static_cast<quint8>(data->at(0));
            if (reasonCode == 0x78)
            {
                QMetaObject::invokeMethod(tp, "stopKeepAliveTimer", Qt::QueuedConnection);
                emit log(tr("Waiting for Data..."));
                delete data;
                //QTimer::singleShot(10000, &longloop, SLOT(quit()));
                //longloop.exec();
                //QMetaObject::invokeMethod(tp, "waitForData", Qt::QueuedConnection);
                //todo reaction to 0x78
                return;
            }
            QString reasonName = responseCode.value(reasonCode, tr("Unknown reason"));
            emit log(tr("Reason code ") + toHex(reasonCode));
            emit log(reasonName);
                }
        delete data;
        return;
    }

    //positive responses
    switch (respCode) {
    case 0x50:
        // startDiagnosticSession
        startDiagHandler(data, param);
        break;
    case 0x5A:
        // readEcuIdentification
        startIdHandler(data, param);
        break;
    case 0x58:
        // interpret DTCs
        DTCHandler(data, param);
        break;
    default:
        miscHandler(data, respCode, param);
        break;
    }
}

/**
 * @brief
 *
 * @param data
 * @param param
 */
void kwp2000::startDiagHandler(QByteArray *data, quint8 param)
{
    diagSession = param;
    delete data;
}

/**
 * @brief
 *
 * @param data
 * @param param
 */
void kwp2000::startIdHandler(QByteArray *data, quint8 param)
{
    if (param == 0x9F) {
        queryModulesHandler(data);
        return;
    }
    else if (param == 0x86) {
        serNum = "";
        serialHandler(data);
    }
    else if (param == 0x90) {
        vin = "";
        vinHandler(data);
    }
    else if (param == 0x91) {
        hwNum.clear();
        shortIdHandler(data);
    }
    else if (param == 0x9A) {
        ecuInfo.coding = "";
        longCodingHandler(data);
    }

    else if (param == 0x9B) {
        ecuInfo.coding = "";
        ecuInfo.shopNum[0] = 0;
        ecuInfo.shopNum[1] = 0;
        ecuInfo.shopNum[2] = 0;
        ecuInfo.swNum = "";
        ecuInfo.swVers = "";
        ecuInfo.systemId = "";
        longIdHandler(data);
    }
    else {
        emit log(tr("Error: Unknown ID Parameter"));
        miscHandler(data, 0x50, param);
    }
    emit newModuleInfo(ecuInfo, hwNum , vin, serNum);
    return;


}

/**
 * @brief
 *
 * @param data
 * @param param
 */
void kwp2000::DTCHandler(QByteArray *data, quint8 NumDTCs)

{
    nDTCs.sum = QString::number(NumDTCs);
    nDTCs.part = "";
    if (data->length() < (NumDTCs * 3))
    {
        emit log(tr("DTC-data too short"));
        delete data;
        return;
    }
    else if (NumDTCs > 0)
    {
        for (int i = 0; i < (NumDTCs * 3); i+=3) {
            quint64 codenumber = 0;
            codenumber |=  (unsigned char)(data->at(i));
            codenumber <<= 8;
            codenumber |=  (unsigned char)(data->at(i+1));
            nDTCs.part.append(QString("%1").arg(codenumber, 5, 10, QChar('0')));
            nDTCs.part.append("-");
            codenumber = (unsigned char)(data->at(i+2));
            nDTCs.part.append(QString("%1").arg(codenumber, 2, 16, QChar('0')));
            nDTCs.part.append(" ");
        }
    }
    else
    {
        nDTCs.part.append(" ");
    }
    emit newDTCs(nDTCs);
    delete data;
}


/**
 * @brief
 *
 * @param data
 * @param respCode
 * @param param
 */
void kwp2000::miscHandler(QByteArray *data, quint8 respCode, quint8 param)
{
    emit log(tr("Misc command: Response code") + toHex(respCode) + tr(", parameter ") + toHex(param));
    delete data;
}

void kwp2000::serialHandler(QByteArray *data)
{
    quint8 len = static_cast<quint8>(data->at(0));
    if (data->length() >= (len + 1))
    {
        for (int i = 1; i < (len + 1); i++) {
            serNum += QChar(data->at(i));
        }
        while(serNum.right(1) == QChar(' ')) {
            serNum.chop(1);
        }
    }
    delete data;
}

void kwp2000::vinHandler(QByteArray *data)
{
    vin = "";
    for (int i = 0; i < data->length(); i++) {
        vin += QChar(data->at(i));
        while(vin.right(1) == QChar(' ')) {
            vin.chop(1);
        }
    }
    delete data;
}

void kwp2000::longCodingHandler(QByteArray *data)
{
    if (data->at(10) != 0x10)
    {
        emit log(tr("LongCoding Error"));
    }
    else
    {
        emit log(tr("Long Coding"), debugMsgLog);

        quint8 len = static_cast<quint8>(data->at(11));
        if (data->length() < (len + 12))
        {
            emit log(tr("LongCoding Date too short"));
            delete data;
            return;
        }
        else if (len > 0)
        {
            quint64 codenumber = 0;
            for (int i = 12; i < (len + 12 - 1); i++) {
                codenumber <<= 8;
                codenumber |=  (unsigned char)(data->at(i));
            }
            ecuInfo.coding.append(QString("%1").arg(codenumber, 8, 16, QChar('0'))).toUpper();
        }
    }
    delete data;
}

/**
 * @brief
 *
 * @param data
 */
void kwp2000::shortIdHandler(QByteArray *data) {
    QStringList tmpList;

    for (int i = 0; i < data->length();) {
        if (static_cast<quint8>(data->at(i)) == 0xFF) {
            break;
        }
        QString tmp;
        quint8 len = static_cast<quint8>(data->at(i));
        for (int j = 1; j < len; j++) {
            tmp += QChar(data->at(i+j));
        }
        i += len;

        while(tmp.right(1) == QChar(' ')) {
            tmp.chop(1);
        }
        tmpList << tmp;
    }

    hwNum = tmpList;
    delete data;
}

/**
 * @brief
 *
 * @param data
 */
void kwp2000::longIdHandler(QByteArray *data)
{
    for (int i = 0; i < 12; i++) {
        ecuInfo.swNum += QChar(data->at(i)); //Software part number
    }

    for (int i = 12; i < 16; i++) {
        if (12 == i)
        {
            ecuInfo.swVers += QChar(data->at(i) & 0x7F); // 0b01111111 // first bit has to be 0
        }
        else {
            ecuInfo.swVers += QChar(data->at(i));  //Software version
        }
    }

    if (data->at(16) == 0x03)
    {
        emit log(tr("Short coding"), debugMsgLog);
        quint64 codenumber = 0;

    for (int i = 17; i < 20; i++) {
        codenumber <<= 8;
        codenumber |=  (unsigned char)(data->at(i));
    }
    ecuInfo.coding.append(QString("%1").arg(codenumber, 8, 10, QChar('0')));
    }
    else if (data->at(16) == 0x10)
    {
        emit log(tr("Start reading long coding"), debugMsgLog);

        QByteArray packet;
        pause();
        packet.clear();
        packet.append(0x1A);
        packet.append(0x9A); //Read Long Coding
        QMetaObject::invokeMethod(tp, "sendData", Qt::BlockingQueuedConnection,
                                  Q_ARG(QByteArray, packet),
                                  Q_ARG(int, normRecvTimeout));
    //    QMetaObject::invokeMethod(tp, "sendKeepAlive", Qt::BlockingQueuedConnection);

    }

    //Shop number 6 Byte: 21bit=3.Block, 10bit=2.Block, 17bit=1.Block
    //8+8+[5 3]+[7 1]+8+8
    for (int i = 20; i < 23; i++) {
        ecuInfo.shopNum[2] <<= 8;
        ecuInfo.shopNum[2] |=  (unsigned char)data->at(i);
    }
    ecuInfo.shopNum[2] >>= 3;

    for (int i = 22; i < 24; i++) {
        ecuInfo.shopNum[1] <<= 8;
        ecuInfo.shopNum[1] |=  (unsigned char)data->at(i);
    }
    ecuInfo.shopNum[1] >>= 1;
    ecuInfo.shopNum[1] &= 0x3FF;  //10 bit

    for (int i = 23; i < 26; i++) {
        ecuInfo.shopNum[0] <<= 8;
        ecuInfo.shopNum[0] |=  (unsigned char)data->at(i);
    }
    ecuInfo.shopNum[0] &= 0x1FFFF;  //17 bit

    for (int i = 26; i < data->length(); i++) {
        ecuInfo.systemId += QChar(data->at(i));
    }
    while(ecuInfo.systemId.right(1) == QChar(' ')) {
        ecuInfo.systemId.chop(1);   //System identifier
    }

    delete data;
}

/**
 * @brief
 *
 * @param data
 */
void kwp2000::queryModulesHandler(QByteArray *data)
{
    QList<QByteArray> dataList = interpretRawData(data);
   // delete data;

    if (dataList.length() != 2) {
        emit log(tr("List modules: Didn't get list of 2 byte arrays"));
        QMetaObject::invokeMethod(tp, "closeChannel", Qt::BlockingQueuedConnection);
        return;
    }

    moduleList.clear();

    //todo: no loop
    for (int i = 0; i < dataList.at(0).length(); i+=4) {
        if (i+4 > dataList.at(0).length()) {
            emit log(tr("Error! List modules: Not complete."));
            QMetaObject::invokeMethod(tp, "closeChannel", Qt::BlockingQueuedConnection);
            return;
        }

        quint8 lsb = static_cast<quint8>(dataList.at(0).at(i+3));
        if (lsb) {
            moduleInfo_t tmp;
            tmp.number = static_cast<quint8>(dataList.at(0).at(i));
            tmp.addr = static_cast<quint8>(dataList.at(0).at(i+1));
            tmp.name = moduleNames.value(tmp.number, tr("Unknown Module"));
            tmp.isPresent = ((lsb & 0x01) == 1);
            tmp.status = (lsb & 0x1E) >> 1;

            // skip modules with address of 0x13, module numbers 0x0 and 0x4 seem to report this addr
            // but these modules dont exist
            if (tmp.addr == 0x13) {
                emit log(tr("Skipping module ") + toHex(tmp.number) + tr(" (addr is 0x13)"), debugMsgLog);
                continue;
            }

            moduleList.insert(tmp.number, tmp);
        }
    }

    emit moduleListRefreshed();
    QMetaObject::invokeMethod(tp, "closeChannel", Qt::BlockingQueuedConnection);
}

/**
 * @brief
 *
 * @param raw
 * @return QList<QByteArray>
 */
QList<QByteArray> kwp2000::interpretRawData(QByteArray *raw)
{
    QList<QByteArray> retList;

    for (int i = 0; i < raw->length();) {
        if (static_cast<quint8>(raw->at(i)) == 0xFF) {
            break;
        }
        QByteArray tmp;
        quint8 len = static_cast<quint8>(raw->at(i));
        for (int j = 1; j < len; j++) {
            tmp += static_cast<quint8>(raw->at(i+j));
        }
        i += len;

        retList << tmp;
    }

    return retList;
}

/**
 * @brief
 *
 */
void kwp2000::initResponseCodes()
{
    responseCode.insert(0x10, "generalReject");
    responseCode.insert(0x11, "serviceNotSupported");
    responseCode.insert(0x12, "subFunctionNotSupported-invalidFormat");
    responseCode.insert(0x21, "busy-RepeatRequest");
    responseCode.insert(0x22, "conditionsNotCorrect or requestSequenceError");
    responseCode.insert(0x23, "routineNotCompleteOrServiceInProgress");
    responseCode.insert(0x31, "requestOutOfRange");
    responseCode.insert(0x33, "securityAccessDenied; securityAccessRequested");
    responseCode.insert(0x35, "invalidKey");
    responseCode.insert(0x36, "exceedNumberOfAttempts");
    responseCode.insert(0x37, "requiredTimeDelayNotExpired");
    responseCode.insert(0x40, "downloadNotAccepted");
    responseCode.insert(0x41, "improperDownloadType");
    responseCode.insert(0x42, "can'tDownloadToSpecifiedAddress");
    responseCode.insert(0x43, "can'tDownloadNumberOfBytesRequested");
    responseCode.insert(0x50, "uploadNotAccepted");
    responseCode.insert(0x51, "improperUploadType");
    responseCode.insert(0x52, "can'tUploadFromSpecifiedAddress");
    responseCode.insert(0x53, "can'tUploadNumberOfBytesRequested");
    responseCode.insert(0x71, "transferSuspended");
    responseCode.insert(0x72, "transferAborted");
    responseCode.insert(0x74, "illegalAddressInBlockTransfer");
    responseCode.insert(0x75, "illegalByteCountInBlockTransfer");
    responseCode.insert(0x76, "illegalBlockTransferType");
    responseCode.insert(0x77, "blockTransferDataChecksumError");
    responseCode.insert(0x78, "requestCorrectlyReceived-ResponsePending");
    responseCode.insert(0x79, "incorrectByteCountDuringBlockTransfer");
    responseCode.insert(0x80, "serviceNotSupportedInActiveDiagnosticSession");
}

/**
 * @brief
 *
 */
void kwp2000::initModuleNames()
{
    moduleNames.insert(0x01, tr("Engine #1"));
    moduleNames.insert(0x02, tr("Transmission"));
    moduleNames.insert(0x03, tr("ABS"));
    moduleNames.insert(0x05, tr("Security Access"));
    moduleNames.insert(0x06, tr("Passenger Seat"));
    moduleNames.insert(0x07, tr("Front Info/Control"));
    moduleNames.insert(0x08, tr("AC & Heating"));
    moduleNames.insert(0x09, tr("Central Electronics #1"));
    moduleNames.insert(0x10, tr("Parking Aid #2"));
    moduleNames.insert(0x11, tr("Engine #2"));
    moduleNames.insert(0x13, tr("Distance Regulation"));
    moduleNames.insert(0x14, tr("Suspension"));
    moduleNames.insert(0x15, tr("Airbags"));
    moduleNames.insert(0x16, tr("Steering"));
    moduleNames.insert(0x17, tr("Instrument Cluster"));
    moduleNames.insert(0x18, tr("Aux Heater"));
    moduleNames.insert(0x19, tr("CAN Gateway"));
    moduleNames.insert(0x20, tr("High Beam Assist"));
    moduleNames.insert(0x22, tr("All Wheel Drive"));
    moduleNames.insert(0x25, tr("Immobiliser"));
    moduleNames.insert(0x26, tr("Convertible Top"));
    moduleNames.insert(0x29, tr("Left Headlight"));
    moduleNames.insert(0x31, tr("Diagnostic Interface"));
    moduleNames.insert(0x34, tr("Level Control"));
    moduleNames.insert(0x35, tr("Central Locking"));
    moduleNames.insert(0x36, tr("Driver Seat"));
    moduleNames.insert(0x37, tr("Radio/Sat Nav"));
    moduleNames.insert(0x39, tr("Right Headlight"));
    moduleNames.insert(0x42, tr("Driver Door"));
    moduleNames.insert(0x44, tr("Steering Assist"));
    moduleNames.insert(0x45, tr("Interior Monitoring"));
    moduleNames.insert(0x46, tr("Comfort System"));
    moduleNames.insert(0x47, tr("Sound System"));
    moduleNames.insert(0x52, tr("Passenger Door"));
    moduleNames.insert(0x53, tr("Parking Brake"));
    moduleNames.insert(0x55, tr("Headlights"));
    moduleNames.insert(0x56, tr("Radio"));
    moduleNames.insert(0x57, tr("TV Tuner"));
    moduleNames.insert(0x61, tr("Battery"));
    moduleNames.insert(0x62, tr("Rear Left Door"));
    moduleNames.insert(0x65, tr("Tire Pressure"));
    moduleNames.insert(0x67, tr("Voice Control"));
    moduleNames.insert(0x68, tr("Wipers"));
    moduleNames.insert(0x69, tr("Trailer Recognition"));
    moduleNames.insert(0x72, tr("Rear Right Door"));
    moduleNames.insert(0x75, tr("Telematics"));
    moduleNames.insert(0x76, tr("Parking Aid"));
    moduleNames.insert(0x77, tr("Telephone"));
}

/**
 * @brief
 *
 * @return const QMap<int, moduleInfo_t>
 */
const QMap<int, moduleInfo_t> &kwp2000::getModuleList() const
{
    return moduleList;
}

/**
 * @brief
 *
 * @return int
 */
int kwp2000::getDiagSession() const
{
    return diagSession;
}

/**
 * @brief
 *
 * @param slow
 * @param norm
 * @param fast
 */
void kwp2000::setTimeouts(int norm)
{
    normRecvTimeout = norm;
}

/**
 * @brief
 *
 * @param time
 */
void kwp2000::setKeepAliveInterval(int time)
{
    tp->setKeepAliveInterval(time);
}

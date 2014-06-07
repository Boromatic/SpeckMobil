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

#include "elm327.h"
#include "util.h"

elm327::elm327(QObject *parent) :
    QObject(parent),
    port(0),
    protectLines(new QMutex()),
    sendCanID(0), recvCanID(0),
    portOpen(false)
{
    // default settings
    settings.name = "COM1";
    settings.dataBits = QSerialPort::Data8;
    settings.flowControl = QSerialPort::NoFlowControl;
    settings.parity = QSerialPort::NoParity;
    settings.rate = QSerialPort::Baud38400;
    settings.stopBits = QSerialPort::OneStop;
}

elm327::~elm327()
{
    closePort();
}

void elm327::openPort()
{
    if (!port) {
        port = new QSerialPort(this);
        port->setReadBufferSize(0x100000);
        connect(port, SIGNAL(readyRead()), this, SLOT(constructLine()));
    }

    {
        if (port) {
            port->close();
        }
        portOpen = false;
    }

    port->setPortName(settings.name);

    if (port && port->open(QIODevice::ReadWrite)) {
        emit log("Port opened.");

        bool ok;
        ok = port->setBaudRate(settings.rate);
        if (!ok) {
            emit log("Serial port rate could not be set.", serialConfigLog);
        }
        ok = port->setDataBits(settings.dataBits);
        if (!ok) {
            emit log("Serial port data bits could not be set.", serialConfigLog);
        }
        ok = port->setParity(settings.parity);
        if (!ok) {
            emit log("Serial port parity could not be set.", serialConfigLog);
        }
        ok = port->setStopBits(settings.stopBits);
        if (!ok) {
            emit log("Serial port stop bits could not be set.", serialConfigLog);
        }
        ok = port->setFlowControl(settings.flowControl);
        if (!ok) {
            emit log("Serial port flow control could not be set.", serialConfigLog);
        }

        emit log("Port configured.");
        portOpen = true;
        emit portOpened(true);
        return;
    }
    portOpen = false;
    emit portOpened(false);
    emit portClosed();
    emit log("Port could not be opened.");
    return;
}

void elm327::closePort()
{
    if (port) {
        port->close();
        emit portClosed();
        emit log("Port closed.");
    }
    portOpen = false;
}

void elm327::write(const QString &txt)
{
    //if (txt2.mid(9,2) == "21")
    //    txt2.append(" 5");
    //if (txt2.left(1) == "B")
    //    txt2.append(" 0");

    emit log("TX: " + txt, rxTxLog);

    if (port && port->isOpen()) {
        port->write((txt + '\r').toLatin1());
    }
}

void elm327::write(const QByteArray &data)
{
    if (data.length() > 8)
        return;

    QString txt;

    for (int i = 0; i < data.length(); i++) {
        quint8 dat = data.at(i);
        txt += toHex(dat, 2) + " ";
    }
    txt.chop(1);

    write(txt);
}

// need to return a list of CAN messages rather than lumped together.
QList<canFrame*>* elm327::getResponseCAN(int &status)
{
    status = 0;
    QString response = getLine();
    QList<canFrame*>* result = new QList<canFrame*>;
    int triesForPrompt = 20;

    if (response == "AT") {
        status |= AT_RESPONSE;
        response = getLine(); // echos must be on, get another line
    }

    if (response.length() == 0) {
        status |= TIMEOUT_ERROR;
        response = getLine();
    }
    else if (response.startsWith("OK")) {
        status |= OK_RESPONSE;
        response = getLine();
    }
    else if (response.startsWith("STOPPED")) {
        status |= STOPPED_RESPONSE;
        response = getLine();
    }
    else if (response.startsWith("?")) {
        status |= UNKNOWN_RESPONSE;
        response = getLine();
    }
    else if (response.startsWith("NO DATA")) {
        status |= NO_DATA_RESPONSE;
        response = getLine();
    }
    else if (response.startsWith("CAN ERROR")) {
        status |= CAN_ERROR;
        response = getLine();
    }

    if (status != 0) {
        triesForPrompt = 2;
    }

    // receive data until prompt appears
    for (int i = 0; !response.endsWith(">"); i++) {
        if (i >= triesForPrompt) {
            status |= NO_PROMPT_ERROR;
            break;
        }

        if (response.startsWith("STOPPED")) {
            status |= STOPPED_RESPONSE;
        }
        else if (response.length() != 0) {   //skip empty lines
            canFrame* newCF = hexToCF(response);
            if (!newCF) {
                status |= PROCESSING_ERROR;
                response = getLine();
                continue;
            }
            result->append(newCF);
        }
        response = getLine();
    }
    return result;
}

bool elm327::getResponseStatus(int &status)
{
    status = 0;
    QString response = getLine();
    bool ret = false;

    if (response.startsWith("AT")) {
        status |= AT_RESPONSE;
        response = getLine(); // echos must be on, get another line
    }

    if (response.startsWith("OK")) {
        status |= OK_RESPONSE;
        ret = true;
    }
    else if (response.startsWith("?")) {
        status |= UNKNOWN_RESPONSE;
    }

    // receive data until prompt appears
    //FEHLER
    if (response.endsWith(">")) {
        return ret;
    }
    for (int i = 0; !getLine().endsWith(">"); i++) {
        if (i >=3) {
            status |= NO_PROMPT_ERROR;
            break;
        }
    }
    return ret;
}

void elm327::setSerialParams(const serialSettings &in)
{
    if (port && port->isOpen()) {
        port->close();
        portOpen = false;
        emit portClosed();
    }

    settings = in;
}

bool elm327::getPortOpen()
{
    return portOpen;
}

void elm327::setSendCanID(int id)
{
    sendCanID = id;
    QString idStr = toHex(id, 3);
    write("AT SH " + idStr);
}

void elm327::setRecvCanID(int id)
{
    recvCanID = id;
    QString idStr = toHex(id, 3);
    write("AT CRA " + idStr);
}

QString elm327::getResponseStr(int &status)
{
    status = 0;
    QString response = getLine();
    if (response.endsWith(">")) {
        return response;
    }
    // receive data until prompt appears
    for (int i = 0; !getLine().endsWith(">"); i++) {
        if (i >= 3) {
            status |= NO_PROMPT_ERROR;
            break;
        }
    }

    return response;
}

canFrame* elm327::hexToCF(QString &input)
{
    int id, len, tst = 0;
    QByteArray data;
    bool ok;

    QString orig = input;
    input.remove(' ');

    id = input.mid(0, 3).toUInt(&ok, 16);
    if (!ok) {
        tst++;
        return 0;
    }
    len = input.mid(3, 1).toUInt(&ok, 16);
    if (!ok) {
        tst++;
        return 0;
    }

    QString dataStr = input.mid(4);

    if (dataStr.length() % 2) {
        // not divisable by 2, error in data
        tst++;
        return 0;
    }

    for (int i = 0; i < dataStr.length() / 2; i++) {
        quint8 byte = dataStr.mid(i*2, 2).toUShort(&ok, 16);
        if (!ok) {
            tst++;
            return 0;
        }
        data.append(byte);
    }

    if (data.length() != len) {
        tst++;
        return 0;
    }

    return new canFrame(id, len, data);
}

QString elm327::getLine(int timeout, bool wait)
{
    QString result;
    protectLines->lock();
    if (bufferedLines.empty() && wait) {
        linesAvailable.wait(protectLines, timeout);
    }
    if (!bufferedLines.empty()) {
        result = bufferedLines.takeFirst();
    }
    protectLines->unlock();
    return result;
}

void elm327::constructLine()
{
    static QByteArray data;
    data.append(port->readAll());

    if (data.endsWith('\r') || data.endsWith('>')) {
        QString line(data);
        data.clear();

        if (line.endsWith('\r')) {
            line.chop(1);
        }

        if (line.isEmpty()) {
            return;
        }

        emit log("RX: " + line, rxTxLog, false);
        emit receivedData();

        protectLines->lock();
        QStringList templines = line.split("\r");

        bufferedLines << templines;
        linesAvailable.wakeAll();
        protectLines->unlock();
    }
}

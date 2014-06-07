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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QMessageBox>
#include "util.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    kwp(this),
    appSettings(new QSettings("SpeckMobil.ini", QSettings::IniFormat, this)),
    serialConfigured(false)
 {

    ui->setupUi(this);

    serSettings = new serialSettingsDialog(this);
    settingsDialog = new settings(appSettings, this);
    connect(ui->action_About, SIGNAL(triggered()), this, SLOT(aboutdlg()));
    connect(ui->actionApplication_settings, SIGNAL(triggered()), settingsDialog, SLOT(show()));

    connect(&kwp, SIGNAL(log(QString, int)), this, SLOT(log(QString, int)));
    connect(&kwp, SIGNAL(channelOpened(bool)), this, SLOT(channelOpen(bool)));
    connect(&kwp, SIGNAL(elmInitDone(bool)), this, SLOT(elmInitialised(bool)));
    connect(&kwp, SIGNAL(newModuleInfo(ecuLongId,QStringList,QString,QString)), this, SLOT(moduleInfoReceived(ecuLongId, QStringList, QString, QString)));
    connect(&kwp, SIGNAL(newDTCs(DTC)), this, SLOT(DTCReceived(DTC)));
    connect(ui->lineEdit_moduleAddress, SIGNAL(textChanged(QString)), this, SLOT(clearUI()));
    connect(ui->comboBox_modules, SIGNAL(activated(int)), this, SLOT(selectNewModule(int)));
    connect(&kwp, SIGNAL(moduleListRefreshed()), this, SLOT(refreshModules()));
    connect(&kwp, SIGNAL(portClosed()), this, SLOT(portClosed()));
    connect(settingsDialog, SIGNAL(settingsChanged()), this, SLOT(updateSettings()));

    restoreSettings();

    if (serialConfigured) {
        kwp.setSerialParams(serSettings->getSettings());
    }

    connect(serSettings, SIGNAL(settingsApplied()), this, SLOT(connectToSerial()));
    refreshModules(true);
}

MainWindow::~MainWindow()
{
    kwp.closePort();

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event++; // get rid of warning
    serSettings->hide();
    settingsDialog->hide();
    saveSettings();
}

//void MainWindow::hideEvent(QHideEvent *event)
//{
//    event++; // get rid of warning
//}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent* eventWindowChange = static_cast<QWindowStateChangeEvent*>(event);
        if (eventWindowChange->oldState() == Qt::WindowMinimized) {
        }
    }
}

// Serial settings and window settings handled here
// Other user configurable values are set in settingsDialog
void MainWindow::saveSettings()
{
    appSettings->setValue("Serial/configured", serialConfigured);
    serialSettings tmp = serSettings->getSettings();
    appSettings->setValue("Serial/portName", tmp.name);
    appSettings->setValue("Serial/rate", tmp.rate);
    appSettings->setValue("Serial/dataBits", tmp.dataBits);
    appSettings->setValue("Serial/parity", tmp.parity);
    appSettings->setValue("Serial/stopBits", tmp.stopBits);
    appSettings->setValue("Serial/flowControl", tmp.flowControl);
    appSettings->setValue("Log/logLevel", logLevel);
    settingsDialog->save();
    appSettings->setValue("MainWindow/state", saveState());
    appSettings->setValue("MainWindow/geometry", saveGeometry());
    appSettings->sync();
}

// Serial settings and window settings handled here
// Other user configurable values are set in settingsDialog
void MainWindow::restoreSettings()
{
    serialConfigured = appSettings->value("Serial/configured", false).toBool();
    serialSettings tmp;
    bool ok;
    tmp.name = appSettings->value("Serial/portName", QString("COM1")).toString();
    tmp.rate = appSettings->value("Serial/rate", QSerialPort::Baud19200).toInt(&ok);
    tmp.dataBits = static_cast<QSerialPort::DataBits>(appSettings->value("Serial/dataBits", QSerialPort::Data8).toInt(&ok));
    tmp.parity = static_cast<QSerialPort::Parity>(appSettings->value("Serial/parity", QSerialPort::NoParity).toInt(&ok));
    tmp.stopBits = static_cast<QSerialPort::StopBits>(appSettings->value("Serial/stopBits", QSerialPort::OneStop).toInt(&ok));
    tmp.flowControl = static_cast<QSerialPort::FlowControl>(appSettings->value("Serial/flowControl", QSerialPort::NoFlowControl).toInt(&ok));
    serSettings->setSettings(tmp);

    logLevel = appSettings->value("Log/logLevel", 0xFF).toInt();

    settingsDialog->load();
    updateSettings();

    restoreGeometry(appSettings->value("MainWindow/geometry").toByteArray());
    restoreState(appSettings->value("MainWindow/state").toByteArray());
}

void MainWindow::log(const QString &txt, int logLevel)
{
    if (logLevel & this->logLevel) {
        logBuffer << txt;
        if (logLevel != rxTxLog || logBuffer.length() >= 50) {
            flushLogBuffer();
        }
    }
}

void MainWindow::flushLogBuffer()
{
    if (!logBuffer.empty()) {
        ui->plainTextEdit_log->appendPlainText(logBuffer.join("\n"));
        logBuffer.clear();
    }
}

void MainWindow::updateSettings()
{
    kwp.setTimeouts(settingsDialog->norm);
    kwp.setKeepAliveInterval(settingsDialog->keepAliveInterval);
}

void MainWindow::connectToSerial()
{
    saveSettings();
    serialConfigured = true;
    kwp.setSerialParams(serSettings->getSettings());
}

void MainWindow::selectNewModule(int pos)
{
    if (pos == ui->comboBox_modules->findText("Other")) {
        ui->lineEdit_moduleAddress->setText("");
        ui->lineEdit_moduleAddress->setCursorPosition(0);
        ui->lineEdit_moduleAddress->setFocus();
        ui->lineEdit_moduleNum->setText("");
        return;
    }

    int moduleNum = ui->comboBox_modules->itemData(pos).toInt();
    ui->lineEdit_moduleAddress->setText(QString("%2").arg(moduleNum, 2, 16, QChar('0')));
    ui->lineEdit_moduleNum->setText(QString("%1").arg(moduleNum, 2, 16, QChar('0')));
    if (kwp.getPortOpen() && kwp.getElmInitialised() && kwp.getChannelDest() < 0) {
        QMetaObject::invokeMethod(&kwp, "openChannel", Qt::QueuedConnection,
                                  Q_ARG(int, moduleNum));
    }
}

void MainWindow::on_lineEdit_moduleAddress_textChanged(const QString &arg1)
{
    bool ok;
    int moduleNum = arg1.toInt(&ok, 16);
    if (!ok) {
        return;
    }
    if (moduleNum == 0) {
        return;
    }

    int index = ui->comboBox_modules->findData(moduleNum);
    if (index < 0) {
        index = ui->comboBox_modules->findText("Other");
    }
    ui->comboBox_modules->setCurrentIndex(index);
}

void MainWindow::elmInitialised(bool ok)
{
    if (ok) {
        ui->statusBar->showMessage("ELM327 initialisation complete.");
        ui->pushButton_findModules->setEnabled(true);
        ui->pushButton_openModule->setEnabled(true);
    }
    else {
        ui->statusBar->showMessage("ELM327 initialisation failed.");
    }
}

void MainWindow::on_pushButton_findModules_clicked()
{
    QMetaObject::invokeMethod(&kwp, "openGW_refresh", Qt::QueuedConnection,
                              Q_ARG(bool, true));
}

void MainWindow::on_pushButton_readErrors_clicked()
{
    emit log("DiagSession = " + kwp.getDiagSession(), debugMsgLog);
    kwp.readErrors();
}

//void MainWindow::on_pushButton_sendOwn_clicked()
//{
//    bool ok = 0;
//    int parm1;
//    int parm2;

//    parm1 = ui->lineEdit_dataByte1->text().toInt(&ok, 16);
//    if (!ok)
//    {
//        emit log("Error in SendOwn");
//        return;
//    }

//    parm2 = ui->lineEdit_dataByte2->text().toInt(&ok, 16);
//    if (!ok)
//    {
//        emit log("Error in SendOwn");
//        return;
//    }

//    kwp.sendOwn(parm1, parm2);
//}

void MainWindow::on_pushButton_resetErrors_clicked()
{
    emit log("Delete Errors", debugMsgLog);
    kwp.deleteErrors();
}

void MainWindow::on_pushButton_connect_clicked()
{
    QMetaObject::invokeMethod(&kwp, "openPort", Qt::QueuedConnection);
    ui->pushButton_connect->setEnabled(false);

}

void MainWindow::on_pushButton_startDiag_clicked()
{
    QMetaObject::invokeMethod(&kwp, "startDiag", Qt::QueuedConnection,
                              Q_ARG(int, 0x89));
    emit log("startDiag clicked", debugMsgLog);

}

void MainWindow::portClosed()
{
    ui->pushButton_connect->setEnabled(true);
}

void MainWindow::on_pushButton_openModule_clicked()
{
    if (kwp.getChannelDest() >= 0) {
        QMetaObject::invokeMethod(&kwp, "closeChannel", Qt::QueuedConnection);
    }
    else {
        if (ui->lineEdit_moduleAddress->text() == "") {
            return;
        }
        bool ok;
        int moduleNum = ui->lineEdit_moduleAddress->text().toInt(&ok, 16);
        if (moduleNum == 0) {
            return;
        }
        if (kwp.getPortOpen() && kwp.getElmInitialised()) {
            QMetaObject::invokeMethod(&kwp, "openChannel", Qt::QueuedConnection,
                                      Q_ARG(int, moduleNum));
        }
    }
}

void MainWindow::on_actionSerial_port_settings_triggered()
{
    serSettings->show();
}

void MainWindow::channelOpen(bool status)
{
    if (status) {
        ui->pushButton_openModule->setIcon(QIcon(":/resources/green.png"));
        ui->pushButton_openModule->setText("Close Module");
        ui->lineEdit_moduleAddress->setEnabled(false);
        ui->comboBox_modules->setEnabled(false);
        ui->pushButton_readErrors->setEnabled(true);
        ui->pushButton_resetErrors->setEnabled(true);
        //ui->pushButton_sendOwn->setEnabled(true);
        ui->pushButton_startDiag->setEnabled(true);
    }
    else {
        ui->pushButton_openModule->setIcon(QIcon(":/resources/red.png"));
        ui->pushButton_openModule->setText("Open Module");
        ui->lineEdit_moduleAddress->setEnabled(true);
        ui->comboBox_modules->setEnabled(true);
        ui->pushButton_readErrors->setEnabled(false);
        ui->pushButton_resetErrors->setEnabled(false);
        //ui->pushButton_sendOwn->setEnabled(false);
        ui->pushButton_startDiag->setEnabled(false);
    }
}

void MainWindow::diagActive(bool status)
{
    if (status) {
        ui->pushButton_startDiag->setText("Stop Diag");
    }
    else {
        ui->pushButton_startDiag->setText(("Start Duag"));
    }
}


void MainWindow::moduleInfoReceived(ecuLongId ecuInfo, QStringList hwNum , QString vin, QString serNum)
{
    ui->lineEdit_moduleId->setText(ecuInfo.systemId);
    ui->lineEdit_swNum->setText(ecuInfo.swNum);
    ui->lineEdit_coding->setText(ecuInfo.coding);
    ui->lineEdit_swVers->setText(ecuInfo.swVers);
    QString shop;
    shop.append(QString("%1").arg(ecuInfo.shopNum[0], 5, 10, QChar('0')));
    shop.append(" ");
    shop.append(QString("%1").arg(ecuInfo.shopNum[1], 3, 10, QChar('0')));
    shop.append(" ");
    shop.append(QString("%1").arg(ecuInfo.shopNum[2], 5, 10, QChar('0')));
    ui->lineEdit_shopNum->setText(shop);
    ui->lineEdit_hwNum->setText(hwNum.join(" "));
    ui->lineEdit_vin->setText(vin);
    ui->lineEdit_serNum->setText(serNum);
}

void MainWindow::aboutdlg()
{
    QString abouttext = QString("<b>SpeckMobil %1 - Speckmarschall Diagnosis Software for VW cars</b> is more than a glass of beer.").arg(VER);
    QMessageBox::about(this, tr("About SpeckMobil"), abouttext);
}

void MainWindow::DTCReceived(DTC DTCs)
{
    ui->lineEdit_numDTC->setText(DTCs.sum);
    ui->lineEdit_DTCs->setText(DTCs.part);
}

void MainWindow::clearUI()
{
    ui->lineEdit_hwNum->clear();
    ui->lineEdit_coding->clear();
    ui->lineEdit_moduleId->clear();
    ui->lineEdit_shopNum->clear();
    ui->lineEdit_swVers->clear();
    ui->lineEdit_DTCs->clear();
    ui->lineEdit_numDTC->clear();
    ui->lineEdit_swNum->clear();
    ui->lineEdit_vin->clear();
    ui->lineEdit_serNum->clear();
}

void MainWindow::refreshModules(bool quickInit)
{
    if (quickInit) {
        ui->comboBox_modules->addItem("Engine", 1);
        ui->comboBox_modules->addItem("Transmission", 2);
        ui->comboBox_modules->addItem("Other");
        return;
    }

    QMap<int, moduleInfo_t> modules = kwp.getModuleList();
    QList<int> moduleNums = modules.keys();

    ui->comboBox_modules->clear();

    if (modules.empty()) {
        ui->comboBox_modules->addItem("Engine", 1);
        ui->comboBox_modules->addItem("Transmission", 2);
        ui->comboBox_modules->addItem("Other");
        return;
    }

    for (int i = 0; i < moduleNums.length(); i++) {
        int num = moduleNums.at(i);

        ui->comboBox_modules->addItem(modules[num].name, modules[num].number);

        switch (modules[num].status) {
        case 0x0: //0000 ->x000
            ui->comboBox_modules->setItemData(i, "Module OK", Qt::ToolTipRole);
            ui->comboBox_modules->setItemData(i, QBrush(Qt::darkGreen), Qt::ForegroundRole);
            break;
        case 0x2: //0010 ->x010
            ui->comboBox_modules->setItemData(i, "Malfunction", Qt::ToolTipRole);
            ui->comboBox_modules->setItemData(i, QBrush(Qt::darkRed), Qt::ForegroundRole);
            break;
        case 0x3: //0011
            ui->comboBox_modules->setItemData(i, "Not registered", Qt::ToolTipRole);
            ui->comboBox_modules->setItemData(i, QBrush(Qt::red), Qt::ForegroundRole);
            break;
        case 0xC: //1100 ->11xx
            ui->comboBox_modules->setItemData(i, "Can't be reached", Qt::ToolTipRole);
            ui->comboBox_modules->setItemData(i, QBrush(Qt::red), Qt::ForegroundRole);
            break;
        default:
            break;
        }
    }

    ui->comboBox_modules->addItem("Other");
}

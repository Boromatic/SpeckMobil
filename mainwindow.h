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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include "kwp2000.h"
#include "settings.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    friend class settings;
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    Ui::MainWindow *ui;
    serialSettingsDialog* serSettings;
    settings* settingsDialog;

    kwp2000 kwp;
    QSettings* appSettings;
    bool serialConfigured;
    int logLevel;
    QStringList logBuffer;

    void closeEvent(QCloseEvent *event);
    void changeEvent(QEvent *event);
    void saveSettings();
    void restoreSettings();
    void flushLogBuffer();

private slots:
    void log(const QString &txt, int logLevel = stdLog);
    void connectToSerial();
    void on_actionSerial_port_settings_triggered();
    void channelOpen(bool status);
    void diagActive(bool status);
    void selectNewModule(int pos);
    void on_pushButton_openModule_clicked();
    void on_pushButton_findModules_clicked();
    void on_pushButton_connect_clicked();
    void on_pushButton_readErrors_clicked();
    void on_pushButton_resetErrors_clicked();
    void on_pushButton_startDiag_clicked();
    //void on_pushButton_sendOwn_clicked();
    void on_lineEdit_moduleAddress_textChanged(const QString &arg1);
    void portClosed();
    void elmInitialised(bool ok);
    void moduleInfoReceived(ecuLongId ecuInfo, QStringList hwNum , QString vin, QString serNum);
    void DTCReceived(DTC DTCs);
    void clearUI();
    void refreshModules(bool quickInit = false);
    void updateSettings();
    void aboutdlg();
};

#endif // MAINWINDOW_H

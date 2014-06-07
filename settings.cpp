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

#include "settings.h"
#include "ui_settings.h"
#include "mainwindow.h"
#include "tp20.h"

settings::settings(QSettings* appSettings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settings),
    appSettings(appSettings),
    receiveTimeValidator(1, 255, this),
    keepAliveValidator(1, 10000, this)
{
    ui->setupUi(this);

    // TODO: Fix validators allowing 0
    ui->lineEdit_normal->setValidator(&receiveTimeValidator);
    ui->lineEdit_keepAliveInterval->setValidator(&keepAliveValidator);
}

settings::~settings()
{
    delete ui;
}

void settings::showEvent(QShowEvent* event)
{
    ui->lineEdit_normal->setText(QString::number(norm));
    ui->lineEdit_keepAliveInterval->setText(QString::number(keepAliveInterval));
}

void settings::on_buttonBox_accepted()
{
    norm = ui->lineEdit_normal->text().toUInt();
    keepAliveInterval = ui->lineEdit_keepAliveInterval->text().toUInt();

    save();
    emit settingsChanged();
}

void settings::on_buttonBox_rejected()
{

}

void settings::load()
{
    norm = appSettings->value("Timeouts/norm", 100).toInt();
    keepAliveInterval = appSettings->value("KeepAlive/interval", T_CTa).toInt();
}

void settings::save()
{
    appSettings->setValue("Timeouts/norm", norm);
    appSettings->setValue("KeepAlive/interval", keepAliveInterval);
    appSettings->sync();
}

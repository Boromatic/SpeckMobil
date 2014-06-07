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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QSettings>
#include <QIntValidator>

namespace Ui {
class settings;
}

class settings : public QDialog
{
    Q_OBJECT

public:
    explicit settings(QSettings* appSettings, QWidget *parent = 0);
    ~settings();

    void showEvent(QShowEvent *event);
    
    int slow, norm, fast;
    int keepAliveInterval;
    void load();
    void save();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::settings *ui;
    QSettings* appSettings;

    QIntValidator receiveTimeValidator;
    QIntValidator keepAliveValidator;
signals:
    void settingsChanged();
};

#endif // SETTINGS_H

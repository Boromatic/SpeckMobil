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


#ifndef CANFRAME_H
#define CANFRAME_H

#include <QObject>
#include <QByteArray>

class canFrame : public QObject
{
    Q_OBJECT
public:
    explicit canFrame(int canID, int length, const QByteArray &data, QObject *parent = 0);
    int canID;
    int length;
    QByteArray data;
};

#endif // CANFRAME_H

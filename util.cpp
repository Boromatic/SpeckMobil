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

#include "util.h"

QString toHex(int num, int places)
{
    return QString("%1").arg(num, places, 16, QChar('0')).toUpper();
}

QString toHex(unsigned int num, int places)
{
    return QString("%1").arg(num, places, 16, QChar('0')).toUpper();
}

QString intToBinary(int num, int places)
{
    return QString("%1").arg(num, places, 2, QChar('0'));
}

QString uintToBinary(unsigned int num, int places)
{
    return QString("%1").arg(num, places, 2, QChar('0'));
}

QString doubleToStr(double num, int prec)
{
    return QString("%1").arg(num, 0, 'f', prec);
}

int fromHex(QString str)
{
    bool ok;
    return str.toInt(&ok, 16);
}

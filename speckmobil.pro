#
#SpeckMobil - Experimental TP2.0-KWP2000 Software

#Copyright (C) 2014 Matthias Amberg

#Derived from VAG Blocks, Copyright 2013 Jared Wiltshire

#SpeckMobil is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#any later version.

#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

QT      += core gui \
        widgets \
        serialport

TARGET  = SpeckMobil
TEMPLATE = app

VERSION = 0.1

SOURCES += main.cpp \
        mainwindow.cpp \
        elm327.cpp \
        tp20.cpp \
        canframe.cpp \
        kwp2000.cpp \
        util.cpp \
        serialsettings.cpp \
        settings.cpp

HEADERS += mainwindow.h \
        elm327.h \
        tp20.h \
        canframe.h \
        kwp2000.h \
        util.h \
        serialsettings.h \
        settings.h

FORMS   += mainwindow.ui \
        serialsettings.ui \
        settings.ui

RESOURCES += \
        icons.qrc

RC_FILE += resources/icon.rc

OTHER_FILES += \
    todo.txt

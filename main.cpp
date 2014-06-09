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

#include <QtWidgets/QApplication>
#include <QMessageBox>
#include "mainwindow.h"

int main(int argc, char *argv[])
{

    QApplication application(argc, argv);
    QMessageBox msgBox;
    msgBox.setText(MainWindow::tr("<b>SpeckMobil - Speckmarschall Diagnosis Software for VW cars</b><br><br>This is experimental software! "
                   "It is tested only on a VW Golf Goal MK V. Use it with caution and respect your country's law,"
                   " especially when working with airbags, brakes, etc.<br>"
                   "This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY."));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.addButton("OK", QMessageBox::YesRole);
    msgBox.exec();
    QCoreApplication::setOrganizationName("Speckmarschall");
    QCoreApplication::setOrganizationDomain("speckmarschall.de");
    QCoreApplication::setApplicationName("SpeckMobil");

    MainWindow window;
    window.show();
    
    return application.exec();
}

#-------------------------------------------------
#
# Project created by QtCreator 2011-11-22T10:09:14
#
#-------------------------------------------------
#
#   This file is part of QSTLink2.
#
#   QSTLink2 is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   QSTLink2 is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with QSTLink2.  If not, see <http://www.gnu.org/licenses/>.

QT       += core gui xml

TARGET = qstlink2
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    stlinkv2.cpp \
    LibUsb.cpp \
    devices.cpp \
    dialog.cpp \
    transferthread.cpp

HEADERS  += mainwindow.h \
    stlinkv2.h \
    LibUsb.h \
    devices.h \
    dialog.h \
    transferthread.h

FORMS    += mainwindow.ui \
    dialog.ui

LIBS += -L/usr/lib -lusb





















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
win32:CONFIG += console

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

unix:LIBS += -L/usr/lib -lusb
win32:LIBS += -L"$$_PRO_FILE_PWD_/libs/" -llibusb

RESOURCES += res/ressources.qrc

# Icon for windows
win32:RC_FILE = qstlink2.rc
# OSX
ICON = res/images/icon.icns

TARGET = qstlink2
target.path = /usr/bin
INSTALLS += target

conf.path = /etc/udev/rules.d
conf.files = 49-stlinkv2.rules
INSTALLS += conf

misc.path = /usr/share/qstlink2
misc.files = res/devices.xml res/help.html
INSTALLS += misc


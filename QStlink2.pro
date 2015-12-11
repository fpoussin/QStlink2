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

QT += core gui xml
contains(QT_MAJOR_VERSION, 5) { QT += widgets }
#win32:CONFIG += console
win32:CONFIG += winusb

TEMPLATE = app
TARGET = qstlink2
VERSION = 1.2.2

message(Building version $$VERSION for Qt $$QT_VERSION)

VERSTR = '\\"$${VERSION}\\"'  # place quotes around the version string
DEFINES += __QSTL_VER__=\"$${VERSTR}\" # create a VER macro containing the version string

INCLUDEPATH += $$PWD/inc

FORMS += ui/mainwindow.ui \
    ui/dialog.ui

SOURCES += src/main.cpp\
        src/mainwindow.cpp \
    src/stlinkv2.cpp \
    src/devices.cpp \
    src/dialog.cpp \
   src/transferthread.cpp \
    src/loader.cpp

HEADERS  += inc/mainwindow.h \
    inc/stlinkv2.h \
    inc/devices.h \
    inc/dialog.h \
    inc/transferthread.h \
    inc/compat.h \
    inc/loader.h

include(QtUsb/src/QtUsb.pri)

win32 {
    TARGET = qstlink2_$${VERSION}
    CONFIG(console) {
        message(Building with debug console.)
        TARGET = qstlink2_console_$${VERSION}
    }
}

RESOURCES += res/ressources.qrc loaders/loaders.qrc

# Icon for windows
win32:RC_FILE = res/qstlink2.rc
# OSX
macx:ICON = res/images/icon.icns

target.path = /usr/bin
INSTALLS += target

conf.path = /etc/udev/rules.d
conf.files = 49-stlinkv2.rules
INSTALLS += conf

misc.path = /usr/share/qstlink2
misc.files = res/devices.xml res/help.html
INSTALLS += misc

unix:!macx {
    icon.path = /usr/share/pixmaps
    icon.files = res/images/qstlink2.png
    INSTALLS += icon

    launcher.path = /usr/share/applications
    launcher.files = res/qstlink2.desktop
    INSTALLS += launcher
}

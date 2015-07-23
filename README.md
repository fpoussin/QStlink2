**QSTLink2**
========

**Description**  

QSTLink2 is a cross-platform STLinkV2 GUI.

It can perform the following actions:
 - Write
 - Read
 - Verify
 - Erase

**Downloads**

Windows binaries:  
 - https://drive.google.com/folderview?id=0BzWyTGWIwcYQS0JRQkw2Q3J6R1U&usp=sharing 
 - https://github.com/fpoussin/QStlink2/releases  

Ubuntu PPA: 
 - https://launchpad.net/~fpoussin/+archive/ppa  

**Building on Linux**

*Build dependencies for Qt4:*
 - libusb-1.0
 - QtUSB (included)
 - qt4-qmake
 - qt4-dev
 - libqt4-gui
 - libqt4-xml

*Build dependencies for Qt5:*
 - libusb-1.0
 - QtUSB (included)
 - qt5-qmake
 - qtbase5-dev
 - libqt5gui5
 - libqt5xml5
 
*Build steps:*

    git clone git@github.com:fpoussin/QStlink2.git
    git submodule update --init
    qmake
    make
    sudo make install  # Optional


**Building in Windows**

You will need to run these commands from MSVC's CLI (I use MSVC2010)
You might need to install windows driver SDK (WDK) to get WinUSB libs/headers.
You can also build from Qt Creator

I recommend that you build/download a static version of Qt for Windows.

*Dependencies:*
 - MSVC 2008 or better
 - WinUsb (WDK)
 - QtUSB (included)
 - Qt 5.2 SDK or better recommended

*Build steps:*

    git clone git@github.com:fpoussin/QStlink2.git
    git submodule update --init
    qmake
    nmake


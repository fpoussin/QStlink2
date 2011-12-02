/*
This file is part of QSTLink2.

    QSTLink2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QSTLink2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QSTLink2.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DEVICES_H
#define DEVICES_H

#include <QObject>
#include <QDomDocument>
#include <QFile>
#include <QDebug>
#include <QString>
#include <QVector>

#define qInformal() qWarning()

class Device : public QObject
{
    Q_OBJECT
public:
    explicit Device(QObject *parent = 0);
    QString type;
    quint32 core_id;
    quint32 chip_id;
    quint32 flash_base;
    quint32 flash_size_reg;
    quint32 flash_size;
    quint32 flash_pgsize;
    quint32 sysflash_base;
    quint32 sysflash_size;
    quint32 sysflash_pgsize;
    quint32 sram_base;
    quint32 sram_size;
};

class DeviceList : public QObject
{
    Q_OBJECT
public:
    explicit DeviceList(QObject *parent = 0);
    bool IsLoaded();
    quint16 getDevicesCount();
    bool search(const quint32 chip_id); // returns the location of the device in the QVector.
    Device *cur_device;

private:
    QDomDocument *doc;
    bool loaded;
    QVector<Device*> devices;
    Device *default_device;
};

#endif // DEVICES_H

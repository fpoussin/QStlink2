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
#include <QMap>
#include <compat.h>

class Device : public QObject
{
    Q_OBJECT
public:
    explicit Device(QObject *parent = 0);
    explicit Device(Device *device); // Copy contructor
    quint32 operator[] (QString x) const { return mMap[x]; }
    quint32& operator[] (QString x) { return mMap[x]; }
    bool contains(QString x) { return mMap.contains(x); }
    QString repr(void) const;
    QString mType;
    QString mLoaderFile;
private:
    QMap<QString, quint32> mMap;
};

class DeviceList : public QObject
{
    Q_OBJECT
public:
    explicit DeviceList(QObject *parent = 0);
    bool IsLoaded() const;
    quint16 getDevicesCount() const;
    bool search(const quint32 chip_id); // returns the location of the device in the QVector.
    Device *mCurDevice;

private:
    QDomDocument *mDoc;
    bool mLoaded;
    QVector<Device*> mDevices;
    Device *mDefaultDevice;
};

#endif // DEVICES_H

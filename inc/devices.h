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
#include "compat.h"

/**
 * @brief
 *
 */
class DeviceInfo : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     *
     * @param parent
     */
    explicit DeviceInfo(QObject *parent = 0);
    /**
     * @brief Copy constructor
     *
     * @param device Object you are copying
     */
    explicit DeviceInfo(const DeviceInfo *device);
    /**
     * @brief
     *
     * @param k
     * @return quint32 operator
     */
    quint32 operator[] (QString k) const { return mMap[k]; }
    /**
     * @brief
     *
     * @param k
     * @return quint32 &operator
     */
    quint32& operator[] (QString k) { return mMap[k]; }
    /**
     * @brief
     *
     * @param k
     * @return quint32
     */
    quint32 value (QString k) { return mMap.value(k); }
    /**
     * @brief
     *
     * @param k
     * @param v
     */
    void insert (QString k, quint32 v) { mMap.insert(k, v); }
    /**
     * @brief
     *
     * @param x
     * @return bool
     */
    bool contains(QString x) { return mMap.contains(x); }
    /**
     * @brief
     *
     * @return QString
     */
    QString repr(void) const;
    QString mType; /**< device type */
    QString mLoaderFile; /**< associated loader bin file */
private:
    QMap<QString, quint32> mMap; /**< values map */
};

/**
 * @brief
 *
 */
class DeviceInfoList : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     *
     * @param parent
     */
    explicit DeviceInfoList(QObject *parent = 0);
    /**
     * @brief
     *
     * @return bool true if has finished loading.
     */
    bool IsLoaded() const;
    /**
     * @brief count devices.
     *
     * @return quint16 devices count.
     */
    quint16 getDevicesCount() const;
    /**
     * @brief Checks if the chip Id is in the list.
     *
     * @param chip_id ST's chip ID.
     * @return bool true if device is in the list.
     */
    bool search(const quint32 chip_id);
    DeviceInfo *mCurDevice; /**< ptr to current device */

private:
    QDomDocument *mDoc; /**< XML document */
    bool mLoaded; /**< loaded status */
    QVector<DeviceInfo*> mDevices; /**< devices list */
    DeviceInfo *mDefaultDevice; /**< default devices (to copy default values from) */
};

#endif // DEVICES_H

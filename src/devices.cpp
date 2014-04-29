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
#include "devices.h"

Device::Device(QObject *parent) :
    QObject(parent)
{
    this->mType = "UNKNOWN";
    this->mLoaderFile = "UNKNOWN";
}

Device::Device(Device *device)
{
    this->mType = device->mType;
    this->mMap = device->mMap;
}

DeviceList::DeviceList(QObject *parent) :
    QObject(parent)
{
    this->mLoaded = false;
    qDebug("Loading device list.");
    this->mDoc = new QDomDocument("stlink");
    QFile file("/usr/share/qstlink2/devices.xml");
    if (!file.open(QIODevice::ReadOnly)) {
        qInformal() << "Could not open the devices.xml file. Using internal data.";
        file.setFileName(":/devices.xml");
    }
    if (!mDoc->setContent(&file)) {
        file.close();
        qCritical() << "Devices list failed to load.";
        return;
    }
    file.close();
    qInformal() << "Devices list loaded.";

    this->mDefaultDevice = new Device(this);
    bool isInt;
    QDomElement docElem = mDoc->documentElement();
    QDomNode n = docElem.firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if(!e.isNull()) {

            if (e.tagName() == "devices_default") {
                QDomNodeList childs = e.childNodes();
                for (int i = 0;i < childs.count();i++) {
                    QDomElement el = childs.at(i).toElement();
                    qDebug() << e.tagName() << "->" << el.tagName();
                    (*this->mDefaultDevice)[el.tagName()] = (quint32)el.text().toUInt(&isInt, 16);
                    if (!isInt)
                        qCritical() << el.tagName() << "Failed to parse number!";
                }
            }
            else if (e.tagName() == "regs_default") {
                QDomNodeList regs = e.childNodes();
                for (int a = 0;a < regs.count(); a++) {
                    QDomElement el = regs.at(a).toElement();
                    qDebug() << el.tagName() << "->" << el.text().toUInt(0, 16);

                    (*this->mDefaultDevice)[el.tagName()] = (quint32)el.text().toUInt(&isInt, 16);
                    if (!isInt)
                        qCritical() << el.tagName() << "Failed to parse number!";
                }
            }
        }
        n = n.nextSibling();
    }

    n = docElem.firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if(!e.isNull()) {

            if (e.tagName() == "devices") {
                QDomNodeList devices = e.childNodes();
                for (int a = 0;a < devices.count(); a++) {
                    this->mDevices.append(new Device(this->mDefaultDevice)); // Copy from the default device.


                    QDomElement device = devices.at(a).toElement();
                    this->mDevices.last()->mType = device.attribute("type");

                    QDomNodeList childs = device.childNodes();
                    for (int i = 0;i < childs.count();i++) {
                        QDomElement el = childs.at(i).toElement();
                        qDebug() << device.tagName() << "->" << device.attribute("type") << "->" << el.tagName() ;
                        (*this->mDevices.last())[el.tagName()] = (quint32)el.text().toUInt(&isInt, 16);
                        if (!isInt && el.tagName() != "loader")
                            qCritical() << el.tagName() << "Failed to parse number!";

                        if (el.tagName() == "loader") {
                            this->mDevices.last()->mLoaderFile = el.text();
                        }
                    }
                }
            }

        }
        n = n.nextSibling();
    }

    this->mLoaded = true;
    return;
}

bool DeviceList::IsLoaded() const {

    return this->mLoaded;
}

bool DeviceList::search(const quint32 chip_id) {
    qDebug() << "Looking for:" << QString::number(chip_id, 16);
    for (int i=0; i < this->mDevices.count(); i++) {
        if ((*this->mDevices.at(i))["chip_id"] == chip_id) {
            this->mCurDevice = this->mDevices.at(i);
            qDebug() << "Found chipID";
            qDebug() << mCurDevice->repr();
            return true;
        }
    }
    qCritical() << "Did not find chipID!";
    return false;
}

quint16 DeviceList::getDevicesCount() const {

    return this->mDevices.count();
}

QString Device::repr(void) const {

    QMapIterator<QString, quint32> i(mMap);
    QString tmp("\r\n");
    tmp.append("Type: "+this->mType);
    while (i.hasNext()) {
        i.next();
        tmp.append("\r\n" + QString(i.key()) + ": 0x" + QString::number(i.value(), 16));
    }
    tmp.append("\r\nLoader: " + this->mLoaderFile);
    return tmp;
}


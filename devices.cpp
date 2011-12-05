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
    this->type = "UNKNOWN";
    this->core_id = 0;
    this->chip_id = 0;
    this->flash_base = 0;
    this->flash_size = 0;
    this->flash_size_reg = 0;
    this->flash_pgsize = 0;
    this->sysflash_base = 0;
    this->sysflash_size = 0;
    this->sysflash_pgsize = 0;
    this->sram_base = 0;
    this->sram_size = 0;
}

DeviceList::DeviceList(QObject *parent) :
    QObject(parent)
{
    this->loaded = false;
    qDebug("Loading device list.");
    this->doc = new QDomDocument("stlink");
    QFile file("/usr/share/qstlink2/devices.xml");
    if (!file.open(QIODevice::ReadOnly)) {
        qInformal() << "Could not open the devices.xml file. Using internal file.";
        file.setFileName(":/devices.xml");
    }
    if (!doc->setContent(&file)) {
        file.close();
        qCritical() << "Devices list failed to load.";
        return;
    }
    file.close();
    qInformal() << "Devices list loaded.";

    QDomElement docElem = doc->documentElement();
    QDomNode n = docElem.firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if(!e.isNull()) {
            qDebug() << e.tagName();

            if (e.tagName() == "default") {
                QDomNodeList childs = e.childNodes();
                this->default_device = new Device(this);
                for (int i = 0;i < childs.count();i++) {
                    QDomElement el = childs.at(i).toElement();
                    qDebug() << e.tagName() << "->" << el.tagName();
                    if (el.tagName() == "core_id")
                        this->default_device->core_id = el.text().toInt(0, 16);
                    else if (el.tagName() == "chip_id")
                        this->default_device->chip_id = el.text().toInt(0, 16);
                    else if (el.tagName() == "flash_base")
                        this->default_device->flash_base = el.text().toInt(0, 16);
                    else if (el.tagName() == "flash_size")
                        this->default_device->flash_size = el.text().toInt(0, 16);
                    else if (el.tagName() == "flash_size_reg")
                        this->default_device->flash_size_reg = el.text().toInt(0, 16);
                    else if (el.tagName() == "flash_pgsize")
                        this->default_device->flash_pgsize = el.text().toInt(0, 16);
                    else if (el.tagName() == "sysflash_base")
                        this->default_device->sysflash_base = el.text().toInt(0, 16);
                    else if (el.tagName() == "sysflash_size")
                        this->default_device->sysflash_size = el.text().toInt(0, 16);
                    else if (el.tagName() == "sysflash_pgsize")
                        this->default_device->sysflash_pgsize = el.text().toInt(0, 16);
                    else if (el.tagName() == "sram_base")
                        this->default_device->sram_base = el.text().toInt(0, 16);
                    else if (el.tagName() == "sram_size")
                        this->default_device->sram_size = el.text().toInt(0, 16);
                }
            }
            else if (e.tagName() == "devices") {
                QDomNodeList devices = e.childNodes();
                for (int a = 0;a < devices.count(); a++) {
                    this->devices.append(new Device(this));

                    // appending default values from the xml <default> to all devices.
                    this->devices.last()->core_id = this->default_device->core_id;
                    this->devices.last()->chip_id = this->default_device->chip_id;
                    this->devices.last()->flash_base = this->default_device->flash_base;
                    this->devices.last()->flash_size = this->default_device->flash_size;
                    this->devices.last()->flash_size_reg = this->default_device->flash_size_reg;
                    this->devices.last()->flash_pgsize = this->default_device->flash_pgsize;
                    this->devices.last()->sysflash_base = this->default_device->sysflash_base;
                    this->devices.last()->sysflash_size = this->default_device->sysflash_size;
                    this->devices.last()->sysflash_pgsize = this->default_device->sysflash_pgsize;
                    this->devices.last()->sram_base = this->default_device->sram_base;
                    this->devices.last()->sram_size = this->default_device->sram_size;

                    QDomElement device = devices.at(a).toElement();
                    this->devices.last()->type = device.attribute("type");

                    QDomNodeList childs = device.childNodes();
                    for (int i = 0;i < childs.count();i++) {
                        QDomElement el = childs.at(i).toElement();
                        qDebug() << device.tagName() << "->" << device.attribute("type") << "->" << el.tagName() ;
                        if (el.tagName() == "core_id")
                            this->devices.last()->core_id = el.text().toInt(0, 16);
                        else if (el.tagName() == "chip_id")
                            this->devices.last()->chip_id = el.text().toInt(0, 16);
                        else if (el.tagName() == "flash_base")
                            this->devices.last()->flash_base = el.text().toInt(0, 16);
                        else if (el.tagName() == "flash_size")
                            this->devices.last()->flash_size = el.text().toInt(0, 16);
                        else if (el.tagName() == "flash_size_reg")
                            this->devices.last()->flash_size_reg = el.text().toInt(0, 16);
                        else if (el.tagName() == "flash_pgsize")
                            this->devices.last()->flash_pgsize = el.text().toInt(0, 16);
                        else if (el.tagName() == "sysflash_base")
                            this->devices.last()->sysflash_base = el.text().toInt(0, 16);
                        else if (el.tagName() == "sysflash_size")
                            this->devices.last()->sysflash_size = el.text().toInt(0, 16);
                        else if (el.tagName() == "sysflash_pgsize")
                            this->devices.last()->sysflash_pgsize = el.text().toInt(0, 16);
                        else if (el.tagName() == "sram_base")
                            this->devices.last()->sram_base = el.text().toInt(0, 16);
                        else if (el.tagName() == "sram_size")
                            this->devices.last()->sram_size = el.text().toInt(0, 16);
                    }
                }
            }
        }
        n = n.nextSibling();
    }
    this->loaded = true;
    return;
}

bool DeviceList::IsLoaded() {

    return this->loaded;
}

bool DeviceList::search(const quint32 chip_id) {
    qDebug() << "Looking for:" << chip_id;
    for (int i=0; i < this->devices.count(); i++) {
        if (this->devices.at(i)->chip_id == chip_id) {
            this->cur_device = this->devices.at(i);
            qDebug() << "Found chipID";
            return true;
        }
    }
    qDebug() << "Did not find chipID!";
    return false;
}

quint16 DeviceList::getDevicesCount()
{
    return this->devices.count();
}

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
#include "version.h"

svnversion::svnversion(QObject *parent) :
    QObject(parent)
{
    qDebug("Loading device list.");
    QDomDocument doc("info");
    QFile file(":/svn-info.xml");
    if (!file.open(QIODevice::ReadOnly)) {
        qInformal() << "Could not open the internal svn-info.xml file. ";
        file.close();
        return;
    }
    if (!doc.setContent(&file)) {
        file.close();
        qCritical() << "SVN info failed to load.";
        return;
    }
    file.close();
    qInformal() << "SVN info loaded.";

    QDomElement docElem = doc.documentElement();
    QDomNode n = docElem.firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if(!e.isNull()) {
            qDebug() << e.tagName();

            if (e.tagName() == "entry") {

                this->m_revision = e.attribute("revision").toInt();
//                QDomNodeList entry = e.childNodes();
//                for (int a = 0;a < entry.count(); a++) {
//                    QDomElement el = entry.at(a).toElement();
//                    qDebug() << el.tagName() << "->" << el.text();

//                }
            }
        }
        n = n.nextSibling();
    }
}

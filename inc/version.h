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
#ifndef VERSION_H
#define VERSION_H

#include <QObject>
#include <QDomDocument>
#include <QFile>
#include <QDebug>
#include <QString>
#include <compat.h>

class svnversion : public QObject
{
    Q_OBJECT
public:
    explicit svnversion(QObject *parent = 0);
    QString getBuildDate(void) const { return QString(__DATE__)+" "+QString(__TIME__); }
    quint32 getRevision(void) const { return m_revision; }
    
signals:
    
public slots:

private:
    quint32 m_revision;
};

#endif // VERSION_H

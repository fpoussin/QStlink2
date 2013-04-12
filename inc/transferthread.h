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
#ifndef TRANSFERTHREAD_H
#define TRANSFERTHREAD_H

#include <QThread>
#include <QDebug>
#include <QString>
#include <stlinkv2.h>
#include <compat.h>

class transferThread : public QThread
{
    Q_OBJECT
public:
    explicit transferThread(QObject *parent = 0);
    void run();
    void setParams(stlinkv2 *stlink, QString filename, bool write, bool verify);

signals:
    void sendProgress(quint32 p);
    void sendStatus(const QString &s);
    void sendLoaderStatus(const QString &s);
    void sendError(const QString &s);
    void sendLock(bool enabled);
    void sendLog(const QString &s);

public slots:
    void halt();

private slots:

private:
    void sendWithLoader(const QString &filename);
    void receive(const QString &filename);
    void verify(const QString &filename);

    QString m_filename;
    bool m_write;
    stlinkv2 *stlink;
    bool m_stop;
    bool m_erase;
    bool m_verify;
};

#endif // TRANSFERTHREAD_H

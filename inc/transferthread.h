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

/**
 * @brief
 *
 */
class transferThread : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief
     *
     * @param parent
     */
    explicit transferThread(QObject *parent = 0);
    /**
     * @brief
     *
     */
    void run();
    /**
     * @brief
     *
     * @param mStlink
     * @param filename
     * @param write
     * @param verify
     */
    void setParams(stlinkv2 *mStlink, QString filename, bool write, bool verify);

signals:
    /**
     * @brief
     *
     * @param p
     */
    void sendProgress(quint32 p);
    /**
     * @brief
     *
     * @param s
     */
    void sendStatus(const QString &s);
    /**
     * @brief
     *
     * @param s
     */
    void sendLoaderStatus(const QString &s);
    /**
     * @brief
     *
     * @param s
     */
    void sendError(const QString &s);
    /**
     * @brief
     *
     * @param enabled
     */
    void sendLock(bool enabled);
    /**
     * @brief
     *
     * @param s
     */
    void sendLog(const QString &s);

public slots:
    /**
     * @brief
     *
     */
    void halt();

private slots:

private:
    /**
     * @brief
     *
     * @param filename
     */
    void sendWithLoader(const QString &filename);
    /**
     * @brief
     *
     * @param filename
     */
    void receive(const QString &filename);
    /**
     * @brief
     *
     * @param filename
     */
    void verify(const QString &filename);

    QString mFilename; /**< TODO: describe */
    bool mWrite; /**< TODO: describe */
    stlinkv2 *mStlink; /**< TODO: describe */
    bool mStop; /**< TODO: describe */
    bool mErase; /**< TODO: describe */
    bool mVerify; /**< TODO: describe */
};

#endif // TRANSFERTHREAD_H

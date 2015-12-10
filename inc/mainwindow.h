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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QFileDialog>
#include <QFile>
#include <QByteArray>
#include <QMessageBox>

#include "stlinkv2.h"
#include "devices.h"
#include "dialog.h"
#include "transferthread.h"
#include "compat.h"

namespace Ui {
    class MainWindow;
}

/**
 * @brief
 *
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

    /**
     * @brief
     *
     */
    enum {
        ACTION_NONE = 0,
        ACTION_SEND = 1,
        ACTION_RECEIVE = 2,
        ACTION_VERIFY = 3
    };

public:
    /**
     * @brief
     *
     * @param parent
     */
    explicit MainWindow(QWidget *parent = 0);
    /**
     * @brief
     *
     */
    ~MainWindow();
    transferThread *mTfThread; /**< TODO: describe */

public slots:
    /**
     * @brief
     *
     * @return bool
     */
    bool connect();
    /**
     * @brief
     *
     */
    void disconnect();
    /**
     * @brief
     *
     * @param p
     */
    void updateProgress(quint32 p);
    /**
     * @brief
     *
     * @param s
     */
    void updateStatus(const QString &s);
    /**
     * @brief
     *
     * @param s
     */
    void updateLoaderStatus(const QString &s);
    /**
     * @brief
     *
     * @param p
     */
    void updateLoaderPct(quint32 p);
    /**
     * @brief
     *
     * @param path
     */
    void send(const QString &path);
    /**
     * @brief
     *
     * @param path
     */
    void receive(const QString &path);
    /**
     * @brief
     *
     * @param path
     */
    void verify(const QString &path);
    /**
     * @brief
     *
     */
    void eraseFlash();
    /**
     * @brief
     *
     */
    void showHelp();
    /**
     * @brief
     *
     * @param event
     */
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *mUi; /**< TODO: describe */
    Dialog mDialog; /**< TODO: describe */
    stlinkv2 *mStlink; /**< TODO: describe */
    DeviceInfoList *mDevices; /**< TODO: describe */
    QString mFilename; /**< TODO: describe */
    QString mUsername; /**< TODO: describe */
    quint32 mLastAction; /**< TODO: describe */

private slots:
    /**
     * @brief
     *
     * @param enabled
     */
    void lockUI(bool enabled);
    /**
     * @brief
     *
     * @param s
     */
    void log(const QString &s);
    /**
     * @brief
     *
     */
    void getVersion();
    /**
     * @brief
     *
     */
    void getMode();
    /**
     * @brief
     *
     * @return bool
     */
    bool getMCU();
    /**
     * @brief
     *
     */
    void getStatus();
    /**
     * @brief
     *
     */
    void send();
    /**
     * @brief
     *
     */
    void receive();
    /**
     * @brief
     *
     */
    void verify();
    /**
     * @brief
     *
     */
    void repeat();
    /**
     * @brief
     *
     */
    void resetMCU();
    /**
     * @brief
     *
     */
    void hardReset();
    /**
     * @brief
     *
     */
    void runMCU();
    /**
     * @brief
     *
     */
    void haltMCU();
    /**
     * @brief
     *
     */
    void setModeJTAG();
    /**
     * @brief
     *
     */
    void setModeSWD();
    /**
     * @brief
     *
     */
    void quit();

};

#endif // MAINWINDOW_H

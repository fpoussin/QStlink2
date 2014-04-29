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
#include <stlinkv2.h>
#include <devices.h>
#include <QString>
#include <QFileDialog>
#include <QFile>
#include <QByteArray>
#include <dialog.h>
#include <transferthread.h>
#include <compat.h>
#include <QMessageBox>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum {
        ACTION_NONE = 0,
        ACTION_SEND = 1,
        ACTION_RECEIVE = 2,
        ACTION_VERIFY = 3
    };

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    transferThread *tfThread;

public slots:
    bool Connect();
    void Disconnect();
    void updateProgress(quint32 p);
    void updateStatus(const QString &s);
    void updateLoaderStatus(const QString &s);
    void updateLoaderPct(quint32 p);
    void Send(const QString &path);
    void Receive(const QString &path);
    void Verify(const QString &path);
    void eraseFlash();
    void showHelp();
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *mUi;
    Dialog mDialog;
    stlinkv2 *mStlink;
    DeviceList *mDevices;
    QString mFilename;
    QString mUsername;
    quint32 mLastAction;

private slots:
    void lockUI(bool enabled);
    void log(const QString &s);
    void getVersion();
    void getMode();
    bool getMCU();
    void getStatus();
    void Send();
    void Receive();
    void Verify();
    void Repeat();
    void ResetMCU();
    void HardReset();
    void RunMCU();
    void HaltMCU();
    void setModeJTAG();
    void setModeSWD();
    void Quit();

};

#endif // MAINWINDOW_H

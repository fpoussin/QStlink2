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
#include "dialog.h"
#include "transferthread.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    transferThread *tfThread;

public slots:
    void Connect();
    void Disconnect();
    void updateProgress(quint32 p);
    void updateStatus(QString s);
    void Send(QString path);
    void Receive(QString path);

private:
    Ui::MainWindow *ui;
    Dialog dialog;
    stlinkv2 *stlink;
    DeviceList *devices;
    QString filename;
    QString username;
    bool isroot;

private slots:
    void lockUI(bool enabled);
    void log(QString s);
    void getVersion();
    void getMode();
    void getMCU();
    void getStatus();
    void Send();
    void Receive();
    void ResetMCU();
    void RunMCU();
    void HaltMCU();
    void setModeJTAG();
    void setModeSWD();

};

#endif // MAINWINDOW_H

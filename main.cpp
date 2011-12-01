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
#include <QtGui/QApplication>
#include "mainwindow.h"
#include <QStringList>

quint8 verbose_level = 2;
QStringList args;
bool show = true;
bool flash = false;
bool wr = false;
QString path;

void myMessageOutput(QtMsgType type, const char *msg)
{
    if (verbose_level) {
        switch (type) {
        case QtFatalMsg:
                fprintf(stderr, "Fatal: %s\n", msg);
                abort();
        case QtCriticalMsg:
            if (verbose_level >= 2)
                fprintf(stderr, "Error: %s\n", msg);
            break;
        case QtWarningMsg:
            if (verbose_level >= 3)
                fprintf(stderr, "Warning: %s\n", msg);
            break;
        case QtDebugMsg:
            if (verbose_level >= 4)
                fprintf(stdout, "Debug: %s\n", msg);
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    args = QCoreApplication::arguments();
    foreach (const QString &str, args) {
             if (str.contains("quiet"))
                 verbose_level = 0;
             else if (str.contains("error"))
                 verbose_level = 2;
             else if (str.contains("warn"))
                 verbose_level = 3;
             else if (str.contains("debug"))
                 verbose_level = 4;
             if (str.contains("cli"))
                 show = false;
             if (str.contains("flash")) {
                 if (str.contains("write")) {
                    wr = true;
                 }
                 else
                     wr = false;
                 flash = true;
                 path = str.split(":").last();
             }
         }

    qDebug() << "Verbose level:" << verbose_level;
    qInstallMsgHandler(myMessageOutput);
    MainWindow *w = new MainWindow;
    if (show)
        w->show();

    else {

        if (QString(getenv("USER")) != "root") {
            qDebug("You need to run the program as root in order to access USB subsystems. Use sudo.");
            return 1;
        }

        if (flash && !path.isEmpty()) {

            qDebug() << "File Path:" << path;
            w->Connect();
            if (wr)
                w->Send(path);
            else
                w->Receive(path);

            sleep(1);
            while (w->tfThread->isRunning())
                usleep(100000);
            w->Disconnect();
            w->close();
            delete w;
            return 0;
            }

            else {
                qCritical() << endl << "Missing parameters" << endl
                << "Command line usage:" << endl
                         << "cli: Enable command line mode" << endl
                         << "flash:[read|write]:PATH = Manual dumping/flashing of the device" << endl
                        << "debug: Enable debug output" << endl;
                w->close();
                delete w;
                return 0;
            }

    }

    return a.exec();
}

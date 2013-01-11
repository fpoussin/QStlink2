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
#include <QDebug>
#include <QFile>

#ifdef WIN32
#define usleep(num) Sleep(num/1000)
#endif

#define QtInfoMsg QtWarningMsg // Little hack to have an "info" level of output.

quint8 verbose_level = 5; // Level = info by default
bool show = true;
bool write_flash, read_flash, erase, verify = false;
QString path;

void myMessageOutput(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtFatalMsg: // Always print!
            fprintf(stderr, "Fatal: %s\n", msg);
            abort();
    case QtCriticalMsg:
        if (verbose_level >= 1)
            fprintf(stderr, "Error: %s\n", msg);
        break;
    case QtInfoMsg: // Since there is no "Info" level, we use qWarning which we alias with #define...
        if (verbose_level >= 2)
            fprintf(stdout, "Info: %s\n", msg);
        break;
    case QtDebugMsg:
        if (verbose_level >= 5)
            fprintf(stdout, "Debug: %s\n", msg);
        break;
    }
}

void showHelp()
{
    qCritical() << "Missing parameters, help below:";
    QFile help_file(":/help.html");
    if (!help_file.open(QIODevice::ReadOnly))
        return;
    QString help = help_file.readAll();
    help_file.close();
    qInformal() << help.remove(QRegExp("(<[^>]+>)|\t\b")); // Clearing HTML tags.
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    quint8 i = 0;
    QStringList args = QCoreApplication::arguments();
    QRegExp path_reg("[\\/]"); // Checks for a Unix or Windows path.
    foreach (const QString &str, args) {

            if (!i++) // Skip first one
                continue;

//            if (i >= args.size()) // Skip last one (Path)
//                break;
            if (!str.contains(path_reg)) {
                 if (str.contains('h') || str == "--help") {
                    showHelp();
                    return 0;
                 }
                 if (str.contains('q') || str == "--quiet")
                    verbose_level = 0;
                 if (str.contains('v') || str == "--verbose")
                    verbose_level = 5;
                 if (str.contains('c') || str == "--cli")
                    show = false;
                 if (str.contains('e') || str == "--erase")
                    erase = true;
                 if (str.contains('w') || str == "--write")
                    write_flash = true;
                 if (str.contains('r') || str == "--read")
                    read_flash = true;
                 if (str.contains('V') || str == "--verify")
                    verify = true;
            }
         }

    if (!show) {
        if ((!erase) && (args.size() <= 2) && (!args.last().contains(path_reg))) {
            qCritical() << "Invalid options";
            showHelp();
            return 1;
        }
        else if ((!erase) && (args.size() >= 2) && (!args.last().contains(path_reg))) {
            qCritical() << "Invalid path:" << args.last();
            showHelp();
            return 1;
        }
        if (args.last().contains(path_reg))
            path = args.last(); // Path is always the last argument.
    }
    qDebug() << "Verbose level:" << verbose_level;
    qInstallMsgHandler(myMessageOutput);
    MainWindow *w = new MainWindow;
    if (show) {
        w->show();
    }

    else {
        if (!path.isEmpty()) {

            qInformal() << "File Path:" << path;
            qInformal() << "Erasing:" << erase;
            qInformal() << "Writing:" << write_flash;
            if (!w->Connect())
                return 1;

            if (verify)
                qInformal() << "Verify not yet implemented.";

            if (write_flash)
                w->Send(path, erase);
            else if (read_flash)
                w->Receive(path);
            else
                 qInformal() << "Not doing anything";

            usleep(300000); //300 msec
            while (w->tfThread->isRunning())
                usleep(100000);
            w->Disconnect();
            w->close();
            return 0;
            }
        else if (erase) {
            if (!w->Connect())
                return 1;
            w->eraseFlash();
            w->Disconnect();
            return 0;
        }
        else if (!show) {
            showHelp();
            w->close();
            return 0;
        }
        return 1;
    }
    return a.exec();
}

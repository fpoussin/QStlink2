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
#include <QApplication>
#include <mainwindow.h>
#include <QStringList>
#include <QDebug>
#include <QFile>
#include "compat.h"

bool show = true;
bool write_flash = false, read_flash = false, erase = false, verify = false;
QString path;

static quint8 verbose_level = 3; // Level = info by default
static QElapsedTimer timer;

#if QT_VERSION >= 0x050000
    static void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        QByteArray localMsg = msg.toLocal8Bit();
        (void)context;
        switch (type) {
        case QtFatalMsg: // Always print!
                fprintf(stderr, "%lld - Fatal: %s\n", timer.elapsed(), localMsg.constData());
                abort();
        case QtCriticalMsg:
            if (verbose_level >= 1)
                fprintf(stderr, "%lld - Error: %s\n", timer.elapsed(), localMsg.constData());
            break;
#if QT_VERSION >= 0x050500
        case QtWarningMsg:
            if (verbose_level >= 2)
                fprintf(stderr, "%lld - Warning: %s\n", timer.elapsed(), localMsg.constData());
            break;
#endif
        case QtInfoMsg: // Since there is no "Info" level before 5.5.0, we use qWarning which we alias with #define...
            if (verbose_level >= 2)
                fprintf(stdout, "%lld - Info: %s\n", timer.elapsed(), localMsg.constData());
            break;
        case QtDebugMsg:
            if (verbose_level >= 5)
                fprintf(stdout, "%lld - Debug: %s\n", timer.elapsed(), localMsg.constData());
            break;
        }
    }
#else
    static void myMessageOutput(QtMsgType type, const char *msg)
    {
        switch (type) {
        case QtFatalMsg: // Always print!
                fprintf(stderr, "%lld - Fatal: %s\n", timer.elapsed(), msg);
                abort();
        case QtCriticalMsg:
            if (verbose_level >= 1)
                fprintf(stderr, "%lld - Error: %s\n", timer.elapsed(), msg);
            break;
        case QtInfoMsg: // Since there is no "Info" level, we use qWarning which we alias with #define...
            if (verbose_level >= 2)
                fprintf(stdout, "%lld - Info: %s\n", timer.elapsed(), msg);
            break;
        case QtDebugMsg:
            if (verbose_level >= 5)
                fprintf(stdout, "%lld - Debug: %s\n", timer.elapsed(), msg);
            break;
        }
    }
    #define qInstallMessageHandler qInstallMsgHandler
#endif

void showHelp()
{
    QFile help_file(":/help.html");
    if (!help_file.open(QIODevice::ReadOnly))
        return;
    QString help = help_file.readAll();
    help_file.close();
    qInfo() << help.remove(QRegExp("(<[^>]+>)|\t\b")); // Clearing HTML tags.
    qInfo("Version: %s", __QSTL_VER__);
}

bool checkParam(QString str, char s, QString l)
{
    // We check is does not start with '--' and contains the letter we look for.
    if(str.startsWith('-') && !str.startsWith("--") && str.contains(s))
        return true;
    // If it starts with '--' we check the name is correct
    if(str.startsWith("--") && str.remove("--") == l)
        return true;

    return false;
}

int main(int argc, char *argv[])
{
    timer.start();
    QApplication a(argc, argv);
    quint8 i = 0;
    QStringList args = QCoreApplication::arguments();
    QRegExp args_regex("^(-[a-zA-Z]+|--[a-z]+)$"); // Checks for a parameter (-a -B --abc...)
    foreach (const QString &str, args) {

            if (!i++) // Skip first one
                continue;

            if (str.contains(args_regex)) {
                 if (checkParam(str, 'h', "help")) {
                    showHelp();
                    return 0;
                 }
                 if (checkParam(str, 'q', "quiet"))
                    verbose_level = 0;
                 if (checkParam(str, 'v', "verbose"))
                    verbose_level = 5;
                 if (checkParam(str, 'c', "cli"))
                    show = false;
                 if (checkParam(str, 'e', "erase"))
                    erase = true;
                 if (checkParam(str, 'w', "write"))
                    write_flash = true;
                 if (checkParam(str, 'r', "read"))
                    read_flash = true;
                 if (checkParam(str, 'V', "verify"))
                    verify = true;
            }
         }

    if (!show) {
        if ((!erase) && (args.size() <= 2) && args.last().contains(args_regex)) {
            qCritical("Invalid options");
            showHelp();
            return 1;
        }
        else if ((!erase) && (args.size() >= 2) && args.last().contains(args_regex)) {
            qCritical() << "Invalid path:" << args.last();
            showHelp();
            return 1;
        }
        if (!args.last().contains(args_regex))
            path = args.last(); // Path is always the last argument.
    }
    qDebug("Verbose level: %u", verbose_level);
    qDebug("Version: %s", __QSTL_VER__);
    qInstallMessageHandler(myMessageOutput);
    MainWindow *w = new MainWindow;
    if (show) {
        w->show();
    }

    else {
        if (!path.isEmpty()) {

            qInfo() << "File Path:" << path;
            qInfo() << "Erase:" << erase;
            qInfo() << "Write:" << write_flash;
            qInfo() << "Verify:" << verify;
            if (!w->connect()) {
                return 1;
            }
            if (write_flash) {
                w->send(path);
                while (w->mTfThread->isRunning()) { SleepThread::msleep(100); }
            }
            else if (read_flash) {
                w->receive(path);
                while (w->mTfThread->isRunning()) { SleepThread::msleep(100); }
            }
            if (verify) {
                w->verify(path);
                while (w->mTfThread->isRunning()) { SleepThread::msleep(100); }
            }
            SleepThread::msleep(300);
            w->disconnect();
            w->close();
            return 0;
            }
        else if (erase) {
            qInfo("Only erasing flash");
            if (!w->connect())
                return 1;
            w->eraseFlash();
            w->disconnect();
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

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
#include <QCommandLineParser>
#include <QElapsedTimer>
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
#endif

int main(int argc, char *argv[])
{
    timer.start();
    QCoreApplication::setApplicationName("QSTlink2");
    QCoreApplication::setApplicationVersion(__QSTL_VER__);
    QApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QString().asprintf("QSTlink2 v%s", __QSTL_VER__));
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(QStringList() << "q"
                                                      << "quiet",
                                        "Supress output."));
    parser.addOption(QCommandLineOption(QStringList() << "d"
                                                      << "debug",
                                        "Debug output."));
    parser.addOption(QCommandLineOption(QStringList() << "c"
                                                      << "cli",
                                        "Commande line mode."));
    parser.addOption(QCommandLineOption(QStringList() << "e"
                                                      << "erase",
                                        "Erase memory."));
    parser.addOption(QCommandLineOption(QStringList() << "r"
                                                      << "read",
                                        "Read to file."));
    parser.addOption(QCommandLineOption(QStringList() << "w"
                                                      << "write",
                                        "Write file."));
    parser.addOption(QCommandLineOption(QStringList() << "v"
                                                      << "verify",
                                        "Verify file."));
    parser.addPositionalArgument("file", "Bin file");
    parser.process(a);

    if (parser.isSet("quiet"))
        verbose_level = 0;
    else if (parser.isSet("debug"))
        verbose_level = 5;
    if (parser.isSet("cli"))
        show = false;
    if (parser.isSet("erase"))
        erase = true;
    if (parser.isSet("write"))
        write_flash = true;
    if (parser.isSet("read"))
        read_flash = true;
    if (parser.isSet("verify"))
        verify = true;

    if (parser.positionalArguments().size() > 0)
        path = parser.positionalArguments().at(0);

    qDebug("Verbose level: %u", verbose_level);
    qDebug("Version: %s", __QSTL_VER__);
    qInstallMessageHandler(myMessageOutput);
    MainWindow *w = new MainWindow;
    if (show) {
        w->show();
    }

    else {
#if defined(WINDOWS)
        // detach from the current console window
        FreeConsole();
        // create a separate new console window
        AllocConsole();
        // attach the new console to this application's process
        AttachConsole(GetCurrentProcessId());

        freopen("CON", "w", stdout);
        freopen("CON", "w", stderr);
        freopen("CON", "r", stdin);
#endif
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
                while (w->mTfThread->isRunning()) {
                    QThread::msleep(100);
                }
            } else if (read_flash) {
                w->receive(path);
                while (w->mTfThread->isRunning()) {
                    QThread::msleep(100);
                }
            }
            if (verify) {
                w->verify(path);
                while (w->mTfThread->isRunning()) {
                    QThread::msleep(100);
                }
            }
            QThread::msleep(300);
            w->disconnect();
            w->close();
            return 0;
        } else if (erase) {
            qInfo("Only erasing flash");
            if (!w->connect())
                return 1;
            w->eraseFlash();
            w->disconnect();
            return 0;
        }
        return 1;
    }
    return a.exec();
}

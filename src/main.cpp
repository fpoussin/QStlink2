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
#include <compat.h>

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
                fprintf(stderr, "%d - Fatal: %s\n", (int)timer.elapsed(), localMsg.constData());
                abort();
        case QtCriticalMsg:
            if (verbose_level >= 1)
                fprintf(stderr, "%d - Error: %s\n", (int)timer.elapsed(), localMsg.constData());
            break;
        case QtInfoMsg: // Since there is no "Info" level, we use qWarning which we alias with #define...
            if (verbose_level >= 2)
                fprintf(stdout, "%d - Info: %s\n", (int)timer.elapsed(), localMsg.constData());
            break;
        case QtDebugMsg:
            if (verbose_level >= 5)
                fprintf(stdout, "%d - Debug: %s\n", (int)timer.elapsed(), localMsg.constData());
            break;
        }
    }
#else
    static void myMessageOutput(QtMsgType type, const char *msg)
    {
        switch (type) {
        case QtFatalMsg: // Always print!
                fprintf(stderr, "%u - Fatal: %s\n", (quint32)timer.elapsed(), msg);
                abort();
        case QtCriticalMsg:
            if (verbose_level >= 1)
                fprintf(stderr, "%u - Error: %s\n", (quint32)timer.elapsed(), msg);
            break;
        case QtInfoMsg: // Since there is no "Info" level, we use qWarning which we alias with #define...
            if (verbose_level >= 2)
                fprintf(stdout, "%u - Info: %s\n", (quint32)timer.elapsed(), msg);
            break;
        case QtDebugMsg:
            if (verbose_level >= 5)
                fprintf(stdout, "%u - Debug: %s\n", (quint32)timer.elapsed(), msg);
            break;
        }
    }
    #define qInstallMessageHandler qInstallMsgHandler
#endif

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

bool shortParam(const QString &str, char p)
{
    // We check is does not start with '--' and contains the letter we look for.
    if (str.startsWith('-') && str.at(1) != '-' && str.contains(p))
        return true;

    return false;
}

int main(int argc, char *argv[])
{
    timer.start();
    QApplication a(argc, argv);
    quint8 i = 0;
    QStringList args = QCoreApplication::arguments();
    QRegExp path_reg("[\\/]"); // Checks for a Unix or Windows path.
    foreach (const QString &str, args) {

            if (!i++) // Skip first one
                continue;

            if (!str.contains(path_reg)) {
                 if (str.contains('h') || str == "--help") {
                    showHelp();
                    return 0;
                 }
                 if (shortParam(str, 'q') || str == "--quiet")
                    verbose_level = 0;
                 if (shortParam(str, 'v') || str == "--verbose")
                    verbose_level = 5;
                 if (shortParam(str, 'c') || str == "--cli")
                    show = false;
                 if (shortParam(str, 'e') || str == "--erase")
                    erase = true;
                 if (shortParam(str, 'e') || str == "--write")
                    write_flash = true;
                 if (shortParam(str, 'r') || str == "--read")
                    read_flash = true;
                 if (shortParam(str, 'V') || str == "--verify")
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
    qInstallMessageHandler(myMessageOutput);
    MainWindow *w = new MainWindow;
    if (show) {
        w->show();
    }

    else {
        if (!path.isEmpty()) {

            qInformal() << "File Path:" << path;
            qInformal() << "Erase:" << erase;
            qInformal() << "Write:" << write_flash;
            qInformal() << "Verify:" << verify;
            if (!w->Connect())
                return 1;

            if (verify)
                qInformal() << "Verify not yet implemented.";

            if (write_flash)
                w->Send(path, erase);
            else if (read_flash)
                w->Receive(path);
            if (verify)
                w->Verify(path);

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

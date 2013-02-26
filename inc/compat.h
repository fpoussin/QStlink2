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
#ifndef COMPAT_H
#define COMPAT_H
#include <qapplication.h>

#if QT_VERSION >= 0x040700
    // QElapsedTimer was introduced in QT 4.7
    #include <QElapsedTimer>
    QElapsedTimer timer;
#else
    #include <QTime>
    QTime timer;
#endif

#ifdef WIN32
    #define usleep(num) Sleep(num/1000)
#endif

#define QtInfoMsg QtWarningMsg // Little hack to have an "info" level of output.

quint8 verbose_level = 5; // Level = info by default
#if QT_VERSION >= 0x050000
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
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
void myMessageOutput(QtMsgType type, const char *msg)
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

#endif // COMPAT_H

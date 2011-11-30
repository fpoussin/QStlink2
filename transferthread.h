#ifndef TRANSFERTHREAD_H
#define TRANSFERTHREAD_H

#include <QThread>
#include <QDebug>
#include <QString>
#include "stlinkv2.h"

class transferThread : public QThread
{
    Q_OBJECT
public:
    explicit transferThread(QObject *parent = 0);
    void run();
    void setParams(stlinkv2 *stlink, QString filename, bool write);

signals:
    void sendProgress(quint32 p);
    void sendStatus(QString s);
    void sendError(QString s);
    void sendLock(bool enabled);

public slots:
    void halt();

private slots:

private:
    void send(QString filename);
    void receive(QString filename);

    int i;
    QString filename;
    bool write;
    stlinkv2 *stlink;
    bool stop;

};

#endif // TRANSFERTHREAD_H

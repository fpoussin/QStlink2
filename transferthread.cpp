#include "transferthread.h"

transferThread::transferThread(QObject *parent) :
    QThread(parent)
{
    qDebug() << "New Transfer Thread";
    this->stop = false;
}

void transferThread::run()
{
    if (this->write)
        this->send(this->filename);
    else
        this->receive(this->filename);
}
void transferThread::halt()
{
    this->stop = true;
}

void transferThread::setParams(stlinkv2 *stlink, QString filename, bool write)
{
    this->stlink = stlink;
    this->filename = filename;
    this->write = write;
}

void transferThread::send(QString filename)
{

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Could not open the file.");
        return;
    }

    emit sendLock(true);

    this->stlink->resetMCU(); // We stop the MCU
    quint32 program_size = 4; // WORD (4 bytes)
    quint32 from = this->stlink->device->flash_base;
    quint32 to = this->stlink->device->flash_base+file.size();
    qDebug() << "Writing from" << QString::number(from, 16) << "to" << QString::number(to, 16);
    QByteArray buf;
    quint32 addr, progress, read;
    char buf2[program_size];

    // Unlock flash
    if (!this->stlink->unlockFlash())
        return;

    this->stlink->setProgramSize(program_size); // WORD (4 bytes)

    // Erase flash
    if (!this->stlink->setMassErase(true))
        return;

    emit sendStatus("Erasing flash... This might take some time.");

    this->stlink->setSTRT();
    while(this->stlink->isBusy()) {
        usleep(500000); // 500ms
    }

    qDebug() << "\n";
    // Remove erase flash flag
    if (this->stlink->setMassErase(false))
        return;

    qDebug() << "\n";
    // We finally enable flash programming
    if (!this->stlink->setFlashProgramming(true))
        return;

    while(this->stlink->isBusy()) {
        usleep(100000); // 100ms
    }

    // Unlock flash again. Seems like this is mandatory.
    if (!this->stlink->unlockFlash())
        return;

    for (int i=0; i<=file.size(); i+=program_size)
    {
        buf.clear();

        if (this->stop)
            break;

        if (file.atEnd()) {
            qDebug() << "EOF";
            break;
        }
        memset(buf2, 0, program_size);
        if ((read = file.read(buf2, program_size)) <= 0)
            break;
        buf.append(buf2, read);

        addr = this->stlink->device->flash_base+i;
        this->stlink->writeMem32(addr, buf);
        progress = (i*100)/file.size();
        emit sendProgress(progress);
        qDebug() << "Progress:"<< QString::number(progress)+"%";
        emit sendStatus("Transfered "+QString::number(i/1024)+" kilobytes out of "+QString::number(file.size()/1024));
    }
    file.close();

    emit sendProgress(100);
    emit sendStatus("Transfer done");

    // We disable flash programming
    if (this->stlink->setFlashProgramming(false))
        return;

    // Lock flash
    this->stlink->lockFlash();

    this->stlink->resetMCU();
    this->stlink->runMCU();

    emit sendLock(false);
}

void transferThread::receive(QString filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite)) {
        qCritical("Could not save the file.");
        return;
    }
    emit sendLock(true);
    this->stlink->resetMCU(); // We stop the MCU
    quint32 program_size = this->stlink->device->flash_pgsize*4;
    quint32 from = this->stlink->device->flash_base;
    quint32 to = this->stlink->device->flash_base+from;
    qDebug() << "Reading from" << QString::number(from, 16) << "to" << QString::number(to, 16);
    quint32 addr, progress;

    for (quint32 i=0; i<this->stlink->device->flash_size; i+=program_size)
    {
        if (this->stop)
            break;
        addr = this->stlink->device->flash_base+i;
        if (this->stlink->readMem32(addr, program_size) < 0)
            break;
        file.write(this->stlink->recv_buf);
        progress = (i*100)/this->stlink->device->flash_size;
        emit sendProgress(progress);
        qDebug() << "Progress:"<< QString::number(progress)+"%";
        emit sendStatus("Transfered "+QString::number(i/1024)+" kilobytes out of "+QString::number(this->stlink->device->flash_size/1024));
    }
    file.close();
    emit sendProgress(100);
    emit sendStatus("Transfer done");
    this->stlink->runMCU();
    emit sendLock(false);
}

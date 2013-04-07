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
#include "transferthread.h"

transferThread::transferThread(QObject *parent) :
    QThread(parent)
{
    qDebug() << "New Transfer Thread";
    this->m_stop = false;
}

void transferThread::run()
{
    if (this->m_write) {

//        if ((*this->stlink->device)["chip_id"] == STM32::ChipID::F4)
//            this->send(this->m_filename);
//        else
            this->sendWithLoader(this->m_filename);
        if (this->m_verify)
            this->verify(this->m_filename);
    }
    else if (!this->m_verify) {
        this->receive(this->m_filename);
    }
    else {
        this->verify(this->m_filename);
    }
}
void transferThread::halt()
{
    this->m_stop = true;
    emit sendStatus("Aborted");
    emit sendLog("Transfer Aborted");
}

void transferThread::setParams(stlinkv2 *stlink, QString filename, bool write, bool erase, bool verify)
{
    this->stlink = stlink;
    this->m_filename = filename;
    this->m_write = write;
    this->m_erase = erase;
    this->m_verify = verify;
}

void transferThread::send(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Could not open the file.");
        return;
    }
    emit sendLock(true);
    this->m_stop = false;
    this->stlink->hardResetMCU(); // We stop the MCU
    quint8 program_size = 2; // WORD (4 bytes)
    if ((*this->stlink->device)["chip_id"] == STM32::ChipID::F4)
         program_size = 4; // WORD (4 bytes)
    // The MCU will write program_size 32 times to empty the MCU buffer.
    // We don't have to write to USB only a few bytes at a time, the MCU handles flash programming for us.
    quint32 step_size = 128;
    quint32 from = (*this->stlink->device)["flash_base"];
    quint32 to = (*this->stlink->device)["flash_base"]+file.size();
    qInformal() << "Writing from" << "0x"+QString::number(from, 16) << "to" << "0x"+QString::number(to, 16);
    QByteArray buf;
    quint32 addr, progress, oldprogress, read;
    char *buf2 = new char[step_size];

    this->stlink->isLocked();

    // Unlock flash
    if (!this->stlink->unlockFlash())
        return;

    // Not supported on F0
    if ((*this->stlink->device)["chip_id"] == STM32::ChipID::F4)
        this->stlink->setProgramSize(program_size);

    if (this->m_erase) {
        emit sendStatus("Erasing flash... This might take some time.");

        if (!this->stlink->eraseFlash()) {
            qCritical() << "eraseFlash Failed!";
            return;
        }
    }

    // We finally enable flash programming
    if (!this->stlink->setFlashProgramming(true)) {
        qCritical() << "setFlashProgramming Failed!";
        return;
    }

    progress = 0;
    for (int i=0; i<=file.size(); i+=step_size)
    {
        buf.clear();

        if (this->m_stop)
            break;

        if (file.atEnd()) {
            qDebug() << "End Of File";
            break;
        }

        memset(buf2, 0, step_size);
        if ((read = file.read(buf2, step_size)) <= 0)
            break;
        qDebug() << "Read" << read << "Bytes from disk";
        buf.append(buf2, read);

        addr = (*this->stlink->device)["flash_base"]+i;
        this->stlink->writeMem32(addr, buf);
        oldprogress = progress;
        progress = (i*100)/file.size();
        if (progress > oldprogress) { // Push only if number has increased
            emit sendProgress(progress);
            qInformal() << "Progress:"<< QString::number(progress)+"%";
        }
        emit sendStatus("Transfered "+QString::number(i/1024)+" kilobytes out of "+QString::number(file.size()/1024));
    }
    file.close();
    delete buf2;

    emit sendProgress(100);
    emit sendStatus("Transfer done");
    emit sendLog("Transfer done");
    qInformal() << "Transfer done";

//    // We disable flash programming
//    if (this->stlink->setFlashProgramming(false))
//        return;

//    // Lock flash
//    this->stlink->lockFlash();

    this->stlink->hardResetMCU();
    this->stlink->resetMCU();
    this->stlink->runMCU();

    emit sendLock(false);
}

void transferThread::sendWithLoader(const QString &filename)
{
    qInformal() << "Using loader";
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Could not open the file.");
        return;
    }
    emit sendLock(true);
    this->m_stop = false;
    this->stlink->hardResetMCU(); // We stop the MCU
    quint32 step_size = 2048;
    if ((*this->stlink->device)["sram_size"] > 0)
        step_size = (*this->stlink->device)["sram_size"]-2048;
    quint32 from = (*this->stlink->device)["flash_base"];
    quint32 to = (*this->stlink->device)["flash_base"]+file.size();
    qInformal() << "Writing from" << "0x"+QString::number(from, 16) << "to" << "0x"+QString::number(to, 16);
//    QByteArray buf;
    quint32 addr, progress, oldprogress, read;
    char *buf2 = new char[step_size];

    this->stlink->resetMCU();
/*
    // Unlock flash
    if (!this->stlink->unlockFlash())
        return;

    while(this->stlink->isBusy()) {
        usleep(100000); // 100ms
    }

    if (this->m_erase) {
        emit sendStatus("Erasing flash... This might take some time.");

        if (!this->stlink->eraseFlash()) {
            qCritical() << "eraseFlash Failed!";
            return;
        }
    }
*/
    this->stlink->clearBuffer();
    this->stlink->sendLoader();
    this->stlink->runMCU(); // Will stop at the loop beginning

    while (this->stlink->getStatus() == "Status: Core Running") { // Wait for the breakpoint
            usleep(100000); // 100ms
            if (this->m_stop) break;
    }

    const quint32 bkp1 = this->stlink->readRegister(15);
    qDebug() << "Current PC reg" << QString::number(bkp1, 16);

    if (bkp1 < (*this->stlink->device)["sram_base"] || bkp1 > (*this->stlink->device)["sram_base"]+(*this->stlink->device)["sram_size"]) {

        qCritical() << "Current PC is not in the RAM area" << QString::number((*this->stlink->device)["sram_base"], 16);
        return;
    }

    progress = 0;
    this->stlink->clearBuffer();
    quint32 status = 0;
    for (int i=0; i<=file.size(); i+=step_size) {

        if (this->m_stop)
            break;

        if (file.atEnd()) {
            qDebug() << "End Of File";
            break;
        }

        memset(buf2, 0, step_size);
        if ((read = file.read(buf2, step_size)) <= 0)
            break;
        qDebug() << "Read" << read << "Bytes from disk";
        QByteArray buf(buf2, read);


        addr = (*this->stlink->device)["flash_base"]+i;

        if (!this->stlink->setLoaderBuffer(addr, buf)) {
            emit sendStatus("Failed to set loader parameters.");
            break;
        }
        // Step over breakpoint.
        this->stlink->writeRegister(bkp1+2, 15);
        this->stlink->runMCU();

        while (this->stlink->getStatus() == "Status: Core Running") { // Wait for the breakpoint
                usleep(100000); // 100ms
                if (this->m_stop) break;
        }

        status = this->stlink->getLoaderStatus();
        if (status & Loader::Masks::ERR) {
            qCritical() << "Loader reported an error!";
            break;
        }

        if (status & Loader::Masks::DEL) {
            qInformal() << "Page(s) deleted";
        }
//        this->stlink->getLoaderParams();

        oldprogress = progress;
        progress = (i*100)/file.size();
        if (progress > oldprogress) { // Push only if number has increased
            emit sendProgress(progress);
            qInformal() << "Progress:"<< QString::number(progress)+"%";
        }
        emit sendStatus("Transfered "+QString::number(i/1024)+" kilobytes out of "+QString::number(file.size()/1024));
    }
    file.close();
    delete buf2;

    qDebug() << "Current PC reg" << QString::number(this->stlink->readRegister(15), 16);

    emit sendProgress(100);
    emit sendStatus("Transfer done");
    emit sendLog("Transfer done");
    qInformal() << "Transfer done";

    this->stlink->hardResetMCU();
    this->stlink->resetMCU();
    this->stlink->runMCU();

    emit sendLock(false);
}

void transferThread::receive(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite)) {
        qCritical("Could not save the file.");
        return;
    }
    emit sendLock(true);
    this->m_stop = false;
    this->stlink->hardResetMCU(); // We stop the MCU
    const quint32 buf_size = 1024;
    const quint32 from = (*this->stlink->device)["flash_base"];
    const quint32 to = (*this->stlink->device)["flash_base"]+from;
    const quint32 flash_size = (*this->stlink->device)["flash_size"]*1024;
    qInformal() << "Reading from" << QString::number(from, 16) << "to" << QString::number(to, 16);
    quint32 addr, progress, oldprogress;

    progress = 0;
    this->stlink->clearBuffer();
    for (quint32 i=0; i<flash_size; i+=buf_size)
    {
        if (this->m_stop)
            break;
        addr = (*this->stlink->device)["flash_base"]+i;
        if (this->stlink->readMem32(addr, buf_size) < 0)
            break;
        qDebug() << "Wrote" << file.write(this->stlink->recv_buf) << "Bytes to disk";
        oldprogress = progress;
        progress = (i*100)/flash_size;
        if (progress > oldprogress) { // Push only if number has increased
            emit sendProgress(progress);
            qInformal() << "Progress:"<< QString::number(progress)+"%";
        }
        emit sendStatus("Transfered "+QString::number(i/1024)+" kilobytes out of "+QString::number(flash_size/1024));
    }
    file.close();
    emit sendProgress(100);
    emit sendStatus("Transfer done");
    qInformal() << "Transfer done";
    this->stlink->runMCU();
    emit sendLock(false);
}

void transferThread::verify(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Could not open the file.");
        return;
    }
    emit sendLock(true);
    this->m_stop = false;
    this->stlink->hardResetMCU(); // We stop the MCU
    quint32 buf_size = 256;
    quint32 from = (*this->stlink->device)["flash_base"];
    quint32 to = (*this->stlink->device)["flash_base"]+file.size();
    qInformal() << "Reading from" << QString::number(from, 16) << "to" << QString::number(to, 16);
    quint32 addr, progress, oldprogress;
    QByteArray tmp;

    progress = 0;
    this->stlink->clearBuffer();
    for (quint32 i=0; i<file.size(); i+=buf_size)
    {
        if (this->m_stop)
            break;

        tmp = file.read(buf_size);
        addr = (*this->stlink->device)["flash_base"]+i;
        if (this->stlink->readMem32(addr, tmp.size()) < 0) // Read same amount of data as from file.
            break;

        if (this->stlink->recv_buf != tmp) {

            file.close();
            emit sendProgress(100);
            emit sendStatus("Verification failed at 0x"+QString::number(addr, 16));

            QString stmp, sbuf;
            for (int b=0;b<tmp.size();b++) {
                stmp.append(QString().sprintf("%02X ", (uchar)tmp.at(b)));
                sbuf.append(QString().sprintf("%02X ", (uchar)this->stlink->recv_buf.at(b)));
            }
            qCritical() << "Verification failed at 0x"+QString::number(addr, 16) <<
                           "\r\n Expecting:" << stmp << "\r\n       Got:" << sbuf;
            this->stlink->runMCU();
            emit sendLock(false);
            return;
        }
        oldprogress = progress;
        progress = (i*100)/file.size();
        if (progress > oldprogress) { // Push only if number has increased
            emit sendProgress(progress);
            qInformal() << "Progress:"<< QString::number(progress)+"%";
        }
        emit sendStatus("Verified "+QString::number(i/1024)+" kilobytes out of "+QString::number((*this->stlink->device)["flash_size"]));
    }
    file.close();
    emit sendProgress(100);
    emit sendStatus("Verification OK");
    qInformal() << "Verification OK";
    this->stlink->runMCU();
    emit sendLock(false);
}

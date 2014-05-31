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
    mStop = false;
}

void transferThread::run()
{
    if (mWrite) {

//        if ((*this->stlink->device)["chip_id"] == STM32::ChipID::F4)
//            this->send(m_filename);
//        else
            this->sendWithLoader(mFilename);
        if (mVerify)
            this->verify(mFilename);
    }
    else if (!mVerify) {
        this->receive(mFilename);
    }
    else {
        this->verify(mFilename);
    }
}
void transferThread::halt()
{
    mStop = true;
    emit sendStatus("Aborted");
    emit sendLog("Transfer Aborted");
}

void transferThread::setParams(stlinkv2 *stlink, QString filename, bool write, bool verify)
{
    mStlink = stlink;
    mFilename = filename;
    mWrite = write;
    mVerify = verify;
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
    mStop = false;
    mStlink->hardResetMCU(); // We stop the MCU
    quint32 step_size = 2048;
    if ((*mStlink->mDevice)["sram_size"] > 0)
        step_size = (*mStlink->mDevice)["sram_size"]-2048;
    const quint32 from = (*mStlink->mDevice)["flash_base"];
    const quint32 to = (*mStlink->mDevice)["flash_base"]+file.size();
    qInformal() << "Writing from" << "0x"+QString::number(from, 16) << "to" << "0x"+QString::number(to, 16);
//    QByteArray buf;
    quint32 progress, oldprogress, read;
    char *buf2 = new char[step_size];

    mStlink->resetMCU();
    mStlink->clearBuffer();
    mStlink->sendLoader();
    mStlink->runMCU(); // Will stop at the loop beginning

    while (mStlink->getStatus() == STLink::Status::CORE_RUNNING) { // Wait for the breakpoint
            usleep(100000); // 100ms
            if (mStop) break;
    }

    const quint32 bkp1 = mStlink->readRegister(15);
    qDebug() << "Current PC reg" << QString::number(bkp1, 16);

    if (bkp1 < (*mStlink->mDevice)["sram_base"] || bkp1 > (*mStlink->mDevice)["sram_base"]+(*mStlink->mDevice)["sram_size"]) {

        qCritical() << "Current PC is not in the RAM area" << QString::number((*mStlink->mDevice)["sram_base"], 16);
        return;
    }

    progress = 0;
    mStlink->clearBuffer();
    quint32 status = 0, loader_pos = 0;
    for (int i=0; i<=file.size(); i+=step_size) {

        if (mStop)
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


        const quint32 addr = (*mStlink->mDevice)["flash_base"]+i;

        emit sendLoaderStatus("Loading");
        if (!mStlink->setLoaderBuffer(addr, buf)) {
            emit sendStatus("Failed to set loader parameters.");
            break;
        }
        // Step over breakpoint.
        mStlink->writeRegister(bkp1+2, 15);
        mStlink->runMCU();

        while (mStlink->getStatus() == STLink::Status::CORE_RUNNING) { // Wait for the breakpoint

                loader_pos = mStlink->getLoaderPos()-from;
//                qDebug() << "Loader position:" << QString::number(loader_pos, 16);

                oldprogress = progress;
                progress = (loader_pos*100)/file.size();
                if (progress > oldprogress && progress <= 100) { // Push only if number has increased
                    emit sendProgress(progress);
                    qInformal() << "Progress:"<< QString::number(progress)+"%";
                }
                emit sendLoaderStatus("Writing");
                emit sendStatus("Transferred "+QString::number(i/1024)+"/"+QString::number(file.size()/1024)+"KB");
                usleep(20000); // 20ms
                if (mStop) break;
        }

        status = mStlink->getLoaderStatus();
        if (status & Loader::Masks::ERR) {
            qCritical() << "Loader reported an error!";
            break;
        }

        if (status & Loader::Masks::DEL) {
            emit sendLoaderStatus("Erased");
            qInformal() << "Page(s) deleted";
        }
    }
    emit sendLoaderStatus("Idle");
    file.close();
    delete buf2;

    qDebug() << "Current PC reg" << QString::number(mStlink->readRegister(15), 16);

    emit sendProgress(100);
    emit sendStatus("Transfer done");
    emit sendLog("Transfer done");
    qInformal() << "Transfer done";

    mStlink->hardResetMCU();
    mStlink->resetMCU();
    mStlink->runMCU();

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
    mStop = false;
    mStlink->hardResetMCU(); // We stop the MCU
    const quint32 buf_size = 2048;
    const quint32 from = (*mStlink->mDevice)["flash_base"];
    const quint32 to = (*mStlink->mDevice)["flash_base"]+from;
    const quint32 flash_size = (*mStlink->mDevice)["flash_size"]*1024;
    qInformal() << "Reading from" << QString::number(from, 16) << "to" << QString::number(to, 16);
    quint32 addr, progress, oldprogress;

    progress = 0;
    mStlink->clearBuffer();
    for (quint32 i=0; i<flash_size; i+=buf_size)
    {
        if (mStop)
            break;
        addr = (*mStlink->mDevice)["flash_base"]+i;
        if (mStlink->readMem32(addr, buf_size) < 0)
            break;
        qDebug() << "Wrote" << file.write(mStlink->mRecvBuf) << "Bytes to disk";
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
    mStlink->runMCU();
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
    mStop = false;
    mStlink->hardResetMCU(); // We stop the MCU
    quint32 buf_size = 2048;
    quint32 from = (*mStlink->mDevice)["flash_base"];
    quint32 to = (*mStlink->mDevice)["flash_base"]+file.size();
    qInformal() << "Reading from" << QString::number(from, 16) << "to" << QString::number(to, 16);
    quint32 addr, progress, oldprogress;
    QByteArray tmp;

    progress = 0;
    mStlink->clearBuffer();
    for (quint32 i=0; i<file.size(); i+=buf_size)
    {
        if (mStop)
            break;

        tmp = file.read(buf_size);
        addr = (*mStlink->mDevice)["flash_base"]+i;
        if (mStlink->readMem32(addr, tmp.size()) < 0) // Read same amount of data as from file.
            break;

        if (mStlink->mRecvBuf != tmp) {

            file.close();
            emit sendProgress(100);
            emit sendStatus("Verification failed at 0x"+QString::number(addr, 16));

            QString stmp, sbuf;
            for (int b=0;b<tmp.size();b++) {
                stmp.append(QString().sprintf("%02X ", (uchar)tmp.at(b)));
                sbuf.append(QString().sprintf("%02X ", (uchar)mStlink->mRecvBuf.at(b)));
            }
            qCritical() << "Verification failed at 0x"+QString::number(addr, 16) <<
                           "\r\n Expecting:" << stmp << "\r\n       Got:" << sbuf;
            mStlink->runMCU();
            emit sendLock(false);
            return;
        }
        oldprogress = progress;
        progress = (i*100)/file.size();
        if (progress > oldprogress) { // Push only if number has increased
            emit sendProgress(progress);
            qInformal() << "Progress:"<< QString::number(progress)+"%";
        }
        emit sendStatus("Verified "+QString::number(i/1024)+" kilobytes out of "+QString::number(file.size()/1024));
    }
    file.close();
    emit sendProgress(100);
    emit sendStatus("Verification OK");
    qInformal() << "Verification OK";
    mStlink->runMCU();
    emit sendLock(false);
}

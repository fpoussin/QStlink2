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
    qDebug("New Transfer Thread");
    mStop = false;
}

void transferThread::run()
{
    if (mWrite) {
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
    qInfo("Using loader");
    QString tmpStr;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Could not open the file.");
        return;
    }
    emit sendLock(true);
    mStop = false;
    mStlink->hardResetMCU(); // We stop the MCU
    quint32 step_size = 2048;
    if ((*mStlink->mDevice)["buffer_size"] > 0)
        step_size = (*mStlink->mDevice)["buffer_size"]-2048; // Minus the loader's 2k
    const quint32 from = (*mStlink->mDevice)["flash_base"];
    const quint32 to = (*mStlink->mDevice)["flash_base"]+file.size();
    qInfo("Writing from %08x to %08x", from, to);
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
    qDebug("Current PC reg at %08x", bkp1);

    if (bkp1 < (*mStlink->mDevice)["sram_base"] || bkp1 > (*mStlink->mDevice)["sram_base"]+(*mStlink->mDevice)["buffer_size"]) {

        qCritical("Current PC is not in the RAM area: %08x", (*mStlink->mDevice)["sram_base"]);
        return;
    }

    progress = 0;
    mStlink->clearBuffer();
    quint32 status = 0, loader_pos = 0;
    for (int i=0; i<=file.size(); i+=step_size) {

        if (mStop)
            break;

        if (file.atEnd()) {
            qDebug("End Of File");
            break;
        }

        memset(buf2, 0, step_size);
        if ((read = file.read(buf2, step_size)) <= 0)
            break;
        qDebug("Read Bytes %u from disk", read);
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

        emit sendLoaderStatus("Writing");
        while (mStlink->getStatus() == STLink::Status::CORE_RUNNING) { // Wait for the breakpoint

                loader_pos = mStlink->getLoaderPos()-from;
//                qDebug() << "Loader position:" << QString::number(loader_pos, 16);

                oldprogress = progress;
                progress = (loader_pos*100)/file.size();
                if (progress > oldprogress && progress <= 100) { // Push only if number has increased
                    emit sendProgress(progress);
                    qInfo("Progress: %u%%", progress);
                }

                emit sendStatus(tmpStr.sprintf("Transferred %u/%uKB", i/1024, file.size()));
                usleep(20000); // 20ms
                if (mStop) break;
        }

        status = mStlink->getLoaderStatus();
        if (status & Loader::Masks::ERR) {
            qCritical("Loader reported an error!");
            break;
        }

        if (status & Loader::Masks::DEL) {
            emit sendLoaderStatus("Erased");
            qInfo("Page(s) deleted");
        }
    }
    emit sendLoaderStatus("Idle");
    file.close();
    delete buf2;

    qDebug("Current PC reg %08x", mStlink->readRegister(15));

    emit sendProgress(100);
    emit sendStatus("Transfer done");
    emit sendLog("Transfer done");
    qInfo() << "Transfer done";

    mStlink->hardResetMCU();
    mStlink->resetMCU();
    mStlink->runMCU();

    emit sendLock(false);
}

void transferThread::receive(const QString &filename)
{
    QFile file(filename);
    QString tmpStr;
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
    qInfo("Reading from %08x to %08x", from, to);
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
        qDebug("Wrote %d Bytes to disk", file.write(mStlink->mRecvBuf));
        oldprogress = progress;
        progress = (i*100)/flash_size;
        if (progress > oldprogress) { // Push only if number has increased
            emit sendProgress(progress);
            qInfo("Progress: %u%%", progress);
        }
        emit sendStatus(tmpStr.sprintf("Transferred %u/%uKB", i/1024,flash_size/1024));
    }
    file.close();
    emit sendProgress(100);
    emit sendStatus("Transfer done");
    qInfo("Transfer done");
    mStlink->runMCU();
    emit sendLock(false);
}

void transferThread::verify(const QString &filename)
{
    QFile file(filename);
    QString tmpStr;
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
    qInfo("Reading from %08x to %08x", from, to);
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
                stmp.append(tmpStr.sprintf("%02X ", (uchar)tmp.at(b)));
                sbuf.append(tmpStr.sprintf("%02X ", (uchar)mStlink->mRecvBuf.at(b)));
            }
            qCritical("Verification failed at %08X \r\n Expecting: %s\r\n       Got:%s", addr, stmp, sbuf);
            mStlink->runMCU();
            emit sendLock(false);
            return;
        }
        oldprogress = progress;
        progress = (i*100)/file.size();
        if (progress > oldprogress) { // Push only if number has increased
            emit sendProgress(progress);
            qInfo("Progress: %u%%", progress);
        }
        emit sendStatus(tmpStr.sprintf("Verified %u/%uKB", i/1024, file.size()));
    }
    file.close();
    emit sendProgress(100);
    emit sendStatus("Verification OK");
    qInfo("Verification OK");
    mStlink->runMCU();
    emit sendLock(false);
}

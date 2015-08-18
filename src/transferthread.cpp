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
    QFile loader_file(filename);
    if (!loader_file.open(QIODevice::ReadOnly)) {
        qCritical("Could not open the file.");
        return;
    }
    emit sendLock(true);
    mStop = false;
    mStlink->hardResetMCU(); // We stop the MCU
    quint32 step_size = 2048;
    if (mStlink->mDevice->value("buffer_size") > 0)
        step_size = mStlink->mDevice->value("buffer_size")-2048; // Minus the loader's 2k
    const quint32 from = mStlink->mDevice->value("flash_base");
    const quint32 to = mStlink->mDevice->value("flash_base")+loader_file.size();
    qInfo("Writing from %08x to %08x", from, to);
    quint32 progress, oldprogress, read;
    char *buf2 = new char[step_size];

    mStlink->resetMCU();
    mStlink->flush();

    if (!mStlink->sendLoader())
    {
        emit sendLog("Failed to send loader!");
        return;
    }
    emit sendLog("Loader uploaded");

    mStlink->runMCU(); // The loader will stop at the beginning of the loop

    while (mStlink->getStatus() == STLink::Status::CORE_RUNNING) { // Wait for the breakpoint
            QThread::msleep(100);
            if (mStop) break;
    }

    const quint32 bkp1 = mStlink->readRegister(15);
    qDebug("Current PC reg at %08x", bkp1);

    if (bkp1 < mStlink->mDevice->value("sram_base") || bkp1 > mStlink->mDevice->value("sram_base")+mStlink->mDevice->value("buffer_size")) {

        qCritical("Current PC is not in the RAM area: %08x", mStlink->mDevice->value("sram_base"));
        return;
    }

    progress = 0;
    mStlink->flush();
    quint32 status = 0, loader_pos = 0;
    for (int i=0; i<=loader_file.size(); i+=step_size) {

        if (mStop || loader_file.atEnd())
            break;

        memset(buf2, 0, step_size);
        if ((read = loader_file.read(buf2, step_size)) <= 0)
            break;
        qDebug("Read Bytes %u from disk", read);
        QByteArray buf(buf2, read);

        const quint32 addr = mStlink->mDevice->value("flash_base")+i;

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
                progress = (loader_pos*100)/loader_file.size();
                if (progress > oldprogress && progress <= 100) { // Push only if number has increased
                    emit sendProgress(progress);
                    qInfo("Progress: %u%%", progress);
                }

                emit sendStatus(QString().sprintf("Transferred %u/%lldKB", i/1024, loader_file.size()));
                QThread::msleep(20);
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
    loader_file.close();
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
    QByteArray buffer;
    QString tmpStr;
    if (!file.open(QIODevice::ReadWrite)) {
        qCritical("Could not save the file.");
        return;
    }
    emit sendLock(true);
    mStop = false;
    mStlink->hardResetMCU(); // We stop the MCU
    const quint32 buf_size = 2048;
    const quint32 from = mStlink->mDevice->value("flash_base");
    const quint32 to = mStlink->mDevice->value("flash_base")+from;
    const quint32 flash_size = mStlink->mDevice->value("flash_size")*1024;
    qInfo("Reading from %08x to %08x", from, to);
    quint32 addr, progress, oldprogress;

    progress = 0;
    mStlink->flush();
    for (quint32 i=0; i<flash_size; i+=buf_size)
    {
        if (mStop)
            break;
        buffer.clear();
        addr = mStlink->mDevice->value("flash_base")+i;
        if (mStlink->readMem32(&buffer, addr, buf_size) < 0)
            break;
        qDebug("Wrote %lld Bytes to disk", file.write(buffer));
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
    QString tmp_str;
    QByteArray usb_buffer, file_buffer;
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Could not open the file.");
        return;
    }
    emit sendLock(true);
    mStop = false;
    mStlink->hardResetMCU(); // We stop the MCU
    quint32 buf_size = 2048;
    quint32 from =  mStlink->mDevice->value("flash_base");
    quint32 to =  mStlink->mDevice->value("flash_base")+file.size();
    qInfo("Reading from %08x to %08x", from, to);
    quint32 addr, progress, oldprogress;

    progress = 0;
    mStlink->flush();
    for (quint32 i=0; i<file.size(); i+=buf_size)
    {
        if (mStop)
            break;

        file_buffer = file.read(buf_size);
        addr =  mStlink->mDevice->value("flash_base")+i;
        usb_buffer.clear();
        if (mStlink->readMem32(&usb_buffer, addr, file_buffer.size()) < 0) // Read same amount of data as from file.
            break;

        if (usb_buffer != file_buffer) {

            file.close();
            emit sendProgress(100);
            emit sendStatus("Verification failed at 0x"+QString::number(addr, 16));

            QString stmp, sbuf;
            for (int b=0;b<file_buffer.size();b++) {
                stmp.append(tmp_str.sprintf("%02X ", (uchar)file_buffer.at(b)));
                sbuf.append(tmp_str.sprintf("%02X ", (uchar)usb_buffer.at(b)));
            }
            qCritical("Verification failed at %08X \r\n Expecting: %s\r\n       Got:%s",
                      addr, stmp.toStdString().c_str(), sbuf.toStdString().c_str());
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
        emit sendStatus(tmp_str.sprintf("Verified %u/%lldKB", i/1024, file.size()));
    }
    file.close();
    emit sendProgress(100);
    emit sendStatus("Verification OK");
    qInfo("Verification OK");
    mStlink->runMCU();
    emit sendLock(false);
}

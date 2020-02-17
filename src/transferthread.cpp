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
#include <QPointer>

transferThread::transferThread(QObject *parent)
    : QThread(parent)
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
    } else if (!mVerify) {
        this->receive(mFilename);
    } else {
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
    const quint32 sram_base = mStlink->mDevice->value("sram_base");
    const quint32 buffer_size = mStlink->mDevice->value("buffer_size");
    if (buffer_size > 0)
        step_size = buffer_size - 2048; // Minus the loader's 2k
    const quint32 from = mStlink->mDevice->value("flash_base");
    const quint32 to = mStlink->mDevice->value("flash_base") + loader_file.size() - 1;
    qInfo("Writing from %08x to %08x", from, to);
    quint32 progress, oldprogress;

    mStlink->resetMCU();
    mStlink->haltMCU();
    mStlink->flush();

    if (!mStlink->sendLoader()) {
        emit sendLog("Failed to send loader!");
        emit sendLock(false);
        return;
    }
    emit sendLog("Loader uploaded");

    mStlink->runMCU(); // The loader will stop at main()
    while (mStlink->getStatus() == STLink::Status::RUNNING) { // Wait for the breakpoint
        QThread::msleep(100);
        quint32 tbkp = mStlink->readRegister(15);
        qDebug("Waiting for breakpoint 1... at 0x0%08X", tbkp);
        if (mStop) {
            emit sendProgress(100);
            emit sendLock(false);
            return;
        }
    }

    const quint32 bkp1 = mStlink->readRegister(15);
    qDebug("Loop breakpoint at 0x%08X", bkp1);

    if (bkp1 < sram_base || bkp1 > sram_base + buffer_size) {

        qCritical("Current PC is not in the RAM area: %08x", bkp1);
        emit sendProgress(100);
        emit sendLock(false);
        return;
    }

    progress = 0;
    mStlink->flush();
    quint32 status = 0, loader_pos = 0;
    for (int i = 0; i <= loader_file.size(); i += step_size) {

        if (mStop || loader_file.atEnd())
            break;

        quint32 bkp2 = mStlink->readRegister(15);
        if (bkp1 != bkp2) {
            qCritical("PC is not at the correct address: %08x", bkp2);
            emit sendLog("PC register at the wrong address, aborting!");
            emit sendProgress(100);
            emit sendLock(false);
            return;
        }
        qDebug("+ Current PC reg at 0x%08x", bkp2);

        QByteArray buf(loader_file.read(step_size));
        qDebug("Read Bytes %u from disk", buf.size());

        const quint32 addr = mStlink->mDevice->value("flash_base") + i;

        emit sendLoaderStatus("Loading");
        if (!mStlink->setLoaderBuffer(addr, buf)) {
            emit sendStatus("Failed to set loader parameters.");
            break;
        }
        // Step over breakpoint.
        //mStlink->haltMCU();
        if (mStlink->getStatus() == STLink::Status::RUNNING)
            mStlink->haltMCU();
        if (!mStlink->writeRegister(bkp1 + 2, 15)) {
            emit sendLog("Failed to set PC register");
            emit sendProgress(100);
            emit sendLock(false);
            return;
        }
        mStlink->runMCU();

        emit sendLoaderStatus("Writing");

        while (mStlink->getStatus() == STLink::Status::RUNNING) { // Wait for the breakpoint

            loader_pos = mStlink->getLoaderPos() - from;
            qDebug("Loader position: 0x%x", loader_pos + from);

            quint32 tbkp = mStlink->readRegister(15);
            qDebug("Waiting for breakpoint 2... at 0x0%08X", tbkp);

            oldprogress = progress;
            progress = (loader_pos * 100) / loader_file.size();
            if (progress > oldprogress && progress <= 100) { // Push only if number has increased
                emit sendProgress(progress);
                qInfo("Progress: %u%%", progress);
            }

            emit sendStatus(QString().sprintf("Transferred %u/%lldKB", i / 1024, loader_file.size() / 1024));
            QThread::msleep(30);
            if (mStop)
                break;
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
    const quint32 to = mStlink->mDevice->value("flash_base") + from;
    const quint32 flash_size = mStlink->mDevice->value("flash_size") * 1024;
    qInfo("Reading from %08x to %08x", from, to);
    quint32 addr, progress, oldprogress;

    progress = 0;
    mStlink->flush();
    for (quint32 i = 0; i < flash_size; i += buf_size) {
        if (mStop)
            break;
        buffer.clear();
        addr = mStlink->mDevice->value("flash_base") + i;
        if (mStlink->readMem32(&buffer, addr, buf_size) < 0)
            break;
        qDebug("Wrote %lld Bytes to disk", file.write(buffer));
        oldprogress = progress;
        progress = (i * 100) / flash_size;
        if (progress > oldprogress) { // Push only if number has increased
            emit sendProgress(progress);
            qInfo("Progress: %u%%", progress);
        }
        emit sendStatus(tmpStr.sprintf("Transferred %u/%uKB", i / 1024, flash_size / 1024));
    }
    file.close();
    emit sendProgress(100);
    emit sendStatus("Transfer done");
    qInfo("Transfer done");
    mStlink->runMCU();
    emit sendLock(false);
}

void transferThread::verify(const QString &filename, quint32 address)
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
    quint32 base;
    if (address > 0)
        base = address;
    else
        base = mStlink->mDevice->value("flash_base");
    quint32 from = base;
    quint32 to = base + file.size();
    qInfo("Reading from %08x to %08x", from, to);
    quint32 addr, progress, oldprogress;

    progress = 0;
    mStlink->flush();
    for (quint32 i = 0; i < file.size(); i += buf_size) {
        if (mStop)
            break;

        file_buffer = file.read(buf_size);
        addr = base + i;
        usb_buffer.clear();
        if (mStlink->readMem32(&usb_buffer, addr, file_buffer.size()) < 0) // Read same amount of data as from file.
            break;

        if (usb_buffer != file_buffer) {

            file.close();
            emit sendProgress(100);
            emit sendStatus("Verification failed at 0x" + QString::number(addr, 16));

            QString stmp, sbuf;
            for (int b = 0; b < file_buffer.size(); b++) {
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
        progress = (i * 100) / file.size();
        if (progress > oldprogress) { // Push only if number has increased
            emit sendProgress(progress);
            qInfo("Progress: %u%%", progress);
        }
        emit sendStatus(tmp_str.sprintf("Verified %u/%lldKB", i / 1024, file.size() / 1024));
    }
    file.close();
    emit sendProgress(100);
    emit sendStatus("Verification OK");
    qInfo("Verification OK");
    mStlink->runMCU();
    emit sendLock(false);
}

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
#include "stlinkv2.h"

//using namespace std;

stlinkv2::stlinkv2(QObject *parent) :
    QThread(parent)
{
    mUsbDevice = new QUsbDevice;
    mUsbMgr = new QUsbManager;
    mModeId = -1;
    mCoreId = 0;
    mChipId = 0;
    mVersion.stlink = 0;
    mConnected = false;

    QtUsb::DeviceConfig cfg;
    QtUsb::DeviceFilter f1, f2;

    f1.guid = USB_STLINK_GUID;
    f1.pid = USB_STLINKv2_PID;
    f1.vid = USB_ST_VID;

    f2 = f1;
    f2.guid = USB_NUCLEO_GUID;
    f2.pid = USB_NUCLEO_PID;

    cfg.readEp = USB_PIPE_IN;
    cfg.writeEp = USB_PIPE_OUT;
    cfg.config = USB_CONFIGURATION;
    cfg.alternate = USB_ALTERNATE;
    cfg.interface = USB_INTERFACE;

    mUsbMgr->addDevice(f1);
    mUsbMgr->addDevice(f2);

    mUsbDevice->setConfig(cfg);
    mUsbDevice->setFilter(f1);
    mUsbDevice->setDebug(true);
    mUsbDevice->setTimeout(USB_TIMEOUT_MSEC);

    QObject::connect(mUsbMgr, SIGNAL(deviceInserted(QtUsb::FilterList)), this, SLOT(scanNewDevices(QtUsb::FilterList)));
}

stlinkv2::~stlinkv2()
{
    QObject::disconnect(mUsbMgr, SIGNAL(deviceInserted(QtUsb::FilterList)), this, SLOT(scanNewDevices(QtUsb::FilterList)));
    this->disconnect();
    delete mUsbDevice;
    delete mUsbMgr;
}

qint32 stlinkv2::connect()
{
    quint32 open = mUsbDevice->open();
    if (open == 0) {
        this->flush();
        mConnected = true;
    }
    return open;
}

void stlinkv2::disconnect()
{
//    this->DebugCommand(STLink::Cmd::Dbg::Exit, 0, 2);
    QByteArray buf;
    this->command(&buf, STLink::Cmd::Reset, 0x80, 8);
    mUsbDevice->close();
    mConnected = false;
}

void stlinkv2::setSTLinkIDs()
{
    QtUsb::DeviceConfig cfg = mUsbDevice->getConfig();
    QtUsb::DeviceFilter filt;

    cfg.readEp = USB_PIPE_IN;
    cfg.writeEp = USB_PIPE_OUT;
    mUsbDevice->setConfig(cfg);

    filt.guid = USB_STLINK_GUID;
    filt.vid = USB_ST_VID;
    filt.pid = USB_STLINKv2_PID;
    filt.cfg = cfg;
    mUsbDevice->setFilter(filt);
}

void stlinkv2::setNucleoIDs()
{
    QtUsb::DeviceConfig cfg = mUsbDevice->getConfig();
    QtUsb::DeviceFilter filt;

    cfg.readEp = USB_PIPE_IN;
    cfg.writeEp = USB_PIPE_OUT_NUCLEO;
    mUsbDevice->setConfig(cfg);

    filt.guid = USB_NUCLEO_GUID;
    filt.vid = USB_ST_VID;
    filt.pid = USB_NUCLEO_PID;
    filt.cfg = cfg;
    mUsbDevice->setFilter(filt);
}

bool stlinkv2::isConnected()
{
    return mConnected;
}

void stlinkv2::flush()
{
    PrintFuncName();
    mUsbDevice->flush();
}

stlinkv2::STVersion stlinkv2::getVersion()
{
    PrintFuncName();
    QByteArray buf;
    this->command(&buf, STLink::Cmd::GetVersion, 0x80, 6);
    char b0 = buf.at(0);
    char b1 = buf.at(1);
    mVersion.stlink = (b0 & 0xf0) >> 4;
    mVersion.jtag = ((b0 & 0x0f) << 2) | ((b1 & 0xc0) >> 6);
    mVersion.swim = b1 & 0x3f;
    return mVersion;
}

quint8 stlinkv2::getMode()
{
    PrintFuncName();
    QByteArray buf;
    if (this->command(&buf, STLink::Cmd::GetCurrentMode, 0, 2)) {
        return mModeId = buf.at(0);
    }
    return mModeId;
}

quint8 stlinkv2::getStatus()
{
    PrintFuncName();
    QByteArray buf;
    this->debugCommand(&buf, STLink::Cmd::Dbg::GetStatus, 0, 2);
    return buf.at(0);
}

quint32 stlinkv2::getCoreID()
{
    PrintFuncName();
    QByteArray buf;
    this->debugCommand(&buf, STLink::Cmd::Dbg::ReadCoreID, 0, 4);
    mCoreId = qFromLittleEndian<quint32>((uchar*)buf.constData());
    qInfo() << "CoreID:" << QString::number(mCoreId, 16);
    return mCoreId;
}

quint32 stlinkv2::getChipID()
{
    PrintFuncName();
    QByteArray buf;

    if (mCoreId == 0xFFFFFFFF || mCoreId == 0x00000000)
        return 0;

    if (mCoreId == Cortex::CoreID::M0_R0) {
        this->readMem32(&buf, Cortex::Reg::CM0_CHIPID);
        qInfo("CM0 Searching at %08X", Cortex::Reg::CM0_CHIPID);
    }
    else {
        this->readMem32(&buf, Cortex::Reg::CM3_CHIPID);
        qInfo("CM3/4 Searching at %08X", Cortex::Reg::CM3_CHIPID);
    }
    mChipId = qFromLittleEndian<quint32>((uchar*)buf.data());
    mChipId &= 0xFFF;
    // CM4 rev0 fix
    if (((mChipId & 0xFFF) == STM32::ChipID::F2) && (mCoreId == Cortex::CoreID::M4_R0)) {
      qDebug("STM32F4 rev 0 errata");
      mChipId = STM32::ChipID::F4;
    }
    qInfo("ChipID: 0x%03X", mChipId);
    return mChipId;
}

quint32 stlinkv2::getRevID()
{
    PrintFuncName();
    QByteArray buf;

    this->readMem32(&buf, Cortex::Reg::CM3_CHIPID);
    mRevId = buf.at(2) | (buf.at(3) << 8);

    qInfo() << "RevID:" << QString::number(mRevId, 16);
    return mRevId;
}
quint32 stlinkv2::readFlashSize()
{
    PrintFuncName();
    QByteArray buf;

    this->readMem32(&buf, mDevice->value("flash_size_reg"));
    mDevice->insert("flash_size", qFromLittleEndian<quint32>((uchar*)buf.data()));
    if (mChipId == STM32::ChipID::F4 || mChipId == STM32::ChipID::F4_HD) {
        mDevice->insert("flash_size", mDevice->value("flash_size") >> 16);
    }
    else {
        mDevice->insert("flash_size", mDevice->value("flash_size")&0xFFFF );
    }
    qInfo("Flash size: %d KB", mDevice->value("flash_size"));
    return mDevice->value("flash_size");
}

void stlinkv2::setModeJTAG()
{
    PrintFuncName();
    QByteArray buf;
    this->getMode();
//    if (mode_id != STLINK_DEV_DEBUG_MODE)
    this->setExitModeDFU();
    this->debugCommand(&buf, STLink::Cmd::Dbg::EnterMode, STLink::Cmd::Dbg::EnterJTAG, 0);
}

void stlinkv2::setModeSWD()
{
    PrintFuncName();
    QByteArray buf;
    this->getMode();
//    if (mode_id != STLINK_DEV_DEBUG_MODE)
    this->setExitModeDFU();
    this->debugCommand(&buf, STLink::Cmd::Dbg::EnterMode, STLink::Cmd::Dbg::EnterSWD, 0);
}

void stlinkv2::setExitModeDFU()
{
    PrintFuncName();
    QByteArray buf;
    this->command(&buf, STLink::Cmd::DFUCommand, STLink::Cmd::DFUExit, 0);
}

void stlinkv2::resetMCU()
{
    PrintFuncName();
    QByteArray buf;
    this->debugCommand(&buf, STLink::Cmd::Dbg::ResetSys, 0, 2);
}

void stlinkv2::hardResetMCU()
{
    PrintFuncName();
    QByteArray buf;
//    this->DebugCommand(STLinkDebugExit, 0, 2);
//    this->setExitModeDFU();
    this->command(&buf, STLink::Cmd::Reset, 0, 8);
    this->debugCommand(&buf, STLink::Cmd::Dbg::HardReset, 0x02, 2);
}

void stlinkv2::runMCU()
{
    PrintFuncName();
    QByteArray buf;
    this->debugCommand(&buf, STLink::Cmd::Dbg::RunCore, 0, 2);
}

void stlinkv2::haltMCU()
{
    PrintFuncName();
    QByteArray buf;
    this->debugCommand(&buf, STLink::Cmd::Dbg::StepCore, 0, 2);
}

bool stlinkv2::eraseFlash()
{
    PrintFuncName();
    QByteArray buf;
    // We set the mass erase flag
    if (!this->setMassErase(true))
        return false;

    // We set the STRT flag in order to start the mass erase
    qInfo("Erasing flash... This might take some time.");
    this->setSTRT();
    while(this->isBusy()) { // then we wait for completion
        QThread::msleep(500);
    }

    // We remove the mass erase flag
    if (this->setMassErase(false))
        return false;

    return true;
}

bool stlinkv2::unlockFlash()
{
//    if (this->isLocked()) {
        PrintFuncName();
        QByteArray buf;
        uchar endian_buf[4];

        const quint32 addr = mDevice->value("flash_int_reg") + mDevice->value("KEYR_OFFSET");

        qToLittleEndian(STM32::Flash::KEY1, endian_buf);
        buf.append((const char*)endian_buf, sizeof(endian_buf));
        this->writeMem32(addr,  buf);

        buf.clear();
        qToLittleEndian(STM32::Flash::KEY2, endian_buf);
        buf.append((const char*)endian_buf, sizeof(endian_buf));
        this->writeMem32(addr,  buf);

        if (this->isLocked()) {
            qCritical("Failed to unlock flash!") ;
            return false;
//        }
    }
    return true;
}

bool stlinkv2::lockFlash()
{
    if (!this->isLocked()) {
        PrintFuncName();
        QByteArray buf;
        uchar endian_buf[4];
        quint32 addr, lock;
        quint32 fcr = this->readFlashCR();
        lock = fcr | (1 << mDevice->value("CR_LOCK"));
        addr = mDevice->value("flash_int_reg") + mDevice->value("CR_OFFSET");
        qToLittleEndian(lock, endian_buf);
        buf.append((const char*)endian_buf, sizeof(endian_buf));
        this->writeMem32(addr,  buf);
        if (!this->isLocked()) {
            qCritical("Failed to lock flash!") ;
            return false;
        }
    }
    return true;
}

bool stlinkv2::unlockFlashOpt()
{
// TODO
//    if (this->isOptLocked()) {
//        qDebug() << "***[unlockFlashOpt]***";
//        uchar buf[4];
//        quint32 addr;

//        addr = (*this->device)["OPT_KEYR_OFFSET"]

//        qToLittleEndian(FLASH_OPTKEY1, buf);
//         this->send_buf.append((const char*)buf, sizeof(buf));
//        this->writeMem32(addr,  this->send_buf);

//         this->send_buf.clear();
//        qToLittleEndian(FLASH_OPTKEY2, buf);
//         this->send_buf.append((const char*)buf, sizeof(buf));
//        this->writeMem32(addr,  this->send_buf);

//        if (this->isOptLocked()) {
//            qCritical() << "Failed to unlock flash!" ;
//            return false;
//        }
//    }
    return true;
}

bool stlinkv2::isLocked()
{
    PrintFuncName();
    bool res = false;
    const quint32 cr = this->readFlashCR();
//    qDebug() << "Lock bit" << (*this->device)["CR_LOCK"];
    res = cr & (1 << mDevice->value("CR_LOCK"));

    qDebug("Flash locked: %d", res);
    return res;
}

quint32 stlinkv2::readFlashSR()
{
    PrintFuncName();
    quint32 res;
    QByteArray buf;

    readMem32(&buf, mDevice->value("flash_int_reg") + mDevice->value("SR_OFFSET"), sizeof(quint32));
    res =  qFromLittleEndian<quint32>((const uchar*)buf.data());
    qDebug() << "Flash status register: 0x"+QString::number(res, 16) << regPrint(res);
    return res;
}

quint32 stlinkv2::readFlashCR()
{
    PrintFuncName();
    quint32 res;
    QByteArray buf;

    readMem32(&buf, mDevice->value("flash_int_reg") + mDevice->value("CR_OFFSET"), sizeof(quint32));
    res =  qFromLittleEndian<quint32>((const uchar*)buf.data());
    qDebug() << "Flash control register:" << "0x"+QString::number(res, 16) << regPrint(res);
    return res;
}

quint32 stlinkv2::writeFlashCR(quint32 mask, bool value)
{
//    if (this->isLocked())
//        this->unlockFlash();

    PrintFuncName();
    QByteArray buf;
    uchar endian_buf[4];
    quint32 addr, val;
    quint32 fcr = this->readFlashCR();
    if (value)
        val = fcr | mask ; // We append bits (OR)
    else
        val = fcr & ~mask; // We remove bits (NOT AND)
    qDebug() << "Flash control register new value: 0x"+QString::number(val, 16) << regPrint(val);

    addr = mDevice->value("flash_int_reg") + mDevice->value("CR_OFFSET");

    qToLittleEndian(val, endian_buf);
    buf.append((const char*)endian_buf, sizeof(endian_buf));
    this->writeMem32(addr,  buf);
    return this->readFlashCR();
}

bool stlinkv2::setFlashProgramming(bool val)
{
    PrintFuncName();
    const quint32 mask = (1 << STM32::Flash::CR_PG);
    const bool res = (this->writeFlashCR(mask, val) & mask) == mask;

    qDebug("Flash programming enabled: %d", res);
    return res;
}

bool stlinkv2::setMassErase(bool val)
{
    PrintFuncName();
    const quint32 mask = (1 << STM32::Flash::CR_MER);

    return (this->writeFlashCR(mask, val) & mask) == mask;
}

bool stlinkv2::setSTRT()
{
    PrintFuncName();
    quint32 mask = 0;

    if (mChipId == STM32::ChipID::F4 || mChipId == STM32::ChipID::F4_HD) {
        mask |= (1 << STM32::Flash::F4_CR_STRT);
    }
    else {
        mask |= (1 << STM32::Flash::CR_STRT);
    }
    return (this->writeFlashCR(mask, true) & mask) == mask;
}

void stlinkv2::setProgramSize(quint8 size)
{
    PrintFuncName();

    bool bit1 = false, bit2 = false;
    quint32 mask = 0;

    switch (size) {
        case 1: // BYTE
            bit1=false;
            bit2=false;
            break;
        case 2: // HALF-WORD
            bit1=false;
            bit2=true;
            break;
        case 4: // WORD
            bit1=true;
            bit2=false;
            break;
        case 8: // DOUBLE WORD
            bit1=true;
            bit2=true;
            break;
    }

    mask |= (bit2 << STM32::Flash::CR_PGSIZE);
    mask |= (bit1 << (STM32::Flash::CR_PGSIZE+1));
    qDebug("Program Size Mask: %08X", mask);
    this->writeFlashCR(mask, true);
}

bool stlinkv2::isBusy()
{
    PrintFuncName();
    bool res;

    const quint32 sr = this->readFlashSR();
    res = sr & (1 << mDevice->value("SR_BSY"));

    qDebug("Flash busy: %d", res);
    return res;
}

qint32 stlinkv2::writeMem32(quint32 addr, const QByteArray& buf)
{
    PrintFuncName() << QString().sprintf("Writing %d bytes to 0x%08X", buf.size(), addr);
    QByteArray cmdbuf, sendbuf(buf);

    int remain = buf.size() % 4;
    if (remain != 0) {
        qWarning("Data is not 32 bit aligned! Padding with %d Bytes", remain);
        sendbuf.append(QByteArray(remain, 0));
    }

    // Any writing to flash while busy = ART processor hangs
    if (addr >= mDevice->value("flash_base") && addr <= mDevice->value("flash_base")+(mDevice->value("flash_size")*1024)) {
        while(this->isBusy()) {
            QThread::msleep(100);
        }
        // Check if write succeeded
        if (mDevice->value("chip_id") == STM32::ChipID::F4 || mDevice->value("chip_id") == STM32::ChipID::F4_HD) {
            if (this->readFlashSR() & 242)
                qCritical("Flash write failed!");
        }
        else if (this->readFlashSR() & (1 << mDevice->value("SR_PER")))
            qCritical("Flash write failed!");
    }

    cmdbuf.append(STLink::Cmd::DebugCommand);
    cmdbuf.append(STLink::Cmd::Dbg::WriteMem32bit);
    uchar _addr[4], _len[2];
    qToLittleEndian(addr, _addr);
    qToLittleEndian((quint16)sendbuf.size(), _len);
    cmdbuf.append((const char*)_addr, sizeof(_addr));
    cmdbuf.append((const char*)_len, sizeof(_len));
    this->sendCommand(cmdbuf); // Send the header

    // The actual data we are writing is on the second command
    return this->sendCommand(sendbuf) - remain;
}

qint32 stlinkv2::readMem32(QByteArray* buf, quint32 addr, quint16 len)
{
    PrintFuncName() << QString().sprintf("Reading at %08X", addr);
    Q_CHECK_PTR(buf);
    QByteArray cmd_buf;
    if (len % 4 != 0)
        len += len % 4;
    cmd_buf.append(STLink::Cmd::DebugCommand);
    cmd_buf.append(STLink::Cmd::Dbg::ReadMem32bit);
    uchar cmd[4], _len[2];
    qToLittleEndian(addr, cmd);
    qToLittleEndian(len, _len);
    cmd_buf.append((const char*)cmd, sizeof(cmd));
    cmd_buf.append((const char*)_len, sizeof(_len)); //length the data we are requesting
    this->sendCommand(cmd_buf);
    return mUsbDevice->read(buf, len);
}

qint32 stlinkv2::command(QByteArray* buf, quint8 st_cmd0, quint8 st_cmd1, quint32 resp_len)
{
    Q_CHECK_PTR(buf);
    QByteArray cmd;
    cmd.append(st_cmd0);
    cmd.append(st_cmd1);

    this->sendCommand(cmd);
    if (resp_len > 0)
    {
        return mUsbDevice->read(buf, resp_len);
    }
    return 0;
}

qint32 stlinkv2::debugCommand(QByteArray* buf, quint8 st_cmd1, quint8 st_cmd2, quint32 resp_len)
{
    Q_CHECK_PTR(buf);
    QByteArray cmd;
    cmd.append(STLink::Cmd::DebugCommand);
    cmd.append(st_cmd1);
    cmd.append(st_cmd2);

    qint32 res = this->sendCommand(cmd);
//    if (res > 0)
//        qDebug() << res << " Bytes sent";
//    else
//        qCritical() << "Error: " << res;

    if (resp_len > 0)
    {
        return mUsbDevice->read(buf, resp_len);
    }
    return res;
}

bool stlinkv2::writeRegister(quint32 val, quint8 index) // Not working on F4 ?
{
    PrintFuncName();
    QByteArray cmd, tmp;
    cmd.append(STLink::Cmd::DebugCommand);
    cmd.append(STLink::Cmd::Dbg::WriteReg);
    cmd.append(index);
    uchar tval[4];
    qToLittleEndian(val, tval);
    cmd.append((const char*)tval, sizeof(tval));
    this->sendCommand(cmd);
    mUsbDevice->read(&tmp, 2);

    const quint32 tmpval = this->readRegister(index);
    if (tmpval != val) {
        qCritical("Failed to set register %d to %08X, current value is %08x", index, val, tmpval);
        return false;
    }
    qDebug("Set register %d to %08X", index, val);
    return true;
}

quint32 stlinkv2::readRegister(quint8 index)
{
    PrintFuncName();
    QByteArray cmd, value;
    cmd.append(STLink::Cmd::DebugCommand);
    cmd.append(STLink::Cmd::Dbg::ReadReg);
    cmd.append(index);
    this->sendCommand(cmd);

    mUsbDevice->read(&value, 4);
    return qFromLittleEndian<quint32>((const uchar*)value.data());
}

void stlinkv2::writePC(quint32 val) // Not working
{
    PrintFuncName();
    QByteArray cmd;
    cmd.append(STLink::Cmd::DebugCommand);
    cmd.append(STLink::Cmd::Dbg::WriteRegPC);
    cmd.append((quint8)0x07);
    uchar tval[4];
    qToLittleEndian(val, tval);
    cmd.append((const char*)tval, sizeof(tval));
    this->sendCommand(cmd);
}

qint32 stlinkv2::sendCommand(const QByteArray& cmd)
{
    qint32 ret = 0;

    ret = mUsbDevice->write(&cmd, cmd.size());
    if (ret > 0) {

    }
    else {
        PrintError();
    }
    return ret;
}

bool stlinkv2::sendLoader() {

    mLoader.loadBin(mDevice->mLoaderFile);

    QByteArray loader_data;
    int i=0;
    const int step = 128;
    int sent;
    for (; i < mLoader.refData().size()/step; i++) {

        loader_data = QByteArray(mLoader.refData().constData()+(i*step), step);
        sent = this->writeMem32(mDevice->value("sram_base")+(i*step), loader_data);
        if (sent != step)
        {
            qCritical("Loader: Only sent %d out of %d", sent, step);
            return false;
        }
    }
    const int mod = mLoader.refData().size()%step;
    loader_data = QByteArray(mLoader.refData().constData()+mLoader.refData().size()-mod, mod);
    sent = this->writeMem32(mDevice->value("sram_base")+(i*step), loader_data);
    if (sent != mod)
    {
        qCritical("Loader: Only sent %d out of %d", sent, mod);
        return false;
    }

//    this->writeMem32((*this->device)["sram_base"], m_loader.refData());
    this->writeRegister(mDevice->value("sram_base"), 15); // PC register to sram base.

    return true;
}

bool stlinkv2::setLoaderBuffer(const quint32 addr, const QByteArray& buf) {

    using namespace Loader::Addr;
    uchar ar_tmp[4];
    QByteArray write_buf, read_buf;
    quint32 buffer_size = buf.size();

    qToLittleEndian(addr, ar_tmp);
    write_buf = QByteArray((const char*)ar_tmp, 4);
    if (this->writeMem32(PARAMS+OFFSET_DEST, write_buf) < 0)
    {
        qCritical("Failed to set loader write address!");
        return false;
    }

    qToLittleEndian(buffer_size, ar_tmp);
    write_buf = QByteArray((const char*)ar_tmp, 4);
    if (this->writeMem32(PARAMS+OFFSET_LEN, write_buf) < 0)
    {
        qCritical("Failed to set loader write length!");
        return false;
    }

    read_buf.clear();
    this->readMem32(&read_buf, PARAMS+OFFSET_DEST);
    const quint32 dest = qFromLittleEndian<quint32>((uchar*)read_buf.data());

    read_buf.clear();
    this->readMem32(&read_buf, PARAMS+OFFSET_LEN);
    const quint32 len = qFromLittleEndian<quint32>((uchar*)read_buf.data());

    if ((dest != addr) || (buffer_size != len))
    {
        qCritical("Failed to set loader settings!");
        qCritical("Expected data destination and length: 0x%08X - %d", addr, buf.size());
        qCritical("Current data destination and length: 0x%08X - %d", dest, len);
        return false;
    }

    emit bufferPct(0);
    int i=0;
    const int step = 2048;
    for (; i < buf.size()/step; i++) {

        write_buf = QByteArray(buf.constData()+(i*step), step);
        this->writeMem32(BUFFER+(i*step), write_buf);
        emit bufferPct(((step*(i+1))*100)/buf.size());
    }
    const int mod = buf.size() % step;
    if (mod > 0)  {
        write_buf = QByteArray(buf.constData()+buf.size()-mod, mod);
        this->writeMem32(BUFFER+(i*step), write_buf);
    }
    emit bufferPct(100);
    return true;
}

quint32 stlinkv2::getLoaderStatus() {

    PrintFuncName();
    QByteArray read_buf;
    using namespace Loader::Addr;
    this->readMem32(&read_buf, PARAMS+OFFSET_STATUS);
    quint32 tmp = qFromLittleEndian<quint32>((uchar*)read_buf.constData());
//    qDebug() << this->regPrint(tmp);
    return tmp;
}

quint32 stlinkv2::getLoaderPos() {

    PrintFuncName();
    QByteArray read_buf;
    using namespace Loader::Addr;
    this->readMem32(&read_buf, PARAMS+OFFSET_POS);
    quint32 tmp = qFromLittleEndian<quint32>((uchar*)read_buf.data());
//    qDebug() << this->regPrint(tmp);
    return tmp;
}

void stlinkv2::getLoaderParams() {

    PrintFuncName();
    QByteArray read_buf;
    using namespace Loader::Addr;
    this->readMem32(&read_buf, PARAMS+OFFSET_DEST);
    const quint32 dest = qFromLittleEndian<quint32>((uchar*)read_buf.data());
    this->readMem32(&read_buf, PARAMS+OFFSET_LEN);
    const quint32 len = qFromLittleEndian<quint32>((uchar*)read_buf.data());

    this->readMem32(&read_buf, PARAMS+OFFSET_TEST);
    const quint32 test = qFromLittleEndian<quint32>((uchar*)read_buf.data());

    qDebug("Data destination and length: 0x%08X - %d - test: 0x%08X", dest, len, test);
}

QString stlinkv2::regPrint(quint32 reg) const
{
    QString top("Register dump:\r\nBit | ");
    QString bottom("\r\nVal | ");
    QString tmp;
    int pos = 0;
    for (int i=0;i<32;i++) {
        pos = 31-i;
        top.append(QString::number(pos)+" ");
        tmp = QString::number((bool)(reg & 1 << pos), 2);
        if (pos >= 10)
            bottom.append(tmp+"  ");
        else
            bottom.append(tmp+" ");
    }
    return top+"|"+bottom+"|";
}

void stlinkv2::scanNewDevices(QtUsb::FilterList list)
{
    QtUsb::DeviceFilter f1, f2;

    f1.pid = USB_STLINKv2_PID;
    f1.vid = USB_ST_VID;

    f2.pid = USB_NUCLEO_PID;
    f2.vid = USB_ST_VID;

    if (mUsbMgr->findDevice(f1, list) >= 0)
    {
        emit deviceDetected("STLink V2 inserted");
    }

    else if (mUsbMgr->findDevice(f2, list) >= 0)
    {
        emit deviceDetected("Nucleo inserted");
    }
}

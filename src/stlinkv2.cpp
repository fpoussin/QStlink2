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

    cfg.readEp = USB_PIPE_OUT;
    cfg.writeEp = USB_PIPE_IN;
    cfg.config = USB_CONFIGURATION;
    cfg.alternate = 0;
    cfg.interface = 1;

    mUsbMgr->addDevice(f1);
    mUsbMgr->addDevice(f2);

    mUsbDevice->setConfig(cfg);
    mUsbDevice->setFilter(f1);
    mUsbDevice->setDebug(false);
    mUsbDevice->setTimeout(USB_TIMEOUT_MSEC);
}

stlinkv2::~stlinkv2()
{
    this->disconnect();
    delete mUsbDevice;
    delete mUsbMgr;
}

qint32 stlinkv2::connect()
{
    if (mUsbDevice->getPid() == USB_STLINKv2_PID)
    {
        mUsbDevice->setEndPoints(USB_PIPE_IN, USB_PIPE_OUT);
    }

    else if (mUsbDevice->getPid() == USB_NUCLEO_PID)
    {
        mUsbDevice->setEndPoints(USB_PIPE_IN, USB_PIPE_OUT_NUCLEO);
    }

    qint32 open = mUsbDevice->open();
    if ((open >= 0)) {
        mUsbDevice->read(&mRecvBuf, 2048); // We clean the usb buffer
        mConnected = true;
    }
    return open;
}

void stlinkv2::disconnect()
{
//    this->DebugCommand(STLink::Cmd::Dbg::Exit, 0, 2);
    this->Command(STLink::Cmd::Reset, 0x80, 8);
    mUsbDevice->close();
    mConnected = false;
}

void stlinkv2::setSTLinkIDs()
{
    mUsbDevice->setGuid(USB_STLINK_GUID);
    mUsbDevice->setDeviceIds(USB_STLINKv2_PID, USB_ST_VID);
}

void stlinkv2::setNucleoIDs()
{
    mUsbDevice->setGuid(USB_NUCLEO_GUID);
    mUsbDevice->setDeviceIds(USB_NUCLEO_PID, USB_ST_VID);
}

bool stlinkv2::isConnected()
{
    return mConnected;
}

void stlinkv2::clearBuffer()
{
    PrintFuncName();
    QByteArray tmp;
    mCmdBuf.clear();
    mRecvBuf.clear();
    mUsbDevice->read(&tmp, 128);
}

stlinkv2::STVersion stlinkv2::getVersion()
{
    PrintFuncName();
    this->Command(STLink::Cmd::GetVersion, 0x80, 6);
    char b0 = mRecvBuf.at(0);
    char b1 = mRecvBuf.at(1);
    mVersion.stlink = (b0 & 0xf0) >> 4;
    mVersion.jtag = ((b0 & 0x0f) << 2) | ((b1 & 0xc0) >> 6);
    mVersion.swim = b1 & 0x3f;
    return mVersion;
}

quint8 stlinkv2::getMode()
{
    PrintFuncName();
    if (this->Command(STLink::Cmd::GetCurrentMode, 0, 2)) {
        return mModeId = mRecvBuf.at(0);
    }
    return mModeId;
}

quint8 stlinkv2::getStatus()
{
    PrintFuncName();
    this->DebugCommand(STLink::Cmd::Dbg::GetStatus, 0, 2);
    return mRecvBuf.at(0);
}

quint32 stlinkv2::getCoreID()
{
    PrintFuncName();
    this->DebugCommand(STLink::Cmd::Dbg::ReadCoreID, 0, 4);
    mCoreId = qFromLittleEndian<quint32>((uchar*)mRecvBuf.constData());
    qInfo() << "CoreID:" << QString::number(mCoreId, 16);
    return mCoreId;
}

quint32 stlinkv2::getChipID()
{
    PrintFuncName();

    if (mCoreId == 0xFFFFFFFF || mCoreId == 0x00000000)
        return 0;

    if (mCoreId == Cortex::CoreID::M0_R0) {
        this->readMem32(Cortex::Reg::CM0_CHIPID);
        qInfo() << "CM0 Searching at" << QString::number(Cortex::Reg::CM0_CHIPID, 16);
    }
    else {
        this->readMem32(Cortex::Reg::CM3_CHIPID);
        qInfo() << "CM3/4 Searching at" << QString::number(Cortex::Reg::CM3_CHIPID, 16);
    }
    mChipId = qFromLittleEndian<quint32>((uchar*)mRecvBuf.constData());
    mChipId &= 0xFFF;
    // CM4 rev0 fix
    if (((mChipId & 0xFFF) == STM32::ChipID::F2) && (mCoreId == Cortex::CoreID::M4_R0)) {
      qDebug() << "STM32F4 rev 0 errata";
      mChipId = STM32::ChipID::F4;
    }
    qInfo() << "ChipID:" << QString::number(mChipId, 16);
    return mChipId;
}

quint32 stlinkv2::getRevID()
{
    PrintFuncName();

    this->readMem32(Cortex::Reg::CM3_CHIPID);
    mRevId = mRecvBuf.at(2) | (mRecvBuf.at(3) << 8);

    qInfo() << "RevID:" << QString::number(mRevId, 16);
    return mRevId;
}
quint32 stlinkv2::readFlashSize()
{
    PrintFuncName();

    this->readMem32((*mDevice)["flash_size_reg"]);
    (*mDevice)["flash_size"] = qFromLittleEndian<quint32>((uchar*)mRecvBuf.constData());
    if (mChipId == STM32::ChipID::F4 || mChipId == STM32::ChipID::F4_HD) {
        (*mDevice)["flash_size"] = (*mDevice)["flash_size"] >> 16;
    }
    else {
        (*mDevice)["flash_size"] &= 0xFFFF;
    }
    qInfo() << "Flash size:" << (*mDevice)["flash_size"] << "KB";
    return (*mDevice)["flash_size"];
}

void stlinkv2::setModeJTAG()
{
    PrintFuncName();
    this->getMode();
//    if (mode_id != STLINK_DEV_DEBUG_MODE)
    this->setExitModeDFU();
    this->DebugCommand(STLink::Cmd::Dbg::EnterMode, STLink::Cmd::Dbg::EnterJTAG, 0);
}

void stlinkv2::setModeSWD()
{
    PrintFuncName();
    this->getMode();
//    if (mode_id != STLINK_DEV_DEBUG_MODE)
    this->setExitModeDFU();
    this->DebugCommand(STLink::Cmd::Dbg::EnterMode, STLink::Cmd::Dbg::EnterSWD, 0);
}

void stlinkv2::setExitModeDFU()
{
    PrintFuncName();
    this->Command(STLink::Cmd::DFUCommand, STLink::Cmd::DFUExit, 0);
}

void stlinkv2::resetMCU()
{
    PrintFuncName();
    this->DebugCommand(STLink::Cmd::Dbg::ResetSys, 0, 2);
}

void stlinkv2::hardResetMCU()
{
    PrintFuncName();
//    this->DebugCommand(STLinkDebugExit, 0, 2);
//    this->setExitModeDFU();
    this->Command(STLink::Cmd::Reset, 0, 8);
    this->DebugCommand(STLink::Cmd::Dbg::HardReset, 0x02, 2);
}

void stlinkv2::runMCU()
{
    PrintFuncName();
    this->DebugCommand(STLink::Cmd::Dbg::RunCore, 0, 2);
}

void stlinkv2::haltMCU()
{
    PrintFuncName();
    this->DebugCommand(STLink::Cmd::Dbg::StepCore, 0, 2);
}

bool stlinkv2::eraseFlash()
{
    PrintFuncName();
    // We set the mass erase flag
    if (!this->setMassErase(true))
        return false;

    // We set the STRT flag in order to start the mass erase
    qInfo() << "Erasing flash... This might take some time.";
    this->setSTRT();
    while(this->isBusy()) { // then we wait for completion
        usleep(500000); // 500ms
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
        uchar buf[4];

        const quint32 addr = (*mDevice)["flash_int_reg"] + (*mDevice)["KEYR_OFFSET"];

        qToLittleEndian(STM32::Flash::KEY1, buf);
        mSendBuf.append((const char*)buf, sizeof(buf));
        this->writeMem32(addr,  mSendBuf);

        mSendBuf.clear();
        qToLittleEndian(STM32::Flash::KEY2, buf);
        mSendBuf.append((const char*)buf, sizeof(buf));
        this->writeMem32(addr,  mSendBuf);

        if (this->isLocked()) {
            qCritical() << "Failed to unlock flash!" ;
            return false;
//        }
    }
    return true;
}

bool stlinkv2::lockFlash()
{
    if (!this->isLocked()) {
        PrintFuncName();
        uchar buf[4];
        quint32 addr, lock;
        quint32 fcr = this->readFlashCR();
        lock = fcr | (1 << (*mDevice)["CR_LOCK"]);
        addr = (*mDevice)["flash_int_reg"] + (*mDevice)["CR_OFFSET"];
        qToLittleEndian(lock, buf);
        mSendBuf.append((const char*)buf, sizeof(buf));
        this->writeMem32(addr,  mSendBuf);
        if (!this->isLocked()) {
            qCritical() << "Failed to lock flash!" ;
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
    res = cr & (1 << (*mDevice)["CR_LOCK"]);

    qDebug() << "Flash locked:" << res;
    return res;
}

quint32 stlinkv2::readFlashSR()
{
    PrintFuncName();
    quint32 res;

    readMem32((*mDevice)["flash_int_reg"] + (*mDevice)["SR_OFFSET"], sizeof(quint32));
    res =  qFromLittleEndian<quint32>((const uchar*)mRecvBuf.constData());
    qDebug() << "Flash status register:" << "0x"+QString::number(res, 16) << regPrint(res);
    return res;
}

quint32 stlinkv2::readFlashCR()
{
    PrintFuncName();
    quint32 res;

    readMem32((*mDevice)["flash_int_reg"] + (*mDevice)["CR_OFFSET"], sizeof(quint32));
    res =  qFromLittleEndian<quint32>((const uchar*)mRecvBuf.constData());
    qDebug() << "Flash control register:" << "0x"+QString::number(res, 16) << regPrint(res);
    return res;
}

quint32 stlinkv2::writeFlashCR(quint32 mask, bool value)
{
//    if (this->isLocked())
//        this->unlockFlash();

    PrintFuncName();
    uchar buf[4];
    quint32 addr, val;
    quint32 fcr = this->readFlashCR();
    if (value)
        val = fcr | mask ; // We append bits (OR)
    else
        val = fcr & ~mask; // We remove bits (NOT AND)
    qDebug() << "Flash control register new value:" << "0x"+QString::number(val, 16) << regPrint(val);

    addr = (*mDevice)["flash_int_reg"] + (*mDevice)["CR_OFFSET"];

    qToLittleEndian(val, buf);
    mSendBuf.append((const char*)buf, sizeof(buf));
    this->writeMem32(addr,  mSendBuf);
    return this->readFlashCR();
}

bool stlinkv2::setFlashProgramming(bool val)
{
    PrintFuncName();
    const quint32 mask = (1 << STM32::Flash::CR_PG);
    const bool res = (this->writeFlashCR(mask, val) & mask) == mask;

    qDebug() << "Flash programming enabled:" << res;
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
    qDebug() << "Program Size Mask:" << "0x"+QString::number(mask, 16);
    this->writeFlashCR(mask, true);
}

bool stlinkv2::isBusy()
{
    PrintFuncName();
    bool res;

    const quint32 sr = this->readFlashSR();
    res = sr & (1 << (*mDevice)["SR_BSY"]);

    qDebug() << "Flash busy:" << res;
    return res;
}

void stlinkv2::writeMem32(quint32 addr, QByteArray &buf)
{
    PrintFuncName() << " Writing" << buf.size() << "bytes to" << "0x"+QString::number(addr, 16).toUpper();
    uint remain = buf.size() % 4;
    if (remain != 0) {
        qWarning() << "Data is not 32 bit aligned! Padding with" << QString::number(remain) << "Bytes";
        buf.append(QByteArray(remain, 0));
    }

    // Any writing to flash while busy = ART processor hangs
    if (addr >= (*mDevice)["flash_base"] && addr <= (*mDevice)["flash_base"]+((*mDevice)["flash_size"]*1024)) {
        while(this->isBusy()) {
            usleep(100000); // 100ms
        }
        // Check if write succeeded
        if ((*mDevice)["chip_id"] == STM32::ChipID::F4 || (*mDevice)["chip_id"] == STM32::ChipID::F4_HD) {
            if (this->readFlashSR() & 242)
                qCritical() << "Flash write failed!";
        }
        else if (this->readFlashSR() & (1 << (*mDevice)["SR_PER"]))
            qCritical() << "Flash write failed!";
    }
    mCmdBuf.append(STLink::Cmd::DebugCommand);
    mCmdBuf.append(STLink::Cmd::Dbg::WriteMem32bit);
    uchar _addr[4], _len[2];
    qToLittleEndian(addr, _addr);
    qToLittleEndian((quint16)buf.size(), _len);
    mCmdBuf.append((const char*)_addr, sizeof(_addr));
    mCmdBuf.append((const char*)_len, sizeof(_len));
//    this->cmd_buf.append((quint16)buf.size());
    this->SendCommand();

    // the actual data we are writing is on the second command
    mCmdBuf.append(buf);
    this->SendCommand();
    buf.clear();
}

qint32 stlinkv2::readMem32(quint32 addr, quint16 len)
{
    PrintFuncName() << " Reading at" << "0x"+QString::number(addr, 16).toUpper();
    if (len % 4 != 0)
        return 0;
    mCmdBuf.append(STLink::Cmd::DebugCommand);
    mCmdBuf.append(STLink::Cmd::Dbg::ReadMem32bit);
    uchar cmd[4], _len[2];
    qToLittleEndian(addr, cmd);
    qToLittleEndian(len, _len);
    mCmdBuf.append((const char*)cmd, sizeof(cmd));
    mCmdBuf.append((const char*)_len, sizeof(_len)); //length the data we are requesting
    this->SendCommand();
    mRecvBuf.clear();
    return mUsbDevice->read(&mRecvBuf, len);
}

qint32 stlinkv2::Command(quint8 st_cmd0, quint8 st_cmd1, quint32 resp_len)
{
    mCmdBuf.append(st_cmd0);
    mCmdBuf.append(st_cmd1);

    this->SendCommand();
    if (resp_len > 0)
        return mUsbDevice->read(&mRecvBuf, resp_len);
    return 0;
}

qint32 stlinkv2::DebugCommand(quint8 st_cmd1, quint8 st_cmd2, quint32 resp_len)
{
    mCmdBuf.append(STLink::Cmd::DebugCommand);
    mCmdBuf.append(st_cmd1);
    mCmdBuf.append(st_cmd2);

    qint32 res = this->SendCommand();
//    if (res > 0)
//        qDebug() << res << " Bytes sent";
//    else
//        qCritical() << "Error: " << res;

    if (resp_len > 0)
        return mUsbDevice->read(&mRecvBuf, resp_len);
    return res;
}

bool stlinkv2::writeRegister(quint32 val, quint8 index) // Not working on F4 ?
{
    PrintFuncName();
    mCmdBuf.append(STLink::Cmd::DebugCommand);
    mCmdBuf.append(STLink::Cmd::Dbg::WriteReg);
    mCmdBuf.append(index);
    uchar tval[4];
    qToLittleEndian(val, tval);
    mCmdBuf.append((const char*)tval, sizeof(tval));
    this->SendCommand();
    QByteArray tmp;
    mUsbDevice->read(&tmp, 2);

    const quint32 tmpval = this->readRegister(index);
    if (tmpval != val) {
        qCritical() << "Failed to set register" << index << "at" << "0x"+QString::number(val, 16) << "current val is" << "0x"+QString::number(tmpval, 16);
        return false;
    }
    qDebug() << "Set register" << index << "at" << "0x"+QString::number(val, 16);
    return true;
}

quint32 stlinkv2::readRegister(quint8 index)
{
    PrintFuncName();
    mCmdBuf.append(STLink::Cmd::DebugCommand);
    mCmdBuf.append(STLink::Cmd::Dbg::ReadReg);
    mCmdBuf.append(index);
    this->SendCommand();

    QByteArray tmp;
    mUsbDevice->read(&tmp, 4);
    return qFromLittleEndian<quint32>((const uchar*)tmp.constData());
}

void stlinkv2::writePC(quint32 val) // Not working
{
    PrintFuncName();
    mCmdBuf.append(STLink::Cmd::DebugCommand);
    mCmdBuf.append(STLink::Cmd::Dbg::WriteRegPC);
    mCmdBuf.append((quint8)0x07);
    uchar tval[4];
    qToLittleEndian(val, tval);
    mCmdBuf.append((const char*)tval, sizeof(tval));
    this->SendCommand();
}

qint32 stlinkv2::SendCommand()
{
    qint32 ret = 0;

//    while (this->cmd_buf.size() < 16)
//        this->cmd_buf.append((char)0);

    ret = mUsbDevice->write(&mCmdBuf, mCmdBuf.size());
    mCmdBuf.clear();
    if (ret > 0) {
//        qDebug() << "Command sent successfully.";
    }
    else {
        PrintError();
    }
    return ret;
}

void stlinkv2::sendLoader() {

    mLoader.loadBin(mDevice->mLoaderFile);

    QByteArray tmp;
    int i=0;
    const int step = 128;
    for (; i < mLoader.refData().size()/step; i++) {

        tmp = QByteArray(mLoader.refData().constData()+(i*step), step);
        this->writeMem32((*mDevice)["sram_base"]+(i*step), tmp);
    }
    const int mod = mLoader.refData().size()%step;
    tmp = QByteArray(mLoader.refData().constData()+mLoader.refData().size()-mod, mod);
    this->writeMem32((*mDevice)["sram_base"]+(i*step), tmp);

//    this->writeMem32((*this->device)["sram_base"], m_loader.refData());
    this->writeRegister((*mDevice)["sram_base"], 15); // PC register to sram base.
}

bool stlinkv2::setLoaderBuffer(const quint32 addr, const QByteArray& buf) {

    using namespace Loader::Addr;
    uchar ar_tmp[4];
    qToLittleEndian(addr, ar_tmp);
    QByteArray tmp((const char*)ar_tmp, 4);
    this->writeMem32(PARAMS+OFFSET_DEST, tmp);

    qToLittleEndian(buf.size(), ar_tmp);
    tmp = QByteArray((const char*)ar_tmp, 4);
    this->writeMem32(PARAMS+OFFSET_LEN, tmp);

    this->readMem32(PARAMS+OFFSET_DEST);
    const quint32 dest = qFromLittleEndian<quint32>((uchar*)mRecvBuf.constData());
    this->readMem32(PARAMS+OFFSET_LEN);
    const quint32 len = qFromLittleEndian<quint32>((uchar*)mRecvBuf.constData());

    qDebug() << "Data destination and length:" << "0x"+QString::number(dest, 16) << len;

    if ((dest != addr) || ((quint32)buf.size() != len)) {

        qCritical() << "Failed to set loader settings!";
        qCritical() << "Expected data destination and length:" << "0x"+QString::number(addr, 16) << buf.size();
        return false;
    }

    emit bufferPct(0);
    int i=0;
    const int step = 2048;
    for (; i < buf.size()/step; i++) {

        tmp = QByteArray(buf.constData()+(i*step), step);
        this->writeMem32(BUFFER+(i*step), tmp);
        emit bufferPct(((step*(i+1))*100)/buf.size());
    }
    const int mod = buf.size()%step;
    if (mod>0)  {
        tmp = QByteArray(buf.constData()+buf.size()-mod, mod);
        this->writeMem32(BUFFER+(i*step), tmp);
    }
    emit bufferPct(100);
    return true;
}

quint32 stlinkv2::getLoaderStatus() {

    PrintFuncName();
    using namespace Loader::Addr;
    this->readMem32(PARAMS+OFFSET_STATUS);
    quint32 tmp = qFromLittleEndian<quint32>((uchar*)mRecvBuf.constData());
//    qDebug() << this->regPrint(tmp);
    return tmp;
}

quint32 stlinkv2::getLoaderPos() {

    PrintFuncName();
    using namespace Loader::Addr;
    this->readMem32(PARAMS+OFFSET_POS);
    quint32 tmp = qFromLittleEndian<quint32>((uchar*)mRecvBuf.constData());
//    qDebug() << this->regPrint(tmp);
    return tmp;
}

void stlinkv2::getLoaderParams() {

    PrintFuncName();
    using namespace Loader::Addr;
    this->readMem32(PARAMS+OFFSET_DEST);
    const quint32 dest = qFromLittleEndian<quint32>((uchar*)mRecvBuf.constData());
    this->readMem32(PARAMS+OFFSET_LEN);
    const quint32 len = qFromLittleEndian<quint32>((uchar*)mRecvBuf.constData());

    this->readMem32(PARAMS+OFFSET_TEST);
    const quint32 test = qFromLittleEndian<quint32>((uchar*)mRecvBuf.constData());

    qDebug() << "Data destination and length:" << "0x"+QString::number(dest, 16) << len << "test:" << "0x"+QString::number(test, 16);
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

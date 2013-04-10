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
    this->usb = new QUsb;
    this->mode_id = -1;
    this->core_id = 0;
    this->chip_id = 0;
    this->version.stlink = 0;
    this->connected = false;
}

stlinkv2::~stlinkv2()
{
    this->disconnect();
    delete usb;
}

qint32 stlinkv2::connect()
{
    qint32 open = this->usb->open();
    if ((open > 0)) {
        this->usb->read(&this->recv_buf, 2048); // We clean the usb buffer
    }
    this->connected = true;
    return open;
}

void stlinkv2::disconnect()
{
    this->DebugCommand(STLink::Cmd::Dbg::Exit, 0, 2);
    this->usb->close();
    this->connected = false;
}

bool stlinkv2::isConnected()
{
    return this->connected;
}

void stlinkv2::clearBuffer()
{
    qDebug() << "***[clearBuffer]***";
    QByteArray tmp;
    this->cmd_buf.clear();
    this->recv_buf.clear();
    this->usb->read(&tmp, 128);
}

stlinkv2::STVersion stlinkv2::getVersion() {

    qDebug() << "***[getVersion]***";
    this->Command(STLink::Cmd::GetVersion, 0x80, 6);
    char b0 = this->recv_buf.at(0);
    char b1 = this->recv_buf.at(1);
    this->version.stlink = (b0 & 0xf0) >> 4;
    this->version.jtag = ((b0 & 0x0f) << 2) | ((b1 & 0xc0) >> 6);
    this->version.swim = b1 & 0x3f;
    return this->version;
}

quint8 stlinkv2::getMode() {

    qDebug() << "***[getMode]***";
    if (this->Command(STLink::Cmd::GetCurrentMode, 0, 2)) {
        return this->mode_id = this->recv_buf.at(0);
    }
    return this->mode_id;
}

quint8 stlinkv2::getStatus()
{
    qDebug() << "***[getStatus]***";
    this->DebugCommand(STLink::Cmd::Dbg::GetStatus, 0, 2);
    return this->recv_buf.at(0);
}

quint32 stlinkv2::getCoreID() {

    qDebug() << "***[getCoreID]***";
    this->DebugCommand(STLink::Cmd::Dbg::ReadCoreID, 0, 4);
    this->core_id = qFromLittleEndian<quint32>((uchar*)this->recv_buf.constData());
    qInformal() << "CoreID:" << QString::number(this->core_id, 16);
    return this->core_id;
}

quint32 stlinkv2::getChipID()
{
    qDebug() << "***[getChipID]***";

    if (this->core_id == 0xFFFFFFFF || this->core_id == 0x00000000)
        return 0;

    if (this->core_id == Cortex::CoreID::M0_R0) {
        this->readMem32(Cortex::Reg::CM0_CHIPID);
        qInformal() << "CM0 Searching at" << QString::number(Cortex::Reg::CM0_CHIPID, 16);
    }
    else {
        this->readMem32(Cortex::Reg::CM3_CHIPID);
        qInformal() << "CM3/4 Searching at" << QString::number(Cortex::Reg::CM3_CHIPID, 16);
    }
    this->chip_id = qFromLittleEndian<quint32>((uchar*)this->recv_buf.constData());
    this->chip_id &= 0xFFF;
    // CM4 rev0 fix
    if (((this->chip_id & 0xFFF) == STM32::ChipID::F2) && (this->core_id == Cortex::CoreID::M4_R0)) {
      qDebug() << "STM32F4 rev 0 errata";
      this->chip_id = STM32::ChipID::F4;
    }
    qInformal() << "ChipID:" << QString::number(this->chip_id, 16);
    return this->chip_id;
}

quint32 stlinkv2::getRevID()
{
    qDebug() << "***[getRevID]***";

    this->readMem32(Cortex::Reg::CM3_CHIPID);
    this->rev_id = this->recv_buf.at(2) | (this->recv_buf.at(3) << 8);

    qInformal() << "RevID:" << QString::number(this->rev_id, 16);
    return this->rev_id;
}
quint32 stlinkv2::readFlashSize()
{
    qDebug() << "***[readFlashSize]***";
    this->readMem32((*this->device)["flash_size_reg"]);
    (*this->device)["flash_size"] = qFromLittleEndian<quint32>((uchar*)this->recv_buf.constData());
    if (this->chip_id == STM32::ChipID::F4) {
        (*this->device)["flash_size"] = (*this->device)["flash_size"] >> 16;
    }
    else {
        (*this->device)["flash_size"] &= 0xFFFF;
    }
    qInformal() << "Flash size:" << (*this->device)["flash_size"] << "KB";
    return (*this->device)["flash_size"];
}

void stlinkv2::setModeJTAG()
{
    qDebug() << "***[setModeJTAG]***";
    this->getMode();
//    if (this->mode_id != STLINK_DEV_DEBUG_MODE)
    this->setExitModeDFU();
    this->DebugCommand(STLink::Cmd::Dbg::EnterMode, STLink::Cmd::Dbg::EnterJTAG, 0);
}

void stlinkv2::setModeSWD()
{
    qDebug() << "***[setModeSWD]***";
    this->getMode();
//    if (this->mode_id != STLINK_DEV_DEBUG_MODE)
    this->setExitModeDFU();
    this->DebugCommand(STLink::Cmd::Dbg::EnterMode, STLink::Cmd::Dbg::EnterSWD, 0);
}

void stlinkv2::setExitModeDFU()
{
    qDebug() << "***[setExitModeDFU]***";
    this->Command(STLink::Cmd::DFUCommand, STLink::Cmd::DFUExit, 0);
}

void stlinkv2::resetMCU()
{
    qDebug() << "***[resetMCU]***";
    this->DebugCommand(STLink::Cmd::Dbg::ResetSys, 0, 2);
}

void stlinkv2::hardResetMCU()
{
    qDebug() << "***[hardResetMCU]***";
//    this->DebugCommand(STLinkDebugExit, 0, 2);
//    this->setExitModeDFU();
    this->Command(STLink::Cmd::Reset, 0, 8);
    this->DebugCommand(STLink::Cmd::Dbg::HardReset, 0x02, 2);
}

void stlinkv2::runMCU()
{
    qDebug() << "***[runMCU]***";
    this->DebugCommand(STLink::Cmd::Dbg::RunCore, 0, 2);
}

void stlinkv2::haltMCU()
{
    qDebug() << "***[haltMCU]***";
    this->DebugCommand(STLink::Cmd::Dbg::StepCore, 0, 2);
}

bool stlinkv2::eraseFlash()
{
    qDebug() << "***[eraseFlash]***";
    // We set the mass erase flag
    if (!this->setMassErase(true))
        return false;

    // We set the STRT flag in order to start the mass erase
    qInformal() << "Erasing flash... This might take some time.";
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
        qDebug() << "***[unlockFlash]***";
        uchar buf[4];

        const quint32 addr = (*this->device)["flash_int_reg"] + (*this->device)["KEYR_OFFSET"];

        qToLittleEndian(STM32::Flash::KEY1, buf);
        this->send_buf.append((const char*)buf, sizeof(buf));
        this->writeMem32(addr,  this->send_buf);

        this->send_buf.clear();
        qToLittleEndian(STM32::Flash::KEY2, buf);
        this->send_buf.append((const char*)buf, sizeof(buf));
        this->writeMem32(addr,  this->send_buf);

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
        qDebug() << "***[lockFlash]***";
        uchar buf[4];
        quint32 addr, lock;
        quint32 fcr = this->readFlashCR();
        lock = fcr | (1 << (*this->device)["CR_LOCK"]);
        addr = (*this->device)["flash_int_reg"] + (*this->device)["CR_OFFSET"];
        qToLittleEndian(lock, buf);
        this->send_buf.append((const char*)buf, sizeof(buf));
        this->writeMem32(addr,  this->send_buf);
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
    qDebug() << "***[isLocked]***";
    bool res = false;
    const quint32 cr = this->readFlashCR();
//    qDebug() << "Lock bit" << (*this->device)["CR_LOCK"];
    res = cr & (1 << (*this->device)["CR_LOCK"]);

    qDebug() << "Flash locked:" << res;
    return res;
}

quint32 stlinkv2::readFlashSR()
{
    qDebug() << "***[readFlashSR]***";
    quint32 res;

    readMem32((*this->device)["flash_int_reg"] + (*this->device)["SR_OFFSET"], sizeof(quint32));
    res =  qFromLittleEndian<quint32>((const uchar*)this->recv_buf.constData());
    qDebug() << "Flash status register:" << "0x"+QString::number(res, 16) << regPrint(res);
    return res;
}

quint32 stlinkv2::readFlashCR()
{
    qDebug() << "***[readFlashCR]***";
    quint32 res;

    readMem32((*this->device)["flash_int_reg"] + (*this->device)["CR_OFFSET"], sizeof(quint32));
    res =  qFromLittleEndian<quint32>((const uchar*)this->recv_buf.constData());
    qDebug() << "Flash control register:" << "0x"+QString::number(res, 16) << regPrint(res);
    return res;
}

quint32 stlinkv2::writeFlashCR(quint32 mask, bool value)
{
//    if (this->isLocked())
//        this->unlockFlash();

    qDebug() << "***[writeFlashCR]***";
    uchar buf[4];
    quint32 addr, val;
    quint32 fcr = this->readFlashCR();
    if (value)
        val = fcr | mask ; // We append bits (OR)
    else
        val = fcr & ~mask; // We remove bits (NOT AND)
    qDebug() << "Flash control register new value:" << "0x"+QString::number(val, 16) << regPrint(val);

    addr = (*this->device)["flash_int_reg"] + (*this->device)["CR_OFFSET"];

    qToLittleEndian(val, buf);
    this->send_buf.append((const char*)buf, sizeof(buf));
    this->writeMem32(addr,  this->send_buf);
    return this->readFlashCR();
}

bool stlinkv2::setFlashProgramming(bool val)
{
    qDebug() << "***[setFlashProgramming]***";
    const quint32 mask = (1 << STM32::Flash::CR_PG);
    const bool res = (this->writeFlashCR(mask, val) & mask) == mask;

    qDebug() << "Flash programming enabled:" << res;
    return res;
}

bool stlinkv2::setMassErase(bool val)
{
    qDebug() << "***[setMassErase]***";
    const quint32 mask = (1 << STM32::Flash::CR_MER);

    return (this->writeFlashCR(mask, val) & mask) == mask;
}

bool stlinkv2::setSTRT()
{
    qDebug() << "***[setSTRT]***";
    quint32 mask = 0;

    if (this->chip_id == STM32::ChipID::F4) {
        mask |= (1 << STM32::Flash::F4_CR_STRT);
    }
    else {
        mask |= (1 << STM32::Flash::CR_STRT);
    }
    return (this->writeFlashCR(mask, true) & mask) == mask;
}

void stlinkv2::setProgramSize(quint8 size)
{
    qDebug() << "***[setProgramSize]***";

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
    qDebug() << "***[isBusy]***";
    bool res;

    const quint32 sr = this->readFlashSR();
    res = sr & (1 << (*this->device)["SR_BSY"]);

    qDebug() << "Flash busy:" << res;
    return res;
}

void stlinkv2::writeMem32(quint32 addr, QByteArray &buf)
{
    qDebug() << "+++++[writeMem32] Writing" << buf.size() << "bytes to" << "0x"+QString::number(addr, 16).toUpper();
    if (buf.size() % 4 != 0) {
        qCritical() << "Data is not 32 bit aligned!";
        return;
    }

    // Any writing to flash while busy = ART processor hangs
    if (addr >= (*this->device)["flash_base"] && addr <= (*this->device)["flash_base"]+((*this->device)["flash_size"]*1024)) {
        while(this->isBusy()) {
            usleep(100000); // 100ms
        }
        // Check if write succeeded
        if ((*this->device)["chip_id"] == STM32::ChipID::F4) {
            if (this->readFlashSR() & 242)
                qCritical() << "Flash write failed!";
        }
        else if (this->readFlashSR() & (1 << (*this->device)["SR_PER"]))
            qCritical() << "Flash write failed!";
    }
    this->cmd_buf.append(STLink::Cmd::DebugCommand);
    this->cmd_buf.append(STLink::Cmd::Dbg::WriteMem32bit);
    uchar _addr[4], _len[2];
    qToLittleEndian(addr, _addr);
    qToLittleEndian((quint16)buf.size(), _len);
    this->cmd_buf.append((const char*)_addr, sizeof(_addr));
    this->cmd_buf.append((const char*)_len, sizeof(_len));
//    this->cmd_buf.append((quint16)buf.size());
    this->SendCommand();

    // the actual data we are writing is on the second command
    this->cmd_buf.append(buf);
    this->SendCommand();
    buf.clear();
}

qint32 stlinkv2::readMem32(quint32 addr, quint16 len)
{
    qDebug() << "+++++[readMem32] Reading at" << "0x"+QString::number(addr, 16).toUpper();
    if (len % 4 != 0)
        return 0;
    this->cmd_buf.append(STLink::Cmd::DebugCommand);
    this->cmd_buf.append(STLink::Cmd::Dbg::ReadMem32bit);
    uchar cmd[4], _len[2];
    qToLittleEndian(addr, cmd);
    qToLittleEndian(len, _len);
    this->cmd_buf.append((const char*)cmd, sizeof(cmd));
    this->cmd_buf.append((const char*)_len, sizeof(_len)); //length the data we are requesting
    this->SendCommand();
    this->recv_buf.clear();
    return this->usb->read(&this->recv_buf, len);
}

qint32 stlinkv2::Command(quint8 st_cmd0, quint8 st_cmd1, quint32 resp_len)
{
    this->cmd_buf.append(st_cmd0);
    this->cmd_buf.append(st_cmd1);

    this->SendCommand();
    if (resp_len > 0)
        return this->usb->read(&this->recv_buf, resp_len);
    return 0;
}

qint32 stlinkv2::DebugCommand(quint8 st_cmd1, quint8 st_cmd2, quint32 resp_len)
{
    this->cmd_buf.append(STLink::Cmd::DebugCommand);
    this->cmd_buf.append(st_cmd1);
    this->cmd_buf.append(st_cmd2);

    qint32 res = this->SendCommand();
//    if (res > 0)
//        qDebug() << res << " Bytes sent";
//    else
//        qCritical() << "Error: " << res;

    if (resp_len > 0)
        return this->usb->read(&this->recv_buf, resp_len);
    return res;
}

bool stlinkv2::writeRegister(quint32 val, quint8 index) // Not working on F4 ?
{
    qDebug() << "***[writeRegister]***";
    this->cmd_buf.append(STLink::Cmd::DebugCommand);
    this->cmd_buf.append(STLink::Cmd::Dbg::WriteReg);
    this->cmd_buf.append(index);
    uchar tval[4];
    qToLittleEndian(val, tval);
    this->cmd_buf.append((const char*)tval, sizeof(tval));
    this->SendCommand();
    QByteArray tmp;
    this->usb->read(&tmp, 2);

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
    qDebug() << "***[readRegister]***";
    this->cmd_buf.append(STLink::Cmd::DebugCommand);
    this->cmd_buf.append(STLink::Cmd::Dbg::ReadReg);
    this->cmd_buf.append(index);
    this->SendCommand();

    QByteArray tmp;
    this->usb->read(&tmp, 4);
    return qFromLittleEndian<quint32>((const uchar*)tmp.constData());
}

void stlinkv2::writePC(quint32 val) // Not working
{
    qDebug() << "***[writePC]***";
    this->cmd_buf.append(STLink::Cmd::DebugCommand);
    this->cmd_buf.append(STLink::Cmd::Dbg::WriteRegPC);
    this->cmd_buf.append((quint8)0x07);
    uchar tval[4];
    qToLittleEndian(val, tval);
    this->cmd_buf.append((const char*)tval, sizeof(tval));
    this->SendCommand();
}

qint32 stlinkv2::SendCommand()
{
    qint32 ret = 0;

//    while (this->cmd_buf.size() < 16)
//        this->cmd_buf.append((char)0);

    ret = this->usb->write(&this->cmd_buf, this->cmd_buf.size());
    this->cmd_buf.clear();
    if (ret > 0) {
//        qDebug() << "Command sent successfully.";
    }
    else {
        qCritical() << "SendCommand() Error!";
    }
    return ret;
}

void stlinkv2::sendLoader() {

    m_loader.loadBin(this->device->loader_file);

    QByteArray tmp;
    int i=0;
    const int step = 128;
    for (; i < m_loader.refData().size()/step; i++) {

        tmp = QByteArray(m_loader.refData().constData()+(i*step), step);
        this->writeMem32((*this->device)["sram_base"]+(i*step), tmp);
    }
    const int mod = m_loader.refData().size()%step;
    tmp = QByteArray(m_loader.refData().constData()+m_loader.refData().size()-mod, mod);
    this->writeMem32((*this->device)["sram_base"]+(i*step), tmp);

//    this->writeMem32((*this->device)["sram_base"], m_loader.refData());
    this->writeRegister((*this->device)["sram_base"], 15); // PC register to sram base.
}

bool stlinkv2::setLoaderBuffer(quint32 addr, const QByteArray& buf) {

    using namespace Loader::Addr;
    uchar ar_tmp[4];
    qToLittleEndian(addr, ar_tmp);
    QByteArray tmp((const char*)ar_tmp, 4);
    this->writeMem32(PARAMS+OFFSET_DEST, tmp);

    qToLittleEndian(buf.size(), ar_tmp);
    tmp = QByteArray((const char*)ar_tmp, 4);
    this->writeMem32(PARAMS+OFFSET_LEN, tmp);

    this->readMem32(PARAMS+OFFSET_DEST);
    const quint32 dest = qFromLittleEndian<quint32>((uchar*)this->recv_buf.constData());
    this->readMem32(PARAMS+OFFSET_LEN);
    const quint32 len = qFromLittleEndian<quint32>((uchar*)this->recv_buf.constData());

    qDebug() << "Data destination and length:" << "0x"+QString::number(dest, 16) << len;

    if ((dest != addr) || ((quint32)buf.size() != len)) {

        qCritical() << "Failed to set loader settings!";
        qCritical() << "Expected data destination and length:" << "0x"+QString::number(addr, 16) << buf.size();
        return false;
    }

    int i=0;
    const int step = 2048;
    for (; i < buf.size()/step; i++) {

        tmp = QByteArray(buf.constData()+(i*step), step);
        this->writeMem32(BUFFER+(i*step), tmp);
    }
    const int mod = buf.size()%step;
    if (mod>0)  {
        tmp = QByteArray(buf.constData()+buf.size()-mod, mod);
        this->writeMem32(BUFFER+(i*step), tmp);
    }
    return true;
}

quint32 stlinkv2::getLoaderStatus() {

    qDebug() << "***[getLoaderStatus]***";
    using namespace Loader::Addr;
    this->readMem32(PARAMS+OFFSET_STATUS);
    quint32 tmp = qFromLittleEndian<quint32>((uchar*)this->recv_buf.constData());
//    qDebug() << this->regPrint(tmp);
    return tmp;
}

quint32 stlinkv2::getLoaderPos() {

    qDebug() << "***[getLoaderPos]***";
    using namespace Loader::Addr;
    this->readMem32(PARAMS+OFFSET_POS);
    quint32 tmp = qFromLittleEndian<quint32>((uchar*)this->recv_buf.constData());
//    qDebug() << this->regPrint(tmp);
    return tmp;
}

void stlinkv2::getLoaderParams() {

    qDebug() << "***[getLoaderParams]***";
    using namespace Loader::Addr;
    this->readMem32(PARAMS+OFFSET_DEST);
    const quint32 dest = qFromLittleEndian<quint32>((uchar*)this->recv_buf.constData());
    this->readMem32(PARAMS+OFFSET_LEN);
    const quint32 len = qFromLittleEndian<quint32>((uchar*)this->recv_buf.constData());

    this->readMem32(PARAMS+OFFSET_TEST);
    const quint32 test = qFromLittleEndian<quint32>((uchar*)this->recv_buf.constData());

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

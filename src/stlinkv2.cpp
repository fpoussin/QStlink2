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
    this->libusb = new LibUsb;
    this->verbose = 0;
    this->mode_id = -1;
    this->core_id = 0;
    this->chip_id = 0;
    this->STLink_ver = 0;
    this->connected = false;
}

stlinkv2::~stlinkv2()
{
    this->disconnect();
    delete libusb;
}

qint32 stlinkv2::connect()
{
    qint32 open;
    if ((open = this->libusb->open()))
        this->libusb->read(&this->recv_buf, 6192); // We clean the usb buffer
    this->connected = true;
    return open;
}

void stlinkv2::disconnect()
{
    this->DebugCommand(STLink::Cmd::Dbg::Exit, 0, 2);
    this->libusb->close();
    this->connected = false;
}

bool stlinkv2::isConnected()
{
    return this->connected;
}

QString stlinkv2::getVersion() {

    qDebug() << "***[getVersion]***";
    this->Command(STLink::Cmd::GetVersion, 0x80, 6);
    char b0 = this->recv_buf.at(0);
    char b1 = this->recv_buf.at(1);
    this->STLink_ver = (b0 & 0xf0) >> 4;
    this->JTAG_ver = ((b0 & 0x0f) << 2) | ((b1 & 0xc0) >> 6);
    this->SWIM_ver = b1 & 0x3f;
    this->version = "ST-Link Version: "+QString::number(this->STLink_ver)+
                    "\nJTAG Version: "+QString::number(this->JTAG_ver)+
                    "\nSWIM Version: "+QString::number(this->SWIM_ver);
    return this->version;
}

QString stlinkv2::getMode() {

    qDebug() << "***[getMode]***";
    if (this->Command(STLink::Cmd::GetCurrentMode, 0, 2)) {
        QString mode;
        switch (this->mode_id = this->recv_buf.at(0)) {
        case -1:
            mode = "Unknown";
            break;
        case 0:
            mode = "DFU";
            break;
        case 1:
            mode = "Mass Storage";
            break;
        case 2:
            mode = "Debug";
            break;
        default:
            mode = "Unknown";
            break;
        }
        this->mode = "Mode: "+mode;
        return this->mode;
    }
    return this->mode;
}

QString stlinkv2::getStatus()
{
    qDebug() << "***[getStatus]***";
    this->DebugCommand(STLink::Cmd::Dbg::GetStatus, 0, 2);
    if ((uchar)this->recv_buf.at(0) == STLink::Status::CORE_RUNNING)
        return QString("Status: Core Running");
    else if ((uchar)this->recv_buf.at(0) == STLink::Status::CORE_HALTED)
        return QString("Status: Core Halted");
    else
        return QString("Status: Unknown");
}

QString stlinkv2::getCoreID() {

    qDebug() << "***[getCoreID]***";
    this->DebugCommand(STLink::Cmd::Dbg::ReadCoreID, 0, 4);
    this->core_id = qFromLittleEndian<quint32>((uchar*)this->recv_buf.constData());
    qInformal() << "CoreID:" << QString::number(this->core_id, 16);
    return QString::number(this->core_id, 16);
}

QString stlinkv2::getChipID()
{
    qDebug() << "***[getChipID]***";

    if (this->core_id == 0xFFFFFFFF || this->core_id == 0x00000000)
        return "Unknown";

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
    return QString::number(this->chip_id, 16);
}

QString stlinkv2::getRevID()
{
    qDebug() << "***[getRevID]***";

    this->readMem32(Cortex::Reg::CM3_CHIPID);
    this->rev_id = this->recv_buf.at(2) | (this->recv_buf.at(3) << 8);

    qInformal() << "RevID:" << QString::number(this->rev_id, 16);
    return QString::number(this->rev_id, 16);
}
quint32 stlinkv2::readFlashSize()
{
    qDebug() << "***[readFlashSize]***";
    this->readMem32((*this->device)["flash_size_reg"]);
    this->flash_size = qFromLittleEndian<quint32>((uchar*)this->recv_buf.constData());
    if (this->chip_id == STM32::ChipID::F4) {
        this->flash_size = this->flash_size >> 16;
    }
    else {
        this->flash_size &= 0xFFFF;
    }
    qInformal() << "Flash size:" << this->flash_size << "KB";
    return this->flash_size;
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
    this->DebugCommand(STLink::Cmd::Dbg::RunCore, 0, 2);
}

void stlinkv2::haltMCU()
{
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
        usleep(1000000); // 500ms
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

    res = cr & (1 << (*this->device)["CR_LOCK"]);

    qDebug() << "Flash locked:" << res;
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

quint32 stlinkv2::writeFlashCR(const quint32 &mask, const bool &value)
{
//    if (this->isLocked())
//        this->unlockFlash();

    qDebug() << "***[writeFlashCR]***";
    uchar buf[4];
    quint32 addr, val;
    quint32 fcr = this->readFlashCR();
    if (value)
        val = mask | fcr; // We append bits (AND)
    else
        val = mask ^ fcr; // We remove bits (XOR)
    qDebug() << "Flash control register new value:" << "0x"+QString::number(val, 16) << regPrint(val);

    addr = (*this->device)["flash_int_reg"] + (*this->device)["CR_OFFSET"];

    qToLittleEndian(val, buf);
    this->send_buf.append((const char*)buf, sizeof(buf));
    this->writeMem32(addr,  this->send_buf);
    return this->readFlashCR();
}

bool stlinkv2::setFlashProgramming(const bool &val)
{
    qDebug() << "***[setFlashProgramming]***";
    const quint32 mask = (1 << STM32::Flash::CR_PG);
    const bool res = (this->writeFlashCR(mask, val) & mask) == mask;

    qDebug() << "Flash programming enabled:" << res;
    return res;
}

bool stlinkv2::setMassErase(const bool &val)
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

void stlinkv2::setProgramSize(const quint8 &size)
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

    readMem32((*this->device)["flash_int_reg"] + (*this->device)["SR_OFFSET"], sizeof(quint32));
    const quint32 sr = qFromLittleEndian<quint32>((const uchar*)this->recv_buf.constData());
    if(this->chip_id == STM32::ChipID::F4) {
        res = sr & (1 << STM32::Flash::F4_SR_BSY);
    }
    else {
        res = sr & (1 << STM32::Flash::SR_BSY);;
    }
    qDebug() << "Flash busy:" << res;
    return res;
}

void stlinkv2::writeMem32(const quint32 &addr, QByteArray &buf)
{
    qDebug() << "+++++[writeMem32] Writing" << buf.size() << "bytes to" << "0x"+QString::number(addr, 16).toUpper();
    if (buf.size() % 4 != 0) {
        qCritical() << "Data is not 32 bit aligned!";
        return;
    }

    // Any writing to flash while busy = ART processor hangs
    if (addr >= (*this->device)["flash_base"] && addr <= (*this->device)["flash_base"]+(*this->device)["flash_size"])
        while(this->isBusy())
            usleep(100000); // 100ms

    this->cmd_buf.append(STLink::Cmd::DebugCommand);
    this->cmd_buf.append(STLink::Cmd::Dbg::WriteMem32bit);
    uchar _addr[4];
    qToLittleEndian(addr, _addr);
    this->cmd_buf.append((const char*)_addr, sizeof(_addr));
    this->cmd_buf.append((quint16)buf.size());
    this->SendCommand();

    // the actual data we are writing is on the second command
    this->cmd_buf.append(buf);
    this->SendCommand();
    buf.clear();
}

qint32 stlinkv2::readMem32(const quint32 &addr, const quint16 &len)
{
    qDebug() << "+++++[readMem32] Reading at" << "0x"+QString::number(addr, 16).toUpper();
    if (len % 4 != 0)
        return 0;
    this->cmd_buf.append(STLink::Cmd::DebugCommand);
    this->cmd_buf.append(STLink::Cmd::Dbg::ReadMem32bit);
    uchar cmd[4];
    uchar _len[2];
    qToLittleEndian(addr, cmd);
    qToLittleEndian(len, _len);
    this->cmd_buf.append((const char*)cmd, sizeof(cmd));
    this->cmd_buf.append((const char*)_len, sizeof(_len)); //length the data we are requesting
    this->SendCommand();
    return this->libusb->read(&this->recv_buf, len);
}

qint32 stlinkv2::Command(const quint8 &st_cmd0, const quint8 &st_cmd1, const quint32 &resp_len)
{
    this->cmd_buf.append(st_cmd0);
    this->cmd_buf.append(st_cmd1);

    this->SendCommand();
    if (resp_len > 0)
        return this->libusb->read(&this->recv_buf, resp_len);
    return 0;
}

qint32 stlinkv2::DebugCommand(const quint8 &st_cmd1, const quint8 &st_cmd2, const quint32 &resp_len)
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
        return this->libusb->read(&this->recv_buf, resp_len);
    return res;
}


qint32 stlinkv2::SendCommand()
{
    qint32 ret = 0;
    ret = this->libusb->write(&this->cmd_buf, this->cmd_buf.size());
    this->cmd_buf.clear();
    if (ret > 0) {
//        qDebug() << "Command sent successfully.";
    }
    else {
        qCritical() << "Error!";
    }
    return ret;
}

QString stlinkv2::regPrint(const quint32 reg)
{
    const QString regstr(QString::number(reg, 2));
    QString top("\r\n");
    QString bottom("\r\n");
    for (qint32 i=0;i<regstr.length();i++) {
        const quint8 pos = regstr.length()-(i+1);
        top.append(QString::number(pos)+" ");
        if (pos >= 10)
            bottom.append(QString(regstr.at(i))+"  ");
        else
            bottom.append(QString(regstr.at(i))+" ");
    }
    return top+bottom;
}

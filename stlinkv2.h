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
#ifndef STLINKV2_H
#define STLINKV2_H

#include <QObject>
#include <QThread>
#include <LibUsb.h>
#include <QFile>
#include <QByteArray>
#include <QtEndian>
#include <devices.h>

#define qInformal() qWarning()

const quint8 STLINK_OK = 0x80;
const quint8 STLINK_FALSE = 0x81;
const quint8 STLINK_CORE_RUNNING = 0x80;
const quint8 STLINK_CORE_HALTED = 0x81;
const quint8 STLINK_CORE_UNKNOWN_STATE = 2; /* Not reported, internal use. */

const quint8 STLINK_DEV_DFU_MODE = 0x00;
const quint8 STLINK_DEV_MASS_MODE = 0x01;
const quint8 STLINK_DEV_DEBUG_MODE = 0x02;
const quint8 STLINK_DEV_UNKNOWN_MODE = -1;

const quint32 CM3_REG_CHIPID = 0xE0042000;
const quint32 CM3_REG_CPUID = 0xE000ED00;
const quint32 CM3_REG_FP_CTRL = 0xE0002000;
const quint32 CM3_REG_FP_COMP0 = 0xE0002008;

// Constant STM32 memory map figures
const quint32 STM32_FLASH_BASE = 0x08000000;
const quint32 STM32_SRAM_BASE = 0x20000000;

// stm32 chipids, only lower 12 bits...
const quint32 STM32_CHIPID_F1_MEDIUM = 0x410;
const quint32 STM32_CHIPID_F2 = 0x411;
const quint32 STM32_CHIPID_F1_LOW = 0x412;
const quint32 STM32_CHIPID_F4 = 0x413;
const quint32 STM32_CHIPID_F1_HIGH = 0x414;
const quint32 STM32_CHIPID_L1_MEDIUM = 0x416;
const quint32 STM32_CHIPID_F1_CONN = 0x418;
const quint32 STM32_CHIPID_F1_VL_MEDIUM = 0x420;
const quint32 STM32_CHIPID_F1_VL_HIGH = 0x428;
const quint32 STM32_CHIPID_F1_XL = 0x430;

const quint32 CORE_M3_R1 = 0x1BA00477;
const quint32 CORE_M3_R2 = 0x4BA00477;
const quint32 CORE_M4_R0 = 0x2BA01477;

const quint8 STLinkGetVersion = 0xF1;
const quint8 STLinkDebugCommand = 0xF2;
const quint8 STLinkDFUCommand = 0xF3;
const quint8 STLinkDFUExit = 0x07;
const quint8 STLinkDFUGetVersion = 0x08;
const quint8 STLinkGetCurrentMode = 0xF5;
const quint8 STLinkReset = 0xF7;

const quint8 STLinkDebugReadMem32bit = 0x07;
const quint8 STLinkDebugWriteMem32bit = 0x08;
const quint8 STLinkDebugWriteMem8bit = 0x0d;
const quint8 STLinkDebugEnterMode = 0x20;
const quint8 STLinkDebugEnterSWD = 0xA3;
const quint8 STLinkDebugEnterJTAG = 0x00;
const quint8 STLinkDebugExit = 0x21;
const quint8 STLinkDebugReadCoreID = 0x22;
const quint8 STLinkDebugGetStatus = 0x01;
const quint8 STLinkDebugForceDebug = 0x02;
const quint8 STLinkDebugResetSys = 0x03;
const quint8 STLinkDebugReadAllRegs = 0x04;
const quint8 STLinkDebugRunCore = 0x09;
const quint8 STLinkDebugStepCore = 0x0A;
const quint8 STLinkDebugHardReset = 0x3C; // Unsure
const quint8 STLinkDebugReadCoreRegs = 0x3A; // All regs fetched at once

/* stm32f FPEC flash controller interface, pm0063 manual */
// TODO - all of this needs to be abstracted out....
const quint32 FLASH_REGS_ADDR = 0x40022000;
const quint8 FLASH_REGS_SIZE = 0x28;

const quint32 FLASH_ACR = FLASH_REGS_ADDR + 0x00;
const quint32 FLASH_KEYR = FLASH_REGS_ADDR + 0x04;
const quint32 FLASH_OPT_KEYR = FLASH_REGS_ADDR + 0x08;
const quint32 FLASH_SR = FLASH_REGS_ADDR + 0x0c;
const quint32 FLASH_CR = FLASH_REGS_ADDR + 0x10;
const quint32 FLASH_AR = FLASH_REGS_ADDR + 0x14;
const quint32 FLASH_OBR = FLASH_REGS_ADDR + 0x1c;
const quint32 FLASH_WRPR = FLASH_REGS_ADDR + 0x20;

// Needed to unlock the flash before erase/writing
const quint32 FLASH_RDPTR_KEY = 0x00a5;
const quint32 FLASH_KEY1 = 0x45670123;
const quint32 FLASH_KEY2 = 0xcdef89ab;
const quint32 FLASH_OPTKEY1 = 0x08192A3B;
const quint32 FLASH_OPTKEY2 = 0x4C5D6E7F;

const quint8 FLASH_SR_BSY = 0;
const quint8 FLASH_SR_EOP = 5;

const quint8 FLASH_CR_PG = 0;
const quint8 FLASH_CR_PER = 1;
const quint8 FLASH_CR_MER = 2;
const quint8 FLASH_CR_STRT = 6;
const quint8 FLASH_CR_LOCK = 7;
const quint8 FLASH_CR_PGSIZE = 8;

//32L = 32F1 same CoreID as 32F4!
const quint32 STM32L_FLASH_REGS_ADDR = 0x40023c00;
const quint32 STM32L_FLASH_ACR = STM32L_FLASH_REGS_ADDR + 0x00;
const quint32 STM32L_FLASH_PECR = STM32L_FLASH_REGS_ADDR + 0x04;
const quint32 STM32L_FLASH_PDKEYR = STM32L_FLASH_REGS_ADDR + 0x08;
const quint32 STM32L_FLASH_PEKEYR = STM32L_FLASH_REGS_ADDR + 0x0c;
const quint32 STM32L_FLASH_PRGKEYR = STM32L_FLASH_REGS_ADDR + 0x10;
const quint32 STM32L_FLASH_OPTKEYR = STM32L_FLASH_REGS_ADDR + 0x14;
const quint32 STM32L_FLASH_SR = STM32L_FLASH_REGS_ADDR + 0x18;
const quint32 STM32L_FLASH_OBR = STM32L_FLASH_REGS_ADDR + 0x0c;
const quint32 STM32L_FLASH_WRPR = STM32L_FLASH_REGS_ADDR + 0x20;

//STM32F4
const quint32 FLASH_F4_REGS_ADDR = 0x40023c00;
const quint32 FLASH_F4_KEYR = FLASH_F4_REGS_ADDR + 0x04;
const quint32 FLASH_F4_OPT_KEYR = FLASH_F4_REGS_ADDR + 0x08;
const quint32 FLASH_F4_SR = FLASH_F4_REGS_ADDR + 0x0c;
const quint32 FLASH_F4_CR = FLASH_F4_REGS_ADDR + 0x10;
const quint32 FLASH_F4_OPT_CR = FLASH_F4_REGS_ADDR + 0x14;
const quint8 FLASH_F4_CR_STRT = 16;
const quint8 FLASH_F4_CR_LOCK = 31;
const quint8 FLASH_F4_CR_SER = 1;
const quint8 FLASH_F4_CR_SNB = 3;
const quint8 FLASH_F4_CR_SNB_MASK = 0x38;
const quint8 FLASH_F4_SR_BSY = 16;

class stlinkv2 : public QThread
{
    Q_OBJECT
public:
    explicit stlinkv2(QObject *parent = 0);
    ~stlinkv2();
    Device *device;
    quint32 STLink_ver;
    quint32 JTAG_ver;
    quint32 SWIM_ver;
    quint32 core_id;
    quint32 chip_id;
    quint32 flash_size;
    QByteArray cmd_buf;
    QByteArray recv_buf;
    QByteArray send_buf;
    bool isConnected();

signals:

public slots:
    qint32 connect();
    void disconnect();
    void resetMCU();
    void hardResetMCU();
    void haltMCU();
    void runMCU();
    void setModeJTAG();
    void setModeSWD();
    void setExitModeDFU();
    qint32 readMem32(const quint32 &addr, const quint16 &len = 4);
    void writeMem32(const quint32 &addr, QByteArray &buf);
    bool isLocked();
    void eraseFlash();
    bool unlockFlash();
    bool lockFlash();
    bool unlockFlashOpt();
    bool isBusy();
    bool setFlashProgramming(const bool &val);
    bool setMassErase(const bool &val);
    void setSTRT();
    void setProgramSize(const quint8 &size);
    quint32 readFlashSize();
    QString getStatus();
    QString getVersion();
    QString getMode();
    QString getCoreID();
    QString getChipID();

private:
    LibUsb *libusb;
    qint32 Command(const quint8 &st_cmd0, const quint8 &st_cmd1, const quint32 &resp_len);
    qint32 DebugCommand(const quint8 &st_cmd1, const quint8 &st_cmd2, const quint32 &resp_len);
    quint32 readFlashCR();
    quint32 writeFlashCR(const quint32 &mask, const bool &value);
    qint32 SendCommand();
    quint16 ST_VendorID;
    quint16 ST_ProductID;
    quint8 verbose;
    QString version;
    QString mode;
    qint8 mode_id;
    bool connected;
};

#endif // STLINKV2_H

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

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0)

#define STLINK_OK 0x80
#define STLINK_FALSE 0x81
#define STLINK_CORE_RUNNING 0x80
#define STLINK_CORE_HALTED 0x81
#define STLINK_CORE_UNKNOWN_STATE 2 /* Not reported, internal use. */

#define STLINK_DEV_DFU_MODE 0x00
#define STLINK_DEV_MASS_MODE 0x01
#define STLINK_DEV_DEBUG_MODE 0x02
#define STLINK_DEV_UNKNOWN_MODE	-1

#define CM3_REG_CHIPID 0xE0042000
#define CM3_REG_CPUID 0xE000ED00
#define CM3_REG_FP_CTRL 0xE0002000
#define CM3_REG_FP_COMP0 0xE0002008

// Constant STM32 memory map figures
#define STM32_FLASH_BASE 0x08000000
#define STM32_SRAM_BASE 0x20000000

// stm32 chipids, only lower 12 bits...
#define STM32_CHIPID_F1_MEDIUM 0x410
#define STM32_CHIPID_F2 0x411
#define STM32_CHIPID_F1_LOW 0x412
#define STM32_CHIPID_F4 0x413
#define STM32_CHIPID_F1_HIGH 0x414
#define STM32_CHIPID_L1_MEDIUM 0x416
#define STM32_CHIPID_F1_CONN 0x418
#define STM32_CHIPID_F1_VL_MEDIUM 0x420
#define STM32_CHIPID_F1_VL_HIGH 0x428
#define STM32_CHIPID_F1_XL 0x430

#define CORE_M3_R1 0x1BA00477
#define CORE_M3_R2 0x4BA00477
#define CORE_M4_R0 0x2BA01477

#define STLinkGetVersion 0xF1
#define STLinkDebugCommand 0xF2
#define STLinkDFUCommand 0xF3
#define STLinkDFUExit 0x07
#define STLinkDFUEnter 0x08 // Unsure...
#define STLinkGetCurrentMode 0xF5
#define STLinkDebugReadMem32bit 0x07 // 0x35?
#define STLinkDebugWriteMem32bit 0x08 //0x36?
#define STLinkDebugWriteMem8bit 0x0d

#define STLinkDebugEnterMode 0x20
#define STLinkDebugEnterSWD 0xA3
#define STLinkDebugEnterJTAG 0x00
#define STLinkDebugExit 0x21
#define STLinkDebugReadCoreID 0x22
#define STLinkDebugGetStatus 0x01
#define STLinkDebugForceDebug 0x02
#define STLinkDebugResetSys 0x03
#define STLinkDebugReadAllRegs 0x04
#define STLinkDebugRunCore 0x09
#define STLinkDebugStepCore 0x0A

/* stm32f FPEC flash controller interface, pm0063 manual */
// TODO - all of this needs to be abstracted out....
#define FLASH_REGS_ADDR 0x40022000
#define FLASH_REGS_SIZE 0x28

#define FLASH_ACR (FLASH_REGS_ADDR + 0x00)
#define FLASH_KEYR (FLASH_REGS_ADDR + 0x04)
#define FLASH_SR (FLASH_REGS_ADDR + 0x0c)
#define FLASH_CR (FLASH_REGS_ADDR + 0x10)
#define FLASH_AR (FLASH_REGS_ADDR + 0x14)
#define FLASH_OBR (FLASH_REGS_ADDR + 0x1c)
#define FLASH_WRPR (FLASH_REGS_ADDR + 0x20)

// Needed to unlock the flash before erase/writing
#define FLASH_RDPTR_KEY 0x00a5
#define FLASH_KEY1 0x45670123
#define FLASH_KEY2 0xcdef89ab
#define FLASH_OPTKEY1 0x08192A3B
#define FLASH_OPTKEY2 0x4C5D6E7F

#define FLASH_SR_BSY 0
#define FLASH_SR_EOP 5

#define FLASH_CR_PG 0
#define FLASH_CR_PER 1
#define FLASH_CR_MER 2
#define FLASH_CR_STRT 6
#define FLASH_CR_LOCK 7
#define FLASH_CR_PGSIZE 8

//32L = 32F1 same CoreID as 32F4!
#define STM32L_FLASH_REGS_ADDR 0x40023c00
#define STM32L_FLASH_ACR (STM32L_FLASH_REGS_ADDR + 0x00)
#define STM32L_FLASH_PECR (STM32L_FLASH_REGS_ADDR + 0x04)
#define STM32L_FLASH_PDKEYR (STM32L_FLASH_REGS_ADDR + 0x08)
#define STM32L_FLASH_PEKEYR (STM32L_FLASH_REGS_ADDR + 0x0c)
#define STM32L_FLASH_PRGKEYR (STM32L_FLASH_REGS_ADDR + 0x10)
#define STM32L_FLASH_OPTKEYR (STM32L_FLASH_REGS_ADDR + 0x14)
#define STM32L_FLASH_SR (STM32L_FLASH_REGS_ADDR + 0x18)
#define STM32L_FLASH_OBR (STM32L_FLASH_REGS_ADDR + 0x0c)
#define STM32L_FLASH_WRPR (STM32L_FLASH_REGS_ADDR + 0x20)

//STM32F4
#define FLASH_F4_REGS_ADDR 0x40023c00
#define FLASH_F4_KEYR (FLASH_F4_REGS_ADDR + 0x04)
#define FLASH_F4_OPT_KEYR (FLASH_F4_REGS_ADDR + 0x08)
#define FLASH_F4_SR (FLASH_F4_REGS_ADDR + 0x0c)
#define FLASH_F4_CR (FLASH_F4_REGS_ADDR + 0x10)
#define FLASH_F4_OPT_CR (FLASH_F4_REGS_ADDR + 0x14)
#define FLASH_F4_CR_STRT 16
#define FLASH_F4_CR_LOCK 31
#define FLASH_F4_CR_SER 1
#define FLASH_F4_CR_SNB 3
#define FLASH_F4_CR_SNB_MASK 0x38
#define FLASH_F4_SR_BSY 16

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

signals:

public slots:
    int connect();
    void disconnect();
    int setup();
    void resetMCU();
    void haltMCU();
    void runMCU();
    void setModeJTAG();
    void setModeSWD();
    void setExitModeDFU();
    int readMem32(quint32 addr, quint16 len = 4);
    void writeMem32(quint32 addr, QByteArray &buf);
    bool isLocked();
    void eraseFlash();
    bool unlockFlash();
    bool lockFlash();
    bool isBusy();
    quint32 readFlashCR();
    quint32 writeFlashCR(quint32 mask, bool value);
    bool setFlashProgramming(bool val);
    bool setMassErase(bool val);
    void setSTRT();
    void setProgramSize(quint8 size);
    quint32 readFlashSize();
    QString getStatus();
    QString getVersion();
    QString getMode();
    QString getCoreID();
    QString getChipID();

private:
    LibUsb *libusb;
    int Command(quint8 st_cmd0, quint8 st_cmd1, int resp_len);
    int DebugCommand(quint8 st_cmd1, quint8 st_cmd2, int resp_len);
    int SendCommand();
    void write_uint32(uchar* buf, quint32 ui);
    void write_uint16(uchar* buf, quint16 ui);
    quint32 read_uint32(const uchar *c);
    quint16 ST_VendorID;
    quint16 ST_ProductID;
    short verbose;
    QString version;
    QString mode;
    char mode_id;
};

#endif // STLINKV2_H

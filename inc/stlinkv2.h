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
#include <compat.h>

namespace STLink {
    namespace Status {
             const quint8 OK = 0x80;
             const quint8 NOK = 0x81;
             const quint8 CORE_RUNNING = 0x80;
             const quint8 CORE_HALTED = 0x81;
             const quint8 CORE_UNKNOWN_STATE = 2; /* Not reported, internal use. */
    }
    namespace Mode {
            const quint8 DFU = 0x00;
            const quint8 MASS = 0x01;
            const quint8 DEBUG = 0x02;
            const quint8 UNKNOWN = -1;
    }
    namespace Cmd {
            const quint8 GetVersion = 0xF1;
            const quint8 DebugCommand = 0xF2;
            const quint8 DFUCommand = 0xF3;
            const quint8 DFUExit = 0x07;
            const quint8 DFUGetVersion = 0x08;
            const quint8 GetCurrentMode = 0xF5;
            const quint8 Reset = 0xF7;
        namespace Dbg {
                const quint8 ReadMem32bit = 0x07;
                const quint8 WriteMem32bit = 0x08;
                const quint8 WriteMem8bit = 0x0d;
                const quint8 EnterMode = 0x20;
                const quint8 EnterSWD = 0xA3;
                const quint8 EnterJTAG = 0x00;
                const quint8 Exit = 0x21;
                const quint8 ReadCoreID = 0x22;
                const quint8 GetStatus = 0x01;
                const quint8 ForceDebug = 0x02;
                const quint8 ResetSys = 0x03;
                const quint8 ReadAllRegs = 0x04;
                const quint8 RunCore = 0x09;
                const quint8 StepCore = 0x0A;
                const quint8 HardReset = 0x3C; // Unsure
                const quint8 ReadCoreRegs = 0x3A; // All regs fetched at once
        }
    }
}

namespace STM32 {
    namespace ChipID {
        // stm32 chipids, only lower 12 bits...
        const quint32 F1_MEDIUM = 0x410;
        const quint32 F2 = 0x411;
        const quint32 F1_LOW = 0x412;
        const quint32 F4 = 0x413;
        const quint32 F1_HIGH = 0x414;
        const quint32 L1_MEDIUM = 0x416;
        const quint32 F1_CONN = 0x418;
        const quint32 F1_VL_MEDIUM = 0x420;
        const quint32 F1_VL_HIGH = 0x428;
        const quint32 F1_XL = 0x430;
    }
    namespace Flash {
        const quint32 RDPTR_KEY = 0x00a5;
        const quint32 KEY1 = 0x45670123;
        const quint32 KEY2 = 0xcdef89ab;
        const quint32 OPTKEY1 = 0x08192A3B;
        const quint32 OPTKEY2 = 0x4C5D6E7F;

        const quint8 SR_BSY = 0;
        const quint8 SR_EOP = 5;

        const quint8 CR_PG = 0;
        const quint8 CR_PER = 1;
        const quint8 CR_MER = 2;
        const quint8 CR_STRT = 6;
        const quint8 CR_LOCK = 7;
        const quint8 CR_PGSIZE = 8;

        //STM32F4
        const quint8 F4_CR_STRT = 16;
        const quint8 F4_CR_LOCK = 31;
        const quint8 F4_CR_SER = 1;
        const quint8 F4_CR_SNB = 3;
        const quint8 F4_CR_SNB_MASK = 0x38;
        const quint8 F4_SR_BSY = 16;

        const quint8 ACR_OFFSET = 0x00;
        const quint8 KEYR_OFFSET = 0x04;
        const quint8 OPT_KEYR_OFFSET = 0x08;
        const quint8 SR_OFFSET = 0x0c;
        const quint8 CR_OFFSET = 0x10;
        const quint8 AR_OFFSET = 0x14;
        const quint8 OBR_OFFSET = 0x1c;
        const quint8 WRPR_OFFSET = 0x20;
    }
}

namespace Cortex {
    namespace CoreID {
        const quint32 M0_R0 = 0x0BB11477;
        const quint32 M3_R1 = 0x1BA00477;
        const quint32 M3_R2 = 0x4BA00477;
        const quint32 M4_R0 = 0x2BA01477;
    }
    namespace Reg {
        const quint32 CM3_CHIPID = 0xE0042000;
        const quint32 CM0_CHIPID = 0x40015800;
        const quint32 CM3_CPUID = 0xE000ED00;
        const quint32 CM3_FP_CTRL = 0xE0002000;
        const quint32 CM3_FP_COMP0 = 0xE0002008;
    }
}

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
    quint32 rev_id;
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
    bool eraseFlash();
    bool unlockFlash();
    bool lockFlash();
    bool unlockFlashOpt();
    bool isBusy();
    bool setFlashProgramming(const bool &val);
    bool setMassErase(const bool &val);
    bool setSTRT();
    void setProgramSize(const quint8 &size);
    quint32 readFlashSize();
    QString getStatus();
    QString getVersion();
    QString getMode();
    QString getCoreID();
    QString getChipID();
    QString getRevID();

private:
    LibUsb *libusb;
    qint32 Command(const quint8 &st_cmd0, const quint8 &st_cmd1, const quint32 &resp_len);
    qint32 DebugCommand(const quint8 &st_cmd1, const quint8 &st_cmd2, const quint32 &resp_len);
    quint32 readFlashSR();
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
    QString regPrint(const quint32 reg) const;
};

#endif // STLINKV2_H

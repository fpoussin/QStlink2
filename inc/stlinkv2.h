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
#include <QFile>
#include <QByteArray>
#include <QtEndian>
#include <QUsb>
#include <compat.h>
#include <devices.h>
#include <loader.h>

const quint16 USB_ST_VID = 0x0483; /**< USB Vid */
const quint16 USB_STLINK_PID = 0x3744; /**< USB Pid for stlink v1 */
const quint16 USB_STLINKv2_PID = 0x3748; /**< USB Pid for stlink v2 */
const quint16 USB_NUCLEO_PID = 0x374b; /**< USB Pid for nucleo */
const quint8 USB_CONFIGURATION = 1;   /**< The sole configuration. */
const quint8 USB_INTERFACE = 0;   /**< The interface. */
const quint8 USB_ALTERNATE = 0;   /**< The alternate interface. */
const quint8 USB_PIPE_IN = 0x81;   /**< Bulk output endpoint for responses */
const quint8 USB_PIPE_OUT = 0x02;	   /**< Bulk input endpoint for commands */
const quint8 USB_PIPE_OUT_NUCLEO = 0x01;	   /**< Bulk input endpoint for commands */
const quint8 USB_PIPE_ERR = 0x83;	   /**< An apparently-unused bulk endpoint. */
const quint16 USB_TIMEOUT_MSEC = 300; /**< The usb bulk transfer timeout in ms */
const QString USB_STLINK_GUID("DBCE1CD9-A320-4b51-A365-A0C3F3C5FB29"); /**< USB Guid for stlink v2 */
const QString USB_NUCLEO_GUID("8326506F-7260-4854-9C03-26E416F04494"); /**< USB Guid for nucleo */

namespace STLink {
    namespace Status {
             const quint8 OK = 0x80; /**< OK return value */
             const quint8 NOK = 0x81; /**< NOK return value */
             const quint8 CORE_RUNNING = 0x80; /**< Core Running value */
             const quint8 CORE_HALTED = 0x81; /**< Core Halted value */
             const quint8 CORE_UNKNOWN_STATE = 2; /**< Not reported, internal use. */
    }
    namespace Mode {
            const quint8 DFU = 0x00;
            const quint8 MASS = 0x01;
            const quint8 DEBUG = 0x02; /**< TODO: describe */
            const quint8 UNKNOWN = -1; /**< TODO: describe */
    }
    namespace Cmd {
            const quint8 GetVersion = 0xF1; /**< TODO: describe */
            const quint8 DebugCommand = 0xF2; /**< TODO: describe */
            const quint8 DFUCommand = 0xF3; /**< TODO: describe */
            const quint8 DFUExit = 0x07; /**< TODO: describe */
            const quint8 DFUGetVersion = 0x08; /**< TODO: describe */
            const quint8 GetCurrentMode = 0xF5; /**< TODO: describe */
            const quint8 Reset = 0xF7; /**< TODO: describe */
        namespace Dbg {
                const quint8 EnterJTAG = 0x00; /**< TODO: describe */
                const quint8 GetStatus = 0x01; /**< TODO: describe */
                const quint8 ForceDebug = 0x02; /**< TODO: describe */
                const quint8 ResetSys = 0x03; /**< TODO: describe */
                const quint8 ReadAllRegs = 0x04; /**< TODO: describe */
                const quint8 ReadReg = 0x05; /**< TODO: describe */
                const quint8 WriteReg = 0x06; /**< TODO: describe */
                const quint8 ReadMem32bit = 0x07; /**< TODO: describe */
                const quint8 WriteMem32bit = 0x08; /**< TODO: describe */
                const quint8 RunCore = 0x09; /**< TODO: describe */
                const quint8 StepCore = 0x0A; /**< TODO: describe */
                const quint8 SetFP = 0x0B; /**< TODO: describe */
                const quint8 WriteMem8bit = 0x0d; /**< TODO: describe */
                const quint8 EnterMode = 0x20; /**< TODO: describe */
                const quint8 Exit = 0x21; /**< TODO: describe */
                const quint8 ReadCoreID = 0x22; /**< TODO: describe */
                const quint8 WriteRegPC = 0x34; /**< TODO: describe */
                const quint8 ReadCoreRegs = 0x3A; /**< All registers fetched at once */
                const quint8 HardReset = 0x3C; /**< Unsure */
                const quint8 EnterSWD = 0xA3; /**< TODO: describe */
        }
    }
}

namespace STM32 {
    namespace ChipID {
        // stm32 chipids, only lower 12 bits...
        const quint32 F1_MEDIUM = 0x410; /**< TODO: describe */
        const quint32 F2 = 0x411; /**< TODO: describe */
        const quint32 F1_LOW = 0x412; /**< TODO: describe */
        const quint32 F4 = 0x413; /**< TODO: describe */
        const quint32 F1_HIGH = 0x414; /**< TODO: describe */
        const quint32 L1_MEDIUM = 0x416; /**< TODO: describe */
        const quint32 F1_CONN = 0x418; /**< TODO: describe */
        const quint32 F4_HD = 0x419; /**< TODO: describe */
        const quint32 F1_VL_MEDIUM = 0x420; /**< TODO: describe */
        const quint32 F1_VL_HIGH = 0x428; /**< TODO: describe */
        const quint32 F1_XL = 0x430; /**< TODO: describe */
    }
    namespace Flash {
        const quint32 RDPTR_KEY = 0x00a5; /**< TODO: describe */
        const quint32 KEY1 = 0x45670123; /**< TODO: describe */
        const quint32 KEY2 = 0xcdef89ab; /**< TODO: describe */
        const quint32 OPTKEY1 = 0x08192A3B; /**< TODO: describe */
        const quint32 OPTKEY2 = 0x4C5D6E7F; /**< TODO: describe */

        const quint8 SR_BSY = 0; /**< TODO: describe */
        const quint8 SR_EOP = 5; /**< TODO: describe */

        const quint8 CR_PG = 0; /**< TODO: describe */
        const quint8 CR_PER = 1; /**< TODO: describe */
        const quint8 CR_MER = 2; /**< TODO: describe */
        const quint8 CR_STRT = 6; /**< TODO: describe */
        const quint8 CR_LOCK = 7; /**< TODO: describe */
        const quint8 CR_PGSIZE = 8; /**< TODO: describe */

        //STM32F4
        const quint8 F4_CR_STRT = 16; /**< TODO: describe */
        const quint8 F4_CR_LOCK = 31; /**< TODO: describe */
        const quint8 F4_CR_SER = 1; /**< TODO: describe */
        const quint8 F4_CR_SNB = 3; /**< TODO: describe */
        const quint8 F4_CR_SNB_MASK = 0x38; /**< TODO: describe */
        const quint8 F4_SR_BSY = 16; /**< TODO: describe */

        const quint8 ACR_OFFSET = 0x00; /**< TODO: describe */
        const quint8 KEYR_OFFSET = 0x04; /**< TODO: describe */
        const quint8 OPT_KEYR_OFFSET = 0x08; /**< TODO: describe */
        const quint8 SR_OFFSET = 0x0c; /**< TODO: describe */
        const quint8 CR_OFFSET = 0x10; /**< TODO: describe */
        const quint8 AR_OFFSET = 0x14; /**< TODO: describe */
        const quint8 OBR_OFFSET = 0x1c; /**< TODO: describe */
        const quint8 WRPR_OFFSET = 0x20; /**< TODO: describe */
    }
}

namespace Cortex {
    namespace CoreID {
        const quint32 M0_R0 = 0x0BB11477; /**< TODO: describe */
        const quint32 M0_R1 = 0x0BC11477; /**< TODO: describe */
        const quint32 M3_R1 = 0x1BA00477; /**< TODO: describe */
        const quint32 M3_R2 = 0x4BA00477; /**< TODO: describe */
        const quint32 M4_R0 = 0x2BA01477; /**< TODO: describe */
    }
    namespace Reg {
        const quint32 CM3_CHIPID = 0xE0042000; /**< TODO: describe */
        const quint32 CM0_CHIPID = 0x40015800; /**< TODO: describe */
        const quint32 CM3_CPUID = 0xE000ED00; /**< TODO: describe */
        const quint32 CM3_FP_CTRL = 0xE0002000; /**< TODO: describe */
        const quint32 CM3_FP_COMP0 = 0xE0002008; /**< TODO: describe */
    }
}

/**
 * @brief
 *
 */
class stlinkv2 : public QThread
{
    Q_OBJECT

    /**
     * @brief
     *
     */
    struct STVersion {
        quint32 stlink; /**< TODO: describe */
        quint32 jtag; /**< TODO: describe */
        quint32 swim; /**< TODO: describe */
    };

public:
    /**
     * @brief
     *
     * @param parent
     */
    explicit stlinkv2(QObject *parent = 0);
    /**
     * @brief
     *
     */
    ~stlinkv2();
    /**
     * @brief
     *
     * @return bool
     */
    bool isLocked();
    /**
     * @brief
     *
     * @return bool
     */
    bool isBusy();
    /**
     * @brief
     *
     * @return bool
     */
    bool isConnected();

    STVersion mVersion; /**< TODO: describe */
    DeviceInfo *mDevice; /**< TODO: describe */
    quint32 mChipId; /**< TODO: describe */

signals:
    /**
     * @brief
     *
     * @param p
     */
    void bufferPct(quint32 p);
    /**
     * @brief
     *
     * @param name
     */
    void deviceDetected(QString name);

public slots:
    /**
     * @brief
     *
     * @return qint32
     */
    qint32 connect();
    /**
     * @brief
     *
     */
    void disconnect();
    /**
     * @brief
     *
     */
    void setSTLinkIDs(void);
    /**
     * @brief
     *
     */
    void setNucleoIDs(void);

    /**
     * @brief
     *
     */
    void flush();
    /**
     * @brief
     *
     */
    void resetMCU();
    /**
     * @brief
     *
     */
    void hardResetMCU();
    /**
     * @brief
     *
     */
    void haltMCU();
    /**
     * @brief
     *
     */
    void runMCU();
    /**
     * @brief
     *
     */
    void stepMCU();
    /**
     * @brief
     *
     */
    void setModeJTAG();
    /**
     * @brief
     *
     */
    void setModeSWD();
    /**
     * @brief
     *
     */
    void setExitModeDFU();
    /**
     * @brief
     *
     * @param buf
     * @param addr
     * @param len
     * @return qint32
     */
    qint32 readMem32(QByteArray *buf, quint32 addr, quint16 len = 4);
    /**
     * @brief
     *
     * @param addr
     * @param buf
     * @return qint32
     */
    qint32 writeMem32(quint32 addr, const QByteArray &buf);
    /**
     * @brief
     *
     * @param val
     * @param index
     * @return bool
     */
    bool writeRegister(quint32 val, quint8 index);
    /**
     * @brief
     *
     * @param index
     * @return quint32
     */
    quint32 readRegister(quint8 index);
    /**
     * @brief
     *
     * @param val
     */
    void writePC(quint32 val);

    /**
     * @brief
     *
     * @return bool
     */
    bool eraseFlash();
    /**
     * @brief
     *
     * @return bool
     */
    bool unlockFlash();
    /**
     * @brief
     *
     * @return bool
     */
    bool lockFlash();
    /**
     * @brief
     *
     * @return bool
     */
    bool unlockFlashOpt();

    /**
     * @brief
     *
     * @param val
     * @return bool
     */
    bool setFlashProgramming(bool val);
    /**
     * @brief
     *
     * @param val
     * @return bool
     */
    bool setMassErase(bool val);
    /**
     * @brief
     *
     * @return bool
     */
    bool setSTRT();
    /**
     * @brief
     *
     * @param size
     */
    void setProgramSize(quint8 size);
    /**
     * @brief
     *
     * @return quint32
     */
    quint32 readFlashSize();
    /**
     * @brief
     *
     * @return quint8
     */
    quint8 getStatus();
    /**
     * @brief
     *
     * @return stlinkv2::STVersion
     */
    stlinkv2::STVersion getVersion();
    /**
     * @brief
     *
     * @return quint8
     */
    quint8 getMode();
    /**
     * @brief
     *
     * @return quint32
     */
    quint32 getCoreID();
    /**
     * @brief
     *
     * @return quint32
     */
    quint32 getChipID();
    /**
     * @brief
     *
     * @return quint32
     */
    quint32 getRevID();
    /**
     * @brief
     *
     * @return bool
     */
    bool sendLoader();
    /**
     * @brief
     *
     * @param addr
     * @param buf
     * @return bool
     */
    bool setLoaderBuffer(const quint32 addr, const QByteArray& buf);
    /**
     * @brief
     *
     * @return quint32
     */
    quint32 getLoaderStatus();
    /**
     * @brief
     *
     * @return quint32
     */
    quint32 getLoaderPos();
    /**
     * @brief
     *
     */
    void getLoaderParams();

private slots:
    /**
     * @brief
     *
     * @param list
     */
    void scanNewDevices(QtUsb::FilterList list);

private:
    QUsbManager *mUsbMgr; /**< TODO: describe */
    QUsbDevice *mUsbDevice; /**< TODO: describe */
    QtUsb::FilterList mUsbDeviceList; /**< TODO: describe */

    quint32 mCoreId; /**< TODO: describe */
    quint32 mRevId; /**< TODO: describe */
    qint8 mModeId; /**< TODO: describe */
    bool mConnected; /**< TODO: describe */
    LoaderData mLoader; /**< TODO: describe */

    /**
     * @brief
     *
     * @param buf
     * @param st_cmd0
     * @param st_cmd1
     * @param resp_len
     * @return qint32
     */
    qint32 command(QByteArray* buf, quint8 st_cmd0, quint8 st_cmd1, quint32 resp_len);
    /**
     * @brief
     *
     * @param buf
     * @param st_cmd1
     * @param st_cmd2
     * @param resp_len
     * @return qint32
     */
    qint32 debugCommand(QByteArray* buf, quint8 st_cmd1, quint8 st_cmd2, quint32 resp_len);
    /**
     * @brief
     *
     * @return quint32
     */
    quint32 readFlashSR();
    /**
     * @brief
     *
     * @return quint32
     */
    quint32 readFlashCR();
    /**
     * @brief
     *
     * @param mask
     * @param value
     * @return quint32
     */
    quint32 writeFlashCR(quint32 mask, bool value);
    /**
     * @brief
     *
     * @param cmd
     * @return qint32
     */
    qint32 sendCommand(const QByteArray &cmd);
    /**
     * @brief
     *
     * @param reg
     * @return QString
     */
    QString regPrint(quint32 reg) const;
};

#endif // STLINKV2_H

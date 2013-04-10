#ifndef QUsb_H
#define QUsb_H

#include <QObject>
#include <QDebug>
#include <compat.h>
// Include Windows headers
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>

// Include WinUSB headers
#include <winusb.h>
#include <Usb100.h>
#include <Setupapi.h>

// Linked libraries
#pragma comment (lib , "setupapi.lib" )
#pragma comment (lib , "winusb.lib" )

/*
VID_0483&PID_3748.DeviceDesc="STMicroelectronics STLink dongle"
ClassGUID={36FC9E60-C465-11CF-8056-444553540000}
STLink_GUID="{DBCE1CD9-A320-4b51-A365-A0C3F3C5FB29}"
*/

static const GUID OSR_DEVICE_INTERFACE =
{ 0xDBCE1CD9, 0xA320, 0x4b51, { 0xA3, 0x65, 0xA0, 0xC3, 0xF3, 0xC5, 0xFB, 0x29 } };

class QUsb : public QObject
{
    Q_OBJECT

    struct PIPE_ID
    {
        uchar  PipeInId;
        uchar  PipeOutId;
    };
public:
    explicit QUsb(QObject *parent = 0);
    
public slots:
    qint32 open();
    void close();
    qint32 read(QByteArray *buf, quint32 bytes);
    qint32 write(QByteArray *buf, quint32 bytes);

private:
    bool GetDeviceHandle(GUID guidDeviceInterface, PHANDLE hDeviceHandle);
    bool GetWinUSBHandle(HANDLE hDeviceHandle, PWINUSB_INTERFACE_HANDLE phWinUSBHandle);
    bool GetUSBDeviceSpeed(WINUSB_INTERFACE_HANDLE hWinUSBHandle, quint8 *pDeviceSpeed);
    bool QueryDeviceEndpoints(WINUSB_INTERFACE_HANDLE hWinUSBHandle, PIPE_ID* pipeid);

    GUID guidDeviceInterface;
    HANDLE hDeviceHandle;
    WINUSB_INTERFACE_HANDLE hWinUSBHandle;
    uchar DeviceSpeed;
    PIPE_ID PipeID;
};

#endif // QUsb_H

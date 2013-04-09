#ifndef WINUSB_H
#define WINUSB_H

#include <QObject>
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

const quint16 USB_ST_VID = 0x0483;
const quint16 USB_STLINK_PID = 0x3744;
const quint16 USB_STLINKv2_PID = 0x3748;
const quint8 USB_CONFIGURATION = 1;   /* The sole configuration. */
const quint8 USB_PIPE_IN = 0x81;   /* Bulk output endpoint for responses */
const quint8 USB_PIPE_OUT = 0x02;	   /* Bulk input endpoint for commands */
const quint8 USB_PIPE_ERR = 0x83;	   /* An apparently-unused bulk endpoint. */
const quint16 USB_TIMEOUT_MSEC = 300;

/*
VID_0483&PID_3748.DeviceDesc="STMicroelectronics STLink dongle"
ClassGUID={36FC9E60-C465-11CF-8056-444553540000}
STLink_GUID="{DBCE1CD9-A320-4b51-A365-A0C3F3C5FB29}"
*/

static const GUID OSR_DEVICE_INTERFACE =
{ 0x36FC9E60, 0xC465, 0x11CF, { 0x80, 0x56, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

class WinUsb : public QObject
{
    Q_OBJECT

    struct PIPE_ID
    {
        uchar  PipeInId;
        uchar  PipeOutId;
    };
public:
    explicit WinUsb(QObject *parent = 0);
    
public slots:
    qint32 open();
    void close();
    qint32 read(QByteArray *buf, quint32 bytes);
    qint32 write(QByteArray *buf, quint32 bytes);

private:
    bool GetDeviceHandle(GUID guidDeviceInterface, PHANDLE hDeviceHandle);
    bool GetWinUSBHandle(HANDLE hDeviceHandle, PWINUSB_INTERFACE_HANDLE phWinUSBHandle);
    bool GetUSBDeviceSpeed(WINUSB_INTERFACE_HANDLE hDeviceHandle, quint8 *pDeviceSpeed);
    bool QueryDeviceEndpoints(WINUSB_INTERFACE_HANDLE hDeviceHandle, PIPE_ID* pipeid);

    GUID guidDeviceInterface;
    HANDLE hDeviceHandle;
    WINUSB_INTERFACE_HANDLE hWinUSBHandle;
    uchar DeviceSpeed;
    PIPE_ID PipeID;
    /*
    struct usb_dev_handle* OpenAntStick();
    struct usb_interface_descriptor* usb_find_interface(struct usb_config_descriptor* config_descriptor);
    struct usb_dev_handle* device;
    struct usb_interface_descriptor* intf;

    qint32 readEndpoint, writeEndpoint;
    qint32 interface;
    qint32 alternate;
    */
};

#endif // WINUSB_H

#include "qwinusb.h"

QUsb::QUsb(QObject *parent) :
    QObject(parent)
{
    m_guidDeviceInterface = OSR_DEVICE_INTERFACE; //in the INF file

    m_devHandle = INVALID_HANDLE_VALUE;
    m_usbHandle = INVALID_HANDLE_VALUE;

//    cbSize = 0;
}

QUsb::~QUsb()
{
    this->close();
}

qint32 QUsb::open()
{
    if (!GetDeviceHandle(m_guidDeviceInterface, &m_devHandle)) return -1;
    else if (!GetWinUSBHandle(m_devHandle, &m_usbHandle)) return -1;
    else if (!GetUSBDeviceSpeed(m_usbHandle, &m_devSpeed)) return -1;
//    else if (!QueryDeviceEndpoints(m_usbHandle, &PipeID)) return -1;

    m_pipeId.PipeInId = 0x81;
    m_pipeId.PipeOutId = 0x02;

    if (!WinUsb_ResetPipe(m_usbHandle, m_pipeId.PipeInId)) {
        qCritical("Error WinUsb_ResetPipe: %d.\n", GetLastError()); return -1; }
    if (!WinUsb_ResetPipe(m_usbHandle, m_pipeId.PipeOutId)) {
        qCritical("Error WinUsb_ResetPipe: %d.\n", GetLastError()); return -1; }

    ulong timeout = 300; // ms
    if (!WinUsb_SetPipePolicy(m_usbHandle, m_pipeId.PipeInId, PIPE_TRANSFER_TIMEOUT, sizeof(ulong), &timeout)) {
        qCritical("Error WinUsb_SetPipePolicy: %d.\n", GetLastError()); return -1; }
    if (!WinUsb_SetPipePolicy(m_usbHandle, m_pipeId.PipeOutId, PIPE_TRANSFER_TIMEOUT, sizeof(ulong), &timeout)) {
        qCritical("Error WinUsb_SetPipePolicy: %d.\n", GetLastError()); return -1; }

// This breaks the transfer thread for some reasons...
/*    ulong stall = 1; // ms
    if (!WinUsb_SetPipePolicy(m_usbHandle, m_pipeId.PipeInId, AUTO_CLEAR_STALL, sizeof(ulong), &stall)) {
        qCritical("Error WinUsb_SetPipePolicy: %d.\n", GetLastError()); return -1; }
    if (!WinUsb_SetPipePolicy(m_usbHandle, m_pipeId.PipeOutId, AUTO_CLEAR_STALL, sizeof(ulong), &stall)) {
        qCritical("Error WinUsb_SetPipePolicy: %d.\n", GetLastError()); return -1; }
*/
    return 1;
}

void QUsb::close()
{
    CloseHandle(m_devHandle);
    WinUsb_Free(m_usbHandle);
}

qint32 QUsb::read(QByteArray *buf, quint32 bytes)
{
    PrintFuncName();
    if (m_usbHandle==INVALID_HANDLE_VALUE || !m_pipeId.PipeInId)
    {
        return -1;
    }
    bool bResult = true;
    ulong cbRead = 0;
    uchar *buffer = new uchar[bytes];
    bResult = WinUsb_ReadPipe(m_usbHandle, m_pipeId.PipeInId, buffer, bytes, &cbRead, 0);
    // we clear the buffer.
    buf->clear();
//    QString data, s;

    if (cbRead > 0) {
        for (quint32 i = 0; i < bytes; i++) {
            buf->append(buffer[i]);
//            data.append(s.sprintf("%02X",(uchar)buf->at(i))+":");
        }
//        data.remove(data.size()-1, 1); //remove last colon
//        qDebug() << "Received: " << data;
    }
    delete buffer;
    if (!bResult) {
        PrintError("WinUsb_ReadPipe");
        return -1;
    }
    return cbRead;
}

qint32 QUsb::write(QByteArray *buf, quint32 bytes)
{
    PrintFuncName();
    if (m_usbHandle==INVALID_HANDLE_VALUE || !m_pipeId.PipeOutId)
    {
        return -1;
    }

//    QString cmd, s;
//        for (int i=0; i<buf->size(); i++) {
//            cmd.append(s.sprintf("%02X",(uchar)buf->at(i))+":");
//        }
//        cmd.remove(cmd.size()-1, 1); //remove last colon
//        qDebug() << "Sending" << buf->size() << "bytes:" << cmd;

    ulong cbSent = 0;
    bool bResult = WinUsb_WritePipe(m_usbHandle, m_pipeId.PipeOutId, (uchar*)buf->data(), bytes, &cbSent, 0);
    if (!bResult) {
        PrintError("WinUsb_WritePipe");
        return -1;
    }
    return cbSent;
}

bool QUsb::GetDeviceHandle(GUID guidDeviceInterface, PHANDLE hDeviceHandle)
{
    PrintFuncName();
    if (guidDeviceInterface==GUID_NULL)
    {
        return false;
    }

    bool bResult = true;
    HDEVINFO hDeviceInfo;
    SP_DEVINFO_DATA DeviceInfoData;

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = NULL;

    ulong requiredLength = 0;

    LPTSTR lpDevicePath = NULL;

    qint32 index = 0;

    // Get information about all the installed devices for the specified
    // device interface class.
    hDeviceInfo = SetupDiGetClassDevs(
        &guidDeviceInterface,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (hDeviceInfo == INVALID_HANDLE_VALUE)
    {
        // ERROR
        PrintError("SetupDiGetClassDevs");
        goto done;
    }

    //Enumerate all the device interfaces in the device information set.
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (index = 0; SetupDiEnumDeviceInfo(hDeviceInfo, index, &DeviceInfoData); index++)
    {
        //Reset for this iteration
        if (lpDevicePath)
        {
            LocalFree(lpDevicePath);
        }
        if (pInterfaceDetailData)
        {
            LocalFree(pInterfaceDetailData);
        }

        deviceInterfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

        //Get information about the device interface.
        bResult = SetupDiEnumDeviceInterfaces(
           hDeviceInfo,
           &DeviceInfoData,
           &guidDeviceInterface,
           0,
           &deviceInterfaceData);

        // Check if last item
        if (GetLastError () == ERROR_NO_MORE_ITEMS)
        {
            break;
        }

        //Check for some other error
        if (!bResult)
        {
            PrintError("SetupDiEnumDeviceInterfaces");
            goto done;
        }

        //Interface data is returned in SP_DEVICE_INTERFACE_DETAIL_DATA
        //which we need to allocate, so we have to call this function twice.
        //First to get the size so that we know how much to allocate
        //Second, the actual call with the allocated buffer

        bResult = SetupDiGetDeviceInterfaceDetail(
            hDeviceInfo,
            &deviceInterfaceData,
            NULL, 0,
            &requiredLength,
            NULL);


        //Check for some other error
        if (!bResult)
        {
            if ((ERROR_INSUFFICIENT_BUFFER==GetLastError()) && (requiredLength>0))
            {
                //we got the size, allocate buffer
                pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, requiredLength);

                if (!pInterfaceDetailData)
                {
                    // ERROR
                    qCritical("Error allocating memory for the device detail buffer.\n");
                    goto done;
                }
            }
            else
            {
                PrintError("SetupDiEnumDeviceInterfaces");
                goto done;
            }
        }

        //get the interface detailed data
        pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        //Now call it with the correct size and allocated buffer
        bResult = SetupDiGetDeviceInterfaceDetail(
                hDeviceInfo,
                &deviceInterfaceData,
                pInterfaceDetailData,
                requiredLength,
                NULL,
                &DeviceInfoData);

        //Check for some other error
        if (!bResult)
        {
            PrintError("SetupDiGetDeviceInterfaceDetail");
            goto done;
        }

        //copy device path

        size_t nLength = wcslen (pInterfaceDetailData->DevicePath) + 1;
        lpDevicePath = (TCHAR *) LocalAlloc (LPTR, nLength * sizeof(TCHAR));
        StringCchCopy(lpDevicePath, nLength, pInterfaceDetailData->DevicePath);
        lpDevicePath[nLength-1] = 0;

        qDebug("Device path:  %s\n", lpDevicePath);

    }

    if (!lpDevicePath)
    {
        //Error.
        qCritical("Error %d.", GetLastError());
        goto done;
    }

    //Open the device
    *hDeviceHandle = CreateFile (
        lpDevicePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    if (*hDeviceHandle == INVALID_HANDLE_VALUE)
    {
        //Error.
        PrintError("");
        goto done;
    }

    done:
        LocalFree(lpDevicePath);
        LocalFree(pInterfaceDetailData);
        bResult = SetupDiDestroyDeviceInfoList(hDeviceInfo);

    return bResult;
}

bool QUsb::GetWinUSBHandle(HANDLE hDeviceHandle, PWINUSB_INTERFACE_HANDLE phWinUSBHandle)
{
    PrintFuncName();
    if (hDeviceHandle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    if(!WinUsb_Initialize(hDeviceHandle, phWinUSBHandle))
    {
        //Error.
        PrintError("WinUsb_Initialize");
        return false;
    }

    return true;
}

bool QUsb::GetUSBDeviceSpeed(WINUSB_INTERFACE_HANDLE hWinUSBHandle, quint8 *pDeviceSpeed)
{
    PrintFuncName();
    if (!pDeviceSpeed || hWinUSBHandle==INVALID_HANDLE_VALUE)
    {
        return false;
    }

    ulong length = sizeof(quint8);

    if(!WinUsb_QueryDeviceInformation(hWinUSBHandle, DEVICE_SPEED, &length, pDeviceSpeed))
    {
        PrintError("Error getting device speed");
        return false;
    }

    if(*pDeviceSpeed == LowSpeed)
    {
        qDebug("Device speed: %d (Low speed).\n", *pDeviceSpeed);
        return true;
    }
    else if(*pDeviceSpeed == FullSpeed)
    {
        qDebug("Device speed: %d (Full speed).\n", *pDeviceSpeed);
        return true;
    }
    else if(*pDeviceSpeed == HighSpeed)
    {
        qDebug("Device speed: %d (High speed).\n", *pDeviceSpeed);
        return true;
    }
    return false;
}

bool QUsb::QueryDeviceEndpoints(WINUSB_INTERFACE_HANDLE hWinUSBHandle, QUsb::PIPE_ID *pipeid)
{
    PrintFuncName();
    if (hWinUSBHandle==INVALID_HANDLE_VALUE)
    {
        return false;
    }

    bool bResult = true;

    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    ZeroMemory(&InterfaceDescriptor, sizeof(USB_INTERFACE_DESCRIPTOR));

    WINUSB_PIPE_INFORMATION  Pipe;
    ZeroMemory(&Pipe, sizeof(WINUSB_PIPE_INFORMATION));

    bResult = WinUsb_QueryInterfaceSettings(hWinUSBHandle, 0, &InterfaceDescriptor);

    if (bResult)
    {
        for (int index = 0; index < InterfaceDescriptor.bNumEndpoints; index++)
        {
            bResult = WinUsb_QueryPipe(hWinUSBHandle, 0, index, &Pipe);

            if (bResult)
            {
                if (Pipe.PipeType == UsbdPipeTypeControl)
                {
                    qDebug("Endpoint index: %d Pipe type: Control Pipe ID: 0x%02x.\n", index, Pipe.PipeId);
                }
                if (Pipe.PipeType == UsbdPipeTypeIsochronous)
                {
                    qDebug("Endpoint index: %d Pipe type: Isochronous Pipe ID: 0x%02x.\n", index, Pipe.PipeId);
                }
                if (Pipe.PipeType == UsbdPipeTypeBulk)
                {
                    if (USB_ENDPOINT_DIRECTION_IN(Pipe.PipeId))
                    {
                        qDebug("Bulk IN Endpoint index: %d Pipe type: Bulk Pipe ID: 0x%02x.\n", index, Pipe.PipeId);
                        pipeid->PipeInId = Pipe.PipeId;
                    }
                    if (USB_ENDPOINT_DIRECTION_OUT(Pipe.PipeId))
                    {
                        qDebug("Bulk OUT Endpoint index: %d Pipe type: Bulk Pipe ID: 0x%02x.\n", index, Pipe.PipeId);
                        pipeid->PipeOutId = Pipe.PipeId;
                    }

                }
                if (Pipe.PipeType == UsbdPipeTypeInterrupt)
                {
                    qDebug("Endpoint index: %d Pipe type: Interrupt Pipe ID: 0x%02x.\n", index, Pipe.PipeId);
                }
            }
            else
            {
                continue;
            }
        }
    }
    else
    {
        return false;
    }
    return true;
}

void QUsb::PrintError(const QString &func)
{
    const quint32 err = GetLastError();
    switch (err) {

    case 121:
        qCritical() << func << "ERROR_SEM_TIMEOUT" << err;
        break;
    case 997:
        qCritical() << func << "ERROR_IO_PENDING" << err;
        break;
    default:
        qCritical() << func << "Error id:" << err;
        break;
    }
}

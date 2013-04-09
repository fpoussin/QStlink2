#include "winusb.h"

WinUsb::WinUsb(QObject *parent) :
    QObject(parent)
{
    guidDeviceInterface = OSR_DEVICE_INTERFACE; //in the INF file

    hDeviceHandle = INVALID_HANDLE_VALUE;
    hWinUSBHandle = INVALID_HANDLE_VALUE;

    cbSize = 0;
}

qint32 WinUsb::open()
{
    GetDeviceHandle(guidDeviceInterface, &hDeviceHandle);
    GetWinUSBHandle(hDeviceHandle, &hWinUSBHandle);
    GetUSBDeviceSpeed(hWinUSBHandle, &DeviceSpeed);
    QueryDeviceEndpoints(hWinUSBHandle, &PipeID);
}

void WinUsb::close()
{
    CloseHandle(hDeviceHandle);
    WinUsb_Free(hWinUSBHandle);
}

qint32 WinUsb::read(QByteArray *buf, quint32 bytes)
{
    quint32 cbRead = 0;
    bool bResult = WinUsb_ReadPipe(hDeviceHandle, &PipeID.PipeInId, (uchar*)buf->data(), bytes, &cbRead, 0);
    if (!bResult) return -1;
    return cbRead;
}

qint32 WinUsb::write(QByteArray *buf, quint32 bytes)
{
    quint32 cbSent = 0;
    bool bResult = WinUsb_WritePipe(hDeviceHandle, &PipeID.PipeOutId, (uchar*)buf->data(), bytes, &cbSent, 0);
    if (!bResult) return -1;
    return cbSent;
}

bool WinUsb::GetDeviceHandle(GUID guidDeviceInterface, PHANDLE hDeviceHandle)
{
    if (guidDeviceInterface==GUID_NULL)
    {
        return false;
    }

    bool bResult = true;
    HDEVINFO hDeviceInfo;
    SP_DEVINFO_DATA DeviceInfoData;

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = NULL;

    quint32 requiredLength=0;

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
        qCritical("Error SetupDiGetClassDevs: %d.\n", GetLastError());
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
            qCritical("Error SetupDiEnumDeviceInterfaces: %d.\n", GetLastError());
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
                qCritical("Error SetupDiEnumDeviceInterfaces: %d.\n", GetLastError());
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
            qCritical("Error SetupDiGetDeviceInterfaceDetail: %d.\n", GetLastError());
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
        printf("Error %d.", GetLastError());
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
        printf("Error %d.", GetLastError());
        goto done;
    }

    done:
        LocalFree(lpDevicePath);
        LocalFree(pInterfaceDetailData);
        bResult = SetupDiDestroyDeviceInfoList(hDeviceInfo);

    return bResult;
}

bool WinUsb::GetWinUSBHandle(HANDLE hDeviceHandle, PWINUSB_INTERFACE_HANDLE phWinUSBHandle)
{
    if (hDeviceHandle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    if(!WinUsb_Initialize(hDeviceHandle, phWinUSBHandle))
    {
        //Error.
        qCritical("WinUsb_Initialize Error %d.", GetLastError());
        return false;
    }

    return true;
}

bool WinUsb::GetUSBDeviceSpeed(WINUSB_INTERFACE_HANDLE hDeviceHandle, quint8 *pDeviceSpeed)
{
    if (!pDeviceSpeed || hDeviceHandle==INVALID_HANDLE_VALUE)
    {
        return false;
    }

    quint32 length = sizeof(quint8);

    if(!WinUsb_QueryDeviceInformation(hDeviceHandle, DEVICE_SPEED, &length, pDeviceSpeed))
    {
        qCritical("Error getting device speed: %d.\n", GetLastError());
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

bool WinUsb::QueryDeviceEndpoints(WINUSB_INTERFACE_HANDLE hDeviceHandle, WinUsb::PIPE_ID *pipeid)
{
    if (hDeviceHandle==INVALID_HANDLE_VALUE)
    {
        return false;
    }

    bool bResult = true;

    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    ZeroMemory(&InterfaceDescriptor, sizeof(USB_INTERFACE_DESCRIPTOR));

    WINUSB_PIPE_INFORMATION  Pipe;
    ZeroMemory(&Pipe, sizeof(WINUSB_PIPE_INFORMATION));

    bResult = WinUsb_QueryInterfaceSettings(hDeviceHandle, 0, &InterfaceDescriptor);

    if (bResult)
    {
        for (int index = 0; index < InterfaceDescriptor.bNumEndpoints; index++)
        {
            bResult = WinUsb_QueryPipe(hDeviceHandle, 0, index, &Pipe);

            if (bResult)
            {
                if (Pipe.PipeType == UsbdPipeTypeControl)
                {
                    qDebug("Endpoint index: %d Pipe type: Control Pipe ID: %d.\n", index, Pipe.PipeType, Pipe.PipeId);
                }
                if (Pipe.PipeType == UsbdPipeTypeIsochronous)
                {
                    qDebug("Endpoint index: %d Pipe type: Isochronous Pipe ID: %d.\n", index, Pipe.PipeType, Pipe.PipeId);
                }
                if (Pipe.PipeType == UsbdPipeTypeBulk)
                {
                    if (USB_ENDPOINT_DIRECTION_IN(Pipe.PipeId))
                    {
                        qDebug("Endpoint index: %d Pipe type: Bulk Pipe ID: %c.\n", index, Pipe.PipeType, Pipe.PipeId);
                        pipeid->PipeInId = Pipe.PipeId;
                    }
                    if (USB_ENDPOINT_DIRECTION_OUT(Pipe.PipeId))
                    {
                        qDebug("Endpoint index: %d Pipe type: Bulk Pipe ID: %c.\n", index, Pipe.PipeType, Pipe.PipeId);
                        pipeid->PipeOutId = Pipe.PipeId;
                    }

                }
                if (Pipe.PipeType == UsbdPipeTypeInterrupt)
                {
                    qDebug("Endpoint index: %d Pipe type: Interrupt Pipe ID: %d.\n", index, Pipe.PipeType, Pipe.PipeId);
                }
            }
            else
            {
                continue;
            }
        }
    }

done:
    return bResult;
}




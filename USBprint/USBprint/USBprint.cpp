// USBprint.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include <devioctl.h>
#include <usb.h>
#include <usbiodef.h>
#include <usbioctl.h>
#include <usbprint.h>
#include <setupapi.h>
#include <devguid.h>
#include <wdmguid.h>

#include <Ntddpar.h>


/* Code to find the device path for a usbprint.sys controlled
* usb printer and print to it
	http://blog.peter.skarpetis.com/archives/2005/04/07/getting-a-handle-on-usbprintsys/comment-page-1/ 
*/



/* This define is required so that the GUID_DEVINTERFACE_USBPRINT variable is
* declared an initialised as a static locally, since windows does not include it
* in any of its libraries
*/

#define SS_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
static const GUID name \
= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

SS_DEFINE_GUID(GUID_DEVINTERFACE_USBPRINT, 0x28d78fad, 0x5a12, 0x11D1, 0xae,
	0x5b, 0x00, 0x00, 0xf8, 0x03, 0xa8, 0xc2);



char HexToChar(char hex)
{
	if ((hex >= 0) && (hex < 10))
		return hex + '0';
	else if ((hex >= 10) && (hex < 16))
		return hex + 'A';
	else
		return '?';
}

void PrintParallelDeviceID(HANDLE deviceHandle)
{
	printf("Getting the parallel device ID (IOCTL_PAR_QUERY_DEVICE_ID) ...\n");
	const size_t bufferSize = 64 * 1024; // 64 KB per parallel/1284 max device name
	BYTE buffer[bufferSize] = {};
	DWORD cbReturned = 0;
	BOOL bResult = DeviceIoControl(deviceHandle, IOCTL_PAR_QUERY_DEVICE_ID, nullptr, 0, buffer, bufferSize, &cbReturned, nullptr);
	if (!bResult)
	{
		printf("Error getting device ID (IOCTL_PAR_QUERY_DEVICE_ID): %d\n", GetLastError());
		return;
	}

	printf("- returned device ID = '%s' (%d characters)\n", buffer, cbReturned);
}


void Print1284DeviceID(HANDLE deviceHandle)
{
	printf("Getting the parallel device ID (IOCTL_USBPRINT_GET_1284_ID) ...\n");
	const size_t bufferSize = 64*1024; // 64 KB per 1284 max device name
	BYTE buffer[bufferSize] = {};
	DWORD cbReturned = 0;
	BOOL bResult = DeviceIoControl(deviceHandle, IOCTL_USBPRINT_GET_1284_ID, nullptr, 0, buffer, bufferSize, &cbReturned, nullptr);
	if (!bResult)
	{
		printf("Error getting device ID (IOCTL_USBPRINT_GET_1284_ID): %d\n", GetLastError());
		return;
	}

	DWORD deviceSize = 0;
	if (cbReturned >= 2)
	{
		deviceSize = (buffer[0] << 8) + buffer[1];
	}

	printf("- returned device size = %d, device ID = '%s' (%d bytes)\n", deviceSize, buffer + 2, cbReturned);
}


void ProcessCommand(HANDLE usbHandle)
{
	while (true)
	{
		printf("\nEnter command [r/w/q] > ");
		char c = (char)getchar();
		switch (c)
		{
		case 'r':
			printf("\nread\n");
			{
				const size_t dataSize = 64;
				char data[dataSize] = {};
				DWORD dwBytesRead = 0;
				ReadFile(usbHandle, data, dataSize, &dwBytesRead, nullptr);
				if (dwBytesRead > 0)
				{
					printf("- Data read: \n");
					for (int i = 0; i < (int)dwBytesRead; i++)
					{
						printf("%c%c ", HexToChar(data[i] >> 4), HexToChar(data[i] % 0xF));

						if (i > 0 && (i % 8) == 0)
							printf("\n");
					}
					printf("\n");
				}
				else
					printf("- Data read attempt: no bytes (GLE = %d)\n", GetLastError());
			}
			break;

		case 'w':
			printf("\nwrite\n");
			{
				const size_t dataSize = 64;
				char data[dataSize] = {};
				for (int i = 0; i < dataSize; i++)
				{
					data[i] = i;
				}

				DWORD dwBytesWritten = 0;
				WriteFile(usbHandle, data, dataSize, &dwBytesWritten, nullptr);
				if (dwBytesWritten > 0)
					printf("- Data written = %c%c\n", HexToChar(data[0] >> 4), HexToChar(data[0] % 0xF));
				else
					printf("- Data written attempt: no bytes (GLE = %d)\n", GetLastError());
			}
			break;

		case 'q':
			printf("\nquit\n");
			break;
		}
	}
}



void EnumerateParallelUSB()
{
	HDEVINFO devs;
	DWORD devcount;
	SP_DEVINFO_DATA devinfo;
	SP_DEVICE_INTERFACE_DATA devinterface;
	DWORD size;
	GUID intfce;
	PSP_DEVICE_INTERFACE_DETAIL_DATA interface_detail;
	HANDLE usbHandle;
	DWORD dataType;

	intfce = GUID_DEVINTERFACE_USBPRINT;
	devs = SetupDiGetClassDevs(&intfce, 0, 0, DIGCF_PRESENT |
		DIGCF_DEVICEINTERFACE);
	if (devs == INVALID_HANDLE_VALUE) {
		printf("No devs found \n");
		return;
	}
	devcount = 0;
	devinterface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	while (SetupDiEnumDeviceInterfaces(devs, 0, &intfce, devcount, &devinterface)) {
		/* The following buffers would normally be malloced to he correct size
		* but here we just declare them as large stack variables
		* to make the code more readable
		*/
		char driverkey[2048];
		char interfacename[2048];
		char location[2048];
		// char description[2048];

		/* If this is not the device we want, we would normally continue onto the
		* next one or so something like
		* if (!required_device) continue; would be added here
		*/
		devcount++;
		size = 0;
		/* See how large a buffer we require for the device interface details */
		SetupDiGetDeviceInterfaceDetail(devs, &devinterface, 0, 0, &size, 0);
		devinfo.cbSize = sizeof(SP_DEVINFO_DATA);
		interface_detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(calloc(1, size));
		if (interface_detail) {
			interface_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			devinfo.cbSize = sizeof(SP_DEVINFO_DATA);
			if (!SetupDiGetDeviceInterfaceDetail(devs, &devinterface, interface_detail,
				size, 0, &devinfo)) {
				free(interface_detail);
				SetupDiDestroyDeviceInfoList(devs);
				return;
			}
			/* Make a copy of the device path for later use */
			strcpy_s(interfacename, _ARRAYSIZE(interfacename), interface_detail->DevicePath);
			free(interface_detail);
			/* And now fetch some useful registry entries */
			size = sizeof(driverkey);
			driverkey[0] = 0;
			if (!SetupDiGetDeviceRegistryProperty(devs, &devinfo, SPDRP_DRIVER, &dataType,
				(LPBYTE)driverkey, size, 0)) {
				SetupDiDestroyDeviceInfoList(devs);
				return;
			}
			size = sizeof(location);
			location[0] = 0;
			if (!SetupDiGetDeviceRegistryProperty(devs, &devinfo,
				SPDRP_LOCATION_INFORMATION, &dataType,
				(LPBYTE)location, size, 0)) {
				SetupDiDestroyDeviceInfoList(devs);
				return;
			}
			printf("Opening %s ... \n", interfacename);
			usbHandle = CreateFile(interfacename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
				NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL |
				FILE_FLAG_SEQUENTIAL_SCAN, NULL);
			printf("- handle = %p (GLE = %d)\n", usbHandle, GetLastError());
			if (usbHandle != INVALID_HANDLE_VALUE) 
			{
				/* 
				// Get the device ID
				PrintParallelDeviceID(usbHandle);
				Print1284DeviceID(usbHandle);
				*/

				ProcessCommand(usbHandle);

				CloseHandle(usbHandle);
			}
			printf("Closing %s ... \n", interfacename);
		}
	}
	SetupDiDestroyDeviceInfoList(devs);
}



int _tmain(int argc, _TCHAR* argv[])
{
	EnumerateParallelUSB();
	return 0;
}
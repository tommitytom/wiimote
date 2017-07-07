#include "WiimoteScannerWin.h"

#include <assert.h>
#include <iostream>

#define DETAIL_SIZE 512

struct DeviceDesc
{
	HANDLE openDevice;
	WCHAR productName[255];
	DWORD productId;
	char detail[DETAIL_SIZE];
};

WiimoteScannerWindows::WiimoteScannerWindows()
{
	HidD_GetHidGuid(&_hidGuid);
	memset(&_deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
	_deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	_deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
}

WiimoteScannerWindows::~WiimoteScannerWindows()
{
	
}

void WiimoteScannerWindows::scan(ScanResults& added, ScanResults& removed)
{
	DeviceDesc desc;

	_deviceInfoSet = SetupDiGetClassDevs(&_hidGuid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (!_deviceInfoSet)
	{
		std::cout << "No HID Devices Found!" << std::endl;
	}

	int count = 0;
	while (SetupDiEnumDeviceInterfaces(_deviceInfoSet, NULL, &_hidGuid, count, &_deviceInterfaceData))
	{
		memset(&desc, 0, sizeof(desc));
		checkDeviceInterface(desc);

		PSP_DEVICE_INTERFACE_DETAIL_DATA detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)desc.detail;
		XXH64_hash_t hash = XXH64(detail->DevicePath, wcslen(detail->DevicePath) * 2, 0);

		if (_scanResults.find(hash) == _scanResults.end())
		{
			DeviceScanState state;
			state.wiimote = checkDevice(detail->DevicePath, desc);
			state.handle = desc.openDevice;
			_scanResults[hash] = state;

			if (state.wiimote)
			{
				state.useOutputSize = usingToshibaStack();
				added[hash] = state;
			}
		}

		count++;
	}

	if (_deviceInfoSet)
	{
		SetupDiDestroyDeviceInfoList(_deviceInfoSet);
		_deviceInfoSet = NULL;
	}
}

void WiimoteScannerWindows::checkDeviceInterface(DeviceDesc& desc)
{
	BOOL result;
	DWORD requiredSize;

	result = SetupDiGetDeviceInterfaceDetail(_deviceInfoSet, &_deviceInterfaceData, NULL, 0, &requiredSize, NULL);

	assert(requiredSize <= DETAIL_SIZE);

	PSP_DEVICE_INTERFACE_DETAIL_DATA detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)desc.detail;
	detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	result = SetupDiGetDeviceInterfaceDetail(_deviceInfoSet, &_deviceInterfaceData, detail, requiredSize, NULL, &_deviceInfoData);
}

bool WiimoteScannerWindows::checkDevice(LPCTSTR devicePath, DeviceDesc & desc)
{
	desc.openDevice = CreateFile(devicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

	if (desc.openDevice == INVALID_HANDLE_VALUE)
	{
		desc.openDevice = nullptr;
		return false;
	}

	HIDD_ATTRIBUTES hidAttributes;
	hidAttributes.Size = sizeof(hidAttributes);

	BOOL result = HidD_GetAttributes(desc.openDevice, &hidAttributes);
	if (!result)
	{
		CloseHandle(desc.openDevice);
		desc.openDevice = nullptr;
		return false;
	}

	desc.productId = hidAttributes.ProductID;
	HidD_GetProductString(desc.openDevice, desc.productName, 255);

	if (!(hidAttributes.VendorID == 0x057E && (hidAttributes.ProductID == 0x0306) || hidAttributes.ProductID == 0x0330))
	{
		CloseHandle(desc.openDevice);
		desc.openDevice = nullptr;
		return false;
	}

	return true;
}

bool WiimoteScannerWindows::usingToshibaStack()
{
	/*HDEVINFO ParentDeviceInfoSet;
	SP_DEVINFO_DATA ParentDeviceInfoData;
	std::vector<WCHAR> ParentDeviceID;

	bool Result = GetParentDevice(_deviceInfoData.DevInst, ParentDeviceInfoSet, &ParentDeviceInfoData, ParentDeviceID);
	if (!Result)
	{
	return false;
	}

	std::wstring Provider = GetDeviceProperty(ParentDeviceInfoSet, &ParentDeviceInfoData, &DEVPKEY_Device_DriverProvider);

	SetupDiDestroyDeviceInfoList(ParentDeviceInfoSet);

	return (Provider == L"TOSHIBA");*/
	return false;
}

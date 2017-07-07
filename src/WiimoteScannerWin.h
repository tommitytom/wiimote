#pragma once

#include "stdafx.h"

#include <map>
#include "xxhash.h"

struct DeviceDesc;

struct DeviceScanState
{
	bool wiimote = false;
	void* handle = nullptr;
	bool useOutputSize = false;
};

typedef std::map<XXH64_hash_t, DeviceScanState> ScanResults;

class WiimoteScannerWindows
{
private:
	ScanResults _scanResults;

	GUID _hidGuid;
	HDEVINFO _deviceInfoSet;
	SP_DEVINFO_DATA _deviceInfoData;
	SP_DEVICE_INTERFACE_DATA _deviceInterfaceData;

public:
	WiimoteScannerWindows();
	~WiimoteScannerWindows();

	void scan(ScanResults& added, ScanResults& removed);

private:
	void checkDeviceInterface(DeviceDesc& desc);

	bool checkDevice(LPCTSTR devicePath, DeviceDesc& desc);

	bool usingToshibaStack();
};
#include "WiimoteFactory.h"

WiimoteFactoryWindows::WiimoteFactoryWindows()
	:_deviceIndex(0), _deviceInfoSet(NULL), _openDevice(INVALID_HANDLE_VALUE)
{
	HidD_GetHidGuid(&_hidGuid);

	ZeroMemory(&_deviceInfoData, sizeof(SP_DEVINFO_DATA));
	_deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	_deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
}


WiimoteFactoryWindows::~WiimoteFactoryWindows()
{
	for (WiimoteDevice* wiimote : _wiimoteDevices)
	{
		delete wiimote;
	}

	if (_deviceInfoSet)
	{
		SetupDiDestroyDeviceInfoList(_deviceInfoSet);
		_deviceInfoSet = NULL;
	}
}

WiimoteDeviceVector WiimoteFactoryWindows::getWiimoteDevices()
{
	_deviceInfoSet = SetupDiGetClassDevs(&_hidGuid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (!_deviceInfoSet)
	{
		std::cout << "No HID Devices Found!" << std::endl;
		return _wiimoteDevices;
	}

	while (SetupDiEnumDeviceInterfaces(_deviceInfoSet, NULL, &_hidGuid, _deviceIndex, &_deviceInterfaceData))
	{
		//std::cout << "--- New Device ---" << std::endl;
		CheckEnumeratedDeviceInterface();
		_deviceIndex++;
		//std::cout << "------------------" << std::endl;
	}

	if (_deviceIndex == 0)
	{
		std::cout << "No Device Enumerated!" << std::endl;
	}

	return _wiimoteDevices;
}

void WiimoteFactoryWindows::CheckEnumeratedDeviceInterface()
{
	BOOL Result;
	DWORD RequiredSize;

	Result = SetupDiGetDeviceInterfaceDetail(_deviceInfoSet, &_deviceInterfaceData, NULL, 0, &RequiredSize, NULL);

	PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(RequiredSize);
	DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	Result = SetupDiGetDeviceInterfaceDetail(_deviceInfoSet, &_deviceInterfaceData, DeviceInterfaceDetailData, RequiredSize, NULL, &_deviceInfoData);
 
	BOOL IsWiimote = CheckDevice(DeviceInterfaceDetailData->DevicePath);

	if (IsWiimote)
	{
		_wiimoteDevices.push_back(new WiimoteDevice(_openDevice, IsUsingToshibaStack()));
	}
	else
	{
		CloseHandle(_openDevice);
	}

	_openDevice = INVALID_HANDLE_VALUE;
	free(DeviceInterfaceDetailData);
}

bool WiimoteFactoryWindows::CheckDevice(LPCTSTR DevicePath)
{
	_openDevice = CreateFile(DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

	if (_openDevice == INVALID_HANDLE_VALUE)
	{
		//std::cout << "Failed to open Device." << std::endl;
		return false;
	}

	//std::cout << "Device Handle: \t0x" << std::hex << OpenDevice << std::endl;

	HIDD_ATTRIBUTES HidAttributes;
	HidAttributes.Size = sizeof(HidAttributes);

	BOOL Result = HidD_GetAttributes(_openDevice, &HidAttributes);
	if (!Result)
	{
		//std::cout << "Failed to get hid attributes" << std::endl;
		return false;
	}

	//std::cout << "VID&PID: \t" << HidAttributes.VendorID << " - " << HidAttributes.ProductID << std::endl;

	WCHAR ProductName[255];
	ZeroMemory(ProductName, sizeof(ProductName));

	if (HidD_GetProductString(_openDevice, ProductName, 255))
	{
		//std::wcout << "HID Name: \t" << ProductName << std::endl;
	}

	//PrintDriverInfo(DeviceInfoSet, &DeviceInfoData);
	//PrintDeviceTreeInfo(3, DeviceInfoData.DevInst);

	return (HidAttributes.VendorID == 0x057e) && ((HidAttributes.ProductID == 0x0306) || (HidAttributes.ProductID == 0x0330));
}

bool WiimoteFactoryWindows::IsUsingToshibaStack()
{
	HDEVINFO ParentDeviceInfoSet;
	SP_DEVINFO_DATA ParentDeviceInfoData;
	std::vector<WCHAR> ParentDeviceID;

	bool Result = GetParentDevice(_deviceInfoData.DevInst, ParentDeviceInfoSet, &ParentDeviceInfoData, ParentDeviceID);
	if (!Result)
	{
		return false;
	}

	std::wstring Provider = GetDeviceProperty(ParentDeviceInfoSet, &ParentDeviceInfoData, &DEVPKEY_Device_DriverProvider);

	SetupDiDestroyDeviceInfoList(ParentDeviceInfoSet);

	return (Provider == L"TOSHIBA");
}

void WiimoteFactoryWindows::PrintDeviceTreeInfo(UINT Levels, DEVINST ChildDevice)
{
	if (Levels == 0)
	{
		return;
	}

	std::cout << "  +----+  " << std::endl;

	HDEVINFO ParentDeviceInfoSet;
	SP_DEVINFO_DATA ParentDeviceInfoData;
	std::vector<WCHAR> ParentDeviceID;

	bool Result = GetParentDevice(ChildDevice, ParentDeviceInfoSet, &ParentDeviceInfoData, ParentDeviceID);
	if (!Result)
	{
		return;
	}

	std::wcout << "Device ID: \t" << ParentDeviceID.data() << std::endl;
	PrintDriverInfo(ParentDeviceInfoSet, &ParentDeviceInfoData);

	PrintDeviceTreeInfo(Levels - 1, ParentDeviceInfoData.DevInst);

	SetupDiDestroyDeviceInfoList(ParentDeviceInfoSet);
}

void WiimoteFactoryWindows::PrintDriverInfo(HDEVINFO& deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData)
{
	PrintDeviceProperty(deviceInfoSet, deviceInfoData, L"Device Name", &DEVPKEY_NAME);
	PrintDeviceProperty(deviceInfoSet, deviceInfoData, L"Description", &DEVPKEY_Device_DriverDesc);
	PrintDeviceProperty(deviceInfoSet, deviceInfoData, L"Provider", &DEVPKEY_Device_DriverProvider);
	PrintDeviceProperty(deviceInfoSet, deviceInfoData, L"Manufacturer", &DEVPKEY_Device_Manufacturer);
	PrintDeviceProperty(deviceInfoSet, deviceInfoData, L"PDO", &DEVPKEY_Device_PDOName);
	PrintDeviceProperty(deviceInfoSet, deviceInfoData, L"Bus Reported", &DEVPKEY_Device_BusReportedDeviceDesc);
	PrintDeviceProperty(deviceInfoSet, deviceInfoData, L"Device Driver", &DEVPKEY_Device_Driver);
}

void WiimoteFactoryWindows::PrintDeviceProperty(HDEVINFO & deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, const PWCHAR PropertyName, const DEVPROPKEY * Property)
{
	std::wstring Result = GetDeviceProperty(deviceInfoSet, deviceInfoData, Property);
	if (Result.empty())
	{
		std::wcout << "Error getting Device Property (" << PropertyName << "): 0x" << std::hex << GetLastError() << std::endl;
	}
	else
	{
		std::wcout << PropertyName << ": \t" << Result << std::endl;
	}
}

bool WiimoteFactoryWindows::GetParentDevice(const DEVINST & ChildDevice, HDEVINFO & ParentDeviceInfoSet, PSP_DEVINFO_DATA ParentDeviceInfoData, std::vector<WCHAR> & ParentDeviceID)
{
	ULONG Status;
	ULONG ProblemNumber;
	CONFIGRET Result;

	Result = CM_Get_DevNode_Status(&Status, &ProblemNumber, ChildDevice, 0);
	if (Result != CR_SUCCESS)
	{
		std::cout << "Something wrong woth the Device Node!" << std::endl;
		return false;
	}

	DEVINST ParentDevice;

	Result = CM_Get_Parent(&ParentDevice, ChildDevice, 0);
	if (Result != CR_SUCCESS)
	{
		std::cout << "Error getting parent: 0x" << std::hex << Result << std::endl;
		 return false;
	}

	ParentDeviceID.resize(MAX_DEVICE_ID_LEN);

	Result = CM_Get_Device_ID(ParentDevice, ParentDeviceID.data(), (ULONG)ParentDeviceID.size(), 0);
	if (Result != CR_SUCCESS)
	{
		std::cout << "Error getting parent device id: 0x" << std::hex << Result << std::endl;
		return false;
	}

	ParentDeviceInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
	ZeroMemory(ParentDeviceInfoData, sizeof(SP_DEVINFO_DATA));
	ParentDeviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);

	if (!SetupDiOpenDeviceInfo(ParentDeviceInfoSet, ParentDeviceID.data(), NULL, 0, ParentDeviceInfoData))
	{
		std::cout << "Error getting Device Info Data: 0x" << std::hex << GetLastError() << std::endl;
		SetupDiDestroyDeviceInfoList(ParentDeviceInfoSet);
		return false;
	}

	return true;
}

std::wstring WiimoteFactoryWindows::GetDeviceProperty(HDEVINFO & DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, const DEVPROPKEY * Property)
{
	DWORD RequiredSize = 0;
	DEVPROPTYPE DevicePropertyType;

	SetupDiGetDeviceProperty(DeviceInfoSet, DeviceInfoData, Property, &DevicePropertyType, NULL, 0, &RequiredSize, 0);

	std::vector<BYTE> Buffer(RequiredSize, 0);

	BOOL Result = SetupDiGetDeviceProperty(DeviceInfoSet, DeviceInfoData, Property, &DevicePropertyType, Buffer.data(), RequiredSize, NULL, 0);
	if (!Result)
	{
		return std::wstring();
	}

	return std::wstring((PWCHAR)Buffer.data());
}
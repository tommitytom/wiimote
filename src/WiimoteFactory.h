#pragma once

#include "stdafx.h"
#include "WiimoteDevice.h"
#include "Wiimote.h"

class WiimoteFactoryWindows
{
public:
	WiimoteFactoryWindows();
	~WiimoteFactoryWindows();

	WiimoteDeviceVector getWiimoteDevices();

private:
	WiimoteDeviceVector _wiimoteDevices;

	GUID _hidGuid;
	HDEVINFO _deviceInfoSet;
	DWORD _deviceIndex;

	SP_DEVINFO_DATA _deviceInfoData;
	SP_DEVICE_INTERFACE_DATA _deviceInterfaceData;

	HANDLE _openDevice;

	void CheckEnumeratedDeviceInterface();
	bool CheckDevice(LPCTSTR DevicePath);

	bool IsUsingToshibaStack();

	void PrintDeviceTreeInfo(UINT Levels, DEVINST ChildDevice);
	void PrintDriverInfo(HDEVINFO &DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData);
	void PrintDeviceProperty(HDEVINFO &DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, const PWCHAR PropertyName, const DEVPROPKEY * Property);
	
	bool GetParentDevice(const DEVINST & ChildDevice, HDEVINFO & ParentDeviceInfoSet, PSP_DEVINFO_DATA ParentDeviceInfoData, std::vector<WCHAR> & ParentDeviceID);
	std::wstring GetDeviceProperty(HDEVINFO &DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, const DEVPROPKEY * Property);

}; 

typedef WiimoteFactoryWindows WiimoteFactoryImpl;
typedef std::vector<Wiimote*> WiimoteVector;

class WiimoteFactory
{
private:
	WiimoteFactoryImpl _factory;

public:
	WiimoteFactory() {}
	~WiimoteFactory() {}

	WiimoteVector getWiimotes() 
	{
		WiimoteVector ret;
		auto devices = _factory.getWiimoteDevices();
		for (auto* d : devices)
		{
			ret.push_back(new Wiimote(d));
		}

		return ret;
	}
};


#include "WiimoteDevicePosix.h"

WiimoteDevicePosix::WiimoteDevicePosix(hid_device* handle) : _handle(handle), _useOutputReportSize(false)
{
}

WiimoteDevicePosix::~WiimoteDevicePosix()
{
}

bool WiimoteDevicePosix::setup()
{
	return false;
}

bool WiimoteDevicePosix::write(u8* data, size_t size)
{
	return hid_write(_handle, data, size) != -1;
}

bool WiimoteDevicePosix::read(u8* data, size_t size, size_t& bytesRead)
{
	int amount = hid_read(_handle, data, size);
	if (amount != -1)
	{
		bytesRead = amount;
		return true;
	}

	bytesRead = 0;
	return false;
}
#pragma once

#include <vector>
#include <functional>
#include "WiimoteState.h"
#include "../dep/hidapi/hidapi/hidapi.h"

typedef std::vector<u8> DataBuffer;
typedef std::function<void(const u8* data)> DataCallback;

class WiimoteDevicePosix
{
private:
	hid_device* _handle;
	bool _connected = false;
	int _outputReportSize;
	int _inputReportSize;
	const bool _useOutputReportSize;

	std::function<void(u8*)> _callback;

public:
	WiimoteDevicePosix(hid_device* handle);
	~WiimoteDevicePosix();

	bool setup();
	bool write(u8* data, size_t size);
	bool read(u8* data, size_t size, size_t& bytesRead);

	friend class WiimoteManager;
};
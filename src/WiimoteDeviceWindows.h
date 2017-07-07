#pragma once

#include "stdafx.h"
#include <vector>
#include "WiimoteState.h"
#include "concurrentqueue.h"

class WiimoteDeviceWindows;

typedef std::vector<WiimoteDeviceWindows*> WiimoteDeviceVector;
typedef std::vector<UCHAR> DataBuffer;

typedef std::function<void(const u8* data)> DataCallback;

class WiimoteDeviceWindows
{
private:
	bool writeFallback(u8* target, size_t size);

	const bool _useOutputReportSize;

	HANDLE _deviceHandle;
	OVERLAPPED _readIo;

	bool _connected = false;
	bool _valid = false;

	size_t _outputReportSize;
	size_t _inputReportSize;

public:
	WiimoteDeviceWindows(HANDLE deviceHandle, bool useOutputReportSize);
	~WiimoteDeviceWindows();

	bool setup();
	bool read(u8* target, size_t size, size_t& bytesRead);
	bool write(u8* target, size_t size);
	bool connected() const { return _connected; }
	size_t inputReportSize() const { return _inputReportSize; }

	friend class WiimoteManager;
};
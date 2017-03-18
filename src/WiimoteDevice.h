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
public:
	WiimoteDeviceWindows(HANDLE _deviceHandle, bool _useOutputReportSize);
	~WiimoteDeviceWindows();

	bool setup();
	void disconnect();
	bool write(DataBuffer& Buffer);
	bool writeFallback(DataBuffer& Buffer);
	void startReader();
	void continuousReader();
	void setCallback(DataCallback callback) { _callback = callback; }

private:
	const bool _useOutputReportSize;

	HANDLE _deviceHandle;
	HANDLE _readThread;
	OVERLAPPED _readIo;

	bool _run;

	SHORT _outputReportMinSize;

	std::function<void(unsigned char*)> _callback;
};

typedef WiimoteDeviceWindows WiimoteDevice;
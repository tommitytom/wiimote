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
	bool write(DataBuffer& Buffer);
	bool writeFallback(DataBuffer& Buffer);
	void startReader();
	void stopReader();
	void continuousReader();
	void setCallback(DataCallback callback) { _callback = callback; }
	bool connected() const { return _connected; }

private:
	const bool _useOutputReportSize;

	HANDLE _deviceHandle;
	HANDLE _readThread;
	OVERLAPPED _readIo;

	bool _run;
	bool _connected = false;

	int _outputReportSize;
	int _inputReportSize;

	std::function<void(unsigned char*)> _callback;

	friend class WiimoteManager;
};

typedef WiimoteDeviceWindows WiimoteDevice;
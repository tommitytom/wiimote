#pragma once

#include "WiimoteDevice.h"
#include <mutex>
#include <queue>

typedef WiimoteDeviceWindows WiimoteDevice;

struct PendingRead
{
	DataBuffer buffer;
	DataCallback callback;
	RegisterType reg;
};

class Wiimote
{
private:
	WiimoteDevice* _device;
	WiimoteState _state;
	bool _expectingStatusUpdate = false;
	DataCallback _currentRead;
	RegisterType _currentReadRegister;
	bool _valid = false;
	std::mutex _lock; 
	std::queue<PendingRead> _pendingReads;

public:
	Wiimote(WiimoteDevice* device) : _device(device)
	{
		memset(&_state, 0, sizeof(WiimoteState));
		device->setCallback([this](const u8* data) { onMessage(data); });
		_valid = device->setup();
	}

	~Wiimote()
	{

	}

	void start(ReportMode mode = ReportModes::ButtonsAccel);
	void stop() { _device->disconnect(); }

	bool valid() const { return _valid; }
	bool available() { return _valid && updateStatus(); }

	bool setLeds(LedState state);
	bool setReportMode(ReportMode mode);
	bool setRumble(bool rumble);

	bool activateMotionPlus();
	bool deactivateMotionPlus();

	void getState(WiimoteState& state)
	{
		_lock.lock();
		memcpy(&state, &_state, sizeof(WiimoteState));
		_lock.unlock();
	}

private:
	bool resetReportMode();
	bool updateStatus();
	void updateExtension();
	void updateWiimoteCalibration();
	void updateNunchuckCalibration();

	bool activateExtension();

	void identifyExtension();
	void identifyMotionPlus();

	void readRegister(RegisterType type, short size, DataCallback callback);
	bool writeRegister(RegisterType type, const DataBuffer& buffer);
	bool writeReportMode(ReportMode mode);

	void onMessage(const u8* data);
};
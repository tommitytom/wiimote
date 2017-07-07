#pragma once

#include "WiimoteDevice.h"
#include <mutex>
#include <queue>
#include <thread>

#define BIT_ENUM(shift) 1 << shift

typedef WiimoteDeviceWindows WiimoteDevice;
typedef std::function<void(WiimoteButton button, bool down)> WiimoteButtonCallback;
typedef std::function<void(NunchuckButton button, bool down)> NunchuckButtonCallback;
typedef std::function<void(const Vector3&)> Vector3Callback;

namespace WiimoteFeatures {
	enum WiimoteFeature
	{
		None,
		Buttons = BIT_ENUM(1),
		Accelerometer = BIT_ENUM(2),
		Extension = BIT_ENUM(3),
		IR = BIT_ENUM(4),
		MotionPlus = BIT_ENUM(5),

		NoIR = Buttons | Accelerometer | Extension | MotionPlus,
		All = NoIR | IR
	};
}
typedef WiimoteFeatures::WiimoteFeature WiimoteFeature;
typedef int WiimoteFeatureFlags;

struct PendingRead
{
	u8 buffer[22];
	size_t size;
	DataCallback callback;
	RegisterType reg;
};

class Wiimote
{
private:
	WiimoteDevice* _device;
	WiimoteState _state;
	DataCallback _currentRead;
	RegisterType _currentReadRegister;
	std::mutex _lock; 
	std::queue<PendingRead> _pendingReads;
	
	int _features = WiimoteFeatures::None;

	WiimoteState _outState[2];
	int _outStateIdx = 0;
	int _id = -1;
	bool _running = false;

	WiimoteButtonCallback _buttonCallback;
	NunchuckButtonCallback _nunchuckButtonCallback;
	Vector3Callback _accelerometerCallback;
	Vector3Callback _nunchuckAccelerometerCallback;
	Vector3Callback _motionPlusCallback;

	bool _filterAccelerometer = false;
	bool _filterNunchuckAccelerometer = false;
	bool _filterMotionPlus = false;

	std::thread _thread;
	u8 _buffer[22];

public:
	Wiimote(WiimoteDevice* device) : _device(device)
	{
		memset(&_state, 0, sizeof(WiimoteState));
	}

	~Wiimote()
	{
		stop();
	}

	void setID(int id);

	int id() const { return _id; }

	void start(int featureFlags);
	void stop();

	bool available() { return _device->connected(); }

	bool setLeds(LedState state);
	bool setRumble(bool rumble);

	bool activateMotionPlus();
	bool deactivateMotionPlus();

	void onButton(WiimoteButtonCallback callback)
	{
		_buttonCallback = callback;
	}

	void onNunchuckButton(NunchuckButtonCallback callback)
	{
		_nunchuckButtonCallback = callback;
	}

	void onAccelerometer(Vector3Callback callback, bool filterDuplicates = false) 
	{ 
		_accelerometerCallback = callback; 
		_filterAccelerometer = filterDuplicates; 
	}

	void onNunchuckAccelerometer(Vector3Callback callback, bool filterDuplicates = false) 
	{
		_nunchuckAccelerometerCallback = callback; 
		_filterNunchuckAccelerometer = filterDuplicates;
	}

	void onMotionPlus(Vector3Callback callback, bool filterDuplicates = false) 
	{ 
		_motionPlusCallback = callback; 
		_filterMotionPlus = filterDuplicates;
	}

	void enableFeature(WiimoteFeature feature) 
	{
		_features |= feature;
	}

	void disableFeature(WiimoteFeature feature)
	{
		_features &= ~feature;
	}

	void setFeatures(WiimoteFeatureFlags featureFlags)
	{
		_features = featureFlags;
	}

	void update();

	void clearEvents();

	const WiimoteState& getState()
	{
		return _outState[_outStateIdx];
	}

private:
	bool updateStatus();
	void updateExtension();
	void updateWiimoteCalibration();
	void updateNunchuckCalibration();
	bool updateReportMode();

	bool activateExtension();

	void identifyExtension();
	void identifyMotionPlus();

	void readRegister(RegisterType type, short size, DataCallback callback);
	bool writeRegister(RegisterType type, u8* buffer, size_t size);
	bool writeRegister(RegisterType type, u8 val);

	void onMessage(const u8* data);

	void startReader();
	void stopReader();
	void continuousReader();

	u8* buffer(OutputReportType type)
	{
		memset(_buffer, 0, 22);
		_buffer[0] = type;
		return _buffer;
	}

	u8* buffer(OutputReportType type, u8 p1)
	{
		buffer(type)[1] = p1;
		return _buffer;
	}

	friend class WiimoteManager;
};
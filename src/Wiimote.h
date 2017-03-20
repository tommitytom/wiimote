#pragma once

#include "WiimoteDevice.h"
#include <mutex>
#include <queue>

#define BIT_ENUM(shift) 1 << shift

typedef WiimoteDeviceWindows WiimoteDevice;
typedef std::function<void(WiimoteButton button, bool down)> ButtonCallback;

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
	DataBuffer buffer;
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
	ButtonCallback _buttonCallback;
	int _features = WiimoteFeatures::None;

	WiimoteState _outState[2];
	int _outStateIdx = 0;
	int _id = -1;
	bool _running = false;

public:
	Wiimote(WiimoteDevice* device) : _device(device)
	{
		memset(&_state, 0, sizeof(WiimoteState));
		device->setCallback([this](const u8* data) { onMessage(data); });
	}

	~Wiimote()
	{
		stop();
	}

	void start(int featureFlags);
	void stop();

	bool available() { return _device->connected(); }

	bool setLeds(LedState state);
	bool setRumble(bool rumble);

	bool activateMotionPlus();
	bool deactivateMotionPlus();

	void onButton(ButtonCallback callback)
	{
		_buttonCallback = callback;
	}

	const WiimoteState& getState()
	{
		return _outState[_outStateIdx];
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
	bool writeRegister(RegisterType type, const DataBuffer& buffer);

	void onMessage(const u8* data);

	friend class WiimoteManager;
};
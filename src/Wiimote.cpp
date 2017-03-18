#include "stdafx.h"
#include "Wiimote.h"

#include <iostream>

#define CHECK_MASK(val, mask) ((val & mask) != 0x00)

static const int64_t ID_ActiveMotionPlus = 0x0000A4200405;
static const int64_t ID_ActiveMotionPlus_Nunchuck = 0x0000A4200505;
static const int64_t ID_ActiveMotionPlus_Classic = 0x0000A4200705;
static const int64_t ID_Nunchuck = 0x0000A4200000;
static const int64_t ID_Nunchuck2 = 0xFF00A4200000;
static const int64_t ID_Classic = 0x0000A4200101;
static const int64_t ID_ClassicPro = 0x0100A4200101;
static const int64_t ID_WiiUPro = 0x0000A4200120;
static const int64_t ID_Guitar = 0x0000A4200103;
static const u8 ID_InactiveMotionPlus[6] = { 0x00, 0x00, 0xA6, 0x20, 0x00, 0x05 };

void parseButtons(const u8* data, WiimoteState& state)
{
	state.buttons[WiimoteButtons::Up] = CHECK_MASK(data[1], 0x08);
	state.buttons[WiimoteButtons::Down] = CHECK_MASK(data[1], 0x04);
	state.buttons[WiimoteButtons::Left] = CHECK_MASK(data[1], 0x01);
	state.buttons[WiimoteButtons::Right] = CHECK_MASK(data[1], 0x02);
	state.buttons[WiimoteButtons::A] = CHECK_MASK(data[2], 0x08);
	state.buttons[WiimoteButtons::B] = CHECK_MASK(data[2], 0x04);
	state.buttons[WiimoteButtons::One] = CHECK_MASK(data[2], 0x02);
	state.buttons[WiimoteButtons::Two] = CHECK_MASK(data[2], 0x01);
	state.buttons[WiimoteButtons::Plus] = CHECK_MASK(data[1], 0X10);
	state.buttons[WiimoteButtons::Minus] = CHECK_MASK(data[2], 0x10);
	state.buttons[WiimoteButtons::Home] = CHECK_MASK(data[2], 0x80);
}

void parseStatus(const u8* data, WiimoteState& state)
{
	state.batteryLow = CHECK_MASK(data[3], 0x01);
	state.extensionConnected = CHECK_MASK(data[3], 0x02);
	state.speakerEnabled = CHECK_MASK(data[3], 0x04);
	state.irCameraEnabled = CHECK_MASK(data[3], 0x08);
	state.batteryRaw = data[6];
	state.battery = (((100.0f * 48.0f * (float)(state.batteryRaw / 48.0f))) / 192.0f);
	//state.leds = (LedState)data[4];
}

void parseAccel(const u8* data, WiimoteState& state)
{
	state.accelerometerRaw = Vector3(data[3], data[4], data[5]);
	state.accelerometer = (state.accelerometerRaw - state.accelCalibration.offset) * state.accelCalibration.scale;
}

void parseNunchuck(const u8* data, NunchuckState& state)
{
	state.joystickRaw = Vector2(data[0], data[1]);
	state.joystick = (state.joystickRaw - state.joystickCalibration.offset) * state.joystickCalibration.scale;

	state.accelerometerRaw = Vector3(data[2], data[3], data[4]);
	state.accelerometer = (state.accelerometerRaw - state.accelCalibration.offset) * state.accelCalibration.scale;
	
	state.buttons[NunchuckButtons::C] = !CHECK_MASK(data[5], 0x02);
	state.buttons[NunchuckButtons::Z] = !CHECK_MASK(data[5], 0x01);
}

// See https://github.com/Flafla2/Unity-Wiimote/blob/master/Assets/Wiimote/Scripts/WiimoteData/MotionPlusData.cs#L72
static const float c_MagicCalibrationConstant = 0.05f;
// See https://github.com/Flafla2/Unity-Wiimote/blob/master/Assets/Wiimote/Scripts/WiimoteData/MotionPlusData.cs#L63
static const Vector3 c_MotionPlusSpeedZero = Vector3(8063, 8063, 8063);

void parseMotionPlus(const u8* data, MotionPlusState& state)
{
	state.speedRaw.x = (float)(data[0] | ((data[3] & 0xFC) << 6));
	state.speedRaw.y = (float)(data[1] | ((data[4] & 0xFC) << 6));
	state.speedRaw.z = (float)(data[2] | ((data[5] & 0xFC) << 6));
	state.yawSlow = (data[3] & 0x02) == 0x02;
	state.pitchSlow = (data[3] & 0x01) == 0x01;
	state.rollSlow = (data[4] & 0x02) == 0x02;
	state.extensionConnected = (data[4] & 0x01) == 0x01;
	state.speed = (state.speedRaw - c_MotionPlusSpeedZero) * c_MagicCalibrationConstant;
}

void parseExtension(const u8* data, WiimoteState& state)
{
	switch (state.extensionType) 
	{
		case ExtensionTypes::Nunchuck: 
		case ExtensionTypes::Nunchuck_TR:
			parseNunchuck(data, state.nunchuck);
			break;
		case ExtensionTypes::ClassicController:
		case ExtensionTypes::Guitar:
		case ExtensionTypes::Drums:
		case ExtensionTypes::BalanceBoard:
		case ExtensionTypes::TaikoDrum:
		case ExtensionTypes::MotionPlus:
		case ExtensionTypes::MotionPlus_TR:
			break;
	}

	if (state.motionPlusActive)
	{
		parseMotionPlus(data, state.motionPlus);
	}
}

void parseReadData(const u8* data, DataCallback callback, RegisterType reg)
{
	if ((data[3] & 0x08) != 0x00)
	{
		std::cout << "Error reading data from Wiimote: Bytes do not exist." << std::endl;
		return;
	}

	if ((data[3] & 0x07) != 0x00 && reg != RegisterTypes::ExtensionType2)
	{
		std::cout << "Error reading data from Wiimote: Attempt to read from write-only registers." << std::endl;
		return;
	}

	//int size = (data[3] >> 4) + 1;
	//int offset = (data[4] << 8 | data[5]);

	callback(data + 6);
}

bool Wiimote::setReportMode(ReportMode mode)
{
	_lock.lock();
	_state.reportMode = mode;
	_lock.unlock();

	return _device->write(DataBuffer({ OutputReportType::DataReportMode, 0x00, mode }));
}

bool Wiimote::resetReportMode()
{
	_lock.lock();
	ReportMode mode = _state.reportMode;
	_lock.unlock();

	return _device->write(DataBuffer({ OutputReportType::DataReportMode, 0x00, mode }));
}

void Wiimote::start(ReportMode mode) 
{ 
	setReportMode(mode);
	updateWiimoteCalibration();
	identifyMotionPlus();
	_device->startReader();
}

bool Wiimote::setLeds(LedState state)
{
	_lock.lock();
	_state.leds = state;
	u8 s = _state.leds | (_state.rumble == true ? 0x01 : 0x00);
	_lock.unlock();

	return _device->write(DataBuffer({ OutputReportType::LEDs, s }));
}

bool Wiimote::setRumble(bool rumble)
{
	_lock.lock();
	_state.rumble = rumble;
	u8 s = _state.leds | (_state.rumble == true ? 0x01 : 0x00);
	_lock.unlock();

	return _device->write(DataBuffer({ OutputReportType::LEDs, s }));
}

bool Wiimote::updateStatus()
{
	_expectingStatusUpdate = true;
	u8 s = (_state.rumble == true ? 0x01 : 0x00);

	if (!_device->write(DataBuffer({ OutputReportType::Status, s })))
	{
		_expectingStatusUpdate = false;
		return false;
	}

	return true;
}

void Wiimote::updateWiimoteCalibration()
{
	readRegister(RegisterTypes::Calibration, 7, [this](const u8* d) {
		_state.accelCalibration.offset.x = (float)d[0];
		_state.accelCalibration.offset.y = (float)d[1];
		_state.accelCalibration.offset.z = (float)d[2];
		_state.accelCalibration.scale.x = 1.0f / (d[4] - _state.accelCalibration.offset.x);
		_state.accelCalibration.scale.y = 1.0f / (d[5] - _state.accelCalibration.offset.y);
		_state.accelCalibration.scale.z = 1.0f / (d[6] - _state.accelCalibration.offset.z);
	});
}

void Wiimote::updateNunchuckCalibration()
{
	readRegister(RegisterTypes::ExtensionCalibration, 16, [this](const u8* d) {
		NunchuckState& n = _state.nunchuck;

		n.accelCalibration.offset.x = (float)d[0];
		n.accelCalibration.offset.y = (float)d[1];
		n.accelCalibration.offset.z = (float)d[2];
		n.accelCalibration.scale.x = 1.0f / (d[4] - _state.accelCalibration.offset.x);
		n.accelCalibration.scale.y = 1.0f / (d[5] - _state.accelCalibration.offset.y);
		n.accelCalibration.scale.z = 1.0f / (d[6] - _state.accelCalibration.offset.z);

		n.joystickCalibration.offset.x = (float)d[10];
		n.joystickCalibration.offset.y = (float)d[13];
		n.joystickCalibration.scale.x = 1.0f / ((float)d[8] - (float)d[9]);
		n.joystickCalibration.scale.y = 1.0f / ((float)d[11] - (float)d[9]);
	});
}

void Wiimote::updateExtension()
{
	readRegister(RegisterTypes::ExtensionType2, 1, [this](const u8* d) {
		bool motionPlus = d[0] == 0x04;

		if (_state.extensionConnected || motionPlus)
		{
			if (!motionPlus)
			{
				activateExtension();
			}

			identifyExtension();
		} else {
			if (_state.extensionType != ExtensionTypes::None)
			{
				std::cout << ExtensionTypes::toString(_state.extensionType) << " disconnected" << std::endl;
			}

			_state.extensionType = ExtensionTypes::None;
		}
	});
}

void Wiimote::identifyExtension()
{
	readRegister(RegisterTypes::ExtensionType1, 6, [this](const u8* d) {
		int64_t type = ((int64_t)d[0] << 40) | ((int64_t)d[1] << 32) | ((int64_t)d[2]) << 24 | ((int64_t)d[3]) << 16 | ((int64_t)d[4]) << 8 | d[5];

		if ((type | 0xff000000ff00) == (ID_ActiveMotionPlus | 0xff000000ff00))
		{
			//assert(false);
		}

		switch (type)
		{
			case ExtensionTypes::None:
				std::cout << "No extension detected" << std::endl;
				break;
			case ExtensionTypes::ParitallyInserted:
				_state.extensionType = ExtensionTypes::None;
				std::cout << "Partially inserted" << std::endl;
				activateExtension();
				identifyExtension();
				break;
			case ExtensionTypes::Nunchuck:
			case ExtensionTypes::Nunchuck_TR:
			case ExtensionTypes::ClassicController:
			case ExtensionTypes::Guitar:
			case ExtensionTypes::BalanceBoard:
			case ExtensionTypes::Drums:
			case ExtensionTypes::TaikoDrum:
				_state.extensionConnected = true;
				_state.extensionType = (ExtensionType)type;

				std::cout << ExtensionTypes::toString(_state.extensionType) << " connected" << std::endl;
				writeReportMode(ReportModes::ButtonsAccelExtension);

				break;
			case ExtensionTypes::MotionPlus:
			case ExtensionTypes::MotionPlus_TR:
				if (!_state.motionPlusActive)
				{
					_state.motionPlusActive = true;
					writeReportMode(ReportModes::ButtonsExtension19);
					std::cout << "Motion plus active" << std::endl;
				}
				
				break;
			default:
				std::cout << "Unknown extension type" << std::endl;
		}

		switch (_state.extensionType)
		{
			case ExtensionTypes::Nunchuck:
			case ExtensionTypes::Nunchuck_TR:
				updateNunchuckCalibration();
				break;
		}
	});
}

void Wiimote::identifyMotionPlus()
{
	readRegister(RegisterTypes::MotionPlusAvailable, 6, [this](const u8* d) {
		if (d[0] == 0x01)
		{
			_state.wiimoteType = WiimoteTypes::WiimotePlus;
			_state.motionPlusAttached = true;
		} else {
			for (int x = 0; x < 6; x++)
			{
				if (x != 4 && x != 0 && d[x] != ID_InactiveMotionPlus[x])
				{
					_state.motionPlusAttached = false;
					return;
				}
			}

			_state.motionPlusAttached = true;
		}		
	});
}

void Wiimote::readRegister(RegisterType type, short size, DataCallback callback)
{
	_pendingReads.push({ 
		DataBuffer({
			OutputReportTypes::Read,
			(u8)(((type & 0xff000000) >> 24) | (_state.rumble ? 0x01 : 0x00)),
			(u8)((type & 0x00ff0000) >> 16),
			(u8)((type & 0x0000ff00) >> 8),
			(u8)(type & 0x000000ff),
			(u8)((size & 0xff00) >> 8),
			(u8)(size & 0xff)
		}),
		callback,
		type
	 });
}

bool Wiimote::writeRegister(RegisterType type, const DataBuffer& buffer)
{
	DataBuffer data({
		OutputReportTypes::write,
		(u8)(((type & 0xff000000) >> 24) | (_state.rumble ? 0x01 : 0x00)),
		(u8)((type & 0x00ff0000) >> 16),
		(u8)((type & 0x0000ff00) >> 8),
		(u8)(type & 0x000000ff),
		(u8)buffer.size()
	});

	for (size_t i = 0; i < buffer.size(); ++i)
	{
		data.push_back(buffer[i]);
	}

	data.resize(22, 0);

	return _device->write(data);
}

void Wiimote::onMessage(const u8* b)
{
	_lock.lock();

	bool motionPlusExtension = _state.motionPlus.extensionConnected;

	switch (b[0])
	{
		case ReportModes::Buttons:
			parseButtons(b, _state);
			break;

		case ReportModes::ButtonsAccel:
			parseButtons(b, _state);
			parseAccel(b, _state);
			break;

		case ReportModes::ButtonsExtension19:
			parseButtons(b, _state);
			parseExtension(b + 3, _state);
			break;

		case ReportModes::ButtonsAccelExtension:
			parseButtons(b, _state);
			parseAccel(b, _state);
			parseExtension(b + 6, _state);
			break;

		case InputReportTypes::Read:
			if (_currentRead != nullptr)
			{
				DataCallback fn = _currentRead;
				_currentRead = nullptr;
				parseReadData(b, fn, _currentReadRegister);
			} else {
				std::cout << "Read failed - no callback set!" << std::endl;
			}
			
			break;

		case InputReportTypes::Status:
		{
			if (!_expectingStatusUpdate) writeReportMode(_state.reportMode);
			_expectingStatusUpdate = false;

			parseButtons(b, _state);
			parseStatus(b, _state);
			updateExtension();
		}
	}

	if (_state.motionPlusActive && _state.motionPlus.extensionConnected != motionPlusExtension)
	{
		if (_state.motionPlus.extensionConnected)
		{
			std::cout << "Extension connected" << std::endl;
			updateExtension();
		} else {
			std::cout << "Extension disconnected" << std::endl;
			updateExtension();
		}
	}

	if (_currentRead == nullptr && !_pendingReads.empty())
	{
		PendingRead& r = _pendingReads.front();
		_currentRead = r.callback;
		_currentReadRegister = r.reg;
		_device->write(r.buffer);
		_pendingReads.pop();
	}

	_lock.unlock();
}

bool Wiimote::writeReportMode(ReportMode mode)
{
	_state.reportMode = mode;
	return _device->write(DataBuffer({ OutputReportType::DataReportMode, 0x00, mode }));
}

bool Wiimote::activateExtension()
{
	if (!_state.extensionConnected)
	{
		std::cout << "There is a request to activate an Extension controller even though it has not been confirmed to exist!  Trying anyway..." << std::endl;
	}

	std::cout << "Activating extension..." << std::endl;

	return	writeRegister(RegisterTypes::ExtensionInit1, DataBuffer({ 0x55 })) &&
			writeRegister(RegisterTypes::ExtensionInit2, DataBuffer({ 0x00 }));
}

bool Wiimote::activateMotionPlus()
{
	if (!_state.motionPlusAttached) 
	{
		std::cout << "There is a request to activate the Wii Motion Plus even though it has not been confirmed to exist!  Trying anyway..." << std::endl;
	}

	std::cout << "Activating motion plus..." << std::endl;

	return	writeRegister(RegisterTypes::MotionPlusInit1, DataBuffer({ 0x55 })) &&
			writeRegister(RegisterTypes::MotionPlusInit2, DataBuffer({ 0x04 }));
}

bool Wiimote::deactivateMotionPlus()
{
	if (!_state.motionPlusActive)
	{
		std::cout << "Attempting to deactivate motion plus while it is not active!" << std::endl;
	}

	std::cout << "Deactivating motion plus..." << std::endl;

	if (writeRegister(RegisterTypes::ExtensionInit1, DataBuffer({ 0x55 })))
	{
		_state.motionPlusActive = false;
		return true;
	}

	return false;
}

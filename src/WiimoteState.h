#pragma once

#include <ostream>

typedef unsigned char u8;

namespace LedStates {
	enum LedState : unsigned char
	{
		Off = 0,
		One = 0x10,
		Two = 0x20,
		Three = 0x40,
		Four = 0x80,
		All = One | Two | Three | Four
	};
}
typedef LedStates::LedState LedState;

namespace WiimoteButtons {
	enum WiimoteButton : unsigned char
	{
		Up, Down, Left, Right,
		A, B,
		One, Two,
		Plus, Minus,
		Home
	};
}
typedef WiimoteButtons::WiimoteButton WiimoteButton;

namespace NunchuckButtons {
	enum NunchuckButton : unsigned char
	{
		C, Z
	};
}
typedef NunchuckButtons::NunchuckButton NunchuckButton;

namespace OutputReportTypes {
	enum OutputReportType : unsigned char
	{
		LEDs = 0x11,
		DataReportMode = 0x12,
		IRCamera1 = 0x13,
		IRCamera2 = 0x1A,
		SpeakerState = 0x14,
		SpeakerData = 0x18,
		SpeakerMute = 0x19,
		Status = 0x15,
		write = 0x16,
		Read = 0x17
	};
}
typedef OutputReportTypes::OutputReportType OutputReportType;

namespace InputReportTypes {
	enum InputReportType : unsigned char
	{
		Status = 0x20,
		Read = 0x21,
		Ack = 0x22
	};
}
typedef InputReportTypes::InputReportType InputReportType;

namespace ExtensionControllerTypes {
	enum ExtensionControllerType : unsigned char
	{
		/// No Extension Controller is connected.
		None,
		Nunchuck,
		Classic,
		ClassicPro,
		WiUPro,
		/// An activated Wii Motion Plus with no extension controllers in passthrough mode.
		MotionPlus,
		/// An activated Wii Motion Plus with a Nunchuck in passthrough mode.
		/// \warning Nunchuck passthrough is currently not supported.
		MotionPlus_Nunchuck,
		/// An activated Wii Motion Plus with a Classic Controller in passthrough mode. 
		/// \warning Classic Controller passthrough is currently not supported
		MotionPlus_Classic,
		Guitar
	};
}
typedef ExtensionControllerTypes::ExtensionControllerType ExtensionControllerType;

namespace WiimoteTypes {
	enum WiimoteType : unsigned char {
		/// The original Wii Remote (Name: RVL-CNT-01).  This includes all Wii Remotes manufactured for the original Wii.
		Wiimote,
		/// The new Wii Remote Plus (Name: RVL-CNT-01-TR).  Wii Remote Pluses are now standard with Wii U consoles and come
		/// with a built-in Wii Motion Plus extension.
		WiimotePlus,
		/// The Wii U Pro Controller (Name: RVL-CNT-01-UC) behaves identically to a Wii Remote with a Classic Controller
		/// attached.  Obviously the Pro Controller does not support IR so those features will not work.
		ProController
	};
}
typedef WiimoteTypes::WiimoteType WiimoteType;

namespace ReportModes {
	enum ReportMode : unsigned char
	{
		Unknown,
		Buttons = 0x30,
		ButtonsAccel = 0x31,
		ButtonsExtension8 = 0x32,
		ButtonsExtension19 = 0x34,
		ButtonsAccelIR = 0x33,
		ButtonsAccelIRInterleaved1 = 0x3E,
		ButtonsAccelIRInterleaved2 = 0x3F,
		ButtonsAccelExtension = 0x35,
		ButtonsIRExtension = 0x36,
		ButtonsAccelIRExtension = 0x37,
		Extension = 0x3D
	};

	static const char* toString(ReportMode mode)
	{
		switch (mode)
		{
			case ReportModes::Buttons: return "Buttons";
			case ReportModes::ButtonsAccel: return "ButtonsAccel";
			case ReportModes::ButtonsExtension8: return "ButtonsExtension8";
			case ReportModes::ButtonsExtension19: return "ButtonsExtension19";
			case ReportModes::ButtonsAccelIR: return "ButtonsAccelIR";
			case ReportModes::ButtonsAccelIRInterleaved1: return "ButtonsAccelIRInterleaved1";
			case ReportModes::ButtonsAccelIRInterleaved2: return "ButtonsAccelIRInterleaved2";
			case ReportModes::ButtonsAccelExtension: return "ButtonsAccelExtension";
			case ReportModes::ButtonsIRExtension: return "ButtonsIRExtension";
			case ReportModes::ButtonsAccelIRExtension: return "ButtonsAccelIRExtension";
			case ReportModes::Extension: return "Extension";
		}

		return "Unknown";
	}
}
typedef ReportModes::ReportMode ReportMode;

namespace RegisterTypes {
	enum RegisterType : int {
		Calibration = 0x0016,
		Ir = 0x04b00030,
		IrSensitivity1 = 0x04b00000,
		IrSensitivity2 = 0x04b0001a,
		IrMode = 0x04b00033,
		ExtensionInit1 = 0x04a400f0,
		ExtensionInit2 = 0x04a400fb,
		ExtensionType1 = 0x04a400fa,
		ExtensionType2 = 0x04a400fe,
		ExtensionCalibration = 0x04a40020,
		MotionPlusAvailable = 0x04A600FA,
		MotionPlusInit1 = 0x04A600F0,
		MotionPlusInit2 = 0x04a600fe
	};
}
typedef RegisterTypes::RegisterType RegisterType;

namespace ExtensionTypes {
	enum ExtensionType : int64_t {
		None = 0x000000000000,
		Nunchuck = 0x0000A4200000,
		Nunchuck_TR = 0xFF00A4200000,
		ClassicController = 0x0000A4200101,
		Guitar = 0x0000A4200103,
		Drums = 0x0100A4200103,
		BalanceBoard = 0x0000A4200402,
		TaikoDrum = 0x0000A4200111,
		MotionPlus = 0x0000A4200405,
		MotionPlus_TR = 0x0100A4200405,
		ParitallyInserted = 0xffffffffffff
	};

	static const char* toString(ExtensionType type)
	{
		switch (type)
		{
			case ExtensionTypes::None: return "None";
			case ExtensionTypes::Nunchuck: return "Nunchuck";
			case ExtensionTypes::Nunchuck_TR: return "Nunchuck_TR";
			case ExtensionTypes::ClassicController: return "ClassicController";
			case ExtensionTypes::Guitar: return "Guitar";
			case ExtensionTypes::Drums: return "Drums";
			case ExtensionTypes::BalanceBoard: return "BalanceBoard";
			case ExtensionTypes::TaikoDrum: return "TaikoDrum";
			case ExtensionTypes::MotionPlus: return "MotionPlus";
			case ExtensionTypes::MotionPlus_TR: return "MotionPlus_TR";
			case ExtensionTypes::ParitallyInserted: return "ParitallyInserted";
		}

		return "Unknown";
	}
}
typedef ExtensionTypes::ExtensionType ExtensionType;

class Vector2
{
public:
	float x, y;

	Vector2() : x(0), y(0) {}
	Vector2(float _x, float _y) : x(_x), y(_y) {}

	Vector2 operator+(const Vector2& other) { return Vector2(x + other.x, y + other.y); }
	Vector2 operator-(const Vector2& other) { return Vector2(x - other.x, y - other.y); }
	Vector2 operator*(const Vector2& other) { return Vector2(x * other.x, y * other.y); }
	Vector2 operator/(const Vector2& other) { return Vector2(x / other.x, y / other.y); }

	friend std::ostream & operator<<(std::ostream &os, const Vector2& p)
	{
		os << "{ " << p.x << ", " << p.y << " }";
	}
};
class Vector3
{
public:
	float x, y, z;

	Vector3() : x(0), y(0), z(0) {}
	Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

	Vector3 operator+(const Vector3& other) { return Vector3(x + other.x, y + other.y, z + other.z); }
	Vector3 operator-(const Vector3& other) { return Vector3(x - other.x, y - other.y, z - other.z); }
	Vector3 operator*(const Vector3& other) { return Vector3(x * other.x, y * other.y, z * other.z); }
	Vector3 operator/(const Vector3& other) { return Vector3(x / other.x, y / other.y, z / other.z); }
	Vector3 operator*(float v) { return Vector3(x * v, y * v, z * v); }

	friend std::ostream & operator<<(std::ostream &os, const Vector3& p)
	{
		return os << "{ " << p.x << ", " << p.y << ", " << p.z << " }";
	}
};

const int c_WiimoteButtonCount = 11;
typedef bool WiimoteButtonState[c_WiimoteButtonCount];
typedef bool NunchuckButtonState[2];

struct AccelCalibrationInfo
{
	Vector3 offset;
	Vector3 scale;
};

struct JoystickCalibrationInfo
{
	Vector2 offset;
	Vector2 scale;
};

struct NunchuckState
{
	Vector2 joystickRaw;
	Vector2 joystick;
	Vector3 accelerometerRaw;
	Vector3 accelerometer;
	AccelCalibrationInfo accelCalibration;
	JoystickCalibrationInfo joystickCalibration;
	NunchuckButtonState buttons = { false };
};

struct MotionPlusState
{
	Vector3 speedRaw;
	Vector3 speed;
	bool yawSlow = false;
	bool pitchSlow = false;
	bool rollSlow = false;
	bool extensionConnected = false;
};

struct WiimoteState
{
	WiimoteType wiimoteType = WiimoteTypes::Wiimote;
	ReportMode reportMode = ReportModes::Buttons;
	LedState leds = LedStates::Off;
	ExtensionType extensionType = ExtensionTypes::None;

	WiimoteButtonState buttons = { false };
	NunchuckState nunchuck;
	MotionPlusState motionPlus;

	AccelCalibrationInfo accelCalibration;
	Vector3 accelerometerRaw;
	Vector3 accelerometer;

	int batteryRaw = 0;
	float battery = 0;
	bool irCameraEnabled = false;
	bool batteryLow = false;
	bool extensionConnected = false;
	bool extensionActive = false;
	bool speakerEnabled = false;
	bool motionPlusAvailable = false;
	bool motionPlusActive = false;
	bool rumble = false;
};

const int _stateSize = sizeof(WiimoteState);
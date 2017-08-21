# wiimote
A modern Wiimote library written in C++11

## Features
* Supports TR-01 Wiimotes
* Optimized for the dolphin bar
* Continuously scans for new Wiimotes
* Nice C++11 interface using lambdas

### Not yet implemented
* Linux/MacOS support.  Windows only for now, though work has started on a cross platform version
* No special controllers yet (such as classic controller, balance board etc)

## Usage
```cpp
WiimoteManager m;
m.onConnection([](Wiimote* wiimote) { 
	std::cout << "Wiimote " << wiimote->id() + 1 << " connected!" << std::endl;

	wiimote->onButton([wiimote](WiimoteButton button, bool down) {
		const char* buttonName = WiimoteButtons::toString(button);
		if (buttonName) {
			std::cout << buttonName << down ? " pressed" : " released" << std::endl;
		}
	});

	wiimote->onNunchuckButton([wiimote](NunchuckButton button, bool down) {
		const char* buttonName = NunchuckButtons::toString(button);
		if (buttonName) {
			std::cout << buttonName << down ? " pressed" : " released" << std::endl;
		}
	});

	wiimote->onAccelerometer([wiimote](const Vector3& v) {
		std::cout << "Accel: " << v.x << ", " << v.y << ", " << v.z << std::endl;
	}, true);

	
	wiimote->onNunchuckAccelerometer([wiimote](const Vector3& v) {
		std::cout << "Nunchuck Accel: " << v.x << ", " << v.y << ", " << v.z << std::endl;
	}, true);

	wiimote->onMotionPlus([wiimote](const Vector3& v) {
		std::cout << "Motion Plus: " << v.x << ", " << v.y << ", " << v.z << std::endl;
	}, true);

	wiimote->start(WiimoteFeatures::Buttons | WiimoteFeatures::Accelerometer | WiimoteFeatures::Extension);
});

m.onDisconnection([](Wiimote* wiimote) {
	std::cout << "Wiimote disconnected" << std::endl;
	wiimote->clearEvents();
});

m.continuousScan();

while (true) {
	m.update();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
```

## Contact
Author: Tom Yaxley / _tommtiytom at gmail dot com_

Contributions, pull requests, bug reports and issues are welcome!

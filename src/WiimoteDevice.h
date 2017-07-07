#pragma once

#ifdef _WIN32
#include "WiimoteDeviceWindows.h"
typedef WiimoteDeviceWindows WiimoteDevice;
#else
#include "WiimoteDevicePosix.h"
typedef WiimoteDevicePosix WiimoteDevice;
#endif

typedef std::vector<WiimoteDevice*> WiimoteDeviceVector;
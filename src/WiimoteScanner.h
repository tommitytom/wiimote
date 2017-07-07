#pragma once

#ifdef _WIN32
#include "WiimoteScannerWin.h"
typedef WiimoteScannerWindows WiimoteScanner;
#else
#include "WiimoteScannerPosix.h"
#endif
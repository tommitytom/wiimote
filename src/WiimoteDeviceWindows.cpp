#include "WiimoteDeviceWindows.h"

#include <assert.h>

std::string GetLastErrorAsString()
{
	//Get the error message, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}

WiimoteDeviceWindows::WiimoteDeviceWindows(HANDLE deviceHandle, bool useOutputReportSize)
	:_useOutputReportSize(useOutputReportSize), _deviceHandle(deviceHandle), _outputReportSize(0)
{
	ZeroMemory(&_readIo, sizeof(_readIo));

	PHIDP_PREPARSED_DATA preparsedData = NULL;
	HIDP_CAPS caps;
	BOOLEAN result;
	NTSTATUS status;

	result = HidD_GetPreparsedData(_deviceHandle, &preparsedData);
	if (!result)
	{
		std::cout << "GetPreparsedData Failed!" << std::endl;
		return;
	}

	status = HidP_GetCaps(preparsedData, &caps);
	if (status < 0)
	{
		std::cout << "GetPreparsedData Failed!" << std::endl;
		HidD_FreePreparsedData(preparsedData);
		return;
	}

	_outputReportSize = caps.OutputReportByteLength;
	_inputReportSize = caps.InputReportByteLength;

	HidD_FreePreparsedData(preparsedData);

	_valid = true;
}

WiimoteDeviceWindows::~WiimoteDeviceWindows()
{
	if (_deviceHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_deviceHandle);
		_deviceHandle = nullptr;
	}
}

bool WiimoteDeviceWindows::setup()
{
	

	return true;
}

bool WiimoteDeviceWindows::read(u8* target, size_t size, size_t& bytesRead)
{
	DWORD amount;
	BOOL result;
	
	bytesRead = 0;
	result = ReadFile(_deviceHandle, target, (DWORD)size, (LPDWORD)&amount, &_readIo);
	if (!result)
	{
		DWORD error = GetLastError();
		if (error != ERROR_IO_PENDING)
		{
			std::cout << "Read Failed: " << std::hex << error << std::endl;
			return false;
		}

		if (!GetOverlappedResult(_deviceHandle, &_readIo, (LPDWORD)&amount, TRUE))
		{
			error = GetLastError();
			std::cout << "Read Failed: " << GetLastErrorAsString() << std::endl;
			return false;
		}

		if (_readIo.Internal == STATUS_PENDING)
		{
			std::cout << "Read Interrupted" << std::endl;
			if (!CancelIo(_deviceHandle))
			{
				error = GetLastError();
				std::cout << "Cancel IO Failed: " << std::hex << error << std::endl;
			}

			return false;
		}
	}

	bytesRead = amount;

	return true;
}

bool WiimoteDeviceWindows::write(u8* source, size_t size)
{
	DWORD bytesWritten;
	OVERLAPPED overlapped;
	ZeroMemory(&overlapped, sizeof(overlapped));

	if (_useOutputReportSize && (size < (size_t)_outputReportSize))
	{
		size = _outputReportSize;
	}

	BOOL result = WriteFile(_deviceHandle, source, (DWORD)size, &bytesWritten, &overlapped);
	if (!result)
	{
		DWORD error = GetLastError();

		if (error == ERROR_INVALID_USER_BUFFER)
		{
			std::cout << "Falling back to SetOutputReport" << std::endl;
			_connected = writeFallback(source, size);
			return _connected;
		}

		if (error != ERROR_IO_PENDING)
		{
			std::cout << "Write Failed: " << std::hex << GetLastErrorAsString() << std::endl;
			_connected = false;
			return false;
		}
	}

	if (!GetOverlappedResult(_deviceHandle, &overlapped, &bytesWritten, TRUE))
	{
		DWORD error = GetLastError();
		if (error != ERROR_GEN_FAILURE)
		{
			std::cout << "Write Failed: " << GetLastErrorAsString() << std::endl;
		}

		_connected = false;
		return false;
	}

	_connected = true;
	return true;
}

bool WiimoteDeviceWindows::writeFallback(u8* source, size_t size)
{
	BOOL result = HidD_SetOutputReport(_deviceHandle, source, (ULONG)size);
	if (!result)
	{
		DWORD error = GetLastError();
		std::cout << "SetOutputReport Failed: " << std::hex << error << std::endl;
		return false;
	}

	return true;
}



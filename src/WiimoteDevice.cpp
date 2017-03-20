#include "WiimoteDevice.h"

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

DWORD WINAPI WiimoteStart(_In_ LPVOID lpParameter)
{
	WiimoteDeviceWindows* Device = static_cast<WiimoteDeviceWindows*>(lpParameter);
	Device->continuousReader();

	return 0;
}



WiimoteDeviceWindows::WiimoteDeviceWindows(HANDLE _deviceHandle, bool _useOutputReportSize)
	:_useOutputReportSize(_useOutputReportSize), _deviceHandle(_deviceHandle), _readThread(NULL), _run(FALSE), _outputReportSize(0)
{
	ZeroMemory(&_readIo, sizeof(_readIo));
}

WiimoteDeviceWindows::~WiimoteDeviceWindows()
{
	stopReader();

	if (_deviceHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_deviceHandle);
		_deviceHandle = nullptr;
	}
}

bool WiimoteDeviceWindows::setup()
{
	PHIDP_PREPARSED_DATA preparsedData = NULL;
	HIDP_CAPS caps;
	BOOLEAN result;
	NTSTATUS status;

	result = HidD_GetPreparsedData(_deviceHandle, &preparsedData);
	if (!result)
	{
		std::cout << "GetPreparsedData Failed!" << std::endl;
		return false;
	}

	status = HidP_GetCaps(preparsedData, &caps);
	if (status < 0)
	{
		std::cout << "GetPreparsedData Failed!" << std::endl;
		HidD_FreePreparsedData(preparsedData);
		return false;
	}

	_outputReportSize = caps.OutputReportByteLength;
	_inputReportSize = caps.InputReportByteLength;

	HidD_FreePreparsedData(preparsedData);

	return true;
}

void WiimoteDeviceWindows::startReader()
{
	_run = true;
	_readIo.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_readThread = CreateThread(NULL, 0, WiimoteStart, this, 0, NULL);
}

void WiimoteDeviceWindows::stopReader()
{
	if (_run)
	{
		_run = false;
		do {
			SetEvent(_readIo.hEvent);
		} while (WaitForSingleObject(_readThread, 100) == WAIT_TIMEOUT);
	}

	if (_readIo.hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_readIo.hEvent);
		_readIo.hEvent = INVALID_HANDLE_VALUE;
	}
}

void WiimoteDeviceWindows::continuousReader()
{
	UCHAR Buffer[255];
	DWORD BytesRead;

	while(_run)
	{
		ZeroMemory(Buffer, sizeof(Buffer));
		ResetEvent(_readIo.hEvent);
		BytesRead = 0;
		BOOL Result = ReadFile(_deviceHandle, &Buffer, sizeof(Buffer), &BytesRead, &_readIo);
		if (!Result)
		{
			DWORD Error = GetLastError();
			if (Error != ERROR_IO_PENDING)
			{
				std::cout << "Read Failed: " << std::hex << Error << std::endl;
				continue;
			}
			else
			{
				if (!GetOverlappedResult(_deviceHandle, &_readIo, &BytesRead, TRUE))
				{
					Error = GetLastError();
					std::cout << "Read Failed: " << GetLastErrorAsString() << std::endl;
					continue;
				}

				if (_readIo.Internal == STATUS_PENDING)
				{
					std::cout << "Read Interrupted" << std::endl;
					if (!CancelIo(_deviceHandle))
					{
						Error = GetLastError();
						std::cout << "Cancel IO Failed: " << std::hex << Error << std::endl;
					}

					continue;
				}
			}
		}

		assert(BytesRead % _inputReportSize == 0);

		for (size_t i = 0; i < BytesRead / _inputReportSize; i++)
		{
			unsigned char* b = Buffer + i * _inputReportSize;
			_callback(b);
		}
	}
}

bool WiimoteDeviceWindows::write(DataBuffer& buffer)
{
	DWORD bytesWritten;
	OVERLAPPED overlapped;
	ZeroMemory(&overlapped, sizeof(overlapped));

	if (_useOutputReportSize && (buffer.size() < (size_t)_outputReportSize))
	{
		std::cout << "Resizing Buffer" << std::endl;
		buffer.resize(_outputReportSize, 0);
	}

	BOOL Result = WriteFile(_deviceHandle, buffer.data(), (DWORD)buffer.size(), &bytesWritten, &overlapped);
	if (!Result)
	{
		DWORD Error = GetLastError();

		if (Error == ERROR_INVALID_USER_BUFFER)
		{
			std::cout << "Falling back to SetOutputReport" << std::endl;
			_connected = writeFallback(buffer);
			return _connected;
		}
		
		if (Error != ERROR_IO_PENDING)
		{
			std::cout << "Write Failed: " << std::hex << GetLastErrorAsString() << std::endl;
			_connected = false;
			return false;
		}
	}

	if (!GetOverlappedResult(_deviceHandle, &overlapped, &bytesWritten, TRUE))
	{
		DWORD Error = GetLastError();
		if (Error != ERROR_GEN_FAILURE)
		{
			std::cout << "Write Failed: " << GetLastErrorAsString() << std::endl;
		}
		
		_connected = false;
		return false;
	}

	_connected = true;
	return true;
}

bool WiimoteDeviceWindows::writeFallback(DataBuffer & Buffer)
{
	BOOL Result = HidD_SetOutputReport(_deviceHandle, Buffer.data(), (ULONG)Buffer.size());
	if (!Result)
	{
		DWORD Error = GetLastError();
		std::cout << "SetOutputReport Failed: " << std::hex << Error << std::endl;
		return false;
	}

	return true;
}



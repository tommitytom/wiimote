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
	:_useOutputReportSize(_useOutputReportSize), _deviceHandle(_deviceHandle), _readThread(NULL), _run(FALSE), _outputReportMinSize(0)
{
	ZeroMemory(&_readIo, sizeof(_readIo));
}

WiimoteDeviceWindows::~WiimoteDeviceWindows()
{

}

bool WiimoteDeviceWindows::setup()
{
	PHIDP_PREPARSED_DATA PreparsedData = NULL;
	HIDP_CAPS Caps;
	BOOLEAN Result;
	NTSTATUS Status;

	//std::cout << "Setting up: \t" << DeviceHandle << std::endl;
	//std::cout << "Resize Output: \t" << (UseOutputReportSize ? "Yes" : "No") << std::endl;

	Result = HidD_GetPreparsedData(_deviceHandle, &PreparsedData);
	if (!Result)
	{
		std::cout << "GetPreparsedData Failed!" << std::endl;
		return false;
	}

	Status = HidP_GetCaps(PreparsedData, &Caps);
	if (Status < 0)
	{
		std::cout << "GetPreparsedData Failed!" << std::endl;
		HidD_FreePreparsedData(PreparsedData);
		return false;
	}

	/*std::cout << std::dec;

	std::cout << "\tUsage: " << Caps.Usage << std::endl;
	std::cout << "\tUsagePage: " << Caps.UsagePage << std::endl;
	std::cout << "\tInputReportByteLength: " << Caps.InputReportByteLength << std::endl;
	std::cout << "\tOutputReportByteLength: " << Caps.OutputReportByteLength << std::endl;
	std::cout << "\tFeatureReportByteLength: " << Caps.FeatureReportByteLength << std::endl;*/

	_outputReportMinSize = Caps.OutputReportByteLength;

	HidD_FreePreparsedData(PreparsedData);

	return true;
}

void WiimoteDeviceWindows::disconnect()
{
	if (_run)
	{
		_run = false;
		do {
			SetEvent(_readIo.hEvent);
		} while (WaitForSingleObject(_readThread, 100) == WAIT_TIMEOUT);
	}
	
	if (_deviceHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_deviceHandle);
		_deviceHandle = INVALID_HANDLE_VALUE;
	}
	
	if (_readIo.hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_readIo.hEvent);
		_readIo.hEvent= INVALID_HANDLE_VALUE;
	}
}

void WiimoteDeviceWindows::startReader()
{
	_run = true;
	_readIo.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_readThread = CreateThread(NULL, 0, WiimoteStart, this, 0, NULL);
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
					std::cout << "Read Failed: " << std::hex << Error << std::endl;
					continue;
				}

				if(_readIo.Internal == STATUS_PENDING)
				{
					std::cout << "Read Interrupted" << std::endl;
					if(!CancelIo(_deviceHandle))
					{
						Error = GetLastError();
						std::cout << "Cancel IO Failed: " << std::hex << Error << std::endl;
					}
					continue;
				}
			}
		}

		#define INPUT_REPORT_SIZE 22

		assert(BytesRead % INPUT_REPORT_SIZE == 0);

		//std::cout << "Read  " << std::dec << BytesRead << " Bytes from " << "0x" << std::hex << DeviceHandle << " : ";
		for (size_t i = 0; i < BytesRead / INPUT_REPORT_SIZE; i++)
		{
			unsigned char* b = Buffer + i * INPUT_REPORT_SIZE;
			_callback(b);
		}

		//std::cout << std::endl;
	}
}

bool WiimoteDeviceWindows::write(DataBuffer & Buffer)
{
	DWORD BytesWritten;
	OVERLAPPED Overlapped;
	ZeroMemory(&Overlapped, sizeof(Overlapped));
	//std::cout << "0x" << std::hex << DeviceHandle << std::endl;

	if ((_useOutputReportSize) && (Buffer.size() < (size_t)_outputReportMinSize))
	{
		std::cout << "Resizing Buffer" << std::endl;
		Buffer.resize(_outputReportMinSize, 0);
	}

	BOOL Result = WriteFile(_deviceHandle, Buffer.data(), (DWORD)Buffer.size(), &BytesWritten, &Overlapped);
	if (!Result)
	{
		DWORD Error = GetLastError();

		if (Error == ERROR_INVALID_USER_BUFFER)
		{
			std::cout << "Falling back to SetOutputReport" << std::endl;
			return writeFallback(Buffer);
		}
		
		if (Error != ERROR_IO_PENDING)
		{
			std::cout << "Write Failed: " << std::hex << Error << std::endl;
			return false;
		}
	}

	if (!GetOverlappedResult(_deviceHandle, &Overlapped, &BytesWritten, TRUE))
	{
		DWORD Error = GetLastError();
		if (Error != ERROR_GEN_FAILURE)
		{
			std::cout << "Write Failed: " << GetLastErrorAsString() << std::endl;
		}
		
		return false;
	}

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



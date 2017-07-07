#include "WiimoteManager.h"

#include <chrono>

WiimoteManager::WiimoteManager()
{
	
}

WiimoteManager::~WiimoteManager() 
{
	shutdown();
}

void WiimoteManager::scan()
{
	ScanResults added;
	ScanResults removed;
	_scanner.scan(added, removed);

	for (ScanResults::iterator it = removed.begin(); it != removed.end(); ++it)
	{
		auto found = _state.found.find(it->first);
		if (found != _state.found.end())
		{
			_state.found.erase(found);

			_scanLock.lock();
			_state.removed.push_back(found->second);
			_scanLock.unlock();
		}
	}

	for (ScanResults::iterator it = added.begin(); it != added.end(); ++it)
	{
		WiimoteDevice* dev = new WiimoteDevice(it->second.handle, it->second.useOutputSize);
		if (dev->setup())
		{
			Wiimote* wiimote = new Wiimote(dev);
			_state.found[it->first] = wiimote;
		} else {
			delete dev;
		}
	}

	for (WiimoteMap::iterator it = _state.found.begin(); it != _state.found.end(); ++it)
	{
		Wiimote* wiimote = it->second;

		if (!wiimote->_device->connected())
		{
			LedState ledState = LedStates::Off;
			switch (_nextId % 4)
			{
				case 0: ledState = LedStates::One;		break;
				case 1: ledState = LedStates::Two;		break;
				case 2: ledState = LedStates::Three;	break;
				case 3: ledState = LedStates::Four;		break;
			}

			u8 b[2] = { OutputReportType::LEDs, ledState };
			if (wiimote->_device->write(b, 2))
			{
				if (wiimote->id() == -1) wiimote->setID(_nextId++);
				_scanLock.lock();
				_state.added.push_back(wiimote);
				_scanLock.unlock();
			}
		}
	}
}

void WiimoteManager::continuousScan(int interval)
{
	_interval = interval;
	_scanThread = std::thread(&WiimoteManager::continuousScanner, this);
}

void WiimoteManager::stop()
{
	_run = false;
}

void WiimoteManager::update()
{
	_scanLock.lock();

	for (size_t i = 0; i < _state.removed.size(); ++i)
	{
		Wiimote* removed = _state.removed[i];
		for (size_t j = 0; j < _state.connected.size(); ++j)
		{
			if (_state.connected[j] == removed)
			{
				_state.connected.erase(_state.connected.begin() + j);
				break;
			}
		}

		if (_disconnectionCallback != nullptr) _disconnectionCallback(removed);
	}

	for (size_t i = 0; i < _state.added.size(); ++i)
	{
		Wiimote* added = _state.added[i];
		_state.connected.push_back(added);
		if (_connectionCallback != nullptr) _connectionCallback(added);
	}

	_state.removed.clear();
	_state.added.clear();

	_scanLock.unlock();
	
	for (Wiimote* wiimote : _state.connected)
	{
		wiimote->update();
	}
}

void WiimoteManager::shutdown()
{
	stop();

	while (_run)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	for (WiimoteMap::iterator it = _state.found.begin(); it != _state.found.end(); ++it)
	{
		delete it->second;
	}
}

void WiimoteManager::continuousScanner()
{
	size_t sleepTime = 20;
	size_t itrMax = _interval / sleepTime;
	size_t itr = 0;
	_run = true;

	while (_run)
	{
		if (itr++ % itrMax != 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
			continue;
		}

		scan();
	}
}
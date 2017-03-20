#pragma once

#include "Wiimote.h"
#include "WiimoteScanner.h"

#include <thread>

typedef std::map<XXH64_hash_t, Wiimote*> WiimoteMap;
typedef std::vector<Wiimote*> WiimoteVector;

struct WiimoteManagerState
{
	WiimoteMap found;
	WiimoteVector connected;

	WiimoteVector added;
	WiimoteVector removed;
};

class WiimoteManager
{
private:
	WiimoteScanner _scanner;
	WiimoteManagerState _state;

	std::thread _scanThread;
	std::mutex _scanLock;

	std::function<void(Wiimote*)> _connectionCallback;
	std::function<void(Wiimote*)> _disconnectionCallback;

	bool _run = false;

public:
	WiimoteManager();
	~WiimoteManager();

	void scan();
	
	void start();
	void stop();
	void update();
	void shutdown();

	const WiimoteVector& connected() const { return _state.connected; }

	void onConnection(std::function<void(Wiimote*)> callback) { _connectionCallback = callback; }
	void onDisconnection(std::function<void(Wiimote*)> callback) { _disconnectionCallback = callback; }

private:
	void continuousScanner();
};
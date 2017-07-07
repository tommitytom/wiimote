#include "WiimoteManager.h"

int main()
{
	WiimoteManager m;
	m.onConnection([](Wiimote* wiimote)
	{ 
		std::cout << "Wiimote connected!" << std::endl;

		wiimote->onButton([wiimote](WiimoteButton button, bool down) {
			if (button == WiimoteButtons::A && down)
			{
				if (!wiimote->getState().motionPlusActive)
				{
					wiimote->activateMotionPlus();
				} else {
					wiimote->deactivateMotionPlus();
				}
			}

			if (button == WiimoteButtons::One)
			{
				if (down)
				{
					std::cout << "One pressed!" << std::endl;
				} else {
					std::cout << "One released!" << std::endl;

				}
			}
		});

		wiimote->start(WiimoteFeatures::NoIR);
	});

	m.onDisconnection([](Wiimote* wiimote)
	{
		std::cout << "Wiimote disconnected" << std::endl;
	});

	m.continuousScan();

	int i = 0;
	WiimoteState state[2];
	while (true)
	{
		m.update();

		LedState ledState = LedStates::One;
		switch (i)
		{
			case 0: ledState = LedStates::One;		break;
			case 1: ledState = LedStates::Two;		break;
			case 2: ledState = LedStates::Three;	break;
			case 3: ledState = LedStates::Four;		break;
		}

		for (Wiimote* wiimote : m.connected())
		{
			wiimote->setLeds(ledState);
		}

		if (++i == 4) i = 0;
		Sleep(50);
	}

	system("pause");
	std::cout << "Exiting!" << std::endl;

	system("pause");

    return 0;
}


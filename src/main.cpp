#include "WiimoteFactory.h"

int main()
{
	WiimoteFactory factory;
	WiimoteVector wiimotes = factory.getWiimotes();
	
	Wiimote* connected = nullptr;
	for (Wiimote* wiimote : wiimotes)
	{
		if (wiimote->available())
		{
			connected = wiimote;
			wiimote->start(ReportModes::Buttons);
			//wiimote.setLeds(LedState::One);
		}
	}

	bool wmpactive = false;

	int i = 0;
	WiimoteState state[2];
	while (true)
	{
		WiimoteState& s = state[i % 2];
		WiimoteState& p = state[(i + 1) % 2];

		connected->getState(s);
		
		if (s.buttons[WiimoteButtons::A] && s.buttons[WiimoteButtons::A] != p.buttons[WiimoteButtons::A])
		{
			wmpactive = !s.motionPlusActive;
			if (wmpactive)
			{
				connected->activateMotionPlus();
			} else {
				connected->deactivateMotionPlus();
			}
		}

		LedState ledState = LedStates::One;
		switch (i)
		{
			case 0: ledState = LedStates::One;		break;
			case 1: ledState = LedStates::Two;		break;
			case 2: ledState = LedStates::Three;	break;
			case 3: ledState = LedStates::Four;		break;
		}

		//connected->setLeds(ledState);

		if (++i == 4) i = 0;
		Sleep(50);
	}

	system("pause");
	std::cout << "Exiting!" << std::endl;

	for (Wiimote* wiimote : wiimotes)
	{
		wiimote->stop();
		delete wiimote;
	}

	system("pause");

    return 0;
}


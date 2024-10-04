#include "hack.h"
#include "hack_gui.h"
#include "kx_status.h"
#include <iostream>

void consoleStatusCallback(const std::string& message) {
	system("cls");
	std::cout << message << std::endl;
}

int main()
{
	KXStatus status;
	if (!status.CheckStatus())
	{
		system("PAUSE");
		return 0;
	}

	Hack hack(consoleStatusCallback);
	HackGUI gui(hack);
	gui.start();
	gui.run();

	return  0;
}

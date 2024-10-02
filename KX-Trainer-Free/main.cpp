#include "hack.h"
#include "hack_gui.h"
#include "kx_status.h"

int main()
{
	KXStatus status;
	if (!status.CheckStatus())
	{
		system("PAUSE");
		return 0;
	}

	Hack hack;
	HackGUI gui(hack);
	gui.start();
	gui.run();

	return  0;
}

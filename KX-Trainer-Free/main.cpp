#include "hack.h"
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
	hack.find_process();
	hack.base_scan();
	hack.start();
	hack.hacks_loop();

	return  0;
}

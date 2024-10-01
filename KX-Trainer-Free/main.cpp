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
	hack.start();
	hack.run();

	return  0;
}

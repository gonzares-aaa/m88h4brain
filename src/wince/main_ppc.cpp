/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		PocketPC ÉÅÉCÉìé¿ëï
===========================================================================*/

#include "m88p.h"

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPTSTR cmdline, int nwinmode)
{
PpcUI ui(hinst);

	return M88ceMain( &ui, nwinmode );
}

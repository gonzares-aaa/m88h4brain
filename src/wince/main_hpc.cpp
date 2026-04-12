/*===========================================================================
		M88 for HandheldPC
		Original Version (c)cisc
		Handheld PC Version Programmed by Y.Taniuchi ( TAN-Y )

		HandheldPC ÉÅÉCÉìé¿ëï
===========================================================================*/

#include "m88h.h"

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPTSTR cmdline, int nwinmode)
{
HpcUI ui(hinst);

	return M88ceMain( &ui, nwinmode );
}

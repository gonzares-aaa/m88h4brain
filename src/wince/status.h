/*===========================================================================
		M88 for PocketPC
		Original Version (c)cisc
		Pocket PC Version Programmed by Y.Taniuchi ( TAN-Y )

		StatusDisplay ƒNƒ‰ƒXŽÀ‘•
===========================================================================*/

#ifndef win32_status_h
#define win32_status_h

class StatusDisplay
{
public:
	StatusDisplay();
	~StatusDisplay();


	bool Show(int priority, int duration, char* msg, ...);
	void FDAccess(uint dr, bool hd, bool active);
	void UpdateDisplay();
	void WaitSubSys();
};

extern StatusDisplay statusdisplay;

#endif // win32_status_h

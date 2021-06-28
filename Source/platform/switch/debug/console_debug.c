#include <switch/runtime/devices/console.h>

void enableDebugLog(void)
{
	consoleDebugInit(debugDevice_SVC);
}

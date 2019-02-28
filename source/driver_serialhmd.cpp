// WINDOWS COMMANDS:
// BOTH
// g++ -c source/*.cpp -g -Wall -Werror -Iheaders -lsetupapi -std=c++11
// g++ -shared *.o -o out/driver_serialhmd.dll
// OR ONLY
// g++ --shared source/*.cpp -o out/driver_serialhmd.dll -g -Wall -Werror -Iheaders -lsetupapi -std=c++11

// LINUX COMMANDS:
//
//

#include <openvr_driver.h>
//#include "driverlog.h"

#include <vector>

#include <SerialHMDShared.h>
#include <SerialHMDDriver.h>
#include <SerialHMDServer.h>
#include <SerialHMDWatchdog.h>

using namespace vr;

// *******************************************************
// Platform Dependencies
// *******************************************************

#if defined(_WIN32)
// Windows

#include <windows.h>

#define HMD_DLL_EXPORT extern "C" __declspec(dllexport)
#define HMD_DLL_IMPORT extern "C" __declspec(dllimport)

#elif defined(__GNUC__) || defined(COMPILER_GCC) || defined(__APPLE__)
// UNIX

#define HMD_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define HMD_DLL_IMPORT extern "C"

#define __stdcall
#define __out
#define __in
#define BYTE unsigned char
#define WORD unsigned short
#define SHORT short
#define DWORD unsigned int
#define HMODULE void *
#define strcmp strcasecmp

#else
#error "Unsupported Platform."
#endif


SerialHMDWatchdog g_watchdog;
SerialHMDServer g_server;


// *******************************************************
// Creates instances of the other driver parts on request
// First watchdog, and once watchdog recognizes an HMD, the server
// *******************************************************

HMD_DLL_EXPORT void *HmdDriverFactory(const char *pInterfaceName, int *pReturnCode)
{
	if (0 == strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName))
	{
		return &g_server;
	}
	if (0 == strcmp(IVRWatchdogProvider_Version, pInterfaceName))
	{
		return &g_watchdog;
	}

	if (pReturnCode)
		*pReturnCode = VRInitError_Init_InterfaceNotFound;

	return NULL;
}

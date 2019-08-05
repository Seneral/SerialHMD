// ON WINDOWS, compile with MSVC (e.g. Visual Studio) - gcc or g++ won't work since they produce libraries which Steam can't interact with properly!!
// ON LINUX - no tested. I did start off with Linux though, so atleast the base should throw no errors, and I might try again once there are VR games on Linux!

#include <openvr_driver.h>

#include <SerialHMDShared.h>
#include <SerialHMDDriver.h>
#include <SerialHMDServer.h>
#include <SerialHMDWatchdog.h>

#include <vector>

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


// *******************************************************
// Creates instances of the other driver parts on request
// First watchdog, and once watchdog recognizes an HMD, the server
// *******************************************************

SerialHMDServer g_hmdServer;
SerialHMDWatchdog g_hmdWatchdog;

HMD_DLL_EXPORT void *HmdDriverFactory(const char *pInterfaceName, int *pReturnCode)
{
	if (strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName) == 0)
	{
		return &g_hmdServer;
	}
	if (strcmp(IVRWatchdogProvider_Version, pInterfaceName) == 0)
	{
		return &g_hmdWatchdog;
	}

	if (pReturnCode)
		*pReturnCode = VRInitError_Init_InterfaceNotFound;

	return NULL;
}

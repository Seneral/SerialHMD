
#ifndef HMDWATCHDOG_H
#define HMDWATCHDOG_H

#include <SerialHMDShared.h>

#include <WinSerialDeviceUtility.h>
#include <SerialPort.h>

#include <stdio.h>
#include <stdlib.h>

#include <thread>
#include <chrono>

// *******************************************************
// Watchdog frequently checking if HMD has been connected
// *******************************************************

class SerialHMDWatchdog : public IVRWatchdogProvider 
{
  public:
	SerialHMDWatchdog()
	{
		m_pWatchdogThread = nullptr;
	}

	virtual EVRInitError Init(vr::IVRDriverContext *pDriverContext);
	virtual void Cleanup();

  private:
	std::thread *m_pWatchdogThread;
};

#endif // HMDWATCHDOG_H
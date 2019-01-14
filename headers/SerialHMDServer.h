
#ifndef HMDSERVER_H
#define HMDSERVER_H

#include <SerialHMDShared.h>
#include <SerialHMDDriver.h>

// *******************************************************
// Manager/Server of Tracked Devices
// *******************************************************

class SerialHMDServer : public IServerTrackedDeviceProvider
{
  public:
	virtual EVRInitError Init(vr::IVRDriverContext *pDriverContext);
	virtual void Cleanup();
	virtual const char *const *GetInterfaceVersions() { return vr::k_InterfaceVersions; }
	virtual void RunFrame();
	virtual bool ShouldBlockStandbyMode() { return false; }
	virtual void EnterStandby() {}
	virtual void LeaveStandby() {}

  private:
	SerialHMDDriver *m_hmdDriver = nullptr;
	//CSampleControllerDriver *m_pController = nullptr;
};

#endif // HMDSERVER_H
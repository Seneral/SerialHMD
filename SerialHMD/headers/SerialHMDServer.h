
#ifndef HMDSERVER_H
#define HMDSERVER_H

#include <SerialHMDShared.h>
#include <SerialHMDDriver.h>
#include <SerialHMDComm.h>
#include <vector>

// *******************************************************
// Manager/Server of Tracked Devices
// *******************************************************

class SerialHMDServer : public IServerTrackedDeviceProvider
{
  public:
	SerialHMDDriver *m_hmdDriver = nullptr;
	//CSampleControllerDriver *m_pController = nullptr;
	SerialHMDComm *m_hmdComm = nullptr;
	std::vector<ICommStreamReceiver *> m_streamReceiver;

	virtual EVRInitError Init(vr::IVRDriverContext *pDriverContext);
	virtual void Cleanup();
	virtual const char *const *GetInterfaceVersions() { return vr::k_InterfaceVersions; }
	virtual void RunFrame();
	virtual bool ShouldBlockStandbyMode();
	virtual void EnterStandby();
	virtual void LeaveStandby();
};


// *******************************************************
// Local Static Communication Thread Callback Functions
// *******************************************************

void RequestResponse(char ID, std::vector<unsigned char> response);
void RequestTimeout(char ID);
void StreamPacket(char channel, std::vector<unsigned char> data);
void ConnectionLost();

#endif // HMDSERVER_H
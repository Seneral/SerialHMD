#include <SerialHMDServer.h>
#include <DriverLog.h>

// *******************************************************
// Manager/Server of Tracked Devices
// *******************************************************

EVRInitError SerialHMDServer::Init(vr::IVRDriverContext *pDriverContext)
{
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());
	DriverLog("Server: Init \n");

	// Create HMD Driver Instance and record it as a tracked device
	m_hmdDriver = new SerialHMDDriver();
	vr::VRServerDriverHost()->TrackedDeviceAdded(m_hmdDriver->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD, m_hmdDriver);

	// Add future tracked devices here

	return VRInitError_None;
}

void SerialHMDServer::Cleanup()
{
	DriverLog("Server: Cleanup \n");
	CleanupDriverLog();

	delete m_hmdDriver;
	m_hmdDriver = NULL;

	// Delete future other drivers here
}

void SerialHMDServer::RunFrame()
{
	// TODO: Replace with thread
	//if (m_hmdDriver)
	//	m_hmdDriver->RunFrame();

	/* Add future input devices here

	if (m_pController)
		m_pController->RunFrame();
	vr::VREvent_t vrEvent;
	while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent)))
	{
		if (m_pController)
			m_pController->ProcessEvent(vrEvent);
	}*/

	vr::VREvent_t vrEvent;
	while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent))) {}
}

bool ShouldBlockStandbyMode()
{
	DriverLog("Server: Should Block Standby Mode: False \n");
	return false; 
}
void EnterStandby() 
{
	DriverLog("Server: Standby Mode Enter \n");
}
void LeaveStandby() 
{
	DriverLog("Server: Standby Mode Leave \n");
}
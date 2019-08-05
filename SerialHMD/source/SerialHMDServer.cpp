#include <SerialHMDServer.h>
#include <DriverLog.h>

SerialHMDServer *g_hmdServer = nullptr;

// *******************************************************
// Manager/Server of Tracked Devices
// *******************************************************

EVRInitError SerialHMDServer::Init(vr::IVRDriverContext *pDriverContext)
{
	// Initiate context and driver log
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());
	DriverLog("Server: Init \n");

	// Try to opening a serial connection to the SerialHMD
	SerialPort *serialHMDPort = OpenSerialHMDPort();
	if (!serialHMDPort)
	{ // Failed to find or identify SerialHMD
		DriverLog("Server: Failed to find / identify SerialHMD! \n");
		return VRInitError_Driver_Failed;
	}

	// TODO: Check if the HMD display is found
	// return VRInitError_Driver_HMDDisplayNotFound;

	// Create and add HMD (by default not yet connected)
	m_hmdDriver = new SerialHMDDriver();
	vr::VRServerDriverHost()->TrackedDeviceAdded(m_hmdDriver->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD, m_hmdDriver);

	// Setup communication channel including response handling
	g_hmdServer = this;
	m_hmdComm = new SerialHMDComm(serialHMDPort);
	m_hmdComm->RequestResponse = RequestResponse;
	m_hmdComm->RequestTimeout = RequestTimeout;
	m_hmdComm->StreamPacket = StreamPacket;
	m_hmdComm->ConnectionLost = ConnectionLost;

	// Initiate communication and request available sources (HMD, Controllers, Skeleton, etc.)
	m_hmdComm->SendRequest('R', 1000);		 // Reset Request
	m_hmdComm->SendRequest('I', 1000);		 // Identification Request
	m_hmdComm->SendRequest('S', 1000);		 // Available sources requested
	
	// Start communication thread which will receive and handle responses with the given callbacks
	m_hmdComm->StartCommThread();

	return VRInitError_None;
}

void SerialHMDServer::Cleanup()
{
	DriverLog("Server: Cleanup \n");
	CleanupDriverLog();

	if (m_hmdComm)
	{ // Stop communication thread and send a final reset request to SerialHMD
		m_hmdComm->StopCommThread();
		m_hmdComm->SendRequest('R', 1000);
		delete m_hmdComm;
	}
	m_hmdComm = NULL;

	if (m_hmdDriver)
		delete m_hmdDriver;
	m_hmdDriver = NULL;
}

void SerialHMDServer::RunFrame()
{
	if (m_hmdDriver)
	{
		if ((GetKeyState(VK_NUMPAD5) & 0x8000) != 0 || (GetKeyState(VK_TAB) & 0x8000) != 0 || (GetKeyState(0x38) & 0x8000) != 0)
			m_hmdDriver->SetRestPose(true, true);
	}
	/*vr::VREvent_t vrEvent;
	while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent)))
	{ // Handle events like vibration
		//if (m_pController)
		//	m_pController->ProcessEvent(vrEvent);
	}*/
}

bool SerialHMDServer::ShouldBlockStandbyMode()
{
	DriverLog("Server: Should Block Standby Mode: False \n");
	return false; 
}
void SerialHMDServer::EnterStandby()
{
	DriverLog("Server: Standby Mode Enter \n");
}
void SerialHMDServer::LeaveStandby()
{
	DriverLog("Server: Standby Mode Leave \n");
}


// *******************************************************
// Local Static Communication Thread Callback Functions
// *******************************************************

void RequestResponse(char ID, std::vector<unsigned char> data)
{
	if (ID == 'I')
	{ // Identification - does not matter since it has been verified BEFORE starting communication
		char *name = reinterpret_cast<char *>(data.data());
		if (strstr(name, "SerialHMD"))
			DriverLog("Server: Successfully verified identity of SerialHMD! \n");
		else
			DriverLog("Server: ERROR: Failed verify identity of SerialHMD! \n");
	}
	else if (ID == 'S')
	{ // Available source confirmed, request open channel
		for (unsigned int i = 0; i < data.size(); i++)
		{ // Could be multiple sources available

			// Get source type
			char type = data[i];
			if (type != 'H' && type != 'C' && type != 'S')
			{
				DriverLog("Server: Unknown source type '%c'! \n", type);
				continue;
			}

			// Find free channel
			std::vector<bool> flags(17);
			for (unsigned int i = 0; i < g_hmdServer->m_streamReceiver.size(); i++)
				flags[g_hmdServer->m_streamReceiver[i]->streamChannel - 1] = true;
			int freeChannel = static_cast<int>(std::find(flags.begin(), flags.end(), false) - flags.begin()) + 1;

			if (freeChannel > 0 && freeChannel <= 16)
			{ // Send request to assign given source to free channel
				DriverLog("Server: Received available stream '%c', assigning to channel '%d'! \n", type, freeChannel);
				g_hmdServer->m_hmdComm->SendRequestStream(type, freeChannel, 1000);
			}
		}
	}
	else if (ID == 'H' || ID == 'C' || ID == 'S')
	{ // Channel open request for source has been accepted
		if (data.size() != 1)
		{ // Not a singular channel
			DriverLog("Server: Stream channel '%c' opening response corrupted! \n", ID);
			return;
		}
		int channel = static_cast<int>(data.front());
		if (channel <= 0 || channel > 16)
		{ // Invalid channel range
			DriverLog("Server: Stream channel '%c' opening response with invalid channel! \n", channel);
			return;
		}
		for (unsigned int i = 0; i < g_hmdServer->m_streamReceiver.size(); i++)
		{ // Check with already registered sources
			if (g_hmdServer->m_streamReceiver[i]->streamChannel == channel)
			{ // Trying to assign to already assigned channel
				DriverLog("Server: Trying to assign stream to already assigned channel! \n");
				return;
			}
		}
		// Channel is valid and free

		if (ID == 'H')
		{ // Headset source - only one headset is allowed at a time
			
			/*if (!g_hmdServer->m_hmdDriver)
			{ // Create HMD Driver Instance and record it as a tracked device
				g_hmdServer->m_hmdDriver = new SerialHMDDriver();
				vr::VRServerDriverHost()->TrackedDeviceAdded(g_hmdServer->m_hmdDriver->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD, g_hmdServer->m_hmdDriver);
			}*/

			// Handle channel data
			g_hmdServer->m_hmdDriver->streamChannel = channel;
			g_hmdServer->m_hmdDriver->streamType = ID;
			g_hmdServer->m_hmdDriver->m_HMDConnected = true;
			if (std::find(g_hmdServer->m_streamReceiver.begin(), g_hmdServer->m_streamReceiver.end(), g_hmdServer->m_hmdDriver) == g_hmdServer->m_streamReceiver.end())
				g_hmdServer->m_streamReceiver.push_back(g_hmdServer->m_hmdDriver);

			// Incase the first #H*X# is followed by #H*X# or #H*Y#, the headset will be reassigned to the respective channel 
		}
		else if (ID == 'C')
		{
			// TODO Create and add controller object
			// Controller = new SerialHMDController();
			// vr::VRServerDriverHost()->TrackedDeviceAdded(Controller->GetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller, Controller);
			// g_hmdServer->m_streamReceiver.push_back(Controller);
		}
		DriverLog("Server: Confirmed stream '%c' on channel '%d'! \n", ID, channel);
	}
}

void RequestTimeout(char ID)
{
	if (ID == 'I')
	{
		DriverLog("Server: ERROR: Timeout on Identification Request! \n");
	}
	else if (ID == 'S')
	{
		DriverLog("Server: ERROR: Timeout on Source Request! \n");
	}
	else
	{
		DriverLog("Server: ERROR: Timeout on Channel Open Request for Device '%c'! \n", ID);
	}
}

void StreamPacket(char channel, std::vector<unsigned char> data)
{
	for (unsigned int i = 0; i < g_hmdServer->m_streamReceiver.size(); i++)
	{ // Check all receivers if the received packet was on their channel
		if (g_hmdServer->m_streamReceiver[i]->streamChannel == channel)
		{ // Received packet on this channel
			//DriverLog("Server: Received stream packet on channel '%d'! \n", channel);
			g_hmdServer->m_streamReceiver[i]->StreamPacket(data);
			return;
		}
	}
	DriverLog("Server: Received stream packet on unassigned channel '%d'! \n", channel);
}

void ConnectionLost()
{
	// Notify that HMD and all other tracked devices are not connected anymore

	if (g_hmdServer->m_hmdDriver)
	{
		g_hmdServer->m_hmdDriver->m_HMDConnected = false;
		g_hmdServer->m_hmdDriver->UpdatePose();
	}

	// TODO: Controller Connections Lost

	DriverLog("Server: ERROR: Lost connection to SerialHMD! \n");
}

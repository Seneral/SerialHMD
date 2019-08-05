
#ifndef HMDCOMM_H
#define HMDCOMM_H

#include <WinSerialDeviceUtility.h>
#include <SerialPort.h>
#include <SerialHMDShared.h>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>

struct Request
{
	char ID;
	int timeout;
};

// *******************************************************
// Global Static Communication Functions
// *******************************************************

SerialPort *OpenSerialHMDPort();

bool ForceVerifySerialHMDID(SerialPort *port);

// *******************************************************
// SerialHMD Communication Manager
// *******************************************************

class SerialHMDComm
{
  public:

	SerialPort *m_serialHMDPort = nullptr;
	std::thread *m_commThread = nullptr;
	std::vector<unsigned char> m_unreadData;
	std::vector<Request> m_pendingRequests;

	void (*RequestResponse)(char ID, std::vector<unsigned char> response);
	void (*RequestTimeout)(char ID);
	void (*StreamPacket)(char channel, std::vector<unsigned char> data);
	void (*ConnectionLost)();

	SerialHMDComm(SerialPort *serialHMDPort) 
	{
		m_serialHMDPort = serialHMDPort;
	}
	~SerialHMDComm() {}

	bool SendRequest(char ID, int timeout);
	bool SendRequestStream(char ID, unsigned char channel, int timeout);

	void Internal_ResponseReceived(char ID);
	void Internal_HandleTimeout(int timestep);

	bool StartCommThread ();
	void StopCommThread ();
};

#endif

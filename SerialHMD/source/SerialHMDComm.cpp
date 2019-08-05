#include <SerialHMDComm.h>
#include <DriverLog.h>

// *******************************************************
// Global Static Communication Functions
// *******************************************************

SerialPort* OpenSerialHMDPort()
{
	char *hardwareID;
	if (findUSBDevice("VID_0525", &hardwareID) != 0)
	{
		DriverLog("Comm: Could not find hardware ID of SerialHMD! \n");
		return nullptr;
	}

	char **comPorts;
	int numComPorts;
	getConnectedSerialDevices("VID_0525", &comPorts, &numComPorts);
	if (numComPorts == 0)
	{
		DriverLog("Comm: Could not find COM Port of SerialHMD! \n");
		return nullptr;
	}

	char *serRPiComPort = comPorts[0];
	SerialPort *serRPiPort = new SerialPort(serRPiComPort);

	if (ForceVerifySerialHMDID(serRPiPort))
		return serRPiPort;
	DriverLog("Comm: Could not verify ID of SerialHMD! \n");

	serRPiPort->close();
	return nullptr;
}

bool ForceVerifySerialHMDID(SerialPort *port)
{
	int failedResponses = 0;
	port->flush();
	while (port->isConnected() && failedResponses <= 3)
	{
		// Send identification request
		port->writeSerialPort((char *)("#R#"), 3);
		port->writeSerialPort((char *)("#I#"), 3);

		int waitCnt = 10;
		while (waitCnt-- > 0)
		{ // Wait for up to 50ms
			if (port->updateStatus() != 0)
				return false;
			if (port->getQueueSize() > 0)
			{ // Read response and check for identification code
				int responseSize = 64;
				char response[64];
				if (port->readSerialPort(response, &responseSize) != 0)
					return false;

				if (strstr(response, "SerialHMD"))
				{ // Identified as Serial HMD
					return true;
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		DriverLog("Comm: Response was not expected ID! \n");
		failedResponses++;
	}
	return false;
}

// *******************************************************
// Local Static Communication Thread Function
// *******************************************************

bool g_abortCommThread;

void CommThread(SerialHMDComm *comm)
{
	SerialPort *port = comm->m_serialHMDPort;
	while (!g_abortCommThread)
	{
		int error = port->updateStatus();
		if (error != 0)
		{
			DriverLog("Comm: Serial Port Status Error: %d \n", error);
			if (!port->isConnected())
			{ // Port was closed, maybe Serial RPi was disconnected
				DriverLog("Comm: Serial Port was closed! \n");
				comm->ConnectionLost();
				return;
			}
		}

		if (port->getQueueSize() > 0)
		{ // Read response and check for identification code
			int responseSize = 128;
			char response[128];
			int error = port->readSerialPort(response, &responseSize);
			if (error != 0)
			{
				DriverLog("Comm: Serial Port Read Error: %d \n", error);
				if (!port->isConnected())
				{ // Port was closed, maybe Serial RPi was disconnected
					DriverLog("Comm: Serial Port was closed! \n");
					comm->ConnectionLost();
				}
			}

			// Put unread data on stack
			std::vector<unsigned char>& stack = comm->m_unreadData;
			stack.insert(stack.end(), response, response + responseSize);
			unsigned int clearMarker = 0;

			// First, handle all commands/responses from oldest to newest and remove them from the stack
			unsigned int iBegin = 0;
			while ((iBegin = static_cast<unsigned int> (std::find(stack.begin() + iBegin, stack.end(), '#') - stack.begin())) < stack.size())
			{ // Jump to next command/response begin
				// stack[iBegin] is '#'

				if (iBegin + 2 < stack.size())
				{ // Min 3: #ID#
					unsigned char ID = stack[iBegin + 1];
					if (stack[iBegin +2] == static_cast<unsigned char>('#'))
					{ // Singular message without data #ID#
						//DriverLog("Comm: Received Response '%c' \n", static_cast<char>(ID));

						// Process message
						comm->Internal_ResponseReceived(ID);
						comm->RequestResponse(ID, {});

						// Erase data packet from stack
						stack.erase(stack.begin() + iBegin, stack.begin() + iBegin + 3);
						continue;
					}
					else if (stack[iBegin + 2] == static_cast<unsigned char>('*'))
					{ // Singular message with data #ID*D#

						unsigned int iEnd = static_cast<unsigned int> (std::find(stack.begin() + iBegin + 3, stack.end(), '#') - stack.begin());
						if (iEnd < stack.size())
						{ // Includes last #, so it is complete: #ID*D#

							std::vector<unsigned char> data;
							data.assign(stack.begin() + iBegin + 3, stack.begin() + iEnd);

							// Add nullterminator for printing
							data.push_back(0x00);
							data.pop_back();
							//DriverLog("Comm: Received Respone '%c' with data [%d]: '%s' \n", ID, data.size(), data.size() > 0 ? &data[0] : nullptr);

							// Process response
							comm->Internal_ResponseReceived(ID);
							comm->RequestResponse(ID, data);

							// Erase data packet from stack
							stack.erase(stack.begin() + iBegin, stack.begin () + iBegin + 4 + data.size());
							continue;
						}
						else
						{ // Final # marker not yet on stack, return to this marker later
							clearMarker = static_cast<unsigned int>(stack.size()) - iBegin;
							break;
						}
					}
					else
					{ // Assume it's a random # from other data, ignore it
						iBegin++;
						continue;
					}
				}
				else
				{ // Full message not yet on stack, return to this marker later
					clearMarker = static_cast<unsigned int>(stack.size()) - iBegin;
					break;
				}
			}

			// Then, handle all data packets, from newest to oldest, and only pick ONE for each channel (the newest)
			std::vector<bool> flags(16);
			int startPos = static_cast<int>(stack.size() - 1) - static_cast<int>(stack.size() - 2) % 14;
			for (int i = startPos; i > 1; i--)
			{

				if (stack[i] == static_cast<unsigned char>('='))
				{ // Data stream: 'CH = D' with 0 < CH <= 16 being channel, D being 12 bytes of data
					
					int channel = static_cast<int>(stack[i-1]);
					if (channel < 1 || channel > 16)
					{ // Assume it's a random = from other data, ignore it
						continue;
					}

					if (!flags[channel-1])
					{ // Not already received a newer packet for this channel
						if (i + 12 < stack.size())
						{ // Complete stream packet has been send
							
							// Read out data
							std::vector<unsigned char> data;
							data.assign(stack.begin() + i + 1, stack.begin() + i + 13);
							
							// Process stream packet
							comm->StreamPacket(channel, data);
							
							// Fall back a full packet's size, no need to erase
							flags[channel-1] = true;
						}
						else
						{ // Full data packet not yet on stack, return to it later
							clearMarker = static_cast<unsigned int>(stack.size()) - i + 1;
						}
					}

					// Skip to next possible data packet position
					i -= 13;
				}
			}

			// Clear whole stack or, in case a packet has not been fully loaded yet, until the beginning of that packet
			stack.erase(stack.begin(), stack.end() - clearMarker);

			// Assume runtime of 1 ms
			comm->Internal_HandleTimeout(4);
			std::this_thread::sleep_for(std::chrono::microseconds(200));
		}
		else
		{ // No messages received, wait a bit and handle pending request timeouts
		std::this_thread::sleep_for(std::chrono::microseconds(50));
			comm->Internal_HandleTimeout(1);
		}
	}
}

// *******************************************************
// SerialHMDComm - Manager and Interface for SerialHMD Communication
// *******************************************************

bool SerialHMDComm::SendRequest(char ID, int timeout)
{
	if (ID != 'I' && ID != 'S' && ID != 'R')
		return false;

	if (m_serialHMDPort->isConnected())
	{ // Send and record request
		m_serialHMDPort->writeSerialPort(new char[3]{'#', ID, '#'}, 3);
		m_pendingRequests.push_back({ID, timeout});
		return true;
	}
	return false;
}

bool SerialHMDComm::SendRequestStream(char ID, unsigned char channel, int timeout)
{
	if (ID != 'H' && ID != 'C')
		return false;

	if (m_serialHMDPort->isConnected())
	{ // Send and record stream request
		m_serialHMDPort->writeSerialPort(new char[5]{'#', ID, '*', static_cast<char>(channel), '#'}, 5);
		m_pendingRequests.push_back({ID, timeout});
		return true;
	}
	return false;
}

void SerialHMDComm::Internal_ResponseReceived(char ID)
{
	for (unsigned int i = 0; i < m_pendingRequests.size(); i++)
	{ // Search if received response was expected
		if (m_pendingRequests[i].ID == ID)
		{ // Found a match for received response, remove it from pending
			m_pendingRequests.erase(m_pendingRequests.begin() + i);
			return;
		}
	}
	// Not a big deal, maybe new sources have been added
	//DriverLog("Comm: Unexpected response '%c' received! \n", ID);
}

void SerialHMDComm::Internal_HandleTimeout(int timestep)
{
	for (unsigned int i = 0; i < m_pendingRequests.size(); i++)
	{ // Decrease time left for each request
		Request r = m_pendingRequests[i];
		r.timeout -= timestep;
		if (r.timeout <= 0)
		{ // Timeout has been reached, erase it and handle possible results
			m_pendingRequests.erase(m_pendingRequests.begin() + i);
			RequestTimeout (r.ID);
		}
		else
		{ // Apply modifications
			m_pendingRequests[i] = r;
		}
	}
}

bool SerialHMDComm::StartCommThread()
{
	DriverLog("Comm: Initializing communication! \n");

	g_abortCommThread = false;
	m_commThread = new std::thread(CommThread, this);
	if (!m_commThread)
	{
		DriverLog("Comm: Unable to create communication thread! \n");
		return false;
	}

	DriverLog("Comm: Created communication thread! \n");

	return true;
}

void SerialHMDComm::StopCommThread()
{
	g_abortCommThread = true;
	if (m_commThread)
	{
		DriverLog("Comm: Closing Communication Thread! \n");

		m_commThread->join();
		delete m_commThread;
		m_commThread = nullptr;
	}
}
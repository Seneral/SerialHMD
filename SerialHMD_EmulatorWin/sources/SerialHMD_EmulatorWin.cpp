
#include <stdio.h>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>
#include <SerialPort.h>
#include <WinSerialDeviceUtility.h>


// Stats for packet rates

long g_packetsReceived = 0;
long g_packetsDropped = 0;
long g_packetsNone = 0;

std::chrono::time_point<std::chrono::steady_clock> g_lastPacketClock;
double g_lastPacketInterval = 0.0;

double g_avgPacketInterval = 0.0;
double g_minPacketInterval = 1000.0;
double g_maxPacketInterval = 0.0;

double g_highAvgPacketInterval = 0.0;
long g_highAvgPackets = 0;

// Stats for Value Jitter / Accuracy

long g_lastMove = 0, g_numMoves = 0;
double g_startYaw = 0, g_avgYaw = 0, g_diffYaw = 0;
double g_startPitch = 0, g_avgPitch = 0, g_diffPitch = 0;
double g_startRoll = 0, g_avgRoll = 0, g_diffRoll = 0;

inline void ErasePacketLine()
{
	if (g_packetsReceived > 100)
	{
		printf("\0337");
		printf("\n");          
		printf("                                                  \n");
		printf("                                                                                \n");
		printf("                                                                                                              \n");
		printf("\0338");
	}
}


inline void StatValues(int channel, float yaw, float pitch, float roll)
{
	int restDelay = 500;

	if (abs(yaw - g_avgYaw) > 5 || abs(pitch - g_avgPitch) > 5 || abs(roll - g_avgRoll) > 5)
	{
		g_lastMove = g_packetsReceived;
		g_numMoves++;
	}

	if (g_packetsReceived - g_lastMove <= restDelay)
	{
		g_startYaw = g_avgYaw = yaw;
		g_startPitch = g_avgPitch = pitch;
		g_startRoll = g_avgRoll = roll;
	}
	else
	{
		g_avgYaw += (yaw - g_avgYaw) / (g_packetsReceived - g_lastMove - restDelay) * 2;
		g_avgPitch += (pitch - g_avgPitch) / (g_packetsReceived - g_lastMove - restDelay) * 2;
		g_avgRoll += (roll - g_avgRoll) / (g_packetsReceived - g_lastMove - restDelay) * 2;
		g_diffYaw += (abs(yaw - g_avgYaw) - g_diffYaw) / g_packetsReceived;
		g_diffPitch += (abs(pitch - g_avgPitch) - g_diffPitch) / g_packetsReceived;
		g_diffRoll += (abs(roll - g_avgRoll) - g_diffRoll) / g_packetsReceived;
	}

	if (g_packetsReceived > 10)
	{
		ErasePacketLine();
		printf("\0337");
		printf("\n");
		printf("CH %d:  Y: %.2f  P: %.2f  R: %.2f  (%.2fms) \n",
			channel, yaw, pitch, roll, g_lastPacketInterval);
		printf("STATS: %d (%.1fms / %.1fms / %.1fms) - d: %d (%.2f%%) - n: %d (%.2f%%) \n",
			g_packetsReceived, g_minPacketInterval, g_avgPacketInterval, g_maxPacketInterval,
			g_packetsDropped, static_cast<double>(g_packetsDropped) / (g_packetsDropped + g_packetsReceived) * 100.0,
			g_packetsNone, static_cast<double>(g_packetsNone) / (g_packetsReceived) * 100.0);
		printf("VALUES: Y: %.2f +-%.3f |%.3f - P: %.2f +-%.3f |%.3f - R: %.2f +-%.3f |%.3f - HIGH: %d (%.2fms) - Moves: %d \n",
			g_avgYaw, g_diffYaw, g_avgYaw - g_startYaw,
			g_avgPitch, g_diffPitch, g_avgPitch - g_startPitch,
			g_avgRoll, g_diffRoll, g_avgRoll - g_startRoll,
			g_highAvgPackets, g_highAvgPacketInterval, g_numMoves);
		printf("\0338");
	}
}


inline void StatPacketNoneReceived()
{
	g_packetsNone++;
}

inline void StatPacketDropped() 
{
	g_packetsDropped++;
}

inline void StatPacketReceived()
{
	g_lastPacketInterval = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - g_lastPacketClock).count() / 1000.0;
	g_lastPacketClock = std::chrono::high_resolution_clock::now();

	g_packetsReceived++;
	if (g_packetsReceived > 100)
	{
		if (g_lastPacketInterval > 10)
		{
			//ErasePacketLine();
			//printf("%f > %f \n", g_lastPacketInterval, g_avgPacketInterval);
			g_highAvgPackets++;
			g_highAvgPacketInterval += (g_lastPacketInterval - g_highAvgPacketInterval) / g_highAvgPackets;
		}
		else
		{
			g_avgPacketInterval += (g_lastPacketInterval - g_avgPacketInterval) / (g_packetsReceived - g_highAvgPackets);
			g_maxPacketInterval = max(g_maxPacketInterval, g_lastPacketInterval);
		}
		g_minPacketInterval = min(g_minPacketInterval, g_lastPacketInterval);
	}
	else 
	{
		g_avgPacketInterval = g_lastPacketInterval;
	}
}
 
inline float BytesToFloat(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3)
{
	float output;
	unsigned char bytes[] = { b0, b1, b2, b3 };
	memcpy(&output, &bytes, sizeof(output));
	return output;
}

int main()
{
	printf("Test program. Press 'Q' to quit. \n");


	// *******************************************************
	// Wait for Serial RaspberryPi to be connected
	// Can be quickly identified by VID check for 0525
	// *******************************************************
	char* hardwareID;
	if (findUSBDevice("VID_0525", &hardwareID) != 0)
	{
		printf("Waiting for HMD... (Currently, no RPi has been detected) \n");
		while (findUSBDevice("VID_0525", &hardwareID) != 0)
		{
			if (GetAsyncKeyState(0x51) != 0) // Q
				return 0;
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}
	printf("Serial RPi detected! HardwareID: %s \n", hardwareID);


	// *******************************************************
	// Get all currently connected serial devices with given VID and PID
	// Combines results from both getComPorts and findRecordedDevices
	// *******************************************************
	char **comPorts;
	int numComPorts;
	getConnectedSerialDevices("VID_0525", &comPorts, &numComPorts);
	if (numComPorts == 0)
	{
		printf("Error: Could not identify COM port of the connected Serial RPi! \n");
		while (true)
		{
			if (GetAsyncKeyState(0x51) != 0) // Q
				return 1;
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
	}
	if (numComPorts > 1)
	{
		printf("Found %d connected Serial RPis! COM Ports are as follows: \n", numComPorts);
		for (int i = 0; i < numComPorts; i++)
		{
			printf("%d: %s \n", i+1, comPorts[i]);
		}
	}
	else
	{
		printf("Found COM Port of connected Serial RPi: %s! \n", comPorts[0]);
	}

	// *******************************************************
	// Select first Serial RPi to test as HMD and try connecting 
	// Multiple will rarely ever occur in real circumstances
	// *******************************************************
	char *hmdPort = comPorts[0];
	printf("Trying to connect to Serial RPi at port %s... \n", hmdPort);
	SerialPort *port = new SerialPort(hmdPort);
	while (!port->isConnected())
	{ // Continue trying
		printf("Failed to establish connection! \n");
		port = new SerialPort(hmdPort);

		if (GetAsyncKeyState(0x51) != 0) // Q
			return 1;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	printf("Established connection! \n ");
	port->writeSerialPort("#R#", 3);
	port->flush();

	std::vector<unsigned char> stack;
	char *awaitChar = nullptr;

	while (port->isConnected())
	{
		int error = port->updateStatus();
		if (error != 0)
		{
			ErasePacketLine();
			printf("ERROR: Serial Port Status Error: %d \n", error);
			if (!port->isConnected())
			{ // Port was closed, maybe Serial RPi was disconnected
				printf("Serial Port was closed! \n");
				while (true)
				{
					if (GetAsyncKeyState(0x51) != 0) // Q
						return 1;
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
				}
			}
		}

		const char *key = NULL;
		if (GetAsyncKeyState(0x49) & 0x0001) // I
		{
			ErasePacketLine();
			printf("Sending Identification Request '#I#'! \n");
			port->writeSerialPort("#I#", 3);
		}
		if (GetAsyncKeyState(0x53) & 0x0001) // S
		{
			ErasePacketLine();
			printf("Sending Stream List Request '#S#'! \n");
			port->writeSerialPort("#S#", 3);
			awaitChar = nullptr;
		}
		if (GetAsyncKeyState(0x52) & 0x0001) // R
		{
			ErasePacketLine();
			printf("Sending Reset Request '#R#'! \n");
			port->writeSerialPort("#R#", 3);
			awaitChar = nullptr;
		}
		if (GetAsyncKeyState(0x48) & 0x0001) // H
			awaitChar = "H";
		if (GetAsyncKeyState(0x43) & 0x0001) // C
			awaitChar = "C";
		if (awaitChar)
		{
			std::string cmd = "#";
			cmd += awaitChar;
			cmd += "*";
			if (GetAsyncKeyState(0x30) & 0x0001)
				cmd += static_cast<char> (0);
			else if (GetAsyncKeyState(0x31) & 0x0001)
				cmd += static_cast<char> (1);
			else if (GetAsyncKeyState(0x32) & 0x0001)
				cmd += static_cast<char> (2);
			else if (GetAsyncKeyState(0x33) & 0x0001)
				cmd += static_cast<char> (3);
			if (cmd.length () == 4)
			{
				cmd += "#";
				ErasePacketLine();
				printf("Sending Stream Start Request '%s'! \n", &cmd[0]);
				port->writeSerialPort(&cmd[0], 5);
				awaitChar = nullptr;
			}
		}

		if (GetAsyncKeyState(0x51) != 0) // Q
			return 0;

		if (port->getQueueSize() > 0)
		{ // Read response and check for identification code
			int responseSize = 128;
			char response[128];
			int error = port->readSerialPort(response, &responseSize);
			if (error != 0)
			{
				ErasePacketLine();
				printf("ERROR: Serial Port Status Error: %d \n", error);
				if (!port->isConnected())
				{ // Port was closed, maybe Serial RPi was disconnected
					printf("Serial Port was closed! \n");
					while (true)
					{
						if (GetAsyncKeyState(0x51) != 0) // Q
							return 1;
						std::this_thread::sleep_for(std::chrono::milliseconds(200));
					}
				}
			}

			// Put unread data on stack
			stack.insert(stack.end(), response, response + responseSize);
			unsigned int clearMarker = 0;

			// First, handle all commands/responses from oldest to newest and remove them from the stack
			int iBegin = 0;
			while ((iBegin = static_cast<unsigned int> (std::find(stack.begin() + iBegin, stack.end(), '#') - stack.begin())) < stack.size())
			{ // Jump to next command/response begin
				// stack[iBegin] is '#'
				if (iBegin + 2 < stack.size())
				{ // Min 3: #ID#
					unsigned char ID = stack[iBegin + 1];
					if (stack[iBegin + 2] == static_cast<unsigned char>('#'))
					{ // Singular message without data #ID#
						ErasePacketLine();
						printf("Comm: Received Response '%c' \n", static_cast<char>(ID));
						
						if (ID == static_cast<unsigned char>('R'))
						{
							StatValues(1, g_avgYaw, g_avgPitch, g_avgRoll);
							g_packetsReceived = g_packetsDropped = g_packetsNone = 0;
						}
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
							ErasePacketLine();
							printf("Comm: Received Respone '%c' with data [%d]: '%s' \n", ID, data.size(), data.size() > 0? &data[0] : nullptr);
							// Erase data packet from stack
							stack.erase(stack.begin() + iBegin, stack.begin() + iBegin + 4 + data.size());
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
			for (int i = static_cast<int>(stack.size()) - 1; i > 0; i--)
			{
				if (stack[i] == static_cast<unsigned char>('='))
				{ // Data stream: 'CH = D' with 0 < CH <= 16 being channel, D being 12 bytes of data
					int channel = static_cast<int>(stack[i - 1]);
					if (channel < 1 || channel > 16)
					{ // Assume it's a random = from other data, ignore it
						continue;
					}
					if (!flags[channel-1])
					{ // Not already received a newer packet for this channel
						if (i + 12 < stack.size())
						{ // Complete stream packet has been send
							std::vector<unsigned char> data;
							data.assign(stack.begin() + i + 1, stack.begin() + i + 13);

							float yaw = BytesToFloat(data[0], data[1], data[2], data[3]);
							float pitch = BytesToFloat(data[4], data[5], data[6], data[7]);
							float roll = BytesToFloat(data[8], data[9], data[10], data[11]);

							StatPacketReceived();
							StatValues(channel, yaw, pitch, roll);

							// Fall back a full packet's size, no need to erase
							flags[channel-1] = true;
						}
						else
						{ // Full data packet not yet on stack, return to it later
							clearMarker = static_cast<unsigned int>(stack.size()) - i + 1;
						}
					}
					else 
					{
						StatPacketDropped();
					}
					// Skip to next possible data packet position
					i -= 13;
				}
			}

			// Clear whole stack or, in case a packet has not been fully loaded yet, until the beginning of that packet
			stack.erase(stack.begin(), stack.end() - clearMarker);
			std::this_thread::sleep_for(std::chrono::microseconds(200));
		}
		else
		{ // No messages received, wait a bit
			StatPacketNoneReceived();
			std::this_thread::sleep_for(std::chrono::microseconds(50));
		}
		/*
		if (port->getQueueSize() > 0)
		{ // Read response and check for identification code
			int responseSize = 64;
			char response[64];
			int error = port->readSerialPort(response, &responseSize);
			if (error != 0)
			{
				printf("ERROR: Serial Port Status Error: %d \n", error);
				if (!port->isConnected())
				{ // Port was closed, maybe Serial RPi was disconnected
					printf("Serial Port was closed! \n");
					while (true)
					{
						if (GetAsyncKeyState(0x51) != 0) // Q
							return 1;
						std::this_thread::sleep_for(std::chrono::milliseconds(200));
					}
				}
			}

			// Put unread Data on stack
			unreadData.insert(unreadData.end(), response, response + responseSize);

			while (unreadData.size() > 0)
			{
				// Read first sign of data to identify packt type
				if (unreadData.front() == static_cast<unsigned char>('#'))
				{ // Singular message starting with #
					unsigned char ID = unreadData[1];
					if (unreadData[2] == static_cast<unsigned char>('#'))
					{ // Singular message without data #ID#
						printf("Comm: Received Response '%c' \n", static_cast<char>(ID));
						// Erase data packet from stack
						unreadData.erase(unreadData.begin(), unreadData.begin() + 3);
					}
					else if (unreadData[2] = static_cast<unsigned char>('*') && unreadData.size() > 3)
					{ // Singular message with data #ID*D#
						std::vector<unsigned char> data;
						while (unreadData.size() > 0 && (data.size() == 0 || data.back() != static_cast<unsigned char>('#')))
						{ // Record data until ending # is found
							data.push_back(unreadData[3 + data.size()]);
						}
						if (data.size() == 0)
						{ // Should never happen
							printf("Comm: ERROR: Could not extract data out of response with ID '%d'! \n", ID);
							return 1;
						}
						if (data.back() != static_cast<unsigned char>('#'))
						{ // Reached end of available data and not found the ending sign #
							printf("Comm: ERROR: Received response '%c' with incomplete data/finish! \n", ID);
							if (data.size() >= 16) // So long, assume it is corrupted and not unfinished
								unreadData.clear();
							break;
						}
						data.pop_back(); // Delete #
						data.push_back(0x00);
						printf("Comm: Received Respone '%c' with data [%d]: '%s' \n", ID, data.size()-1, &data[0]);
						data.pop_back();
						// Erase data packet from stack
						unreadData.erase(unreadData.begin(), unreadData.begin() + 4 + data.size());
					}
				}
				else
				{ // Data stream, first byte being channel number
					int channel = static_cast<int>(unreadData.front());
					if (unreadData.size() > 12)
					{ // Complete stream packet has been send
						std::vector<unsigned char> data;
						for (int i = 0; i < 12; i++)
						{ // Copy data
							data.push_back(unreadData[i + 1]);
						}
						// Erase data packet from stack
						unreadData.erase(unreadData.begin(), unreadData.begin() + 13);
						printf("Comm: Received Data Packet '%d' with data %s \n", channel, &data[0]);
					}
					else
						break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
		}
		else
		{ // No messages received, wait a bit and handle pending request timeouts
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		*/
	}


	// *******************************************************
	// Communicate on newly established connection
	// Print all received data and send predefined commands
	// *******************************************************
	/*while (ser->isConnected())
	{
		if (ser->getQueueSize() > 0)
		{
			int bufferBytes = MAX_DATA_LENGTH;
			ser->readSerialPort(dataBuffer, &bufferBytes);
			printf(dataBuffer);
			// Reset buffer
			memset(dataBuffer, 0, sizeof(dataBuffer));
		}

		const char *key = NULL;
		if (GetAsyncKeyState(0x48) & 0x0001) // H
			key = "h";
		if (GetAsyncKeyState(0x44) & 0x0001) // D
			key = "d";
		if (GetAsyncKeyState(0x45) & 0x0001) // E
			key = "e";
		if (GetAsyncKeyState(0x53) & 0x0001) // S
			key = "s";
		if (key != NULL)
			ser->writeSerialPort((char*)key, 1);

		if (GetAsyncKeyState(0x51) != 0) // Q
			return 0;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}*/


	// *******************************************************
	// Connection closed, just wait for user to react
	// *******************************************************
	while (true)
	{
		printf("Connection closed! Press 'Q' to quit! \n");

		if (GetAsyncKeyState(0x51) != 0) // Q
			return 0;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return 0;	
}
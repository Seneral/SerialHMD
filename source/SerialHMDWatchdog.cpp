
#include <SerialHMDWatchdog.h>
#include <DriverLog.h>

// *******************************************************
// Watchdog frequently checking if HMD has been connected
// *******************************************************

bool g_watchdogExiting;

void Watchdog()
{
	#if defined(_WIN32)

	int numResets = -1;

	reset: // If HMD failed to connect, reset init
	while (!g_watchdogExiting && numResets < 5)
	{
		numResets++;
		int triesFindCOMPort = 0; // Num tries to find COM Port
		int triesConnectSerRPi = 0;  // Num tries to connect to Serial RPi
		int failedConnectSerRPi = 0;  // Failed times to connect to Serial RPi
		char *serRPiComPort = nullptr; // COM Port
		SerialPort *serRPiPort = nullptr; // Port Interface
		int failedResponses = 0; // Num responses NOT containing requested ID
		int noResponses = 0; // Num 10ms with no response at all

		DriverLog("Waiting for HMD... \n");
		
		//wait: // Wait until a Serial RPi has been detected by windows
		while (!g_watchdogExiting)
		{ // Check for Serial RPis
			char *hardwareID;
			if (findUSBDevice("VID_0525", &hardwareID) == 0)
			{ // Found connected Serial RPi
				DriverLog("Serial RPi connected! HardwareID: %s \n", hardwareID);
				break; // wait
			}

			// Low priority check
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		//find: // Now identify com ports of all connected serial RPis
		while (!g_watchdogExiting)
		{ // Try identify COM Ports

			char **comPorts;
			int numComPorts;
			getConnectedSerialDevices("VID_0525", &comPorts, &numComPorts);

			if (numComPorts > 0)
			{ // Found COM Ports!
				for (int i = 0; i < numComPorts; i++)
					DriverLog("Found Serial RPi COM Port: %s \n", comPorts[i]);
				
				// Select first serial RPi to test as HMD and try connecting
				// Multiple will rarely ever occur in real circumstances
				serRPiComPort = comPorts[0];
				
				break; // port
			}
			else if (triesFindCOMPort > 25) // 5 seconds
			{ // ERROR - might happen on some older devices or with same lag
				DriverLog("ERROR: COM Port could not be identified! \n");
				std::this_thread::sleep_for(std::chrono::seconds(10));
				goto reset; // continue reset
			}
			else
			{
				triesFindCOMPort++;
			}

			// Medium priority, will not fix itself too fast
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		// TODO: for loop labeled ports, iterating all ports with connect + identify

		//connect: // Now try to connect to serial port
		while (!g_watchdogExiting)
		{ // Connect to RPi
			serRPiPort = new SerialPort(serRPiComPort);
			if (serRPiPort->isConnected())
			{ // Successfully connected
				DriverLog("Connected, requesting id... \n");
				break; // connect
			}
			else if (failedConnectSerRPi >= 5) // 5 seconds
			{ // Failed to connect, try other ports if available
				DriverLog("Failed to connect to COM Port %s! \n");
				std::this_thread::sleep_for(std::chrono::seconds(10));
				goto reset; // TODO: goto ports
			}
			else if (triesConnectSerRPi >= 10) // 1 second
			{ // Try connecting again
				failedConnectSerRPi++;
				triesConnectSerRPi = 0;
				DriverLog("Trying to connect to COM Port %s... \n", serRPiComPort);
			}
			else
			{
				triesConnectSerRPi++;
			}

			// Try again medium priotiy, if that happens it most likely won't fix itself too fast
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		//identify: // Now identify connected Serial RPi as HMD
		while (!g_watchdogExiting && serRPiPort->isConnected())
		{ // Set identification request to RPi

			// Update status of port to check connection and update QueueSize
			int error = serRPiPort->updateStatus();
			if (error != 0)
			{
				DriverLog("Serial Port Status Error: %d \n", error);
				if (!serRPiPort->isConnected())
				{ // Port was closed, maybe Serial RPi was disconnected
					DriverLog("Serial Port was closed! \n");
					std::this_thread::sleep_for(std::chrono::seconds(10));
					goto reset; // TODO: goto ports
				}
			}

			if (serRPiPort->getQueueSize() > 0)
			{ // Read response and check for identification code
				int responseSize = 32;
				char response[32];
				int error = serRPiPort->readSerialPort(response, &responseSize);
				DriverLog("Response: %s! \n", response);
				if (error != 0)
				{
					DriverLog("Serial Port Read Error: %d \n", error);
					if (!serRPiPort->isConnected())
					{ // Port was closed, maybe Serial RPi was disconnected
						DriverLog("Serial Port was closed! \n");
						std::this_thread::sleep_for(std::chrono::seconds(10));
						goto reset; // TODO: goto ports
					}
				}

				if (strstr(response, "SerialHMD"))
				{ // Identified as Serial HMD
					DriverLog("Successfully identified RPi as HMD! \n");
					numResets = 0;
					serRPiPort->Close();
					vr::VRWatchdogHost()->WatchdogWakeUp();
					goto idle; //return; // Success, stop watchdog
				}
				else if (failedResponses >= 3)
				{ // Failed to identify as HMD, disconnect
					DriverLog("Invalid Identification! Disconnecting! \n");
					serRPiPort->Close();
					std::this_thread::sleep_for(std::chrono::seconds(10));
					goto reset; // TODO: goto ports
				}
				else
				{ // Not correct (or mixed up) response
					failedResponses++;
					noResponses = 0;
				}
			}
			else if (noResponses >= 40)
			{ // Consider device as disconnected after 2 seconds
				DriverLog("No response from Serial RPi! Disconnecting! \n");
				serRPiPort->Close();
				std::this_thread::sleep_for(std::chrono::seconds(10));
				goto reset; // TODO: goto ports
			}
			else
			{ // No response so far
				noResponses++;
			}

			// Send identification request
			serRPiPort->writeSerialPort((char *)("#id*"), 4);

			// Try again with hight priority, response expeced pretty fast
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	idle:
	while (!g_watchdogExiting)
	{
		char *hardwareID;
		if (findUSBDevice("VID_0525", &hardwareID) != 0)
		{ // Found connected Serial RPi
			DriverLog("Serial RPi disconnected! \n");
			goto reset;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	#else
	// Nothing to do...
	#endif
}

EVRInitError SerialHMDWatchdog::Init(vr::IVRDriverContext *pDriverContext)
{
	VR_INIT_WATCHDOG_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());

	DriverLog("Initialzing watchdog! \n");

	g_watchdogExiting = false;
	m_pWatchdogThread = new std::thread(Watchdog);
	if (!m_pWatchdogThread)
	{
		DriverLog("Unable to create watchdog thread! \n");
		return VRInitError_Driver_Failed;
	}

	DriverLog("Created watchdog thread! \n");

	return VRInitError_None;
}

void SerialHMDWatchdog::Cleanup()
{
	g_watchdogExiting = true;
	if (m_pWatchdogThread)
	{
		DriverLog("Closing watchdog! \n");
		m_pWatchdogThread->join();
		delete m_pWatchdogThread;
		m_pWatchdogThread = nullptr;
	}

	CleanupDriverLog();
}

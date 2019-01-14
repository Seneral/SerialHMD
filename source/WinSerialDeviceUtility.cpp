#include <WinSerialDeviceUtility.h>


// *******************************************************
// Print all USB Devices...
// *******************************************************
int printUSBDevices()
{
	HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, TEXT("USB"), NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
	for (int i = 0;; i++)
	{
		SP_DEVINFO_DATA DeviceInfoData;
		char HardwareID[128];
		DeviceInfoData.cbSize = sizeof(DeviceInfoData);
		if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData))
			printf("Found %d devices total! \n", i);
		SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_HARDWAREID, NULL, (BYTE *)HardwareID, sizeof(HardwareID), NULL);
		printf("Device %d: %s \n", i, HardwareID);
	}
	return 1;
}


// *******************************************************
// Check if a USB device with given VID and PID is connected 
// Returns 0 if true and writes the full hardwareID of it 
// match: VID_xxxx&PID_xxxx 	-- e.g. VID_0525 for Serial RPi
// *******************************************************
int findUSBDevice(const char *match, char **hardwareID)
{
	HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, TEXT("USB"), NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
	for (int i = 0;; i++)
	{
		SP_DEVINFO_DATA DeviceInfoData;
		char HardwareID[128];
		DWORD HardwareIDSize = 1024;
		DeviceInfoData.cbSize = sizeof(DeviceInfoData);
		if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData))
			return 1;
		SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_HARDWAREID, NULL, (BYTE *)HardwareID, 1024, &HardwareIDSize);
		if (strstr(HardwareID, match))
		{
			*hardwareID = new char[HardwareIDSize];
			strcpy(*hardwareID, HardwareID);
			return 0;
		}
	}
	return 1;
}


// *******************************************************
// Get all active / currently connected COM ports
// PortNameComPairs is a list of size numPorts*2
// Containing the name and the COM port respectively
// *******************************************************
int getComPorts(char ***PortNameComPairs, int *numPorts)
{
	// Open key storing active serial ports in registry
	HKEY serialKey; // OUT parameter
	const char *serialPath = "HARDWARE\\DEVICEMAP\\SERIALCOMM";
	// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Enum\USB
	long retO = RegOpenKeyExA(HKEY_LOCAL_MACHINE, serialPath, 0, KEY_READ, &serialKey);
	if (retO == ERROR_SUCCESS)
	{ // Successfully opened key

		// OUT parameters
		DWORD nPorts, maxPortNameSize, maxPortAdressSize;

		// Query info about opened serial ports
		long retQ = RegQueryInfoKeyA(serialKey, NULL, NULL, NULL, NULL, NULL, NULL, &nPorts, &maxPortNameSize, &maxPortAdressSize, NULL, NULL);

		if (retQ == ERROR_SUCCESS)
		{ // Successfully queried info

			//printf("Found %lu active serial ports! \n", nPorts);

			// Setup output parameters
			//printf("Before processing: Value: %d -- Pointer: %p -- Array Pointer: %p \n", *numPorts, numPorts, ports);
			*numPorts = static_cast<int>(nPorts);
			*PortNameComPairs = new char *[*numPorts * 2];
			//printf("After processing: Value: %d -- Pointer: %p -- Array Pointer: %p \n", *numPorts, numPorts, ports);

			// Increase size by one to include null terminator
			maxPortNameSize++;
			maxPortAdressSize++;

			// Iterate over all active serial ports and read their name and adress
			for (int i = 0; i < *numPorts; i++)
			{
				// OUT parameters
				char *portName = new char[maxPortNameSize];
				char *portAdressBuffer = new char[maxPortAdressSize];
				DWORD portNameSize = maxPortNameSize;
				DWORD portAdressSize = maxPortAdressSize;

				// Get name and adress of current port
				long retV = RegEnumValueA(serialKey, i, portName, &portNameSize, NULL, NULL, (BYTE *)portAdressBuffer, &portAdressSize);

				if (retV == ERROR_SUCCESS)
				{ // Successfully read name and value of port

					// Convert to signed char array
					char *portAdress = new char[portAdressSize];
					strcpy(portAdress, portAdressBuffer);

					// Write port name and adress pair to output
					(*PortNameComPairs)[i * 2 + 0] = portName;
					(*PortNameComPairs)[i * 2 + 1] = portAdress;
					//printf("%s : %s \n", portAdress, portName);
				}
				else
				{
					if (retV == 234)
						printf("Failed to read port info: Buffer size not large enough (%ld)!", retV);
					else
						printf("Error while reading port info: %ld \n", retV);
					PortNameComPairs[i] = NULL;
				}
			}
		}
		else
			printf("Error while querying: %ld", retQ);
		RegCloseKey(serialKey);
		return 0;
	}
	else
		printf("Error while opening key: %ld", retO);
	return 1;
}


// *******************************************************
// Find all recorded serial devices and their COM ports with given VID and PID
// DeviceIDComPairs is a list of size numRecDevices*2
// Containung the hardwareID and associated COM port of each device match
// match: VID_xxxx&PID_xxxx 	-- e.g. VID_0525 for Serial RPi
// *******************************************************
int findRecordedDevices(const char *match, char ***DeviceIDComPairs, int *numRecDevices)
{
	// Open key storing recorded USB devices in registry
	HKEY USBKey; // OUT parameter
	const char *USBPath = "SYSTEM\\CurrentControlSet\\Enum\\USB";
	const char *DevParams = "Device Parameters";

	long retUO = RegOpenKeyExA(HKEY_LOCAL_MACHINE, USBPath, 0, KEY_READ, &USBKey);
	if (retUO == ERROR_SUCCESS)
	{ // Successfully opened key

		// OUT parameters
		DWORD numDevices, maxDeviceIDLength;

		// Query info about recorded USB devices
		long retUQ = RegQueryInfoKeyA(USBKey, NULL, NULL, NULL, &numDevices, &maxDeviceIDLength, NULL, NULL, NULL, NULL, NULL, NULL);
		if (retUQ == ERROR_SUCCESS)
		{ // Successfully queried info

			//printf("Found %lu recorded USB devices! \n", numDevices);

			// Setup output parameters
			*DeviceIDComPairs = new char *[numDevices * 2];
			*numRecDevices = 0;

			// Increase size by one to include null terminator
			maxDeviceIDLength++;

			// Iterate over all recorded USB devices and match their name
			for (int d = 0; d < int(numDevices); d++)
			{
				// OUT parameters
				char *hardwareID = new char[maxDeviceIDLength];
				DWORD hardwareIDSize = maxDeviceIDLength;

				// Get name of current device
				long retUE = RegEnumKeyExA(USBKey, d, hardwareID, &hardwareIDSize, NULL, NULL, NULL, NULL);
				if (retUE == ERROR_SUCCESS)
				{ // Successfully read name of current device

					if (strstr(hardwareID, match))
					{ // Device match

						// Compose intermediate device path
						char devicePath[strlen(USBPath) + 2 + hardwareIDSize];
						strcpy(devicePath, USBPath);
						strcat(devicePath, "\\");
						strcat(devicePath, hardwareID);

						// Open device key
						HKEY DeviceKey;
						long retDO = RegOpenKeyExA(HKEY_LOCAL_MACHINE, devicePath, 0, KEY_READ, &DeviceKey);
						if (retDO == ERROR_SUCCESS)
						{ // Successfully opened device key
							for (int sk = 0;; sk++)
							{ // Enumerate all subkeys, usually only one, without querying info bc it's limited

								// Get subkey name
								char *skName = new char[32]; // more than enough
								DWORD skNameSize = 32;
								long retSKE = RegEnumKeyExA(DeviceKey, sk, skName, &skNameSize, NULL, NULL, NULL, NULL);
								if (retSKE == ERROR_SUCCESS)
								{ // Got name of subkey, now open 'Device Parameters'

									// Compose final path
									char paramsPath[strlen(devicePath) + 2 + skNameSize + 2 + strlen(DevParams)];
									strcpy(paramsPath, devicePath);
									strcat(paramsPath, "\\");
									strcat(paramsPath, skName);
									strcat(paramsPath, "\\");
									strcat(paramsPath, DevParams);

									// Open 'Device Parameters' key
									HKEY DevParamsKey;
									long retPO = RegOpenKeyExA(HKEY_LOCAL_MACHINE, paramsPath, 0, KEY_READ, &DevParamsKey);
									if (retPO == ERROR_SUCCESS)
									{
										// Query info about 'Device Parameters' values
										DWORD numParams, maxNameSize, maxValueSize;
										long retPQ = RegQueryInfoKeyA(DevParamsKey, NULL, NULL, NULL, NULL, NULL, NULL, &numParams, &maxNameSize, &maxValueSize, NULL, NULL);
										if (retPQ == ERROR_SUCCESS)
										{
											//printf("Found %lu parameters! \n", numParams);

											for (int p = 0; p < int(numParams); p++)
											{
												// OUT parameters
												char *paramName = new char[maxNameSize];
												char *paramDataBuffer = new char[maxValueSize];
												DWORD paramNameSize = maxNameSize;
												DWORD paramDataSize = maxValueSize;

												// Get name and value of parameter
												long retPE = RegEnumValue(DevParamsKey, p, paramName, &paramNameSize, NULL, NULL, (BYTE *)paramDataBuffer, &paramDataSize);
												if (retPE == ERROR_SUCCESS)
												{ // Successfully read name and value of parameter

													if (strcmp(paramName, "PortName") == 0)
													{ // Found PortName Parameter

														// Convert to signed char array
														char *comPort = new char[paramDataSize];
														strcpy(comPort, paramDataBuffer);

														// Write hardwareID and ComPort pair to output
														(*DeviceIDComPairs)[*numRecDevices * 2 + 0] = hardwareID;
														(*DeviceIDComPairs)[*numRecDevices * 2 + 1] = comPort;
														*numRecDevices = *numRecDevices + 1;

														//printf("Identified %s as %s \n", hardwareID, comPort);
													}
												}
											}
										}
										RegCloseKey(DevParamsKey);
									}
									else
										printf("Error while opening key %s: %ld \n", paramsPath, retPO);
								}
								else // No more subkeys to check, continue to next device
									break;
							}
							RegCloseKey(DeviceKey);
						}
						else
							printf("Error while opening key %s: %ld \n", devicePath, retDO);
					}
				}
			}
		}
		RegCloseKey(USBKey);
		return 0;
	}
	else
		printf("Error while opening key %s: %ld \n", USBPath, retUO);
	return 1;
}

// *******************************************************
// Get all currently connected serial devices with given VID and PID
// Combines results from both getComPorts and findRecordedDevices
// *******************************************************
int getConnectedSerialDevices(const char *match, char ***comPorts, int *numComPorts)
{
	// Find recorded serial devices with given VID and PID
	// This gives us all past RPis and their assigned COM ports
	char **recordedPorts;
	int numRecPorts;
	int retRec = findRecordedDevices(match, &recordedPorts, &numRecPorts);
	if (retRec != 0) 
		return retRec;

	// Get all active serial COM ports
	char **activePorts;
	int numActivePorts;
	int retPorts = getComPorts(&activePorts, &numActivePorts);
	if (retPorts != 0)
		return retPorts;

	// Iterate through all possible combination of COM ports
	// in both recorded and active list and find COM ports that are in both
	*comPorts = new char *[numRecPorts < numActivePorts ? numRecPorts : numActivePorts];
	*numComPorts = 0;
	for (int r = 0; r < numRecPorts; r++)
	{
		for (int c = 0; c < numActivePorts; c++)
		{
			if (strcmp(recordedPorts[r * 2 + 1], activePorts[c * 2 + 1]) == 0)
			{ // Found connected serial device match that is connected
				char *port = activePorts[c * 2 + 1];
				bool in = false;
				for (int p = 0; p < *numComPorts; p++)
					in = in || strcmp((const char*)(*comPorts)[p], port) == 0;
				if (!in)
				{
					(*comPorts)[*numComPorts] = port;
					*numComPorts = *numComPorts + 1;
 				}
			}
		}
	}

	return 0;
}
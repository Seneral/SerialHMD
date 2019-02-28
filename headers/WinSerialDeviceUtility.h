
#ifndef SERIALDEVICEUTILITY_H
#define SERIALDEVICEUTILITY_H

#include <stdio.h>
#include <windows.h>
#include <SetupAPI.h>
#include <string>

// *******************************************************
// Print all USB Devices...
// *******************************************************
int printUSBDevices();

// *******************************************************
// Check if a USB device with given VID and PID is connected
// Returns 0 if true and writes the full hardwareID of it
// match: VID_xxxx&PID_xxxx 	-- e.g. VID_0525 for Serial RPi
// *******************************************************
int findUSBDevice(const char *match, char **hardwareID);

// *******************************************************
// Get all active / currently connected COM ports
// PortNameComPairs is a list of size numPorts*2
// Containing the name and the COM port respectively
// *******************************************************
int getComPorts(char ***PortNameComPairs, int *numPorts);

// *******************************************************
// Find all recorded serial devices and their COM ports with given VID and PID
// DeviceIDComPairs is a list of size numRecDevices*2
// Containung the hardwareID and associated COM port of each device match
// match: VID_xxxx&PID_xxxx 	-- e.g. VID_0525 for Serial RPi
// *******************************************************
int findRecordedDevices(const char *match, char ***DeviceIDComPairs, int *numRecDevices);

// *******************************************************
// Get all currently connected serial devices with given VID and PID
// Combines results from both getComPorts and findRecordedDevices
// *******************************************************
int getConnectedSerialDevices(const char *match, char ***comPorts, int *numComPorts);

#endif // SERIALDEVICEUTILITY_H

#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

class SerialPort
{
private:
	HANDLE handle;
	bool connected;
	COMSTAT status;
	DWORD errors;

public:
	SerialPort(char *portName);
	~SerialPort();

	void close();
	int updateStatus();
	bool isConnected();
	int getQueueSize();
	int readSerialPort(char *buffer, int *bufferBytes);
	int writeSerialPort(char *buffer, int bufferBytes);
	int flush();
};

#endif // SERIALPORT_H
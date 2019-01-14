
#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

class SerialPort
{
  private:
	HANDLE handler;
	bool connected;
	COMSTAT status;
	DWORD errors;

  public:
	SerialPort(char *portName);
	~SerialPort();

	void Close();
	int updateStatus();
	bool isConnected();
	int getQueueSize();
	int readSerialPort(char *buffer, int *bufferBytes);
	int writeSerialPort(char *buffer, int bufferBytes);
};

#endif // SERIALPORT_H
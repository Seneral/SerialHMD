#include "SerialPort.h"

SerialPort::SerialPort(char *port)
{
	this->connected = false;
	this->handler = CreateFileA(static_cast<LPCSTR>(port),
								GENERIC_READ | GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL);
	if (this->handler == INVALID_HANDLE_VALUE)
	{
		long error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND)
			printf("ERROR %ld: %s not available! \n", error, port);
		else
			printf("ERROR %ld!", error);
	}
	else
	{
		DCB dcbSerialParams = {0};

		if (!GetCommState(this->handler, &dcbSerialParams))
			printf("ERROR: Could not get current serial port parameters! \n");
		else
		{
			dcbSerialParams.BaudRate = CBR_9600;
			dcbSerialParams.ByteSize = 8;
			dcbSerialParams.StopBits = ONESTOPBIT;
			dcbSerialParams.Parity = NOPARITY;
			dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

			if (!SetCommState(handler, &dcbSerialParams))
				printf("ERROR: Could not set new serial port parameters! \n");
			else
			{
				this->connected = true;
				PurgeComm(this->handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
			}
		}
	}
}

SerialPort::~SerialPort()
{
	Close();
}

void SerialPort::Close()
{
	if (this->connected)
	{
		this->connected = false;
		CloseHandle(this->handler);
	}
}

int handleError (bool *connected) 
{
	int error = GetLastError();
	if (*connected)
	{
		if (error == ERROR_OPERATION_ABORTED || error == ERROR_BAD_COMMAND)
			*connected = false;
	}
	return error;
}

int SerialPort::updateStatus()
{
	int retStatus = ClearCommError(this->handler, &this->errors, &this->status);
	if (retStatus == 0)
	{ // Encountered error
		return handleError(&this->connected);
	}
	return 0;
}

bool SerialPort::isConnected()
{
	return this->connected;
}

int SerialPort::getQueueSize()
{
	return this->status.cbInQue;
}

int SerialPort::readSerialPort(char *buffer, int *bufferBytes)
{
	int error = updateStatus();
	if (error != 0)
		return error; 
	if (this->status.cbInQue > 0)
	{
		DWORD bytesRead;
		unsigned int unread = this->status.cbInQue;
		unsigned int buf = (unsigned int)*bufferBytes;
		if (ReadFile(this->handler, buffer, unread > buf ? buf : unread, &bytesRead, NULL))
		{
			*bufferBytes = bytesRead;
			return 0;
		}
		else
		{ // Encountered error
			return handleError(&this->connected);
		}
	}
	return 0;
}

int SerialPort::writeSerialPort(char *buffer, int bufferBytes)
{
	DWORD bytesSend;
	if (!WriteFile(this->handler, (void *)buffer, bufferBytes, &bytesSend, 0))
	{
		return updateStatus();
	}
	return 0;
}

#include "SerialPort.h"
#include "DriverLog.h"

int handleError(bool *connected)
{
	int error = GetLastError();
	if (*connected)
	{
		DriverLog("ERROR Code: %d \n", error);
		if (error == ERROR_INVALID_HANDLE)
			*connected = false;
		if (error == ERROR_OPERATION_ABORTED || error == ERROR_BAD_COMMAND)
			*connected = false;
	}
	return error;
}

SerialPort::SerialPort(char *port)
{
	std::string portAdress = "\\\\.\\";
	portAdress += port;
	connected = false;
	handle = CreateFileA(portAdress.c_str(),
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		long error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND)
			DriverLog("ERROR %ld when creating SerialPort: %s not available! \n", error, port);
		else
			DriverLog("ERROR %ld when creating SerialPort! \n", error);
	}
	else
	{
		DCB dcbSerialParams = { 0 };
		if (!GetCommState(handle, &dcbSerialParams))
			DriverLog("ERROR: Could not get current serial port parameters! \n");
		else
		{
			dcbSerialParams.BaudRate = CBR_115200;
			dcbSerialParams.ByteSize = 8;
			dcbSerialParams.StopBits = ONESTOPBIT;
			dcbSerialParams.Parity = NOPARITY;
			dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

			if (!SetCommState(handle, &dcbSerialParams))
				DriverLog("ERROR: Could not set new serial port parameters! \n");
			else
			{
				connected = true;
				PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
			}
		}

/*		COMMTIMEOUTS commTimeouts = { 0 };
		if (!GetCommTimeouts(handle, &commTimeouts))
			printf("ERROR: Could not get current serial port timeouts! \n");
		else
		{
			//commTimeouts

			if (!SetCommTimeouts(handle, &commTimeouts))
				printf("ERROR: Could not set new serial port timeouts! \n");
		}*/
	}
}

SerialPort::~SerialPort()
{
	close();
}

int SerialPort::flush()
{
	int error = updateStatus();
	if (error != 0)
		return error;
	SetEndOfFile(handle);
	//SetFilePointer(handle, 0, NULL, FILE_END);
	return updateStatus();
}

void SerialPort::close()
{
	//if (connected)
	//{
		connected = false;
		CloseHandle(handle);
	//}
}

int SerialPort::updateStatus()
{
	int retStatus = ClearCommError(handle, &errors, &status);
	if (retStatus == 0)
	{ // Encountered error
		return handleError(&connected);
	}
	return 0;
}

bool SerialPort::isConnected()
{
	updateStatus();
	return connected;
}

int SerialPort::getQueueSize()
{
	return status.cbInQue;
}

int SerialPort::readSerialPort(char *buffer, int *bufferBytes)
{
	int error = updateStatus();
	if (error != 0)
		return error; 
	if (status.cbInQue > 0)
	{
		DWORD bytesRead;
		unsigned int unread = status.cbInQue;
		unsigned int buf = (unsigned int)*bufferBytes;
		//memset(buffer, 0x00, buf);
		if (ReadFile(handle, buffer, unread > buf ? buf : unread, &bytesRead, NULL))
		{
			*bufferBytes = bytesRead;
			return 0;
		}
		else
		{ // Encountered error
			return handleError(&connected);
		}
	}
	return 0;
}

int SerialPort::writeSerialPort(char *buffer, int bufferBytes)
{
	DWORD bytesSend;
	int error = updateStatus();
	if (error != 0)
		return error;
	if (!WriteFile(handle, (void *)buffer, bufferBytes, &bytesSend, 0))
	{
		return handleError(&connected);
	}
	return 0;
}

//========= Copyright Valve Corporation ============//

#include "DriverLog.h"

#include <stdio.h>
#include <stdarg.h>

static vr::IVRDriverLog *s_pLogFile = NULL;

#if !defined(WIN32)
#define vsnprintf_s vsnprintf
#endif

/*void addToStream(std::ostringstream &)
{
}

template <typename T, typename... Args>
void addToStream(std::ostringstream &a_stream, T &&a_value, Args &&... a_args)
{
	a_stream << std::forward<T>(a_value);
	addToStream(a_stream, std::forward<Args>(a_args)...);
}

template <typename... Args>
char* concat(Args &&... a_args)
{
	std::ostringstream s;
	addToStream(s, std::forward<Args>(a_args)...);
	return s.c_str();
}*/

bool InitDriverLog(vr::IVRDriverLog *pDriverLog)
{
	if (s_pLogFile)
		return false;
	s_pLogFile = pDriverLog;
	return s_pLogFile != NULL;
}

void CleanupDriverLog()
{
	s_pLogFile = NULL;
}

static void DriverLogVarArgs(const char *pMsgFormat, va_list args)
{
	char buf[1024];
	vsnprintf_s(buf, sizeof(buf), pMsgFormat, args);

	if (s_pLogFile)
		s_pLogFile->Log(buf);
}

void DriverLog(const char *pMsgFormat, ...)
{
	va_list args;
	va_start(args, pMsgFormat);

	DriverLogVarArgs(pMsgFormat, args);

	va_end(args);
}

void DebugDriverLog(const char *pMsgFormat, ...)
{
#ifdef _DEBUG
	va_list args;
	va_start(args, pMsgFormat);

	DriverLogVarArgs(pMsgFormat, args);

	va_end(args);
#endif
}

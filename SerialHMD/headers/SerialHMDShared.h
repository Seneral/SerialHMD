#pragma once

#ifndef HMDSHARED_H
#define HMDSHARED_H

#include <openvr_driver.h>

using namespace vr;

// *******************************************************
// Keys for the Settings API
// *******************************************************

static const char *const k_pch_serialhmd_Section = "driver_serialhmd";
static const char *const k_pch_serialhmd_SerialNumber_String = "serialNumber";
static const char *const k_pch_serialhmd_ModelNumber_String = "modelNumber";
static const char *const k_pch_serialhmd_SecondsFromVsyncToPhotons_Float = "secondsFromVsyncToPhotons";
static const char *const k_pch_serialhmd_DisplayFrequency_Float = "displayFrequency";

static const char *const k_pch_serialhmd_WindowX_Int32 = "windowX";
static const char *const k_pch_serialhmd_WindowY_Int32 = "windowY";
static const char *const k_pch_serialhmd_WindowWidth_Int32 = "windowWidth";
static const char *const k_pch_serialhmd_WindowHeight_Int32 = "windowHeight";
static const char *const k_pch_serialhmd_RenderWidth_Int32 = "renderWidth";
static const char *const k_pch_serialhmd_RenderHeight_Int32 = "renderHeight";
static const char *const k_pch_serialhmd_DistortionK1_Float = "DistortionK1";
static const char *const k_pch_serialhmd_DistortionK2_Float = "DistortionK2";
static const char *const k_pch_serialhmd_ZoomWidth_Float = "ZoomWidth";
static const char *const k_pch_serialhmd_ZoomHeight_Float = "ZoomHeight";
static const char *const k_pch_serialhmd_OnDesktop_Bool = "IsOnDesktop";
static const char *const k_pch_serialhmd_RealDisplay_Bool = "IsRealDisplay";

static const char *const k_pch_serialhmd_DebugMode_Bool = "DebugMode";


// *******************************************************
// Utility
// *******************************************************

inline HmdQuaternion_t HmdQuaternion_Init(double w, double x, double y, double z)
{
	HmdQuaternion_t quat;
	quat.w = w;
	quat.x = x;
	quat.y = y;
	quat.z = z;
	return quat;
}

inline void HmdMatrix_SetIdentity(HmdMatrix34_t *pMatrix)
{
	pMatrix->m[0][0] = 1.f;
	pMatrix->m[0][1] = 0.f;
	pMatrix->m[0][2] = 0.f;
	pMatrix->m[0][3] = 0.f;
	pMatrix->m[1][0] = 0.f;
	pMatrix->m[1][1] = 1.f;
	pMatrix->m[1][2] = 0.f;
	pMatrix->m[1][3] = 0.f;
	pMatrix->m[2][0] = 0.f;
	pMatrix->m[2][1] = 0.f;
	pMatrix->m[2][2] = 1.f;
	pMatrix->m[2][3] = 0.f;
}

inline double DegToRad(double f)
{
	return f * (3.14159265358979323846 / 180);
}

inline float BytesToFloat(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3)
{
	float output;
	unsigned char bytes[] = { b0, b1, b2, b3 };
	memcpy(&output, &bytes, sizeof(output));
	return output;
}

struct _HMDData
{
	double X;
	double Y;
	double Z;
	double Yaw;
	double Pitch;
	double Roll;
};

class ICommStreamReceiver
{
  public:
	unsigned char streamChannel;
	char streamType;

	virtual void StreamPacket(std::vector<unsigned char> packet) = 0;
};

#endif // HMDSHARED_H
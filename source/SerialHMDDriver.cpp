#include <SerialHMDDriver.h>
#include <DriverLog.h>

SerialHMDDriver::SerialHMDDriver()
{
	DriverLog("Driver: Init! \n");

	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;
	
	char buf[1024];
	vr::VRSettings()->GetString(k_pch_serialhmd_Section, k_pch_serialhmd_SerialNumber_String, buf, sizeof(buf));
	m_sSerialNumber = buf;
	vr::VRSettings()->GetString(k_pch_serialhmd_Section, k_pch_serialhmd_ModelNumber_String, buf, sizeof(buf));
	m_sModelNumber = buf;

	m_flIPD = vr::VRSettings()->GetFloat(k_pch_SteamVR_Section, k_pch_SteamVR_IPD_Float);
	m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat(k_pch_serialhmd_Section, k_pch_serialhmd_SecondsFromVsyncToPhotons_Float);
	m_flDisplayFrequency = vr::VRSettings()->GetFloat(k_pch_serialhmd_Section, k_pch_serialhmd_DisplayFrequency_Float);

	m_bDebugMode = vr::VRSettings()->GetBool(k_pch_serialhmd_Section, k_pch_serialhmd_DebugMode_Bool);
	
	DriverLog("Driver: Serial Number: %s\n", m_sSerialNumber.c_str());
	DriverLog("Driver: Model Number: %s\n", m_sModelNumber.c_str());
	DriverLog("Driver: Seconds from Vsync to Photons: %f\n", m_flSecondsFromVsyncToPhotons);
	DriverLog("Driver: Display Frequency: %f\n", m_flDisplayFrequency);
	DriverLog("Driver: IPD: %f\n", m_flIPD);

	// TODO: Own HMD interface
	HMDConnected = true;

	// DISPLAY 

	m_nWindowX = vr::VRSettings()->GetInt32(k_pch_serialhmd_Section, k_pch_serialhmd_WindowX_Int32);
	m_nWindowY = vr::VRSettings()->GetInt32(k_pch_serialhmd_Section, k_pch_serialhmd_WindowY_Int32);
	m_nWindowWidth = vr::VRSettings()->GetInt32(k_pch_serialhmd_Section, k_pch_serialhmd_WindowWidth_Int32);
	m_nWindowHeight = vr::VRSettings()->GetInt32(k_pch_serialhmd_Section, k_pch_serialhmd_WindowHeight_Int32);
	m_nRenderWidth = vr::VRSettings()->GetInt32(k_pch_serialhmd_Section, k_pch_serialhmd_RenderWidth_Int32);
	m_nRenderHeight = vr::VRSettings()->GetInt32(k_pch_serialhmd_Section, k_pch_serialhmd_RenderHeight_Int32);

	m_fDistortionK1 = vr::VRSettings()->GetFloat(k_pch_serialhmd_Section, k_pch_serialhmd_DistortionK1_Float);
	m_fDistortionK2 = vr::VRSettings()->GetFloat(k_pch_serialhmd_Section, k_pch_serialhmd_DistortionK2_Float);
	m_fZoomWidth = vr::VRSettings()->GetFloat(k_pch_serialhmd_Section, k_pch_serialhmd_ZoomWidth_Float);
	m_fZoomHeight = vr::VRSettings()->GetFloat(k_pch_serialhmd_Section, k_pch_serialhmd_ZoomHeight_Float);
	
	m_bOnDesktop = vr::VRSettings()->GetBool(k_pch_serialhmd_Section, k_pch_serialhmd_OnDesktop_Bool);
	m_bRealDisplay = vr::VRSettings()->GetBool(k_pch_serialhmd_Section, k_pch_serialhmd_RealDisplay_Bool);
}

SerialHMDDriver::~SerialHMDDriver()
{
	DriverLog("Driver: Deconstruct! \n");
}


// *******************************************************
// HMD Lifetime
// *******************************************************

EVRInitError SerialHMDDriver::Activate(vr::TrackedDeviceIndex_t unObjectId)
{
	DriverLog("Driver: Activating HMD! \n");

	m_unObjectId = unObjectId;
	m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

	// Write properties of actual HMD as loaded above
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str());
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_RenderModelName_String, m_sModelNumber.c_str());
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserIpdMeters_Float, m_flIPD);
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserHeadToEyeDepthMeters_Float, 0.f);
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_DisplayFrequency_Float, m_flDisplayFrequency);
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_SecondsFromVsyncToPhotons_Float, m_flSecondsFromVsyncToPhotons);
	DriverLog("Driver: Set Prop Model/Serial/IPD/HeadToEye/Freq/VSyncSec \n");

	// return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
	vr::VRProperties()->SetUint64Property(m_ulPropertyContainer, Prop_CurrentUniverseId_Uint64, 11);
	DriverLog("Driver: Set Prop_CurrentUniverseId_Uint64 : 11\n");

	// avoid "not fullscreen" warnings from vrmonitor
	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_IsOnDesktop_Bool, vr::VRSettings()->GetBool(k_pch_serialhmd_Section, "IsOnDesktop"));
	DriverLog("Driver: Set Prop_IsOnDesktop_Bool : %s\n", vr::VRSettings()->GetBool(k_pch_serialhmd_Section, "IsOnDesktop")? "True" : "False");

	// Debug mode activate Windowed Mode (borderless fullscreen), lock to 30 FPS
	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_DisplayDebugMode_Bool, m_bDebugMode);
	DriverLog("Driver: Set Prop_DisplayDebugMode_Bool : %s \n", m_bDebugMode? "True" : "False");

	
	// TODO: Set Hidden Area Mesh
	// IN: vr::VRHiddenArea()
	// SET: ETrackedPropertyError SetHiddenArea(EVREye eEye, EHiddenAreaMeshType type, HmdVector2_t * pVerts, uint32_t unVertCount);
	// GET: uint32_t GetHiddenArea( EVREye eEye, EHiddenAreaMeshType type, HmdVector2_t *pVerts, uint32_t unVertCount, ETrackedPropertyError *peError );
	uint32_t hamVertCount = 6;
	HmdVector2_t hamVertsLeft[hamVertCount] = { { +0.1, 0.05 }, { +0.4, 0.05 }, { +0.5, 0.3 }, { +0.5, 0.6 }, { +0.4, 0.95 }, { +0.1, 0.95 }, { +0, 0.5 } };
	HmdVector2_t hamVertsRight[hamVertCount] = { { -0.1, 0.05 }, { -0.4, 0.05 }, { -0.5, 0.3 }, { -0.5, 0.6 }, { -0.4, 0.95 }, { -0.1, 0.95 }, { -0, 0.5 } };
	ETrackedPropertyError err = vr::VRHiddenArea()->SetHiddenArea(Eye_Left, k_eHiddenAreaMesh_Standard, hamVertsLeft, hamVertCount);
	DriverLog("Driver: SetHiddenArea Left: %ld\n", err);
	err = vr::VRHiddenArea()->SetHiddenArea(Eye_Right, k_eHiddenAreaMesh_Standard, hamVertsRight, hamVertCount);
	DriverLog("Driver: SetHiddenArea Right: %ld\n", err);

	// TODO: Activate HMD

	return VRInitError_None;
}

void SerialHMDDriver::Deactivate()
{
	// TODO: Deactivate HMD

	DriverLog("Driver: Deactivating HMD! \n");

	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
}

void SerialHMDDriver::EnterStandby()
{
	// TODO: Pause HMD Data Stream

	DriverLog("Driver: Enter Standby HMD! \n");
}

void SerialHMDDriver::PowerOff()
{
	// TODO: Power off HMD

	DriverLog("Driver: PowerOff HMD! \n");
}

// *******************************************************
// Pose and Tracking
// *******************************************************

void GetHMDData(_HMDData *hmd)
{
	// TODO: Get actual sensor data
	// Will be executed on separate thread usually 

	hmd->X = 0;
	hmd->Y = 0;
	hmd->Z = 1.8;

	hmd->Yaw = rand() % 360;
	hmd->Pitch = rand() % 360;
	hmd->Roll = rand() % 360;
}

DriverPose_t SerialHMDDriver::GetPose()
{
	//DriverLog("Driver: GetPose \n");

	// TODO: Allows independant execution from threads
	// Will be called once by system on startup
	// And then further executed as fast as possible on a separate thread

	DriverPose_t pose = {0};

	if (HMDConnected)
	{
		pose.poseIsValid = true;
		pose.result = TrackingResult_Running_OK; // TrackingResult_Fallback_RotationOnly
		pose.deviceIsConnected = true;
	}
	else
	{
		pose.poseIsValid = false;
		pose.result = TrackingResult_Uninitialized;
		pose.deviceIsConnected = false;
	}

	pose.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
	pose.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);

	if (HMDConnected)
	{
		// TODO: Own HMD interface handling centering

		GetHMDData(&m_HMD);

		// Set head tracking rotation
		pose.qRotation.w = cos(DegToRad(m_HMD.Yaw) * 0.5) * cos(DegToRad(m_HMD.Roll) * 0.5) * cos(DegToRad(m_HMD.Pitch) * 0.5) + sin(DegToRad(m_HMD.Yaw) * 0.5) * sin(DegToRad(m_HMD.Roll) * 0.5) * sin(DegToRad(m_HMD.Pitch) * 0.5);
		pose.qRotation.x = cos(DegToRad(m_HMD.Yaw) * 0.5) * sin(DegToRad(m_HMD.Roll) * 0.5) * cos(DegToRad(m_HMD.Pitch) * 0.5) - sin(DegToRad(m_HMD.Yaw) * 0.5) * cos(DegToRad(m_HMD.Roll) * 0.5) * sin(DegToRad(m_HMD.Pitch) * 0.5);
		pose.qRotation.y = cos(DegToRad(m_HMD.Yaw) * 0.5) * cos(DegToRad(m_HMD.Roll) * 0.5) * sin(DegToRad(m_HMD.Pitch) * 0.5) + sin(DegToRad(m_HMD.Yaw) * 0.5) * sin(DegToRad(m_HMD.Roll) * 0.5) * cos(DegToRad(m_HMD.Pitch) * 0.5);
		pose.qRotation.z = sin(DegToRad(m_HMD.Yaw) * 0.5) * cos(DegToRad(m_HMD.Roll) * 0.5) * cos(DegToRad(m_HMD.Pitch) * 0.5) - cos(DegToRad(m_HMD.Yaw) * 0.5) * sin(DegToRad(m_HMD.Roll) * 0.5) * sin(DegToRad(m_HMD.Pitch) * 0.5);

		// Set position tracking
		pose.vecPosition[0] = m_HMD.X;
		pose.vecPosition[1] = m_HMD.Y;
		pose.vecPosition[2] = m_HMD.Z;
	}

	return pose;
}

void SerialHMDDriver::RunFrame()
{
	// TODO: Replace with thread

	// In a real driver, this should happen from some pose tracking thread.
	// The RunFrame interval is unspecified and can be very irregular if some other
	// driver blocks it for some periodic task.
	if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid)
	{
		vr::VRServerDriverHost()->TrackedDevicePoseUpdated(m_unObjectId, GetPose(), sizeof(DriverPose_t));
	}
}

// *******************************************************
// Other
// *******************************************************

void *SerialHMDDriver::GetComponent(const char *pchComponentNameAndVersion)
{
	if (!strcmp(pchComponentNameAndVersion, vr::IVRDisplayComponent_Version))
	{
		return (vr::IVRDisplayComponent *)this;
		DriverLog("Driver: Provided requested component '%s'! \n", pchComponentNameAndVersion);
	}

	DriverLog("Driver: Requested unimplemented component '%s'! \n", pchComponentNameAndVersion);

	return NULL;
}

void SerialHMDDriver::DebugRequest(const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize)
{
	DriverLog("Driver: Debug request issued! '%s'! \n", pchRequest);
	if (unResponseBufferSize >= 1)
		pchResponseBuffer[0] = 0;
}

// *******************************************************
// Display Methods
// *******************************************************

// (1920, 0, 1920, 1080)
void SerialHMDDriver::GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight)
{
	DriverLog("Display: GetWindowBounds (%d, %d)(%d, %d) \n", int(m_nWindowX), int(m_nWindowY), int(m_nWindowWidth), int(m_nWindowHeight));

	*pnX = m_nWindowX;
	*pnY = m_nWindowY;
	*pnWidth = m_nWindowWidth;
	*pnHeight = m_nWindowHeight;
}

// true
bool SerialHMDDriver::IsDisplayOnDesktop()
{
	DriverLog("Display: IsDisplayOnDesktop: %s \n", m_bOnDesktop? "true" : "false");
	return m_bOnDesktop;
}

// true
bool SerialHMDDriver::IsDisplayRealDisplay()
{
	DriverLog("Display: IsDisplayRealDisplay: %s \n", m_bRealDisplay? "true" : "false");
	return m_bRealDisplay;
}

// (1920, 1080)
void SerialHMDDriver::GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight)
{
	DriverLog("Display: GetRecommendedRenderTargetSize (%d, %d) \n", int(m_nRenderWidth), int(m_nRenderHeight));

	*pnWidth = m_nRenderWidth;
	*pnHeight = m_nRenderHeight;
}

// (-1, 1, -1, 1) or (-0.3, 0.3, -0.5, 0.5)
void SerialHMDDriver::GetProjectionRaw(EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom)
{
	*pfLeft = -0.3;
	*pfRight = 0.3;
	*pfTop = -0.5;
	*pfBottom = 0.5;
	
	DriverLog("Display: GetProjectionRaw %s (L %f, R %f, T %f, B %f)\n", eEye == Eye_Left? "Left" : "Right", *pfLeft, *pfRight, *pfTop, *pfBottom);
}

// Left: (0, 0, 960, 1080) Right: (960, 0, 960, 1080)
void SerialHMDDriver::GetEyeOutputViewport(EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight)
{
	*pnY = 0;
	*pnWidth = m_nWindowWidth / 2;
	*pnHeight = m_nWindowHeight;

	if (eEye == Eye_Left)
		*pnX = 0;
	else
		*pnX = m_nWindowWidth / 2;

	DriverLog("Display: GetEyeOutputViewport %s (%d, %d)(%d, %d) \n", eEye == Eye_Left? "Left" : "Right", int(*pnX), int(*pnY), int(*pnWidth), int(*pnHeight));
}

// Same values
DistortionCoordinates_t SerialHMDDriver::ComputeDistortion(EVREye eEye, float fU, float fV)
{
	vr::DistortionCoordinates_t coordinates;

	//distortion for lens from https://github.com/HelenXR/openvr_survivor/blob/master/src/head_mount_display_device.cc
	/*
	double rr = sqrt((fU - 0.5f) * (fU - 0.5f) + (fV - 0.5f) * (fV - 0.5f));
	double r2 = rr * (1 + m_fDistortionK1 * (rr * rr) + m_fDistortionK2 * (rr * rr * rr * rr));
	double theta = atan2(fU - 0.5f, fV - 0.5f);
	double hX = sin(theta) * r2 * m_fZoomWidth;
	double hY = cos(theta) * r2 * m_fZoomHeight;

	coordinates.rfBlue[0] = hX + 0.5f;
	coordinates.rfBlue[1] = hY + 0.5f;
	coordinates.rfGreen[0] = hX + 0.5f;
	coordinates.rfGreen[1] = hY + 0.5f;
	coordinates.rfRed[0] = hX + 0.5f;
	coordinates.rfRed[1] = hY + 0.5f;
	*/

	coordinates.rfBlue[0] = fU;
	coordinates.rfBlue[1] = fV;
	coordinates.rfGreen[0] = fU;
	coordinates.rfGreen[1] = fV;
	coordinates.rfRed[0] = fU;
	coordinates.rfRed[1] = fV;

	DriverLog("Display: ComputeDistortion: B(%f,%f) G(%f,%f) R(%f,%f) \n", coordinates.rfBlue[0], coordinates.rfBlue[1], coordinates.rfGreen[0], coordinates.rfGreen[1], coordinates.rfRed[0], coordinates.rfRed[1]);
	return coordinates;
}

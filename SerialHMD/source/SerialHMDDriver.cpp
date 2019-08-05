#include <SerialHMDDriver.h>
#include <DriverLog.h>

SerialHMDDriver::SerialHMDDriver()
{
	DriverLog("Driver: Init \n");
	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;
	
	// RETRIEVE GENERAL SETTINGS

	char buf[1024];
	vr::VRSettings()->GetString(k_pch_serialhmd_Section, k_pch_serialhmd_SerialNumber_String, buf, sizeof(buf));
	m_sSerialNumber = buf;
	vr::VRSettings()->GetString(k_pch_serialhmd_Section, k_pch_serialhmd_ModelNumber_String, buf, sizeof(buf));
	m_sModelNumber = buf;

	m_flIPD = vr::VRSettings()->GetFloat(k_pch_SteamVR_Section, k_pch_SteamVR_IPD_Float);
	m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat(k_pch_serialhmd_Section, k_pch_serialhmd_SecondsFromVsyncToPhotons_Float);
	m_flDisplayFrequency = vr::VRSettings()->GetFloat(k_pch_serialhmd_Section, k_pch_serialhmd_DisplayFrequency_Float);

	m_bDebugMode = vr::VRSettings()->GetBool(k_pch_serialhmd_Section, k_pch_serialhmd_DebugMode_Bool);
	
	/*DriverLog("Driver: Serial Number: %s\n", m_sSerialNumber.c_str());
	DriverLog("Driver: Model Number: %s\n", m_sModelNumber.c_str());
	DriverLog("Driver: Seconds from Vsync to Photons: %f\n", m_flSecondsFromVsyncToPhotons);
	DriverLog("Driver: Display Frequency: %f\n", m_flDisplayFrequency);
	DriverLog("Driver: IPD: %f\n", m_flIPD);*/

	// RETRIEVE DISPLAY SETTINGS

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


	// SET PROPERTIES

	// Write properties of actual HMD as loaded in the constructor
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str());
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_RenderModelName_String, m_sModelNumber.c_str());
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserIpdMeters_Float, m_flIPD);
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_UserHeadToEyeDepthMeters_Float, 0.02f);
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_DisplayFrequency_Float, m_flDisplayFrequency);
	vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, Prop_SecondsFromVsyncToPhotons_Float, m_flSecondsFromVsyncToPhotons);
	vr::VRProperties()->SetUint64Property(m_ulPropertyContainer, Prop_CurrentUniverseId_Uint64, 2);
	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_IsOnDesktop_Bool, vr::VRSettings()->GetBool(k_pch_serialhmd_Section, "IsOnDesktop"));

	// Debug mode activate Windowed Mode (borderless fullscreen), lock to 30 FPS
	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_DisplayDebugMode_Bool, m_bDebugMode);
	//DriverLog("Driver: Set Prop_DisplayDebugMode_Bool : %s \n", m_bDebugMode? "True" : "False");


	// HIDDEN AREA MESH

	const uint32_t hamVertCount = 12*3;
	float vertRim = 0.05f, outerRim = 0.05f, innerRim = 0.98f, outerWidth = 0.3f, innerWidth = 0.3f, vertOuter = 0.2f, vertInner = 0.8f;
	HmdVector2_t // All inner points defining the outline
		TL = { vertOuter, 0 + vertRim }, LT = { outerRim, 0.5f - outerWidth / 2 },
		TR = { vertInner, 0 + vertRim }, RT = { innerRim, 0.5f - innerWidth / 2 },
		BR = { vertInner, 1 - vertRim }, RB = { innerRim, 0.5f + innerWidth / 2 },
		BL = { vertOuter, 1 - vertRim }, LB = { outerRim, 0.5f + outerWidth / 2 };
	HmdVector2_t hamVertsLeft[hamVertCount] = {
		{ 0.0, 0.0 }, TL, LT, // TL Corner
		{ 1.0, 0.0 }, TR, RT, // TR Corner
		{ 1.0, 1.0 }, BR, RB, // BR Corner
		{ 0.0, 1.0 }, BL, LB, // BL Corner
		{ 0.0, 0.0 }, TL, TR, { 0.0, 0.0 }, { 1.0, 0.0 }, TR, // Top side
		{ 1.0, 0.0 }, RT, RB, { 1.0, 0.0 }, { 1.0, 1.0 }, RB, // Right side
		{ 1.0, 1.0 }, BR, BL, { 1.0, 1.0 }, { 0.0, 1.0 }, BL, // Bottom side
		{ 0.0, 1.0 }, LB, LT, { 0.0, 1.0 }, { 0.0, 0.0 }, LT  // Left side
	};
	HmdVector2_t hamVertsRight[hamVertCount];
	for (int i = 0; i < hamVertCount; i++)
		hamVertsRight[i] = { 1-hamVertsLeft[i].v[0], hamVertsLeft[i].v[1] };
	vr::VRHiddenArea()->SetHiddenArea(Eye_Left, k_eHiddenAreaMesh_Standard, hamVertsLeft, hamVertCount);
	vr::VRHiddenArea()->SetHiddenArea(Eye_Right, k_eHiddenAreaMesh_Standard, hamVertsRight, hamVertCount);


	// INITIALIZE STATE

	m_HMDConnected = false;

	return VRInitError_None;
}

void SerialHMDDriver::Deactivate()
{
	// TODO: Deactivate HMD

	DriverLog("Driver: Deactivating HMD! \n");

	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;
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

/*
bool g_mouseView;
int g_mouseX;
int g_mouseY;
float g_mouseYaw;
float g_mousePitch;

_HMDData UpdateMousePoseData()
{
	float yaw = 0, pitch = 0, roll = 0;

#if defined(_WIN32) // Mouse look

	if (!g_mouseView)
	{
		g_mouseView = true;
		POINT pos;
		if (GetCursorPos(&pos))
		{
			g_mouseX = (int)pos.x;
			g_mouseY = (int)pos.y;
			g_mouseYaw = 0;
			g_mousePitch = 0;
		}
	}
	else 
	{
		POINT pos;
		if (GetCursorPos(&pos))
		{
			g_mouseYaw = g_mouseYaw - (float)(pos.x - g_mouseX)*0.1f;
			if (g_mouseYaw < -360 || g_mouseYaw > 360) g_mouseYaw = (float)fmod(g_mouseYaw, 360);
			g_mousePitch = g_mousePitch - (float)(pos.y - g_mouseY)*0.1f;
			if (g_mousePitch < -80) g_mousePitch = -80;
			if (g_mousePitch > 80) g_mousePitch = 80;

			g_mouseX = (int)pos.x;
			g_mouseY = (int)pos.y;
		}

		yaw = g_mouseYaw;
		pitch = g_mousePitch;
		roll = 0;
	}

#endif

	_HMDData poseData;
	poseData.Yaw = yaw;
	poseData.Pitch = pitch;
	poseData.Roll = roll;
	poseData.X = 0;
	poseData.Y = 0;
	poseData.Z = 180;
	return poseData;
}
*/

void SerialHMDDriver::SetRestPose(bool pos, bool rot) 
{
	if (pos)
	{
		m_restPose.X = 0;
		m_restPose.Y = 0;
		m_restPose.Z = 0;
	}
	if (rot)
	{
		m_restPose.Yaw += m_HMDPose.Yaw;
		m_restPose.Pitch += m_HMDPose.Pitch;
		m_restPose.Roll += m_HMDPose.Roll;
	}
}

void SerialHMDDriver::StreamPacket(std::vector<unsigned char> packet)
{ // Packet including pitch, roll and yaw of HMD head

	if (packet.size() != 12)
	{ // Invalid packet size
		DriverLog("Driver: HMD Stream Packet size received is %d (!= 12)! \n", packet.size());
		return;
	}

	m_HMDPose.Yaw = BytesToFloat(packet[0], packet[1], packet[2], packet[3]) - m_restPose.Yaw;
	m_HMDPose.Pitch = BytesToFloat(packet[4], packet[5], packet[6], packet[7]) - m_restPose.Pitch;
	m_HMDPose.Roll = BytesToFloat(packet[8], packet[9], packet[10], packet[11]) - m_restPose.Roll;

	//memcpy(&m_HMDPose.Yaw, &packet[0], 4);
	//memcpy(&m_HMDPose.Pitch, &packet[4], 4);
	//memcpy(&m_HMDPose.Roll, &packet[8], 4);

	m_HMDPose.X = 0;
	m_HMDPose.Y = 0;
	m_HMDPose.Z = 180;

	UpdatePose();
}

void SerialHMDDriver::UpdatePose()
{
	if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid)
	{
		vr::VRServerDriverHost()->TrackedDevicePoseUpdated(m_unObjectId, CreatePose(m_HMDPose), sizeof(DriverPose_t));
	}
	else
	{
		DriverLog("Driver: Trying to update pose for uninitialized HMD! \n");
	}
}

DriverPose_t SerialHMDDriver::CreatePose(_HMDData poseData)
{
	DriverPose_t pose = { 0 };

	pose.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
	pose.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);

	if (m_HMDConnected)
	{
		// Set state to connected
		pose.poseIsValid = true;
		pose.result = TrackingResult_Running_OK; // TrackingResult_Fallback_RotationOnly
		pose.deviceIsConnected = true;

		// Set HMD rotation
		pose.qRotation.w = cos(DegToRad(poseData.Roll) * 0.5) * cos(DegToRad(poseData.Pitch) * 0.5) * cos(DegToRad(poseData.Yaw) * 0.5) + sin(DegToRad(poseData.Roll) * 0.5) * sin(DegToRad(poseData.Pitch) * 0.5) * sin(DegToRad(poseData.Yaw) * 0.5);
		pose.qRotation.x = cos(DegToRad(poseData.Roll) * 0.5) * sin(DegToRad(poseData.Pitch) * 0.5) * cos(DegToRad(poseData.Yaw) * 0.5) - sin(DegToRad(poseData.Roll) * 0.5) * cos(DegToRad(poseData.Pitch) * 0.5) * sin(DegToRad(poseData.Yaw) * 0.5);
		pose.qRotation.y = cos(DegToRad(poseData.Roll) * 0.5) * cos(DegToRad(poseData.Pitch) * 0.5) * sin(DegToRad(poseData.Yaw) * 0.5) + sin(DegToRad(poseData.Roll) * 0.5) * sin(DegToRad(poseData.Pitch) * 0.5) * cos(DegToRad(poseData.Yaw) * 0.5);
		pose.qRotation.z = sin(DegToRad(poseData.Roll) * 0.5) * cos(DegToRad(poseData.Pitch) * 0.5) * cos(DegToRad(poseData.Yaw) * 0.5) - cos(DegToRad(poseData.Roll) * 0.5) * sin(DegToRad(poseData.Pitch) * 0.5) * sin(DegToRad(poseData.Yaw) * 0.5);

		// Set HMD position
		pose.vecPosition[0] = poseData.X;
		pose.vecPosition[1] = poseData.Y;
		pose.vecPosition[2] = poseData.Z;
	}
	else
	{
		// Set state to disconnected
		pose.poseIsValid = false;
		pose.result = TrackingResult_Uninitialized;
		pose.deviceIsConnected = false;
	}

	return pose;
}

DriverPose_t SerialHMDDriver::GetPose()
{
	return CreatePose(m_HMDPose);
}


// *******************************************************
// Other
// *******************************************************

void *SerialHMDDriver::GetComponent(const char *pchComponentNameAndVersion)
{
	if (!strcmp(pchComponentNameAndVersion, vr::IVRDisplayComponent_Version))
	{
		return (vr::IVRDisplayComponent *)this;
		//DriverLog("Driver: Provided requested component '%s'! \n", pchComponentNameAndVersion);
	}

	//DriverLog("Driver: Requested unimplemented component '%s'! \n", pchComponentNameAndVersion);

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

void SerialHMDDriver::GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight)
{
	//DriverLog("Display: GetWindowBounds (%d, %d)(%d, %d) \n", int(m_nWindowX), int(m_nWindowY), int(m_nWindowWidth), int(m_nWindowHeight));

	*pnX = m_nWindowX;
	*pnY = m_nWindowY;
	*pnWidth = m_nWindowWidth;
	*pnHeight = m_nWindowHeight;
}

bool SerialHMDDriver::IsDisplayOnDesktop()
{
	//DriverLog("Display: IsDisplayOnDesktop: %s \n", m_bOnDesktop? "true" : "false");
	return m_bOnDesktop;
}

bool SerialHMDDriver::IsDisplayRealDisplay()
{
	//DriverLog("Display: IsDisplayRealDisplay: %s \n", m_bRealDisplay? "true" : "false");
	return m_bRealDisplay;
}

void SerialHMDDriver::GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight)
{
	//DriverLog("Display: GetRecommendedRenderTargetSize (%d, %d) \n", int(m_nRenderWidth), int(m_nRenderHeight));

	*pnWidth = m_nRenderWidth;
	*pnHeight = m_nRenderHeight;
}

void SerialHMDDriver::GetProjectionRaw(EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom)
{
	*pfLeft = -1.0;
	*pfRight = 1.0;
	*pfTop = -1.0;
	*pfBottom = 1.0;
	
	//DriverLog("Display: GetProjectionRaw %s (L %f, R %f, T %f, B %f)\n", eEye == Eye_Left? "Left" : "Right", *pfLeft, *pfRight, *pfTop, *pfBottom);
}

void SerialHMDDriver::GetEyeOutputViewport(EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight)
{
	*pnX = eEye == Eye_Left ? 0 : m_nWindowWidth / 2;
	*pnY = 0;
	*pnWidth = m_nWindowWidth / 2;
	*pnHeight = m_nWindowHeight;
	
	//DriverLog("Display: GetEyeOutputViewport %s (%d, %d)(%d, %d) \n", eEye == Eye_Left? "Left" : "Right", int(*pnX), int(*pnY), int(*pnWidth), int(*pnHeight));
}

vr::DistortionCoordinates_t SerialHMDDriver::ComputeDistortion(vr::EVREye eEye, float fU, float fV)
{
	vr::DistortionCoordinates_t coords;

	//distortion for lens from https://github.com/HelenXR/openvr_survivor/blob/master/src/head_mount_display_device.cc
	
	double rr = sqrt((fU - 0.5f) * (fU - 0.5f) + (fV - 0.5f) * (fV - 0.5f));
	double r2 = rr * (1 + m_fDistortionK1 * (rr * rr) + m_fDistortionK2 * (rr * rr * rr * rr));
	double theta = atan2(fU - 0.5f, fV - 0.5f);
	float hX = (float)(sin(theta) * r2 * m_fZoomWidth) + 0.5f;
	float hY = (float)(cos(theta) * r2 * m_fZoomHeight) + 0.5f;

	coords.rfBlue[0] = hX;
	coords.rfBlue[1] = hY;
	coords.rfGreen[0] = hX;
	coords.rfGreen[1] = hY;
	coords.rfRed[0] = hX;
	coords.rfRed[1] = hY;

	return coords;
}

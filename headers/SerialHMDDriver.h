
#ifndef HMDDRIVER_H
#define HMDDRIVER_H

#include <SerialHMDShared.h>

#include <math.h>
#include <string.h>

// *******************************************************
// Tracked SerialHMD Device
// *******************************************************

class SerialHMDDriver : public vr::ITrackedDeviceServerDriver, public vr::IVRDisplayComponent
{
  public:
	SerialHMDDriver();
	virtual ~SerialHMDDriver();

	// HMD Lifetime

	virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId);
	virtual void Deactivate();
	virtual void EnterStandby();
	virtual void PowerOff();

	// Pose and Tracking

	virtual DriverPose_t GetPose();
	void RunFrame();

	// Other

	void *GetComponent(const char *pchComponentNameAndVersion);
	virtual void DebugRequest(const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize);

	std::string GetSerialNumber() const { return m_sSerialNumber; }
	std::string GetModelNumber() const { return m_sModelNumber; }

	// Display Methods

	virtual void GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight);

	virtual bool IsDisplayOnDesktop();
	virtual bool IsDisplayRealDisplay();

	virtual void GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight);
	virtual void GetEyeOutputViewport(EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight);

	virtual void GetProjectionRaw(EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom);
	virtual DistortionCoordinates_t ComputeDistortion(EVREye eEye, float fU, float fV);

  private:
	vr::TrackedDeviceIndex_t m_unObjectId;
	vr::PropertyContainerHandle_t m_ulPropertyContainer;

	_HMDData m_HMD;

	std::string m_sSerialNumber;
	std::string m_sModelNumber;
	
	float m_flSecondsFromVsyncToPhotons;
	float m_flDisplayFrequency;
	float m_flIPD;
	bool m_bDebugMode;
	
	int32_t m_nWindowX;
	int32_t m_nWindowY;
	int32_t m_nWindowWidth;
	int32_t m_nWindowHeight;
	int32_t m_nRenderWidth;
	int32_t m_nRenderHeight;
	float m_fDistortionK1;
	float m_fDistortionK2;
	float m_fZoomWidth;
	float m_fZoomHeight;
	bool m_bRealDisplay;
	bool m_bOnDesktop;
};

#endif // HMDDRIVER_H
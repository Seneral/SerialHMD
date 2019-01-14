# SerialHMD
DIY VR driver based on OpenVR. Communicates with a Serial-Enabled RPiZ

# Structure
openvr_driver : HmdDriverFactory -- Entry Point for SteamVR  
SerialHMDWatchdog : IVRWatchdogProvider -- AutoStart OpenVR once HMD has been detected  
SerialHMDServer : IServerTrackedDeviceProvider -- Spawns Tracked Devices  
SerialHMDDriver : ITrackedDeviceServerDriver, IVRDisplayComponent -- HMD interface  

SerialHMDShared -- Shared Constants, Functions, ...  
DriverLog -- Default OpenVR Sample Driver Log  
WinSerialDeviceUtility -- Serial Device Detection (Windows)  
SerialPort -- Serial Communication Wrapper  

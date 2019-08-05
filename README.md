# SerialHMD
DIY VR driver based on OpenVR. Communicates with a Serial-Enabled RPiZ  
Motivation: There's really not a lot of examples out there (and if they are, they are overly complex, abstract and hard to figure out). So this is mainly a resource. Haven't been able to perfect it yet, and not worked on it for months, so I'm not even sure it works with the latest version.

## Structure
driver_serialhmd: Last compiled driver  
SerialHMD: Sources for the Steam Driver  
SerialRPi: Scripts on RPi including necessary parts of RTIMULib2  
SerialHMD_EmulatorWin: Command line tool to test the serial communication in a controlled environment  

### Driver
driver_serialhmd : HmdDriverFactory -- Entry Point for SteamVR  
SerialHMDWatchdog : IVRWatchdogProvider -- AutoStart OpenVR once HMD has been detected  
SerialHMDServer : IServerTrackedDeviceProvider -- Spawns Tracked Devices  
SerialHMDDriver : ITrackedDeviceServerDriver, IVRDisplayComponent -- HMD interface  

SerialHMDShared -- Shared Constants, Functions, ...  
SerialHMDComm -- Serial Communications  
DriverLog -- Default OpenVR Sample Driver Log  
WinSerialDeviceUtility -- Serial Device Detection (Windows)  
SerialPort -- Serial Communication Wrapper  

## Current State
**Serial Communication:** Reliable and extendable to support future plans  
**Headtracking:** works (see issues regarding accuracy) - fallback mouse (uncomment code)  
**Controller:** No support yet  

## Setup
**Driver Compilation:** DON'T use gcc/g++ on Windows, use MSVC (either bare or with Visual Studio). Then put the dll's in driver_serialhmd/bin  
**Driver Setup:** Just set it up accordign to SteamVR driver's instruction (vrpathreg adddriver "path/driver_serialhmd")  
Here's a link to their driver docs: https://github.com/ValveSoftware/openvr/wiki/Driver-Documentation  
**RPI Zero Setup:** See RPI_Setup.txt for details  

## Current Problems and possible fixes
It is working, but the VR experience itself is limited due to tracking accuracy. After fast movements especially, the roll axis sways alot and takes around 0.5s to reorient itself. This is to my knowledge because of two (relatively easily) fixable problems of my setup:  
1. I chose to use the RPiZero built-in USB port for communication (output ttyGS0) without realizing it is software emulated. I assume it is slowing down the system alot and limits the refresh rate of the sensor data. Using an proper UART module is probably the way to go.  
2. I chose the MPU-9250, without realizing it is a lower-power version of the MPU-9150 (see https://sureshjoshi.com/embedded/invensense-imus-what-to-know/). Upgrading to the MPU-9150 would probably improve the accuracy alot.
If you are trying to reproduce results, these two modifications are relatively easy to make.  

## SerialHMD Hardware
**HMD Base:** Zeiss VR One Plus - it is perfect in many ways: Perfect frame size to house all electronics perfectly hidden, it's comfortable even with all the electronics and it's lenses are spectacular.  
The only problem I have with it is that while it supports 4.7"-5.5" screens, you'll find you'll only see a part of a 5.5" screen (even though it perfectly fits the outline).
At least 5% on each side is completely useless, that is considering really trying to see them (looking in opposite direction to ge the most 'leverage'). Putting the screen farther away obviously doesn't work because of the fixed focus.  
**Screen:** In light of above considerations, rather take a 2k 4.7" or to be safe 5" screen over a 5.5" screen if you can get them (although 5.5" seems to be the standard for readily available 2k screens). 2k or more is highly recommended. I got a 2k 5.5" screen from aliexpress for around 77€ including shipping to europe.  
**IMU sensor:** Definitely a MPU-9150. I got a MPU-9250, a smaller version, but it's accuracy is not too impressive.  
**Processor:** Raspberry Pi Zero. I recommend you use the W version, it solves a LOT of headaches if you don't have indepth knowledge of RPis already. And it's just cooler when your HMD has wifi, isn't it?
As far as I can judge so far (considering I got a less-than perfect experience due to other problems) it is perfectly capable of the job.  
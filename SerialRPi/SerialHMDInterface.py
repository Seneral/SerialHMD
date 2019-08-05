#!/usr/bin/env python

import serial
import RTIMU
import time
import math

# CONSTANTS

HMD_IMU_SETTINGS = "RTIMULib"
ID = "SerialHMD-DIY"
NL = "\n"
LOG_FILE = "/home/pi/log_SHMD_Comm.txt"

# PARAMS

port = "ttyGS0" #ttyACM0 for UART
if len(sys.argv) > 1:
    port = sys.argv[1]

# GLOBAL VARIABLES

hmdPoseChannel = -1
serData = bytes(0)
pollInterval = 0.1


# SERIAL SETUP

ser = serial.Serial(
    port='/dev/' + port, 
    baudrate=9600,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=1)
ser.flush()

# RTIMU SETUP

hmdSet = RTIMU.Settings(HMD_IMU_SETTINGS)
hmdIMU = RTIMU.RTIMU(hmdSet)
hmdIMUInit = imu.IMUInit()
if (hmdIMUInit):
    hmdIMU.setSlerpPower(0.02)
    hmdIMU.setGyroEnable(True)
    hmdIMU.setAccelEnable(True)
    hmdIMU.setCompassEnable(True)
    pollInterval = min(pollIntervall, hmdIMU.IMUGetPollInterval())

# LOG STARTUP

with open(LOG_FILE,"w") as log:
    log.write(ID + NL)
    log.write("Port: '/dev/" + port + "':'" + NL)
    if (hmdIMUInit):    
        log.write("HMD IMU: " + hmdIMU.IMUName() + NL)
    else:
        log.write("HMD IMU not available! " + NL)
    log.write("Poll Interval: " + pollInterval + "ms!" + NL)

# COMMUNICATION LOOP

while True:

    # ---- Send out data streams ----

    if (hmdIMUInit and hmdDataStream >= 0):
        
        while (not hmdIMU.IMURead()):
            time.sleep(pollInterval/1000.0/100.0)
            
        #hmdData = hmdIMU.getIMUData()
        #hmdPose = hmdData["fusionPose"]
        #print("r: %f p: %f y: %f" % (math.degrees(hmdPose[0]), math.degrees(hmdPose[1]), math.degrees(hmdPose[2])))

        r, p, y = hmdIMU.getFusionData()
        yaw = struct.pack('f', math.degrees(y))
        pitch = struct.pack('f', math.degrees(p))
        roll = struct.pack('f', math.degrees(r))
        ser.write(b"".join([hmdDataStream, yaw, pitch, roll]))

    # ---- Process incoming messages as commands ----

    # Process data in buffer only if new data arrived		
    if (ser.in_waiting > 0):
        
        # Add new data to potentially existing buffer
        serData = serData + ser.read(ser.in_waiting)
        
        # Log only in data processing
        with open(LOG_FILE,"a") as log:

            # Loop through all commands found in incoming data
            while (serData.len() > 0):

                # Find start of command sequence and dispose of potentially non-command bytes
                while (serData.len() > 0 and serData[0] != bytes('#', 'UTF-8')):
                    serData = serData[1:]
                
                # Found command, but not full command
                if (serData.len() < 3):
                    break
                
                # Extract command ID
                id = serData[1].decode('UTF-8')

                # End of command: without data ( #ID# )
                if (serData[2] == bytes('#', 'UTF-8')):

                    log.write("<- '#" + id.decode('UTF-8') + "#'" + NL)
                    # Identifiation request
                    if (id == bytes('I', 'UTF-8')):
                        log.write("Answering Identification Request..." + NL)
                        ser.write(("#I*" + ID + "#").encode("utf-8"))
                    # Sources request
                    elif (id == bytes('S', 'UTF-8')):
                        log.write("Answering Sources Request..." + NL)
                        sources = "";
                        if (hmdPoseChannel < 0 and hmdIMUInit):
                            sources += "H"
                        ser.write(("#S*" + sources + "#").encode("utf-8"))
                    # Unknown command
                    else:
                        log.write("ERROR: Unknown Command '" + id.decode('UTF-8') + "'!" + NL)
                        ser.write("#E#")
                    # Remove processed command from buffer
                    serData = serData[3:]
                
                # Data indicator: with 1 byte of data ( #ID*D# )
                elif (serData[2] == bytes('*', 'UTF-8') and serData[4] == bytes('#', 'UTF-8')):
                    # Start of data command, but not full command
                    if (serData.len() < 5):
                           break
                    # This always indicates a request to open a new channel of type ID in channel D
                    channel = serData[3];
                    log.write("<- '#" + id.decode('UTF-8') + "*" + str(channel) + "#'" + NL)
                    # HMD Pose stream requested
                    if (id == bytes('H', 'UTF-8')):
                        if (hmdDataStream >= 0):
                            log.write("ERROR: Trying to re-assign HMD Pose Stream!" + NL)
                            ser.write("#E#")
                        elif (not hmdIMUInit):
                            log.write("ERROR: Requested unavailable HMD Pose Stream!" + NL)
                            ser.write("#E#")
                        else:
                            log.write("Answering Head Pose Stream Assign Request..." + NL)
                            ser.write(("#H*" + channel + "#").encode("utf-8"))
                            headDataStream = channel
                    # Controller stream requested
                    if (id == bytes('C', 'UTF-8')):
                        log.write("Ignoring request to assign Controller Stream..." + NL)
                        ser.write("#E#")
                
                # Unknown message format, ID is two bytes long
                else:
                    log.write("ERROR: Unknown message format!" + NL)
                    ser.write("#E#")
            
            # End while message processing
    
    # Potentially sleep, wait for new data (needs testing)
    time.sleep(pollInterval/1000.0/2)
    #time.sleep(0.001)

# End loop - should not happen
ser.close()


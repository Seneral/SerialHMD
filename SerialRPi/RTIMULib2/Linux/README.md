# RTIMULib for Linux

This directory contains the applications for embedded Linux systems such as the BeagleBone, Raspberry Pi, and Intel Edison. This description assumes that the Edison image was built using the meta-edison-rt layer, available on the GitHub repo.

The RTIMULibvrpn demo app, which shows how to integrate RTIMULib with vrpn, has its own build and run instructions in a README.md within the RTIMULibvrpn directory.

### Setting up the Raspberry Pi

#### Connecting the IMU

The easiest way to connect the IMU to the Raspberry Pi is to use something like the Adafruit Pi Plate (http://www.adafruit.com/products/801) as it makes it obvious where the I2C bus 1 pins are and where to pick up 3.3V. Basically, you need to connect the I2C SDA, I2C SCL, 3.3V and GND to the IMU breakout board you are using. Take care with these connections or else disaster may follow!

#### Enabling and Configuring the I2C Bus

Add the following two lines to /etc/modules:

	i2c-bcm2708
	i2c-dev

Then, comment out the following line in /etc/modprobe.d/raspi-blacklist.conf:

	# blacklist i2c-bcm2708

Restart the Raspberry Pi and /dev/i2c-0 and /dev/i2c-1 should appear.

Another thing worth doing on Raspberry Pi is to change the I2C bus speed to 400KHz. Add the following line to /boot/config.txt:

	dtparam=i2c1_baudrate=400000

Simplest thing is then to reboot to enable this change.

The I2C bus should now be ready for operation.

### Setting up the BeagleBone

#### Connecting the IMU

For BeagleBone Green and the Grove IMU board, use the provided cable(s) and connect the IMU board to either the I2C extender or the the I2C connector on the BBG.  If you also have one of the the Grove DHT humidity sensors, use another cable to connect it to the Grove UART connector on the BBG.  Note that RTIMULib only supports a few I2C humidity sensors, so does not currently work with the DHT sensor (you will need the AdaFruit DHT library and include this in your RTIMULib python code to read data from a DHT).  See the py-sensor-test (https://github.com/VCTLabs/py-sensor-test) repo for an example script.

#### Enabling and Configuring the I2C Bus

With either BeagleBone White/Black or Green, start with the default device tree for your board, make sure you have a kernel with cape-manager support, and the beaglebone-universal-io package installed (for the config-pin tool).  The latest 4GB bone-debian-IoT images have all of this ready to go; see the BeagleBoard image page for download details (http://beagleboard.org/latest-images).  You should see the I2C /dev entries below, two on BeagleBone White/Black, or three of them on BeagleBone Green.

If not, check which DT overlays may be enabled in /boot/uEnv.txt, and comment them out for now if enabled.  You can see what is currently loaded with the following command:

    $ cat /sys/devices/platform/bone_capemgr/slots

The default output on BeagleBone Green (ie, with nothing special enabled in /boot/uEnv.txt or loaded by hand) should look like this:

    0: PF----  -1 
    1: PF----  -1 
    2: PF----  -1 
    3: PF----  -1 
    4: P-O-L-   0 Override Board Name,00A0,Override Manuf,univ-emmc


### Setting up the Intel Edison

It's necessary to add an extra directory to ldconfig. Create /etc/ld.so.conf and add the line:

    /usr/local/lib
    
Then run ldconfig so that the RTIMULib libraries will be found.

### Continue I2C Configuration (all embedded platforms)

For initial checkout and configuration, install the I2C tools if not already installed:

	$ sudo apt-get install i2c-tools

Note that different machines and hardware capes/hats/other can change both the number of availaable I2C buses and which one is connected to which GPIO pins (or physical connector).  Both BeagleBone White/Black and Raspberry Pi typically have 2 I2C ports by default, whereas BeagleBone Green has a third I2C port attached to the Grove I2C connector on the board.  This means you need to find the right bus number for your particular hardware and device tree setup.  First make sure any required modules are loaded (as above) then check your /dev directory for I2C bus devices:

    $ ls -l /dev/i2c-*

You should see at least 2 of them, numbered starting at zero:

    crw-rw---- 1 root i2c 89, 0 Jul  6 19:22 /dev/i2c-0
    crw-rw---- 1 root i2c 89, 1 Jul  6 19:56 /dev/i2c-1
    crw-rw---- 1 root i2c 89, 2 Jul  6 19:22 /dev/i2c-2

The above listing is for a BeagleBone Green with the extra Grove I2C connector.  For BeagleBone Black or Raspberry Pi you should see two, start probing with i2cdetect using the higher-numbered device number, ie, replace <bus_num> with "2" or "1":

	sudo i2cdetect -y <bus_num>

If you get this error: "Error: Can't use SMBus Quick Write command on this bus" from the above command, then add "-r" to use direct read instead:

    $ sudo i2cdetect -y -r <bus_num>

will detect any devices on /dev/i2c-<bus_num>.  The default output for the Grove IMU board on BeagleBone Green for /dev/irc-2 should look like this:

         0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
    10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
    20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
    30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
    40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
    50: -- -- -- -- UU UU UU UU -- -- -- -- -- -- -- -- 
    60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- -- 
    70: -- -- -- -- -- -- -- 77                         

The "--" means no I2C device present, the "UU" means those ID's are reserved and in use by something else (in this case the BeagleBone cape-manager uses these), and a value means there is a device with that ID; for the Grove Board, the MPU-9250 is the device at 0x68 and the BMP-180 pressure sensor is the device at 0x77.

If you have the MPU-9150 or MPU-6050 wired up, you should also see it at address 0x68. This is the default address for the devices, and is also expected by the demo programs. If the ID has been reset to 0x69, the address expected by the demo programs will need to be changed (there's a settings file for doing things like that so it's easy to do).  The same applies to the I2C bus number; the demo programs expect to read from sensors attached to the **second** I2C bus, i.e., i2c-1, so if you have them attached to either i2c-0 or i2c-2 you will also need to change this in settings file (see the note below).

By default, the Raspberry Pi I2C devices are owned by root. To fix this, reate a file /etc/udev/rules.d/90-i2c.rules and add the line:

	KERNEL=="i2c-[0-7]",MODE="0666"

Use the following command to make udev re-read the rules:

    $ sudo udevadm control --reload

On BeagleBone the I2C devices are owned by root but the group is i2c, so instead you should make sure your user account is in the i2c group and you should be good to go.  If nothing else, try the same command with sudo; if it works, go back and see what permissions should be updated.

The RTIMULib2 build setup requires that cmake is installed, so on BeagleBone or Raspberry Pi enter:

    $ sudo apt-get install cmake

If the Qt-based GUI programs are to be compiled, also install Qt:

	$ sudo apt-get install libqt4-dev

**Note:** As mentioned above, the default expected I2C bus is i2c-1, so the calibration program will fail if the iI2C bus is 0 or 2.  Just let it fail (or Ctl-C it) and then edit the resulting .ini file created in the current directory.  With this particular package settings file, less is more, so you only need to edit the minimum (and you *do* want to leave auto-discover enabled).  So if your I2C bus is different, edit that one item.  If you don't have one of the currently supported pressure or humidity sensors, then set those to NULL, otherwise leave the other settings as-is.  For the BeagleBone Green with Grove sensors example, the BMP-180 is supported, but the DHT humidity sensors are not, so I set the following options:

    IMUType=7         #   7 = InvenSense MPU-9250
    I2CBus=2
    HumidityType=1    #   1 = Null (no hardware or don't use)

### Build using cmake

Go to the directory where RTIMULib was cloned and enter:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make -j4
    $ sudo make install
    $ sudo ldconfig
    
This will build and install all of the libraries and demo programs. Note that the Intel Edison does not need the "sudo" since the default user is root. The "-j4" part indicates how many cores should be used and will considerably speed up the build on the Pi2 and Edison.

### Build using make

Makefiles are provided for RTIMULibCal, RTIMULibDrive, RTIMULibDrive10 and RTIMULibDrive11. To build, navigate to the directory for the app and enter:

    $ make -j4
    $ sudo make install

### Build using qmake (BeagleBone and Raspberry Pi only)

.pro files are included for RTIMULibDemo and RTIMULibDemoGL. To build using qmake, navigate to the RTIMULibDemo or RTIMULibDemoGL directory and enter:

    $ qmake
    $ make -j4
    $ sudo make install
    
The .pro files can also be used with QtCreator if desired.

### Run the RTIMULibCal Application

RTIMULibCal will either add calibration data to an existing RTIMULib.ini or else create a new one with the calibration data. RTIMULib.ini is *always* used/created in the working directory, so you need to be in the right place or copy your favorite *.ini file to where you want it.  Note that many of the initial tools are more-or-less hard-coded to the repo structure in the top-level README file, however, once everything is built and installed, you **still** need to have your chosen *.ini file in the working directory for most tasks (or be prepared to start from scratch and let it create a new one).

To run the RTIMULibCal application and perform full calibration, you need to first change to the top-level Linux source directory, since it will look for the Octave files in a parallel directory:

    cd src/RTIMULib2/Linux/

If magnetometer ellipsoid fit isn't required, RTIMULibCal can be run anywhere. If ellipsoid fit is required, then the program assumes that the RTEllipsoidFit directory is at the same level as the working directory so that "../RTEllipsoidFit" refers to the directory holding the RTEllipsoidFit.m octave program. If not, ellipsoid fitting will fail. Note - ellipsoid fit is not supported on the Intel Edison.

The normal process is to run the magnetometer min/max option followed by the magnetometer ellipsoid fit option followed finally by the accelerometer min/max option. The program is self-documenting in that the instructions for every option will be displayed when the option is selected.

The resulting RTIMULib.ini can then be used by any other RTIMULib application.

### Run the RTIMULibDrive, RTIMULibDrive10 and RTIMULibDrive11 Demo Apps

RTIMULibDrive is a simple command line program that shows how simple it is to use RTIMULib. RTIMULibDrive10 extends this to also support 10-dof IMUs with pressure/temperature sensors. RTIMULibDrive11 adds humidity sensor support to RTIMULibDrive10.

You should be able to run the program just by entering RTIMULibDrive(10/11). It will try to auto detect the connected IMU If all is well, you should see a line showing the sample rate and the current Euler angles. This is updated 10 times per second, regardless of the sensor sample rate. By default, the driver runs at 50 samples per second in most cases. So, you should see the sample rate indicating around 50 samples per second. The sample rate can be changed by editing the .ini file entry for the appropriate IMU type.

The displayed pose shows the roll, pitch and yaw seen by the IMU. Using an aircraft analogy, the roll axis points from the pilot towards the nose, the pitch axis points from the pilot along the right wing and the yaw axis points from the pilot down towards the ground. Right wing down is a positive roll, nose up is a positive pitch and clockwise rotation is a positive yaw.

Various parameters can be changed by editing the RTIMULib.ini file. These are described later.

Take a look at RTIMULibDrive.cpp. Quite a few of the code lines are just to calculate rates and display outputs!

### Run the RTIMULibDemo and RTIMULibDemoGL Programs (Raspberry Pi only)

To run the program, the Raspberry Pi (or BeagleBone) needs to be running the desktop. To do this (if it isn't already), enter:

	$ startx

Then open a terminal window and enter:

	$ RTIMULibDemo

You should see the GUI pop up and, if everything is ok, it will start displaying data from the IMU and the output of the Kalman filter. If the MPU9150 is at the alternate address, you'll need to edit the RTIMULib.ini file that RTIMULibDemo generated and restart the program.

To calibrate the compass, click on the "Calibrate compass" tab. A new dialog will pop up showing the maximum and minimum readings seen from the magnetometers. You need to waggle the IMU around, ensuring that each axis (roll, pitch and yaw) point straight down and also straight up at some point. You need to do this in an area clear of magnetic fields otherwise the results will be distorted. Eventually, the readings will stop changing meaning that the real max and min values have been obtained. Click on "Ok" to save the values to the RTIMULib.ini file. Provided this .ini file is used in future (it just has to be in the current directory when RTIMULibDemo is run), the calibration will not have to be repeated. Now that RTIMULibDemo is using calibrated magnetometers, the yaw should be much more reliable.

The .ini file created by RTIMULibDemo can also be used by RTIMULibDrive - just run RTIMULibDrive in the same directory and it will pick up the compass calibration data.

Running RTIMULibDemoGL is exactly the same except that it takes place in the RTIMULibDemoGL directory.


#! /bin/bash

echo "Startup SerialHMD... "

cd /home/pi

# dmesgOut="dmesg.txt"
# echo "Writing dmesg to $dmesgOut"

# echo " " > "$dmesgOut"
# echo "dmesg | grep tty" >> "$dmesgOut"
# echo " " >> "$dmesgOut"
# dmesg | grep tty >> "$dmesgOut"
# echo " " >> "$dmesgOut"
# echo " " >> "$dmesgOut"
# echo "dmesg | grep serial" >> "$dmesgOut"
# echo " " >> "$dmesgOut"
# dmesg | grep serial >> "$dmesgOut"

# usbOut="usb.txt"
# echo "Writing usb and dev to $usbOut"

# echo " " > "$usbOut"
# echo "lsusb -t" >> "$usbOut"
# echo " " >> "$usbOut"
# lsusb -t >> "$usbOut"
# echo " " >> "$usbOut"
# echo " " >> "$usbOut"
# echo "lsusb" >> "$usbOut"
# echo " " >> "$usbOut"
# lsusb >> "$usbOut"
# echo " " >> "$usbOut"
# echo " " >> "$usbOut"
# echo "ls /dev/*USB*" >> "$usbOut"
# echo " " >> "$usbOut"
# ls /dev/*USB* >> "$usbOut"
# echo " " >> "$usbOut"
# echo " " >> "$usbOut"
# echo "ls /dev/tty*" >> "$usbOut"
# echo " " >> "$usbOut"
# ls /dev/tty* >> "$usbOut"


logSHMD="log_SHMD_Errors.txt"
sudo python SerialHMDInterface.py ttyGS0 > "$logSHMD" 2>&1
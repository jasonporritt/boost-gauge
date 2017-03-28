# Boost Gauge

This is the code for a gauge built using a Beaglebine, 128x128 SPI-based OLED display, and Neopixel ring. WIP.

## Beaglebone setup

Used Debian Wheezy (kernel 3.8.13-bone50)

Write image to SD card:

```
sudo diskutil unmount /dev/disk3s1
sudo dd bs=1m if=~/Downloads/bone-debian-8.7-iot-armhf-2017-03-19-4gb.img of=/dev/rdisk3
```

### LEDs

clone PixelBone
copy the dtbo to the firmware directory

```
sudo cp dirtrees/BB-PRU-00A0.dtbo /lib/firmware/
```

Try loading the dtbo:

```
echo BB-PRU:00A0 > /sys/devices/platform/bone_capemgr/slots
```



The modules file should look like this:
```
root@beaglebone:~# cat /etc/modules
uio_pruss
fbtft_device
```

### Links

* LCD
  * https://www.element14.com/community/community/designcenter/single-board-computers/next-gen_beaglebone/blog/2015/11/18/beaglebone-black-lcds-with-prebuilt-fbtft-drivers
* LEDs
  * https://github.com/toroidal-code/PixelBone

## Compiling

On Mac, compile using:

```
clang++ -std=c++11 jason-write.cc -o jason-write `pkg-config --cflags --libs cairomm-1.0`
```

On Linux, compile using

```
g++-4.9 -std=c++11 jason-write.cc -o jason-write `pkg-config --cflags --libs cairomm-1.0`
```

## CAN bus

root@beaglebone:~/can-utils# ip link set can0 up type can bitrate 500000
root@beaglebone:~/can-utils# ifconfig can0 up

root@beaglebone:~# echo BB-DCAN1 > /sys/devices/bone_capemgr.*/slots
root@beaglebone:~# modprobe can
root@beaglebone:~# modprobe vcan
root@beaglebone:~# modprobe can-dev
root@beaglebone:~# modprobe can-raw


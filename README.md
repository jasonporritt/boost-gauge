# Boost Gauge

This is the code for a gauge built using a Beaglebine, 128x128 SPI-based OLED display, and Neopixel ring. WIP.


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


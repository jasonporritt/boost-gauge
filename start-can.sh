echo BB-DCAN1 > /sys/devices/bone_capemgr.*/slots
sudo modprobe can
sudo modprobe can-dev
sudo modprobe can-raw
sudo modprobe can-isotp
sudo ip link set can0 up type can bitrate 500000
sudo ifconfig can0 up

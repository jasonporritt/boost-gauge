echo BB-DCAN1 > /sys/devices/bone_capemgr.*/slots
sudo modprobe can
sudo modprobe can-dev
sudo modprobe can-raw
sudo modprobe can-isotp
sudo ip link set can0 type can bitrate 500000 loopback off listen-only off triple-sampling on one-shot off berr-reporting on fd off
sudo ip link set can0 up

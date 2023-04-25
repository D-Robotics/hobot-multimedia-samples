#!/bin/sh

export LD_LIBRARY_PATH=./lib/:$LD_LIBRARY_PATH
echo 118 > /sys/class/gpio/export 
echo out >/sys/class/gpio/gpio118/direction 
echo 0 > /sys/class/gpio/gpio118/value
sleep 0.2
echo 1 > /sys/class/gpio/gpio118/value
echo 1 > /sys/class/vps/mipi_host1/param/snrclk_en
echo 24000000 > /sys/class/vps/mipi_host1/param/snrclk_freq
echo 1 > /sys/class/vps/mipi_host0/param/snrclk_en
echo 24000000 > /sys/class/vps/mipi_host0/param/snrclk_freq
#echo 0xc0020000 > /sys/bus/platform/drivers/ddr_monitor/axibus_ctrl/all
#echo 0x03120000 > /sys/bus/platform/drivers/ddr_monitor/read_qos_ctrl/all
#echo 0x03120000 > /sys/bus/platform/drivers/ddr_monitor/write_qos_ctrl/all
echo 0x00100000 > /sys/bus/platform/drivers/ddr_monitor/axibus_ctrl/all
echo 0x04032211 > /sys/bus/platform/drivers/ddr_monitor/read_qos_ctrl/all
echo 0x04032211 > /sys/bus/platform/drivers/ddr_monitor/write_qos_ctrl/all
devmem 0xa1000044 32 0x1010 //pll
devmem 0xa1000144 32 0x11 //sif mclk
devmem 0xa4000038 32 0x10100000 //axibus_ctrl
devmem 0xA2D10000 32 0x04032211 //read_qos_ctrl
devmem 0xA2D10004 32 0x04032211 //write_qos_ctrl
echo performance > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
echo 1 > /sys/class/vps/mipi_host1/param/stop_check_instart

mount -o remount rw /
cp ./libimx415.so /lib/sensorlib/
service adbd stop
chmod -R 777 sample_usb_cam
chmod -R 777 usb-gadget.sh
/etc/init.d/usb-gadget.sh stop
./usb-gadget.sh start uvc
./sample_usb_cam
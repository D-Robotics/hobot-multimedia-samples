#!/bin/sh
# init-usb-gadget.sh
# sysvinit script

# Using the lsb functions to perform the operations.
. /etc/init.d/functions

# !!IMPORTANT!! include user config for usb-gadget
# source /etc/init.d/.usb/.default-config

# local definitions
SERVICE=usb-gadget
LOCKFILE=/var/run/usb-gadget.lock
DEBUG=true

# gadget default info
SERIAL=0123456789ABCD
MANUF=hobot
PRODUCT=xj3
USE_UVC=false;
USE_HID=false;
USE_UAC1=false;
USE_UAC2=false;
USE_ADB=false;
USE_MSD=false;
USE_ACM=false;
USE_RNDIS=false;
USE_ECM=false;
IS_UVC_ISOC=false;
IS_MULTI_FUNC=false;
USE_MULT_ALTSETTING=false;

# configfs related
USB_GROUP=g_comp
CONFIGFS=/sys/kernel/config
GADGET=$CONFIGFS/usb_gadget
USB_CONFIGFS=$GADGET/$USB_GROUP
FUNCTIONS_DIR=$USB_CONFIGFS/functions
USER_CONFIG=.default-config

BOARD=$(strings /proc/device-tree/model)

case $BOARD in
*)
    UDC=`ls /sys/class/udc` # will identify the 'first' UDC
    UDC_ROLE=/dev/null # Not generic
    ;;
esac

echo "Detecting platform:"
echo " board : $BOARD"
echo " udc   : $UDC"

# Overlay serial with hrut_socuid, hrut_duid, emmc serial number according to priority
SERIAL=$(hrut_duid)         # use hrut_duid as serial number

if [ -z $SERIAL ]; then
    SERIAL=$(hrut_socuid)       # use hrut_socuid as serial number

    if [ -n "$SERIAL" ]; then
        SERIAL=${SERIAL:0-30}       # use socuid last 120 bit data as in dafault
    fi

    if [ -z $SERIAL ]; then
        SERIAL=$(cat /sys/devices/platform/soc/a5010000.dwmmc/mmc_host/mmc0/mmc0:0001/serial)

        if [ -z $SERIAL ]; then
            SERIAL=0123456789ABCD   # Use fixed serial number for other cases(eg. Nand Boot without emmc)
        fi
    fi
fi

# make sure .usb-config is exist!
if [ ! -d /etc/init.d/.usb ]; then
    echo "No .usb directory, break"
    exit 3
fi

is_usb_gadget()
{
    # 1: false
    # 0: true
    usb_mode=$(cat /sys/devices/platform/soc/b2000000.usb/$UDC/role)
    if [ -z $usb_mode ]; then
        return 1;
    fi

    if [ $usb_mode == "device" ]; then
        return 0;
    fi

    return 1;
}

show_user_config()
{
    if $DEBUG; then
        echo "show user config:"
        echo "USB_VID: $USB_VID"
        echo "USB_PID: $USB_PID"
        echo "SERIAL: $SERIAL"
        echo "MANUF: $MANUF"
        echo "PRODUCT: $PRODUCT"
        echo "USE_UVC: $USE_UVC"
        echo "USE_HID: $USE_HID"
        echo "USE_UAC1: $USE_UAC1"
        echo "USE_UAC2: $USE_UAC2"
        echo "USE_ADB: $USE_ADB"
        echo "USE_MSD: $USE_MSD"
        echo "USE_ACM: $USE_ACM"
        echo "USE_RNDIS: $USE_RNDIS"
        echo "USE_ECM: $USE_ECM"
    fi
}

configfs_init()
{
    echo "Setting Vendor and Product ID's"
    echo $USB_VID > idVendor
    echo $USB_PID > idProduct
    echo "OK"

    if $USE_ADB; then
        # force usb2.0 for adb temporarily
        echo 0x0210 > bcdUSB
        echo "high-speed" > max_speed
    fi

    if $USE_RNDIS; then
        echo 0100 > bcdDevice   # v1.0.0, otherwise win10 rndis couldn't install
    fi

    ### Below For Multi Function Gadget Special Setting For Windows ###
    # These code values also alert versions of Windows that 
    # do not support IADs to install a special-purpose bus driver 
    # that correctly enumerates the device. Without these codes 
    # in the device descriptor, the system might fail to enumerate the device, 
    # or the device might not work properly.
    # Some reference url as below:
    #  https://irq5.io/2016/12/22/raspberry-pi-zero-as-multiple-usb-gadgets
    #  https://gist.github.com/geekman/5bdb5abdc9ec6ac91d5646de0c0c60c4

    if $IS_MULTI_FUNC; then
        echo "Setting Multi-func Gadget for Windows"
        echo 0xEF > bDeviceClass
        echo 0x02 > bDeviceSubClass
        echo 0x01 > bDeviceProtocol
    elif $USE_UAC1; then
        echo 0xEF > bDeviceClass
        echo 0x02 > bDeviceSubClass
        echo 0x01 > bDeviceProtocol
    else
        echo "single function gadget"
        if $USE_RNDIS; then
            echo 0x02 > bDeviceClass        # same with legacy/ether.c
        fi
    fi

    echo "Setting English strings"
    mkdir -p strings/0x409
    echo $SERIAL > strings/0x409/serialnumber
    echo $MANUF > strings/0x409/manufacturer
    echo $PRODUCT > strings/0x409/product
    echo "OK"

    echo "Creating Config"
    mkdir configs/c.1
    mkdir configs/c.1/strings/0x409
    echo "Conf 1" > configs/c.1/strings/0x409/configuration
    echo 0xC0 > configs/c.1/bmAttributes
    echo 0x01 > configs/c.1/MaxPower
}

configfs_cleanup()
{
    echo "configfs cleanup"
    rmdir configs/c.1/strings/0x409
    rmdir configs/c.1

    rmdir strings/0x409
}

function_init()
{
    echo "funciton_init, but do nothing, please init on demand"
    # just create functions, but bind/enable according to .usb-config file
    # mkdir $FUNCTIONS_DIR/uvc.0
    # mkdir $FUNCTIONS_DIR/hid.0
    # mkdir $FUNCTIONS_DIR/uac2.0
    # mkdir $FUNCTIONS_DIR/ffs.adb
    # mkdir $FUNCTIONS_DIR/mass_storage.0
    # mkdir $FUNCTIONS_DIR/acm.0
    # mkdir $FUNCTIONS_DIR/rndis.0
}

create_frame() {
    # Example usage:
    # create_frame <function name> <width> <height> <format> <name>

    FUNCTION=$1
    WIDTH=$2
    HEIGHT=$3
    FORMAT=$4
    NAME=$5

    # bytes per pixel
    BYTES=2
    
    if [ $FORMAT == "framebased" ]; then
        BYTES=1.5
    fi

    if [[ $(expr match $NAME ".*-nv12$") != 0 ]]; then
        BYTES=1.5
    fi

    wdir=functions/$FUNCTION/streaming/$FORMAT/$NAME/${HEIGHT}p

    mkdir -p $wdir
    if [ $FORMAT != "framebased" ]; then
        tmp=$(awk 'BEGIN{print '$WIDTH' * '$HEIGHT' * '$BYTES'}')
        echo $tmp > $wdir/dwMaxVideoFrameBufferSize
    fi
    echo $WIDTH > $wdir/wWidth
    echo $HEIGHT > $wdir/wHeight
    tmp=$(awk 'BEGIN{print '$WIDTH' * '$HEIGHT' * '$BYTES' * 8 * 10}')
    echo $tmp > $wdir/dwMinBitRate
    tmp=$(awk 'BEGIN{print '$WIDTH' * '$HEIGHT' * '$BYTES' * 8 * 30}')
    echo $tmp > $wdir/dwMaxBitRate
    #echo 333333 > $wdir/dwDefaultFrameInterval
    echo 166666 > $wdir/dwDefaultFrameInterval
    cat <<EOF > $wdir/dwFrameInterval
166666
333333
666666
1000000
EOF
}

create_uvc()
{
    # Example usage:
    #	create_uvc <target config> <function name>
    #	create_uvc config/c.1 uvc.0
    CONFIG=$1
    FUNCTION=$2

    echo "Creating UVC gadget functionality : $FUNCTION"
    mkdir functions/$FUNCTION

    # uncompressed nv12 (using name to tell difference of nv12 & yuy2)
    # usb2.0
    create_frame $FUNCTION 640 360 uncompressed u-nv12
    create_frame $FUNCTION 800 600 uncompressed u-nv12
    create_frame $FUNCTION 1024 576 uncompressed u-nv12
    create_frame $FUNCTION 1280 720 uncompressed u-nv12

    # usb3.0
    create_frame $FUNCTION 640 360 uncompressed u-nv12-ss
    create_frame $FUNCTION 800 600 uncompressed u-nv12-ss
    create_frame $FUNCTION 1024 576 uncompressed u-nv12-ss
    create_frame $FUNCTION 1280 720 uncompressed u-nv12-ss
    create_frame $FUNCTION 1920 1080 uncompressed u-nv12-ss
    create_frame $FUNCTION 2560 1440 uncompressed u-nv12-ss
    create_frame $FUNCTION 3840 2160 uncompressed u-nv12-ss

    # uncompressed yuy2 (using name to tell difference of nv12 & yuy2)
    # usb2.0
    create_frame $FUNCTION 640 360 uncompressed u-yuy2
    create_frame $FUNCTION 800 600 uncompressed u-yuy2
    create_frame $FUNCTION 1024 576 uncompressed u-yuy2

    # usb3.0
    create_frame $FUNCTION 640 360 uncompressed u-yuy2-ss
    create_frame $FUNCTION 800 600 uncompressed u-yuy2-ss
    create_frame $FUNCTION 1024 576 uncompressed u-yuy2-ss
    create_frame $FUNCTION 1280 720 uncompressed u-yuy2-ss
    create_frame $FUNCTION 1920 1080 uncompressed u-yuy2-ss
    create_frame $FUNCTION 2560 1440 uncompressed u-yuy2-ss
    create_frame $FUNCTION 3840 2160 uncompressed u-yuy2-ss

    # mjpeg
    create_frame $FUNCTION 640 360 mjpeg m
    create_frame $FUNCTION 800 600 mjpeg m
    create_frame $FUNCTION 1024 576 mjpeg m
    create_frame $FUNCTION 1280 720 mjpeg m
    create_frame $FUNCTION 1920 1080 mjpeg m
    create_frame $FUNCTION 2560 1440 mjpeg m
    create_frame $FUNCTION 3840 2160 mjpeg m

    # framebased(h264)
    create_frame $FUNCTION 640 360 framebased f
    create_frame $FUNCTION 800 600 framebased f
    create_frame $FUNCTION 1024 576 framebased f
    create_frame $FUNCTION 1280 720 framebased f
    create_frame $FUNCTION 1920 1080 framebased f
    create_frame $FUNCTION 2560 1440 framebased f
    create_frame $FUNCTION 3840 2160 framebased f

    if $UVC_BULK; then
        echo 1 > functions/$FUNCTION/streaming_bulk
    else
        echo 0 > functions/$FUNCTION/streaming_bulk
    fi

    if [[ $BURST_NUM -gt 0 && $BURST_NUM -le 16 ]]; then
        echo $BURST_NUM > functions/$FUNCTION/streaming_maxburst
    fi

    # usb2.0
    mkdir functions/$FUNCTION/streaming/header/h
    cd functions/$FUNCTION/streaming/header/h
    ln -s ../../uncompressed/u-nv12
    ln -s ../../uncompressed/u-yuy2
    ln -s ../../mjpeg/m
    ln -s ../../framebased/f
    cd ../../class/fs
    ln -s ../../header/h
    cd ../../class/hs
    ln -s ../../header/h
    cd ../../../control
    mkdir header/h
    ln -s header/h class/fs

    cd ../../../

    # usb3.0
    mkdir functions/$FUNCTION/streaming/header/h-ss
    cd functions/$FUNCTION/streaming/header/h-ss
    ln -s ../../uncompressed/u-nv12-ss
    ln -s ../../uncompressed/u-yuy2-ss
    ln -s ../../mjpeg/m
    ln -s ../../framebased/f

    cd ../../class/ss
    ln -s ../../header/h-ss
    cd ../../../control
    mkdir header/h-ss
    ln -s header/h-ss class/ss

    cd ../../../

    # Set the packet size: uvc gadget max size is 3k...
    echo 3072 > functions/$FUNCTION/streaming_maxpacket
    echo 2048 > functions/$FUNCTION/streaming_maxpacket
    echo 1024 > functions/$FUNCTION/streaming_maxpacket

    if [[ $ISOC_MULT -gt 0 && $ISOC_MULT -le 4 ]]; then
        echo $(($ISOC_MULT * 1024)) > functions/$FUNCTION/streaming_maxpacket
    fi

    if $USE_MULT_ALTSETTING && $IS_UVC_ISOC; then
        echo "uvc streaming use multi alternate setting feature, to support more bandwidth case"
        echo 1 > functions/$FUNCTION/streaming_multi_altsetting
    fi

    # Set the PU bmcontrols value 3F:support Brightness/Contrast/Hue/Saturation/Sharpness
    echo 0x3F > functions/uvc.0/control/processing/default/bmControls
    # Set the CT bmcontrols value 220A:support Exposure Time(Absoulut)/Zoom/Roll
    echo 0x220A > functions/uvc.0/control/terminal/camera/default/bmControls

    ln -s functions/$FUNCTION configs/c.1

    echo "OK"
}

delete_uvc()
{
    # Example usage:
    #	delete_uvc <target config> <function name>
    #	delete_uvc config/c.1 uvc.0
    CONFIG=$1
    FUNCTION=$2
    
    echo "Deleting UVC gadget functionality : $FUNCTION"

    rm $CONFIG/$FUNCTION

    # usb2.0
    rm functions/$FUNCTION/control/class/*/h
    rm functions/$FUNCTION/control/class/*/h-ss
    rm functions/$FUNCTION/streaming/class/*/h
    rm functions/$FUNCTION/streaming/class/*/h-ss
    rm functions/$FUNCTION/streaming/header/h/u-nv12
    rm functions/$FUNCTION/streaming/header/h/u-yuy2
    rm functions/$FUNCTION/streaming/header/h/m
    rm functions/$FUNCTION/streaming/header/h/f

    # usb3.0
    rm functions/$FUNCTION/streaming/header/h-ss/u-nv12-ss
    rm functions/$FUNCTION/streaming/header/h-ss/u-yuy2-ss
    rm functions/$FUNCTION/streaming/header/h-ss/m
    rm functions/$FUNCTION/streaming/header/h-ss/f

    # rm uncompressed nv12
    # usb2.0
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12/360p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12/576p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12/600p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12/720p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12

    # usb3.0
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12-ss/360p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12-ss/576p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12-ss/600p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12-ss/720p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12-ss/1080p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12-ss/1440p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12-ss/2160p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-nv12-ss

    # rm uncompressed yuy2
    # usb2.0
    rmdir functions/$FUNCTION/streaming/uncompressed/u-yuy2/360p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-yuy2/576p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-yuy2/600p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-yuy2

    # usb3.0
    rmdir functions/$FUNCTION/streaming/uncompressed/u-yuy2-ss/360p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-yuy2-ss/576p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-yuy2-ss/600p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-yuy2-ss/720p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-yuy2-ss/1080p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-yuy2-ss/1440p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-yuy2-ss/2160p
    rmdir functions/$FUNCTION/streaming/uncompressed/u-yuy2-ss

    # rm mjpeg
    rmdir functions/$FUNCTION/streaming/mjpeg/m/360p
    rmdir functions/$FUNCTION/streaming/mjpeg/m/576p
    rmdir functions/$FUNCTION/streaming/mjpeg/m/600p
    rmdir functions/$FUNCTION/streaming/mjpeg/m/720p
    rmdir functions/$FUNCTION/streaming/mjpeg/m/1080p
    rmdir functions/$FUNCTION/streaming/mjpeg/m/1440p
    rmdir functions/$FUNCTION/streaming/mjpeg/m/2160p
    rmdir functions/$FUNCTION/streaming/mjpeg/m

    # rm framebased
    rmdir functions/$FUNCTION/streaming/framebased/f/360p
    rmdir functions/$FUNCTION/streaming/framebased/f/576p
    rmdir functions/$FUNCTION/streaming/framebased/f/600p
    rmdir functions/$FUNCTION/streaming/framebased/f/720p
    rmdir functions/$FUNCTION/streaming/framebased/f/1080p
    rmdir functions/$FUNCTION/streaming/framebased/f/1440p
    rmdir functions/$FUNCTION/streaming/framebased/f/2160p
    rmdir functions/$FUNCTION/streaming/framebased/f

    rmdir functions/$FUNCTION/streaming/header/h
    rmdir functions/$FUNCTION/streaming/header/h-ss
    rmdir functions/$FUNCTION/control/header/h
    rmdir functions/$FUNCTION/control/header/h-ss

    rmdir functions/$FUNCTION

    echo "OK"
}

create_hid()
{
    # Example usage:
    #	create_hid <target config> <function name>
    #	create_hid config/c.1 hid.0
    CONFIG=$1
    FUNCTION=$2

    echo "Creating HID gadget functionality"
    mkdir functions/$FUNCTION
    echo 1 > functions/$FUNCTION/protocol
    echo 1 > functions/$FUNCTION/subclass
    echo 1024 > functions/$FUNCTION/report_length
    cat $HID_REPORT_DESC > functions/$FUNCTION/report_desc

    ln -s functions/$FUNCTION configs/c.1

    echo "OK"
}

delete_hid()
{
    # Example usage:
    #	delete_hid <target config> <function name>
    #	delete_hid config/c.1 hid.0
    CONFIG=$1
    FUNCTION=$2

    echo "Deleting HID gadget functionality"
    rm $CONFIG/$FUNCTION
    rmdir functions/$FUNCTION

    echo "OK"
}

create_uac1()
{
    # Example usage:
    #	create_uac1 <target config> <function name>
    #	create_uac1 config/c.1 uac1.0
    CONFIG=$1
    FUNCTION=$2

    echo "Creating UAC1 gadget functionality : $FUNCTION"

    mkdir functions/$FUNCTION
    echo 0x3 > functions/$FUNCTION/p_chmask
    echo 48000 > functions/$FUNCTION/c_srate
    echo 0x3 > functions/$FUNCTION/c_chmask

    ln -s functions/$FUNCTION configs/c.1

    echo "OK"
}

delete_uac1()
{
    # Example usage:
    #	delete_uac1 <target config> <function name>
    #	delete_uac1 config/c.1 uac1.0
    CONFIG=$1
    FUNCTION=$2

    echo "Deleting UAC1 gadget functionality"
    rm $CONFIG/$FUNCTION
    rmdir functions/$FUNCTION

    echo "OK"
}

create_uac2()
{
    # Example usage:
    #	create_uac2 <target config> <function name>
    #	create_uac2 config/c.1 uac2.0
    CONFIG=$1
    FUNCTION=$2

    echo "Creating UAC2 gadget functionality : $FUNCTION"

    mkdir functions/$FUNCTION
    echo 0x3 > functions/$FUNCTION/p_chmask
    # echo 48000 > functions/$FUNCTION/c_srate
    echo 0 > functions/$FUNCTION/c_chmask

    ln -s functions/$FUNCTION configs/c.1

    echo "OK"
}

delete_uac2()
{
    # Example usage:
    #	delete_uac2 <target config> <function name>
    #	delete_uac2 config/c.1 uac2.0
    CONFIG=$1
    FUNCTION=$2

    echo "Deleting UAC2 gadget functionality"
    rm $CONFIG/$FUNCTION
    rmdir functions/$FUNCTION

    echo "OK"
}


#########################################################################
#                             Rndis Note!!!                             #
#########################################################################
# https://irq5.io/2016/12/22/raspberry-pi-zero-as-multiple-usb-gadgets
#########################################################################
# The Linux gadgets already have interface association descriptors (IADs) 
# for some devices since a few years ago, such as the RNDIS gadget and 
# the ECM gadget, so we don’t need to worry about that.
#
# If you plug in your Pi Zero USB gadget into a Windows machine, you will 
# notice that it cannot find the drivers for the device, and it also 
# misidentifies the device as a CDC Serial device. This situation has been 
# described quite accurately by user @dlech in a Microsoft Windows 10 forum:
#
# First, some more insight: The Linux rndis gadget function has USB class 
# of 2 and subclass of 2, which matches "USB\Class_02&amp;SubClass_02" in 
# the usbser.inf file. This is why for some people, their device is 
# initially detected as a COM port instead of RNDIS.
#
# He also proposes a few ways to work around this problem. 
# Solution 2 seemed to be the least intrusive and thus easiest to implement:
# 
# Solution 2: If you have control over the RNDIS device and it runs Linux 
# (i.e. BeagleBone), you can tweak the driver to get along with Windows 
# better. For example, if you have a 3.16 kernel or newer, you can setup 
# your gadget using configfs and include os descriptors. […] The key was 
# specifying the compatible and subcompatible ids so that it matches 
# "USB\MS_COMP_RNDIS&amp;MS_SUBCOMP_5162001" in rndiscmp.inf. 
# This causes the Microsoft RNDIS 6.0 driver to be installed for 
# this device.
# Fortunately for us, Microsoft has implemented some proprietary extensions 
# to USB called Microsoft OS Descriptors. This is the mechanism that is 
# used to convince Windows to load the proper RNDIS drivers, even though 
# the device subclass is “incorrect”.
#
# As suggested, this snippet from @dlech’s script sets up the 
# OS Descriptors for our RNDIS device to be automatically installed:
#
# echo 1       > ${g}/os_desc/use
# echo 0xcd    > ${g}/os_desc/b_vendor_code
# echo MSFT100 > ${g}/os_desc/qw_sign
#  
# mkdir ${g}/functions/rndis.usb0
# echo RNDIS   > ${g}/functions/rndis.usb0/os_desc/interface.rndis/compatible_id
# echo 5162001 > ${g}/functions/rndis.usb0/os_desc/interface.rndis/sub_compatible_id
#  
# ln -s ${g}/configs/c.1 ${g}/os_desc
# 
# If adding this still doesn’t work for you on Windows (like it happened 
# to me), continue on to the Debugging section below.
# 
create_rndis()
{
    # Example usage:
    #	create_rndis <target config> <function name>
    #	create_rndis config/c.1 rndis.0
    CONFIG=$1
    FUNCTION=$2

    echo "Creating RNDIS gadget functionality"
    mkdir functions/$FUNCTION

    # set up mac address of remote device
    # echo "42:63:65:13:34:56" > functions/$FUNCTION/host_addr
    # set up local mac address
    # echo "42:63:65:66:43:21" > functions/$FUNCTION/dev_addr

    # add OS specific device descriptors to force Windows to load RNDIS drivers
    # =============================================================================
    # Witout this additional descriptors, most Windows system detect the RNDIS interface as "Serial COM port"
    # To prevent this, the Microsoft specific OS descriptors are added in here
    # !! Important:
    #  If the device already has been connected to the Windows System without providing the
    #  OS descriptor, Windows never asks again for them and thus never installs the RNDIS driver
    #  This behavior is driven by creation of an registry hive, the first time a device without 
    #  OS descriptors is attached. The key is build like this:
    #
    #  HKLM\SYSTEM\CurrentControlSet\Control\usbflags\[USB_VID+USB_PID+bcdRelease\osvc
    #
    #  To allow Windows to read the OS descriptors again, the according registry hive has to be
    #  deleted manually or USB descriptor values have to be cahnged (f.e. USB_PID).

    mkdir -p os_desc
    echo 1 > os_desc/use
    echo 0xcd > os_desc/b_vendor_code
    echo MSFT100 > os_desc/qw_sign

    mkdir -p functions/$FUNCTION/os_desc/interface.rndis
    echo RNDIS > functions/$FUNCTION/os_desc/interface.rndis/compatible_id
    echo 5162001 > functions/$FUNCTION/os_desc/interface.rndis/sub_compatible_id

    ln -s functions/$FUNCTION configs/c.1
    ln -s configs/c.1/ os_desc			# add config 1 to OS descriptors

    echo "OK"
}

delete_rndis()
{
    # Example usage:
    #	delete_rndis <target config> <function name>
    #	delete_rndis config/c.1 rndis.0
    CONFIG=$1
    FUNCTION=$2

    echo "Deleting RNDIS gadget functionality"
    rm os_desc/c.1
    rm $CONFIG/$FUNCTION
    rmdir os_desc
    rmdir functions/$FUNCTION

    echo "OK"
}

create_ecm()
{
    # Example usage:
    #	create_ecm <target config> <function name>
    #	create_ecm config/c.1 ecm.0
    CONFIG=$1
    FUNCTION=$2

    echo "Creating CDC ECM gadget functionality"
    # set up mac address of remote device
    # echo "42:63:65:13:34:56" > functions/$FUNCTION/host_addr
    # set up local mac address
    # echo "42:63:65:66:43:21" > functions/$FUNCTION/dev_addr

    mkdir functions/$FUNCTION

    ln -s functions/$FUNCTION configs/c.1

    echo "OK"
}

delete_ecm()
{
    # Example usage:
    #	delete_ecm <target config> <function name>
    #	delete_ecm config/c.1 ecm.0
    CONFIG=$1
    FUNCTION=$2

    echo "Deleting ECM gadget functionality"
    rm $CONFIG/$FUNCTION
    rmdir functions/$FUNCTION

    echo "OK"
}

create_adb()
{
    # Example usage:
    #	create_adb <target config> <function name>
    #	create_adb config/c.1 ffs.adb
    CONFIG=$1
    FUNCTION=$2

    echo "Creating ACM gadget functionality"
    mkdir functions/$FUNCTION

    ln -s functions/$FUNCTION configs/c.1

    echo "OK"
}

delete_adb()
{
    # Example usage:
    #	delete_adb <target config> <function name>
    #	delete_adb config/c.1 ffs.adb
    CONFIG=$1
    FUNCTION=$2

    echo "Deleting adb gadget functionality"
    rm $CONFIG/$FUNCTION
    rmdir functions/$FUNCTION

    echo "OK"
}

create_msd() {
    # Example usage:
    #	create_msd <target config> <function name> <image file>
    #	create_msd configs/c.1 mass_storage.0 /userdata/mass_storage.img block_size auto_mount
    CONFIG=$1
    FUNCTION=$2
    MSD_STORE=$3
    MSD_BLOCK_SIZE=$4
    MSD_AUTO_MOUNT=$5

    if [ ! -f $MSD_STORE ]
    then
        echo "Creating mass_storage file"
        dd if=/dev/zero of=$MSD_STORE bs=1M count=$MSD_BLOCK_SIZE > /dev/null 2>&1
        mkfs.ext4 $MSD_STORE > /dev/null 2>&1
        echo "OK"
    fi

    echo "Creating MSD gadget functionality"
    mkdir functions/$FUNCTION
    echo 1 > functions/$FUNCTION/stall
    if $MSD_AUTO_MOUNT; then
        mount $MSD_STORE /media/mass_storage
    fi
    echo $MSD_STORE > functions/$FUNCTION/lun.0/file
    echo 1 > functions/$FUNCTION/lun.0/removable
    echo 0 > functions/$FUNCTION/lun.0/cdrom

    ln -s functions/$FUNCTION configs/c.1

    echo "OK"
}

delete_msd() {
    # Example usage:
    #	delete_msd <target config> <function name>
    #	delete_msd configs/c.1 mass_storage.0
    CONFIG=$1
    FUNCTION=$2

    echo "Deleting MSD gadget functionality"
    rm $CONFIG/$FUNCTION
    rmdir functions/$FUNCTION

    echo "OK"
}

create_acm()
{
    # Example usage:
    #	create_acm <target config> <function name>
    #	create_acm config/c.1 acm.0
    CONFIG=$1
    FUNCTION=$2

    echo "Creating ACM gadget functionality"
    mkdir functions/$FUNCTION

    ln -s functions/$FUNCTION configs/c.1

    echo "to be done"
}

delete_acm() {
    # Example usage:
    #	delete_acm <target config> <function name>
    #	delete_acm configs/c.1 acm.0
    CONFIG=$1
    FUNCTION=$2

    echo "Deleting ACM gadget functionality"
    rm $CONFIG/$FUNCTION
    rmdir functions/$FUNCTION

    echo "OK"
}

bind_functions()
{
    echo "Bind functions according to .usb-config file"
    # For win10 case, rndis must be the beginning interface of 
    # multi-function gadget.
    if $USE_RNDIS; then
        echo "bind gadget rndis..."
        create_rndis configs/c.1 rndis.0
    fi

    if $USE_UAC1; then
        echo "bind uac1..."
        create_uac1 configs/c.1 uac1.0
    fi

    # uac2 need to put before uvc, otherwise uvc + uac2 enumerate failed in win10
    if $USE_UAC2; then
        echo "bind uac2..."
        create_uac2 configs/c.1 uac2.0
    fi

    if $USE_UVC; then
        echo "bind uvc..."
        create_uvc configs/c.1 uvc.0
    fi

    if $USE_HID; then
        echo "bind hid..."
        create_hid configs/c.1 hid.0
    fi

    if $USE_ADB; then
        echo "bind adb..."
        create_adb configs/c.1 ffs.adb
    fi

    if $USE_MSD; then
        echo "bind mass storage..."
        create_msd configs/c.1 mass_storage.0 $MSD_FILE $MSD_BLOCK_SIZE $MSD_BLOCK_AUTO_MOUNT
    fi

    if $USE_ACM; then
        echo "bind gadget serial..."
        create_acm configs/c.1 acm.0
    fi

    if $USE_ECM; then
        echo "bind gadget ecm..."
        create_ecm configs/c.1 ecm.0
    fi
}

unbind_functions()
{
    echo "Unbind functions according to .usb-config file"
    if $USE_RNDIS; then
        echo "unbind gadget rndis..."
        delete_rndis configs/c.1 rndis.0
    fi

    if $USE_UAC1; then
        echo "unbind uac1..."
        delete_uac1 configs/c.1 uac1.0
    fi

    if $USE_UAC2; then
        echo "unbind uac2..."
        delete_uac2 configs/c.1 uac2.0
    fi

    if $USE_UVC; then
        echo "unbind uvc..."
        delete_uvc configs/c.1 uvc.0
    fi

    if $USE_HID; then
        echo "unbind hid..."
        delete_hid configs/c.1 hid.0
    fi

    if $USE_ADB; then
        echo "unbind adb..."
        delete_adb configs/c.1 ffs.adb
    fi

    if $USE_MSD; then
        echo "unbind mass storage..."
        delete_msd configs/c.1 mass_storage.0 $MSD_FILE $MSD_BLOCK_SIZE $MSD_BLOCK_AUTO_MOUNT
    fi

    if $USE_ACM; then
        echo "unbind gadget serial..."
        delete_acm configs/c.1 acm.0
    fi

    if $USE_ECM; then
        echo "unbind gadget ecm..."
        delete_ecm configs/c.1 ecm.0
    fi
}

pre_run_binary()
{
    # return value, 3 means wait 3 loop (0.3s a loop)
    if $USE_ADB; then
        mkdir -p /dev/usb-ffs/adb
        mount -o uid=2000,gid=2000 -t functionfs adb /dev/usb-ffs/adb
        start-stop-daemon -S -b -q -n adbd -a /usr/bin/adbd

        echo 3
    fi

    if $USE_UVC; then
        # some base board without oscillator needs command "camera_test 3 1 1 1"
        $UVC_PRE_EXEC

        echo 0
    fi

    echo 0
}

run_binary()
{
    if $USE_UVC; then
        # let user launch uvc app directly (as many sensors and params, easy for debug...)
        # start-stop-daemon -S -b -n $UVC_APP -a $UVC_EXEC -- $UVC_ARGS $UVC_LOG_TO
        :
    fi
}

start_usb_gadget()
{
    echo "Creating the USB gadget"
    echo "Loading composite module"
    modprobe libcomposite
    #modprobe g_ether
    #rmmod g_ether

    sleep 0.3

    echo "Creating gadget directory g_comp"
    mkdir -p $USB_CONFIGFS
    cd $USB_CONFIGFS
    if [ $? -ne 0 ]; then
        echo "Error creating usb gadget in configfs"
        exit 1
    else
        echo "OK"
    fi

    echo "init configfs..."
    configfs_init

    echo "Init functions..."
    function_init
    echo "OK"

    echo "Bind functions..."
    bind_functions

    echo "Pre run userspace daemons(eg. adb)..."
    pre_run_binary

    # just with userspace daemon, needs to wait some time.
    echo "waiting"
    for i in `seq 0 $?`
    do
        echo "."
        sleep 0.3
    done

    echo "OK"

    echo "Binding USB Device Controller"
    echo $UDC > UDC
    echo peripheral > $UDC_ROLE
    cat $UDC_ROLE
    echo "OK"

    echo "Run some userspace daemons(eg. usb_camera)..."
    run_binary
}

stop_usb_gadget()
{
    echo "Stoping & Delete usb-gadget g_comp"
    if $USE_UVC; then
        # let user launch/stop uvc app (as many sensors and params, easy for debug...)
        # start-stop-daemon -K -s 9 -n $UVC_APP
        :
    fi

    echo "waiting..."
    for i in `seq 1 3`
    do
        echo "."
        sleep 0.3
    done

    cd $USB_CONFIGFS
    if [ $? -ne 0 ]; then
        echo "Error creating usb gadget in configfs"
        exit 1
    else
        echo "OK"
    fi

    echo "" > UDC

    echo "Unbind functions..."
    unbind_functions

    # make sure all linked function is *.* format (like ffs.adb, uvc.0, uac2.0...)
    # ls configs/c.1 | grep "\." | xargs -I {} rm configs/c.1/{}

    # configfs clean up
    echo "cleanup configfs..."
    configfs_cleanup

    # delete usb-gadget 
    echo "delete g_comp"
    rmdir $USB_CONFIGFS

    if $USE_ADB; then
        start-stop-daemon -K -q -n adbd
    fi
}

usage()
{
    echo "Usage: $0 {start|stop|restart} [options]"
    echo " options:"
    echo " detail gadget-composite config, using .usb/.default-config in default"
    echo "      adb                         launch adbd"
    echo "      msd                         run as gadget mass storage device"
    echo "      hid                         run as hid gadget"
    echo "      rndis                       run as rndis gadget"
    echo "      ecm                         run as cdc ether gadget"
    echo "      uvc                         run as uvc gadget"
    echo "      uac1                        usb audio class specification 1"
    echo "      uac2                        usb audio class specification 2"
    echo "      uvc-hid                     uvc + hid composite gadget"
    echo "      uvc-uac1                    uvc + uac1 composite gadget"
    echo "      uvc-uac2                    uvc + uac2 composite gadget"
    echo "      uvc-hid-uac1                uvc + hid + uac1 composite gadget"
    echo "      uvc-hid-uac2                uvc + hid + uac2 composite gadget"
    echo "      uvc-rndis                   uvc + rndis composite gadget"
    echo "      uvc-ecm                     uvc + cdc ether composite gadget"
    echo "      uvc-acm                     uvc + acm(serial) composite gadget"
    echo "      uvc-rndis-uac1              uvc + rndis + uac1 composite gadget"
    echo "      uvc-rndis-uac2              uvc + rndis + uac2 composite gadget"
    echo "      uvc-ecm-uac1                uvc + ecm + uac1 composite gadget"
    echo "      uvc-ecm-uac2                uvc + ecm + uac2 composite gadget"
    echo "      rndis-hid                   rndis + hid composite gadget"
    echo "      rndis-uac1                  rndis + uac1 composite gadget"
    echo "      msd-uac1                    msd + uac1 composite gadget"
    echo "      hid-uac1                    hid + uac1 composite gadget"
}

# init script entry
case $1 in
start)
    # check the lockfile to judge process status
    if [ -e $LOCKFILE ]; then
        echo "$SERVICE is already running"
        exit 3
    fi

    # check usb mode
    if ! is_usb_gadget; then
        echo "not usb gadget mode, can't start usb-gadget"
        exit 3
    fi

    #show_user_config

    # choose a user config about usb-gadget, use default one without $2 param
    if [ -z $2 ]; then
        if [ -e "/tmp/.usb-last-config" ]; then
            USER_CONFIG=/tmp/.usb-last-config
        else
            USER_CONFIG=.default-config
        fi
    else
        if [ $2 == "adb" ]; then
            USER_CONFIG=.adb-config
        elif [ $2 == "uvc" ]; then
            USER_CONFIG=.uvc-config
        elif [ $2 == "uac1" ]; then
            USER_CONFIG=.uac1-config
        elif [ $2 == "uac2" ]; then
            USER_CONFIG=.uac2-config
        elif [ $2 == "uvc-hid" ]; then
            USER_CONFIG=.uvc-hid-config
        elif [ $2 == "uvc-uac1" ]; then
            USER_CONFIG=.uvc-uac1-config
        elif [ $2 == "uvc-uac2" ]; then
            USER_CONFIG=.uvc-uac2-config
        elif [ $2 == "uvc-hid-uac1" ]; then
            USER_CONFIG=.uvc-hid-uac1-config
        elif [ $2 == "uvc-hid-uac2" ]; then
            USER_CONFIG=.uvc-hid-uac2-config
        elif [ $2 == "rndis" ]; then
            USER_CONFIG=.rndis-config
        elif [ $2 == "ecm" ]; then
            USER_CONFIG=.ecm-config
        elif [ $2 == "uvc-rndis" ]; then
            USER_CONFIG=.uvc-rndis-config
        elif [ $2 == "uvc-rndis-uac1" ]; then
            USER_CONFIG=.uvc-rndis-uac1-config
        elif [ $2 == "uvc-rndis-uac2" ]; then
            USER_CONFIG=.uvc-rndis-uac2-config
        elif [ $2 == "uvc-ecm" ]; then
            USER_CONFIG=.uvc-ecm-config
        elif [ $2 == "uvc-ecm-uac1" ]; then
            USER_CONFIG=.uvc-ecm-uac1-config
        elif [ $2 == "uvc-ecm-uac2" ]; then
            USER_CONFIG=.uvc-ecm-uac2-config
        elif [ $2 == "uvc-acm" ]; then
            USER_CONFIG=.uvc-acm-config
        elif [ $2 == "rndis-hid" ]; then
            USER_CONFIG=.rndis-hid-config
        elif [ $2 == "msd" ]; then
            USER_CONFIG=.msd-config
        elif [ $2 == "hid" ]; then
            USER_CONFIG=.hid-config
        elif [ $2 == "rndis-uac1" ]; then
            USER_CONFIG=.rndis-uac1-config
        elif [ $2 == "msd-uac1" ]; then
            USER_CONFIG=.msd-uac1-config
        elif [ $2 == "hid-uac1" ]; then
            USER_CONFIG=.hid-uac1-config
        else
            echo "No matched options($2), use default"
            USER_CONFIG=.default-config
        fi
    fi

    # backup user-config for stop usage (ignore the changes before restart)
    if [ $USER_CONFIG != "/tmp/.usb-last-config" ]; then
        cp /etc/init.d/.usb/$USER_CONFIG /tmp/.usb-last-config
    fi

    # check if using uvc isoc mode
    if [ ! -z $3 ]; then
        if [ $3 == "isoc" ] || [ $3 == "ISOC" ]; then
            echo "Using isoc transfer for uvc! default mult(3), burst(0)"
            sed -i 's/UVC_BULK=true/UVC_BULK=false/g' /tmp/.usb-last-config
            sed -i 's/BURST_NUM=9/BURST_NUM=10/g' /tmp/.usb-last-config
            sed -i -e '$a\ISOC_MULT=3' /tmp/.usb-last-config
            sed -i -e '$a\IS_UVC_ISOC=true' /tmp/.usb-last-config
        fi
    fi

    # source user-config
    . /tmp/.usb-last-config

    show_user_config

    # start the usb gadget
    start_usb_gadget

    # create lock file
    touch $LOCKFILE

    echo "usb-gadget start succeed."
    ;;
stop)
    echo "Stopping the USB gadget"

    set +e # Ignore all errors here on a best effort

    # check the lockfile to judge process status
    if [ ! -e $LOCKFILE ]; then
        echo "$SERVICE is not running"
        exit 3
    fi

    # re-source .usb-last-config again. 
    # only system reboot will remove /tmp/.usb-last-config, which means
    # next /etc/init.d/usb-gadget.sh start command will lunch last config
    . /tmp/.usb-last-config

    # stop the usb gadget
    stop_usb_gadget

    # create lock file
    rm $LOCKFILE

    echo "usb-gadget stop succeed."
    ;;

status)
    # check the lockfile to get process status
    if [ -e $LOCKFILE ]; then
        echo "$SERVICE is running"
    else
        echo "$SERVICE is not runnning"
    fi
    ;;

restart|reload)
    if [ -e $LOCKFILE ]; then
        if [ -z $2 ]; then
            $0 stop
        else
            $0 stop $2
        fi
    fi

    if [ -z $2 ]; then
        $0 start
    else
        if [ ! -z $3 ]; then
            $0 start $2 $3
        else
            $0 start $2
        fi
    fi

    echo "usb-gadget restart succeed."
    ;;

help)
    usage
    ;;

*)
    usage
    ;;
esac

exit 0

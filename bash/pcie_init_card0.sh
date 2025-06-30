#!/bin/bash
. ./common_fun.sh

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

# Set card 0 or 1
set_card 0

echo "Reset Process!" 
mipi_disable

# Camera time stamp  source {0: vsync; 1:fsync; 2:frame transfer done; }
timestamp_src 1

# Camera input format conversion config {0 or 1}
camera_input_format_conversion[0]=0
camera_input_format_conversion[1]=0
camera_input_format_conversion[2]=0
camera_input_format_conversion[3]=0
camera_input_format_conversion[4]=0
camera_input_format_conversion[5]=0
camera_input_format_conversion[6]=0
camera_input_format_conversion[7]=0
camera_format_set ${camera_input_format_conversion[@]}


# Video output yuv format  config {"YUYV" or "UYVY"}
video_output_yuv_format 0 "YUYV"
video_output_yuv_format 1 "YUYV"
video_output_yuv_format 2 "YUYV"
video_output_yuv_format 3 "YUYV"
video_output_yuv_format 4 "YUYV"
video_output_yuv_format 5 "YUYV"
video_output_yuv_format 6 "YUYV"
video_output_yuv_format 7 "YUYV"

# Trigger mode config {0:no trigger; 1:reserved; 2:inner trigger; 3:external trigger}
card_trigger_signal_mode       	"2"

# Card external signal input fps config.
# Camera external output fps config.
# The following two configurations are valid only when card_trigger_signal_mode is "3".
card_external_signal_input_fps 	"1" Hz
camera_external_output_fps     	"20" Hz

# Camera inner output fps config
camera_inner_output_fps        	"30" Hz

# Camera 0-7 trigger delay config,unit "microsecond".
camera_triger_delay[0]=0
camera_triger_delay[1]=0
camera_triger_delay[2]=0
camera_triger_delay[3]=0
camera_triger_delay[4]=0
camera_triger_delay[5]=0
camera_triger_delay[6]=0
camera_triger_delay[7]=0
trigger_delay ${camera_triger_delay[@]}

# Camera 0-7 resolution: width height   
camera_resolution 0 1920 1080
camera_resolution 1 1920 1080
camera_resolution 2 1920 1080
camera_resolution 3 1920 1080 
camera_resolution 4 1920 1080
camera_resolution 5 1920 1080
camera_resolution 6 1920 1080
camera_resolution 7 1920 1080
echo "Card Params Init Processed!"

#0---->max96705
#1---->max9295
#2---->max96717f
#camera_serdes_type[0] ---> ch 0 1
#camera_serdes_type[1] ---> ch 2 3
#camera_serdes_type[2] ---> ch 4 5
#camera_serdes_type[2] ---> ch 6 7
#raw camera: 100->ox03c,101->ox08b,102->ar0233,103->ar0820, 104->imx390,105->imx490
camera_serdes_type[0]=1
camera_serdes_type[1]=1
camera_serdes_type[2]=1
camera_serdes_type[3]=1

camera_serdes_cfg ${camera_serdes_type[@]}
echo "Serdes Params Init Processed!"

# Report all process passed and exit
echo "Info: All process in pcie_init.sh passed."

exit 0

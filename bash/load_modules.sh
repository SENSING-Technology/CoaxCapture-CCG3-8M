#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

module_name="xdma"
if lsmod | grep -q $module_name; then
   echo "Module $module_name is already loaded."
   echo "Unlode Pcie driver..."
   rmmod xdma
   rmmod videobuf2-v4l2
   rmmod videobuf2-dma-sg
   rmmod videobuf2-memops
   rmmod videobuf2_common
   rmmod videodev
   rmmod mc
else
	echo "Module $module_name is not loaded."
fi

echo "Loading Pcie driver..."
  modprobe mc
  modprobe videodev
  modprobe videobuf2_common
  modprobe videobuf2-memops
  modprobe videobuf2-dma-sg
  modprobe videobuf2-v4l2

sleep 1s
echo "Loading xdma.ko"
#insmod xdma.ko  interrupt_mode=1
insmod ../xdma_v4l2/xdma.ko  sgdma_timeout=100 interrupt_mode=1 

if [ ! $? == 0 ]; then
  echo "Error: Pcie driver did not load properly."
  echo " FAILED"
  exit 1
fi

#cat /proc/devices | grep video4linux > /dev/null
ls /dev | grep video0 > /dev/null
returnVal=$?
if [ $returnVal == 0 ]; then
  # Installed devices were recognized.
  echo "Pcie driver installed correctly."
  echo "Video devices were recognized."
else
  # No devices were installed.
  echo "Error: Pcie driver installed correctly."
  echo "No devices were recognized."
  echo " FAILED"
  exit 1
fi

echo "DONE"

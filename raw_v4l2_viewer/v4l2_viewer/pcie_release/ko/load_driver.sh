#!/bin/bash

# Make sure only root can run our script
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

# Remove the existing xdma kernel module
lsmod | grep xdma
if [ $? -eq 0 ]; then
   rmmod xdma
fi
echo -n "Loading driver..."
# Use the following command to Load the driver in the default 
# or interrupt drive mode. This will allow the driver to use 
# interrupts to signal when DMA transfers are completed.
modprobe videobuf2-core #debug=7
modprobe videobuf2-v4l2 #debug=7
modprobe videodev
modprobe v4l2-common
modprobe videobuf2-dma-sg
modprobe videobuf2-dma-contig
modprobe videobuf2-vmalloc
insmod xdma.ko
# Use the following command to Load the driver in Polling
# mode rather than than interrupt mode. This will allow the
# driver to use polling to determ when DMA transfers are 
# completed.
#insmod ../driver/xdma.ko poll_mode=1

if [ ! $? == 0 ]; then
  echo "Error: Kernel module did not load properly."
  echo " FAILED"
  exit 1
fi

# Check to see if the xdma devices were recognized
echo ""
cat /proc/devices | grep xdma > /dev/null
returnVal=$?
if [ $returnVal == 0 ]; then
  # Installed devices were recognized.
  echo "The Kernel module installed correctly and the xmda devices were recognized."
else
  # No devices were installed.
  echo "Error: The Kernel module installed correctly, but no devices were recognized."
  echo " FAILED"
  exit 1
fi

#rm *.yuv
#dmesg  -C
#../../pcie-hal/build/pcie_preview

echo " DONE"

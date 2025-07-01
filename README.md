# SENSING GMSL Video Capture Card User Guide  

---

## Overview  
This guide provides instructions for using the SENSING GMSL Video Capture Card (CCG3-8M model) on Ubuntu 18.04/20.04 systems. The CCG3-8M supports up to 8 camera connections via coaxial cables.  

---

## Directory Structure  
The package includes the following key directories and files:  

| Directory/File               | Description                                                                 |
|-------------------------------|-----------------------------------------------------------------------------|
| `bash/`                       | Bash scripts for device management.                                        |
| `Makefile`                    | Build script for drivers and applications (supports compiling per directory). |
| `xdma_v4l2/`                  | PCIe board card driver source code.                                        |
| `include/`                    | Header files for xdma driver.                                               |
| `./tool/pcie_a53_rw`          | Application for reading/writing data to the PCIe board's PS (Processing System). |
| `./tool/pcie_reg_rw`          | Application for reading/writing PCIe board register values.                 |
| `./tool/xdma_v4l2_rw`         | Application for configuring the driver via the V4L2 (Video for Linux 2) interface. |

---

## Prerequisites  
- Ubuntu 18.04 or 20.04.  

---

## Installation & Usage  

### 1. Load the Driver  
Enter the `bash/` directory and execute the following script to load the driver:  
```bash
sudo ./load_modules.sh
```  

**Successful Output:**  
```
Loading Pcie driver...
Pcie driver installed correctly.
Video devices were recognized.
DONE
```  

**Verification:**  
After successful loading, video device files (e.g., `/dev/video0`, `/dev/video1`, `/dev/video2`, `/dev/video3`) will be generated in the `/dev` directory. Control interface files (e.g., `xdma0_bypass`, `xdma0_control`, `xdma0_user`) will also be created. The number (e.g., `0`) corresponds to the PCIe board index (e.g., `0` for the first board, `1` for the second, etc.).  

---

### 2. Initialize the PCIe Board  
Execute the following script to initialize the PCIe board (e.g., board 0) and configure connected cameras:  
```bash
sudo ./pcie_init_card0.sh
```  

**Successful Output:**  
```
Reset Process!
Card Params Init Processed!
Serdes 0 Params Init Processed!
Serdes 1 Params Init Processed!
...
Info: All process in pcie_init.sh passed.
```  
Note: If you have multiple capture cards, you can modify the pcie_init_card*.sh script again, and then execute the command:
```bash
sudo ./pcie_init_card*.sh
```  
---

### 3. Image Testing  
Use the open-source tool `guvcview` to test video capture.  

#### Install `guvcview` (if not installed):  
```bash
sudo apt-get install guvcview
```  

#### Test Video Streams:  
Connect cameras to the board, then run:  
```bash
# For video0 (board 0, channel 0)
guvcview -d /dev/video0

# For video1 (board 0, channel 1)
guvcview -d /dev/video1

# Repeat for video2, video3 (if supported)
```  

**Verify Frame Rate:**  
To capture and verify the frame rate, execute:  
```bash
v4l2-ctl -d /dev/video0 --stream-mmap --stream-count=1000
```  

---

## Configuring Camera Parameters  
To modify parameters for different cameras, adjust:  
1. The `pcie_init_card0.sh` script (for initialization configurations).  
2. The `pcie_a53_rw` application (for low-level register/control adjustments).  

---

## Firmware Programming  
The capture card is pre-programmed at the factory to support YUV, RAW10, or RAW12 modes. To switch modes:  

1. Reprogram the firmware using the corresponding file from the `FirmwareResources/` directory:  
   - **YUYV/UYVY (YUV)**: Use `pcie_zu_fw-xxxx-YUV.tar.gz`.  
   - **RAW12**: Use `pcie_zu_fw-xxxx-RAW12.tar.gz`.  
   - **RAW10**: Use `pcie_zu_fw-xxxx-RAW10.tar.gz`.  

2.The command to upgrade the firmware of the capture card is as follows:
```bash
cd /tools/pcie_zu_tools
sudo ./pcie_zu-updata.sh ./pcie_zu_fw-xxxx-RAW12.tar.gz
``` 
--- 

## Notes  
- Ensure all cameras are properly connected via coaxial cables before initializing the board.  
- If issues occur, check `/dev` directory for video device files and verify driver loading logs.

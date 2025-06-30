#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "pcie_camera.h"
#include "device_manager.h"
#include "CameraGather.h"
#include "PcieCamera.h"
#include "yuvtobgr.h"

#define UNUSED(x) (void)x 

deviceManager* devices = NULL;
PcieCamera* pcieCamera;
pthread_t poll_t;
int pollExit = 0;

static void* cameraPoll(void* arg){
    int camFlags = 0;
    UNUSED(arg);

    pthread_detach(pthread_self());
    pollExit = 1;
    while(pollExit){
        camFlags = 0;
        pcieCamera->pollCamera(&camFlags);
        usleep(1000);
    }

    pthread_exit(NULL);
    return NULL;
}

const char *pcie_camera_GetVersion(){
    return devices->getCardVersion().c_str();
}

int pcie_camera_init(camera_info* info){
    int status = 0;
    devices = new deviceManager(info->group, info->channel_mask, info->width, info->height, info->fps, info->defertime, info->trigger_mode, info->force_init);
    if((status = devices->deviceInit())< 0){
        printf("devices initialize failed! ret=0x%x\n", status);
        devices->deviceDeinit();
        return -1;
    }

    devices->syncTimeToPCIE();

    printf("devices initialize successed! ret=0x%x \n", status);

    pcieCamera = new PcieCamera(info->group, info->channel_mask, info->width, info->height, info->fps);
    if((status = pcieCamera->openCamera()) <  0){
        printf("Camera Gather initialize failed! ret=%d\n", status);
        devices->deviceDeinit();
        delete pcieCamera;
        delete devices;
        return -1;
    }
    printf("start camera\n");
    pcieCamera->startCamera(0);
     printf("start camera ok\n");

    printf("Create loop thread for listen video event\n");
    pthread_create(&poll_t, NULL, cameraPoll, NULL);

    return 0;
}

int pcie_camera_getStatus(void){
    return devices->getCameraStatus();
}

int pcie_camera_readYUVData(int ch, frameInfo *img){
    return pcieCamera->readCamera(ch, img);
}

int pcie_camera_readBGRData(int ch, frameInfo *img){
        int ret = -1;
        frameInfo yuvInfo;
        yuvInfo.isMemcpy = 0;
        //printf("pcie_camera_readBGRData\n");
        ret = pcieCamera->readCamera(ch, &yuvInfo);
        //printf("pcie_camera_readBGRData %d \n", ret);
        if(ret)
                return ret;
        YV12ToBGR24_FFmpeg(yuvInfo.pdata, img->pdata, yuvInfo.width, yuvInfo.height); 
        img->timestamp = yuvInfo.timestamp;
        img->exposuretime = yuvInfo.exposuretime;
            
        return 0;
}

int pcie_camera_readRGBData(int ch, frameInfo *img){
        UNUSED(ch);
        UNUSED(img);
        
        return 0;
}

int pcie_camera_frameStaStatistics(int ch, int* errNum){
    return pcieCamera->readFrameCount(ch, errNum);
}

int pcie_camera_destroy(void){

    pollExit = 0;
//    pthread_join(poll_t, NULL);

    if(pcieCamera){
        pcieCamera->stopCamera();
        pcieCamera->closeCamera();
        delete pcieCamera;
    }
    if(devices){
        devices->deviceDeinit();
        delete devices;
    }

    printf("%s exit\n", __func__);

    return 0;
}



#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "hal_camera.h"
#include "device_manager.h"
#include "PcieCamera.h"
#include "yuvtobgr.h"

#define UNUSED(x) (void)x

using namespace std;

array<shared_ptr<deviceManager>, 5> cards;
array<shared_ptr<PcieCamera>, 5> cameras;

camera_data_callback camera_cb[MAX_BOARD][MAX_CHANS_PER_BOARD];
pthread_t camera_th[MAX_BOARD][MAX_CHANS_PER_BOARD];
int cam_num[MAX_BOARD][MAX_CHANS_PER_BOARD];
int running = 0;

#define OTA_SEL_MAX_CHL_PER_BYTE    (MAX_CHANS_PER_BOARD/2)

const char *hal_camera_GetVersion(int boardnum){
    return cards[boardnum] == nullptr  ? NULL :cards[boardnum]->getCardVersion().c_str();
}

static int camera_hal_GetOtaSelect(int *piOtaSelect){
    int OtaSelect = 0;
    for ( int i = 0 ; i < MAX_CHANS_PER_BOARD; i++ ){
        if ( 0 != piOtaSelect[i] ){
            if ( 0 != OtaSelect ){
                printf("select more than 1 channel, err!! %d\n", OtaSelect);
                return -1;
            }
            /* i: 0->0x0001, 1->0x0002, 2->0x0003, 3->0x0100, 4->0x0200, 5->0x0300 */
            OtaSelect = (i/OTA_SEL_MAX_CHL_PER_BYTE) ? (i%OTA_SEL_MAX_CHL_PER_BYTE+1)<<8 : (i%OTA_SEL_MAX_CHL_PER_BYTE+1);
        }
    }
    return OtaSelect;
}

int hal_camera_init(camera_info* info){
    int status = 0;
    shared_ptr<deviceManager> device  = make_shared<deviceManager>(info->group, info->channel_mask,
                            info->width, info->height, info->fps,
                            info->defertime, info->trigger_mode, info->force_init);

    /* check if need reset mcu on board, set pcie to reset */
    if ( 0 != info->card_mcureset ){
        printf("card[%d] mcu reset!!\n", info->group);
        if(device->CardMcuReset() < 0){
            printf("CardMcuReset failed!\n");
            device->deviceDeinit();
            device.reset();
            return -1;
        }
    }

    /* check if need update camera, set pcie to select cam channel */
    if((status = camera_hal_GetOtaSelect(info->cam_otaselect)) < 0){
        printf("camera_hal_GetOtaSelect failed! ret=0x%x\n", status);
        device->deviceDeinit();
        device.reset();
        return -1;
    }
    if(device->setCameraOtaSelect(status) < 0){
        printf("setCameraOtaSelect failed!\n");
        device->deviceDeinit();
        device.reset();
        return -1;
    }

    /* init device, power on camera */
    if((status = device->deviceInit()) < 0){
        printf("devices initialize failed! ret=0x%x\n", status);
        device.reset();
        return -1;
    }

    /* PC sync time to pcie */
    if(device->syncTimeToPCIE() < 0){
        printf("syncTimeToPCIE failed!\n");
        device->deviceDeinit();
        device.reset();
        return -1;
    }
    printf("devices initialize successed! ret=0x%x \n", status);

    shared_ptr<PcieCamera> pcieCamera = make_shared<PcieCamera>(info->group, info->channel_mask, info->width, info->height, info->fps);
    printf("create pcie camera ok\n");
    if((status = pcieCamera->openCamera()) < 0){
        printf("Camera Gather initialize failed! ret=%d\n", status);
        device->deviceDeinit();
        device.reset();

        return -1;
    }
    printf("open pcie camera ok\n");

    cards[info->group] = device;
    cameras[info->group] = pcieCamera;

    //printf("Create loop thread for listen video event\n");
    //pthread_create(&poll_t, NULL, cameraPoll, NULL);

    return 0;
}

void *camera_thread_handle(void *num)
{
	int board = *(int*)num/MAX_CHANS_PER_BOARD;
	int channel =  *(int*)num%MAX_CHANS_PER_BOARD;
	frameInfo info;

    info.isMemcpy = 0;
	while(running){
		cards[board] == nullptr ? -1 : cameras[board]->readFrameTimeOut(channel, &info, 100);
		camera_cb[board][channel](board*MAX_CHANS_PER_BOARD+channel,&info);
	}

	printf("Thread[%d][%d] finish\n",board,channel);
	return NULL;
}

int hal_camera_start(int board, int mode){
	/*return cards[board] == nullptr ? -1 :cameras[board]->startCamera(mode);*/

	int channel = 0, tmode = mode;
	if(cards[board] == nullptr)
		return -1;

	if(mode == 2){
		running = 1;
		tmode = 0;
		cameras[board]->setDataMode(2);
		for (channel = 0; channel < cameras[board]->getChannels(); channel++){
			if(cameras[board]->getChmask() & (1<<channel)){
				cam_num[board][channel] = board*MAX_CHANS_PER_BOARD+channel;
				pthread_create(&camera_th[board][channel], NULL, camera_thread_handle, (void*)&cam_num[board][channel]);
			}
		}
	}

	return cameras[board]->startCamera(tmode);
}

int hal_camera_stop(int board){
	int channel = 0;
	running = 0;
	if( 2 == (cameras[board]->getDataMode()) ){
		for (channel = 0; channel < cameras[board]->getChannels(); channel++){
			if(cameras[board]->getChmask() & (1<<channel)){
				pthread_join(camera_th[board][channel], NULL);
			}
		}
	}

    return cards[board] == nullptr ? -1 :cameras[board]->stopCamera();
}

int hal_camera_getStatus(int board){
    return cards[board] == nullptr ? -1 :cards[board]->getCameraStatus();
}

int hal_camera_readYUVData(int board, int ch, frameInfo *img){
    return cards[board] == nullptr ? -1 : cameras[board]->readCamera(ch, img);
}

int hal_camera_read_timeout(int board, int ch, frameInfo *img, int timeout){
    return cards[board] == nullptr ? -1 : cameras[board]->readFrameTimeOut(ch, img, timeout);
}

int hal_camera_poll_timeout(int board, frameInfo *img, int *chan, int timeout){
    return cards[board] == nullptr ? -1 : cameras[board]->pollFrameTimeOut(img, chan, timeout);
}

int hal_camera_readBGRData(int board, int ch, frameInfo *img){
    int ret = -1;
    frameInfo yuvInfo;
    yuvInfo.isMemcpy = 0;
    ret = cameras[board]->readCamera(ch, &yuvInfo);
    if(ret)
        return ret;
    YV12ToBGR24_FFmpeg(yuvInfo.pdata, img->pdata, yuvInfo.width, yuvInfo.height);
    img->timestamp = yuvInfo.timestamp;
    img->exposuretime = yuvInfo.exposuretime;

    return 0;
}

int hal_camera_readRGBData(int board, int ch, frameInfo *img){
    UNUSED(board);
    UNUSED(ch);
    UNUSED(img);

    return 0;
}

int hal_camera_frameStaStatistics(int board, int ch, int* errNum){
    return cards[board] == nullptr ? -1 :  cameras[board]->readFrameCount(ch, errNum);
}

int hal_camera_destroy(int board){

    if(cameras[board]){
        cameras[board]->closeCamera();
    }
    cameras[board].reset();

    if(cards[board]){
        cards[board]->deviceDeinit();
        cards[board].reset();
    }

    printf("%s exit\n", __func__);

    return 0;
}

void hal_camera_data_callback(int board, int ch, camera_data_callback data_cb, void *user)
{
	camera_cb[board][ch] = data_cb;
	UNUSED(user);
}


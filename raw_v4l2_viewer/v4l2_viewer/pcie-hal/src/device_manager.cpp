#include "device_manager.h"
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

deviceManager::deviceManager(int board, int chmask, int width[], int height[], int fps[], int defertime[], int trigger[], int force_init)
{
    this->cameraChannels = chmask;

    this->boardNum = board;
    this->force_init = force_init;
    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        this->width[i] = width[i];
        this->height[i] = height[i] ;
        this->fps[i] = fps[i] ;
        this->deferTime[i] = defertime[i];
        this->trigger[i] = trigger[i];
    }

    char pname[32] = {0};

    if(board == 0){
        strcpy(pname, DEVNODE_1);
        if (access(DEVNODE_1, 0) == 0)
        {
            boardIsExist = true;
        }
        else
        {
            boardIsExist = false;
        }
        printf("%s: %s status:%d\n", __func__, DEVNODE_1, boardIsExist);
    }else if( board== 1){
        strcpy(pname, DEVNODE_2);
        if (access(DEVNODE_2, 0) == 0)
        {
            boardIsExist = true;
        }
        else
        {
            boardIsExist = false;
        }
        printf("%s: %s status:%d\n", __func__, DEVNODE_2, boardIsExist);
    }
    else if( board== 2){
        strcpy(pname, DEVNODE_3);
        if (access(DEVNODE_3, 0) == 0)
        {
            boardIsExist = true;
        }
        else
        {
            boardIsExist = false;
        }
        printf("%s: %s status:%d\n", __func__, DEVNODE_3, boardIsExist);
    }
    else if( board== 3){
        strcpy(pname, DEVNODE_3);
        if (access(DEVNODE_3, 0) == 0)
        {
            boardIsExist = true;
        }
        else
        {
            boardIsExist = false;
        }
        printf("%s: %s status:%d\n", __func__, DEVNODE_3, boardIsExist);
    }

    fd = open(pname, 0);
    if (fd < 0)
    {
        printf("%s open fail, file: %s\r\n", __func__, pname);
        boardIsExist = false;
    }

    // time is based on UTC
    setenv("TZ", "Universal", 1);
    tzset();
}

deviceManager::~deviceManager()
{
    if (boardIsExist)
    {
        if (fd > 0)
        {
            close(fd);
            fd = 0;
        }
    }
}

int deviceManager::deviceInit()
{
    return 0;
    int iCount = 0;
    int iChannel = 0;

    if (boardIsExist)
    { 
        int status;

        /* check driver info */
        if (getDrvInfo() != 0)
        {
            return -1;
        }

        /* check card version */
        (void)getCardVersion();

        /* check camera power status*/
        status = getCameraPower();
        if ( 0 != status )
        {
            if( 0 == force_init )
            {
                printf("%s fail, camera in use, return!!! board/stat: %d/0x%04x\r\n", __func__, boardNum, status);
                return -1;
            }
            else
            {
                printf("%s camera in use, force init!!! board/stat: %d/0x%04x\r\n", __func__, boardNum, status);

                iChannel = cameraChannels;
                cameraChannels = 0x0000;
                setCameraPower();
                usleep(100000);
                cameraChannels = iChannel;
            }
        }

        /* Set camera params*/
        if (setCameraParms() != 0)
        {
            printf("%s err! board: %d\r\n", __func__, boardNum);
            return -1;
        }

        /* camera hardware reset*/
        if (setCameraReset() != 0)
        {
            printf("%s err! board: %d\r\n", __func__, boardNum);
            return -1;
        }

        /* camera power up*/
        if (setCameraPower() != 0)
        {
            printf("%s err! board: %d\r\n", __func__, boardNum);
            return -1;
        }

        /* wait cameras power up ok*/
        iChannel = MAX_CHANS_MASK & cameraChannels;
        do
        {
            usleep(600000);
            status = getCameraStatus();
        } while ( (iChannel != status) && (++iCount < 5) );

        if ( 0 == mcuOtaSelect )
        {
            /* if status is not 1, and no ota channel select, power off */
            if ( 0 == status )
            {
                cameraChannels = 0x0000;
                setCameraPower();
            }
            else
            {
                sleep(3);
                /* get camera version */
                for(int i=0; i<MAX_CHANNELS; i++)
                {
                    if( status & (1<<i) )
                        (void)getCameraVersion(i);
                }
            }
        }

        return status;
    }

    return -1;
}

void deviceManager::deviceDeinit(void)
{
    return;
    if (boardIsExist)
    {
        cameraChannels = 0x0000;
        setCameraPower();
    }
}

int deviceManager::setCameraParms(void)
{
    return 0;
    struct xdma_ioc_cameraparams params;
    int ret;

    if (boardIsExist)
    {
        memset((unsigned char *)&params, 0, sizeof(params));
        params.base.magic = 0X586C0C6C;
        params.ch_nums    = MAX_CHANS_MASK & cameraChannels;
        params.trigger    = this->trigger[0];
        for (int i = 0; i < MAX_CHANNELS; i++)
        {
            params.delay[i] = this->deferTime[i];
        }

        ret = ioctl(fd, XDMA_IOCCAMPARAMS, &params);
        if (ret < 0)
        {
            printf("%s fail, %d! board/ch/tr: %d/0x%02x/%d\r\n", __func__, ret, boardNum, params.ch_nums, params.trigger);
            return -1;
        }
        else
        {
            printf("%s ok! board/ch/tr: %d/0x%02x/%d\r\n", __func__, boardNum, params.ch_nums, params.trigger);
        }

        return 0;
    }

    return -1;
}

int deviceManager::getCameraStatus(void)
{
    return 0;
    struct xdma_ioc_camerastatus status;
    int ret;

    if (boardIsExist)
    {
        status.base.magic = 0X586C0C6C;
        status.ch_status = 0;

        ret = ioctl(fd, XDMA_IOCCAMSTATUS, &status);
        if (ret < 0)
        {
            printf("%s fail, %d! board/val: %d/0x%04x\r\n", __func__, ret, boardNum, status.ch_status);
            return -1;
        }
        else
        {
            printf("%s ok! board/val: %d/0x%04x\r\n", __func__, boardNum, status.ch_status);
        }

        return status.ch_status;
    }

    return 0;
}

string& deviceManager::getCardVersion(void)
{
    version = "1.1";
    return version;
    struct xdma_ioc_cardinfo cardinfo;
    int ret;

    if(!version.empty()) return version;

    if (boardIsExist)
    {
        cardinfo.base.magic = 0X586C0C6C;
        cardinfo.version = 0;

        ret = ioctl(fd, XDMA_IOCCARDINFO, &cardinfo);
        if (ret < 0)
        {
            printf("%s fail, %d! board: %d\r\n", __func__, ret, boardNum);
        }
        else
        {
            char temp[16];
            sprintf(temp, "%08x", cardinfo.version);
            version = temp;
            printf("%s ok! board/ver: %d/%s\r\n", __func__, boardNum, temp);
            printf("board[%d] gprmc/pps: %d/%d\r\n", boardNum, cardinfo.gps_gprmc_status, cardinfo.gps_pps_status);
        }
    }

    return version;
}

int deviceManager::getCameraPower(void)
{
    return 0;
    struct xdma_ioc_camerapower power;
    int ret;

    if (boardIsExist)
    {
        power.base.magic = 0X586C0C6C;
        power.ch_power = 0;

        ret = ioctl(fd, XDMA_IOCCAMPOWERGET, &power);
        if (ret < 0)
        {
            printf("%s fail, %d! board/val: %d/0x%04x\r\n", __func__, ret, boardNum, power.ch_power);
            return -1;
        }
        else
        {
            printf("%s ok! board/val: %d/0x%04x\r\n", __func__, boardNum, power.ch_power);
        }

        return power.ch_power;
    }

    return 0;
}

int deviceManager::setCameraPower(void)
{
    return 0;
    struct xdma_ioc_camerapower power;
    int ret;

    if (boardIsExist)
    {
        power.base.magic = 0X586C0C6C;
        power.ch_power = MAX_CHANS_MASK & cameraChannels;

        ret = ioctl(fd, XDMA_IOCCAMPOWERSET, &power);
        if (ret < 0)
        {
            printf("%s fail, %d! board/val: %d/0x%04x\r\n", __func__, ret, boardNum, power.ch_power);
            return -1;
        }
        else
        {
            printf("%s ok! board/val: %d/0x%04x\r\n", __func__, boardNum, power.ch_power);
        }

        return 0;
    }

    return -1;
}

int deviceManager::setCameraReset(void)
{
    return 0;
    struct xdma_ioc_camerareset reset;
    int ret;

    if (boardIsExist)
    {
        reset.base.magic = 0X586C0C6C;
        reset.ch_reset = 0x1;

        ret = ioctl(fd, XDMA_IOCCAMRESET, &reset);
        if (ret < 0)
        {
            printf("%s fail, %d! board/val: %d/0x%04x\r\n", __func__, ret, boardNum, reset.ch_reset);
            return -1;
        }
        else
        {
            usleep(2000);
            reset.ch_reset = 0x0;
            ret = ioctl(fd, XDMA_IOCCAMRESET, &reset);
            if (ret < 0)
            {
                printf("%s fail, %d! board/val: %d/0x%04x\r\n", __func__, ret, boardNum, reset.ch_reset);
                return -1;
            }
            else
            {
                printf("%s ok! board/val: %d/0x%04x\r\n", __func__, boardNum, reset.ch_reset);
            }
        }

        return 0;
    }

    return -1;
}

int deviceManager::syncTimeToPCIE(void)
{
    return 0;
    struct xdma_ioc_synctime synctime;
    int ret;

    if (boardIsExist)
    {
        synctime.base.magic = 0X586C0C6C;
        ret = ioctl(fd, XDMA_IOCSYNCTIME, &synctime);
        if (ret < 0)
        {
            printf("%s fail, %d! board: %d\r\n", __func__, ret, boardNum);
            return -1;
        }
        else
        {
            printf("%s ok! board: %d\r\n", __func__, boardNum);
        }

        return 0;
    }

    return -1;
}

int deviceManager::getDrvInfo(void)
{
    return 0;
    struct xdma_ioc_drvinfo drvinfo;
    int ret;

    if (boardIsExist)
    {
        drvinfo.base.magic = 0X586C0C6C;
        ret = ioctl(fd, XDMA_IOCDRVINFO, &drvinfo);
        if (ret < 0)
        {
            printf("%s fail, %d! board: %d\r\n", __func__, ret, boardNum);
            return -1;
        }
        else
        {
            printf("%s ok! drv ver:%s, who:%s, time:%s\n", __func__, drvinfo.drv_ver, drvinfo.drv_who, drvinfo.drv_time);
        }

        return 0;
    }

    return -1;
}

int deviceManager::setCameraOtaSelect(int iSelectVal)
{
    return 0;
    int ret = 0;
    struct xdma_ioc_camotasel camotasel;

    if (boardIsExist)
    {
        mcuOtaSelect = iSelectVal;
        camotasel.base.magic = 0X586C0C6C;
        camotasel.cam_ota_select = iSelectVal & 0xFFFF;
        ret = ioctl(fd, XDMA_IOCCAMOTASEL, &camotasel);
        if (ret < 0)
        {
            printf("%s fail, %d! board/val: %d/0x%04x\r\n", __func__, ret, boardNum, camotasel.cam_ota_select);
            return -1;
        }
        else
        {
            printf("%s ok! board/val: %d/0x%04x\r\n", __func__, boardNum, camotasel.cam_ota_select);
        }

        return 0;
    }

    return -1;
}

int deviceManager::CardMcuReset(void)
{
    return 0;
    int ret = 0;
    struct xdma_ioc_camotasel camotasel;

    if (boardIsExist)
    {
        /* set reset gpio high, output rising edge */
        camotasel.base.magic = 0X586C0C6C;
        camotasel.cam_ota_select = 0x00010000;
        ret = ioctl(fd, XDMA_IOCCAMOTASEL, &camotasel);
        if (ret < 0)
        {
            printf("%s fail, %d! board/val: %d/0x%04x\r\n", __func__, ret, boardNum, camotasel.cam_ota_select);
            return -1;
        }
        else
        {
            printf("%s ok! board/val: %d/0x%04x\r\n", __func__, boardNum, camotasel.cam_ota_select);
        }

        /* set reset gpio low */
        usleep(10);
        camotasel.base.magic = 0X586C0C6C;
        camotasel.cam_ota_select = 0x00000000;
        ret = ioctl(fd, XDMA_IOCCAMOTASEL, &camotasel);
        if (ret < 0)
        {
            printf("%s fail, %d! board/val: %d/0x%04x\r\n", __func__, ret, boardNum, camotasel.cam_ota_select);
            return -1;
        }
        else
        {
            printf("%s ok! board/val: %d/0x%04x\r\n", __func__, boardNum, camotasel.cam_ota_select);
        }

        return 0;
    }

    return -1;
}

int deviceManager::getCameraVersion(int cam_channel)
{
    return 0;
    struct xdma_ioc_cameraversion cam_version;
    int ret;

    if (boardIsExist)
    {
        memset((unsigned char *)&cam_version, 0, sizeof(cam_version));
        cam_version.base.magic = 0X586C0C6C;
        cam_version.camera_channel = cam_channel;
        ret = ioctl(fd, XDMA_IOCCAMVERSION, &cam_version);
        if (ret < 0)
        {
            printf("%s fail, %d! board/ch: %d/%d\r\n", __func__, ret, boardNum, cam_channel);
            return -1;
        }
        else
        {
            printf("%s ok! board/ch: %d/%d\r\nver: ", __func__, boardNum, cam_channel);
            for ( unsigned int i = 0 ; i < sizeof(cam_version.cam_isp_version) ; i++ )
            {
                printf("%02x ", cam_version.cam_isp_version[i]);
            }
            printf("\r\n");
        }

        return 0;
    }

    return -1;
}


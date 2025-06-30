#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <string>
#include <iostream>

#define MAX_CHANNELS    (8)
#define MAX_CHANS_MASK  (0xFF)

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ltohl(x) (x)
#define ltohs(x) (x)
#define htoll(x) (x)
#define htols(x) (x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ltohl(x) __bswap_32(x)
#define ltohs(x) __bswap_16(x)
#define htoll(x) __bswap_32(x)
#define htols(x) __bswap_16(x)
#endif


#define DEVNODE_1 "/dev/xdma0_user"
#define DEVNODE_2 "/dev/xdma1_user"
#define DEVNODE_3 "/dev/xdma2_user"
#define DEVNODE_4 "/dev/xdma3_user"

#define IICCONTROL        0x1000
#define IICADDRESS        0x1004
#define IICLENGTH         0x1008
#define IICDATA           0x100C

#define IICSTART          1<<1UL
#define IICCLEAR          1<<1UL

#define uint32_t    unsigned int

struct xdma_ioc_base {
        unsigned int magic;
        unsigned int command;
};

struct xdma_ioc_synctime {
        struct xdma_ioc_base base;
        unsigned int synctime;
};

struct xdma_ioc_cameraparams{
        struct xdma_ioc_base base;
        unsigned ch_nums;
        unsigned width_chA;  // 0~3 channels
        unsigned height_chA;
        unsigned fps_chA;
        unsigned delay[8];
        unsigned trigger;    // 0 uncontrol trigger, 1 side to senconds trigger
        unsigned width_chB;  // 4~7 channels
        unsigned height_chB;
        unsigned fps_chB;
};

struct xdma_ioc_camerastatus{
        struct xdma_ioc_base base;
        unsigned int ch_status;
};

struct xdma_ioc_camerapower{
        struct xdma_ioc_base base;
        unsigned int ch_power;
};

struct xdma_ioc_camerareset{
        struct xdma_ioc_base base;
        unsigned int ch_reset;
};

struct xdma_ioc_cameraversion{
    struct xdma_ioc_base base;
    unsigned camera_channel;
    unsigned char cam_isp_version[16];
    unsigned camera_isp_temp;
};

struct xdma_ioc_drvinfo{
    struct xdma_ioc_base base;
    char drv_ver[32];
    char drv_who[32];
    char drv_time[32];
    char reserved[32];
};

struct xdma_ioc_camotasel{
    struct xdma_ioc_base base;
    unsigned cam_ota_select;
};

struct xdma_ioc_cardinfo{
    struct xdma_ioc_base base;
    unsigned version;
    unsigned gps_gprmc_status;
    unsigned gps_pps_status;
};

#define XDMA_IOC_MAGIC  'x'
enum XDMA_IOC_TYPES {
        XDMA_IOC_NOP,
        XDMA_IOC_INFO,
        XDMA_IOC_ICAP_DOWNLOAD,
        XDMA_IOC_MCAP_DOWNLOAD,
        XDMA_IOC_HOT_RESET,
        XDMA_IOC_OCL_RESET,
        XDMA_IOC_OCL_FREQ_SCALING,
        XDMA_IOC_CTRL_SYNCTIME,
        XDMA_IOC_CTRL_CAMPARAMS,
        XDMA_IOC_CTRL_CAMSTATUS,
        XDMA_IOC_CTRL_CAMPOWERGET,
        XDMA_IOC_CTRL_CAMPOWERSET,
        XDMA_IOC_CTRL_CAMRESET,
        XDMA_IOC_CTRL_CAMVERSION,
        XDMA_IOC_CTRL_DRVINFO,
        XDMA_IOC_CTRL_CAMOTASEL,
        XDMA_IOC_CTRL_CARDINFO,
        XDMA_IOC_MAX
};

#define XDMA_IOCSYNCTIME        _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_CTRL_SYNCTIME,       struct xdma_ioc_synctime)
#define XDMA_IOCCAMPARAMS       _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_CTRL_CAMPARAMS,      struct xdma_ioc_cameraparams)
#define XDMA_IOCCAMSTATUS       _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_CTRL_CAMSTATUS,      struct xdma_ioc_camerastatus)
#define XDMA_IOCCAMPOWERGET     _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_CTRL_CAMPOWERGET,    struct xdma_ioc_camerapower)
#define XDMA_IOCCAMPOWERSET     _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_CTRL_CAMPOWERSET,    struct xdma_ioc_camerapower)
#define XDMA_IOCCAMRESET        _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_CTRL_CAMRESET,       struct xdma_ioc_camerareset)
#define XDMA_IOCCAMVERSION      _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_CTRL_CAMVERSION,     struct xdma_ioc_cameraversion)
#define XDMA_IOCDRVINFO         _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_CTRL_DRVINFO,        struct xdma_ioc_drvinfo)
#define XDMA_IOCCAMOTASEL       _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_CTRL_CAMOTASEL,      struct xdma_ioc_camotasel)
#define XDMA_IOCCARDINFO        _IOWR(XDMA_IOC_MAGIC, XDMA_IOC_CTRL_CARDINFO,       struct xdma_ioc_cardinfo)

using namespace std;
class deviceManager
{
  public:
    deviceManager(int board, int chmask,int width[], int height[], int fps[], int defertime[], int trigger[], int force_init);
    ~deviceManager();
    int deviceInit();
    void deviceDeinit(void);

    int getCameraStatus(void);
    int getCameraPower(void);
    string& getCardVersion(void);
    int syncTimeToPCIE(void);
    int getDrvInfo(void);
    int setCameraOtaSelect(int iSelectVal);
    int CardMcuReset(void);
    int getCameraVersion(int cam_channel);

  private:
    int setCameraParms(void);
    int setCameraPower(void);
    int setCameraReset(void);

  private:

#if 0
    void write_control(void *base_addr, int offset, uint32_t val);
    uint32_t read_control(void *base_addr, int offset);
    void*  mmap_control(int fd,long mapsize);

    int pcieWrite(unsigned char proto_addr, char *data, int length);
    int pcieRead(unsigned char proto_addr, unsigned char *data, int length);

    int iicWrite(unsigned char iicaddr, unsigned char *data, int length);
    int iicRead(unsigned char iicaddr, unsigned char *data, int length);
#endif

    int cameraChannels;
    int boardNum;
    bool boardIsExist;
    int cameraReset;
    int width[MAX_CHANNELS];
    int height[MAX_CHANNELS];
    int fps[MAX_CHANNELS];
    int deferTime[MAX_CHANNELS];
    int trigger[MAX_CHANNELS];
    int fov[MAX_CHANNELS];
    int fd;
    string version;
    int force_init;
    int mcuOtaSelect;
};

#endif

#ifndef __CAMERA_COMMON_H
#define __CAMERA_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BOARD              (4)
#define MAX_CHANS_PER_BOARD    (8)
#define MAX_CHANS_MASK         (0xFF)
#define MAX_CHANS_CAM_IMX      (8)
#define MAX_CHANS_CAM_OV       (4)

typedef struct __camera_info{
    int width[MAX_CHANS_PER_BOARD];
    int height[MAX_CHANS_PER_BOARD];
    int fps[MAX_CHANS_PER_BOARD];
    int defertime[MAX_CHANS_PER_BOARD];    // trigger defer time, unit: us
    int trigger_mode[MAX_CHANS_PER_BOARD];  // trigger mode, 0 ramdom, 1 side to seconds

    int channel_mask;       //chmask is 0x00 - 0xff, one bit indicate one channel
    int group;              //which group: group 0 is BoardA, group 1 is BoardB.
    int force_init;         //force to reset and poweron camera in init, no mater whether in use

    int cam_otaselect[MAX_CHANS_PER_BOARD]; //ota select channel, single channel
    int card_mcureset;      //mcu on pcie card, reset
}camera_info;

typedef struct __frameInfo
{
    int index;  // frame index in picture
    int width;
    int height;
    int filed;
    int length;
    int isMemcpy;
    unsigned int exposuretime; // ns
    long long timestamp; // picture timestamp: ns
    long long systime; // pc recv timestamp: ns
    unsigned char *pdata;
    int reserved;  // pc recv count
    long long reserved1;

}frameInfo;


#ifdef __cplusplus
}
#endif
#endif

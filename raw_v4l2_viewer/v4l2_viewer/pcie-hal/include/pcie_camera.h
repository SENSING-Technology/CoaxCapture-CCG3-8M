#ifndef __PCIE_CAMERA_H
#define __PCIE_CAMERA_H

#include "camera_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
* brief: initialize pcie camera.
* input:   
*       camera_info is initialize camera information
* output:
*       return 0 is successed, return -1 is failed
**/
int pcie_camera_init(camera_info* info);

int hal_camera_start(int board, int mode);

int hal_camera_stop(int board);
/*
* brief: get pcie card version.
* output:
*       return: (transfer none): the device version, NULL on error
**/
const char *pcie_camera_GetVersion();

/*
* brief: get current cameras status.
* input: 
* output: 
*       return current all cameras status, bit filed: 0 is camera have issue, 1 is init ok
**/
int pcie_camera_getStatus(void);

/*
*  brief: from which channel get image. use memcpy method, no block.
*  input:
*       img: frameInfo struct for get yuv data.
*  output:
*       return 0: read successed, -1: read failed.
**/
int pcie_camera_readYUVData(int ch, frameInfo *img);

/*
*  brief: from which channel get image. use memcpy method, ours library convert yuv to BGR, no block.
*  input:
*       img: frameInfo struct for get BGR data.
*  output:
*       return 0: read successed, -1: read failed.
**/
int pcie_camera_readBGRData(int ch, frameInfo *img);

/*
*  brief: from which channel get image. use memcpy method, ours library convert yuv to RGB, no block.
*  input:
*       img: frameInfo struct for get RGB data.
*  output:
*       return 0: read successed, -1: read failed.
**/
int pcie_camera_readRGBData(int ch, frameInfo *img);

/*
 * brief: frame which channel get all frame numbers and fault frames
 * input: 
 *       ch: which channel
 *       errNum: obtain fault frames
 * output:
 *       return all frame numbers
 * */
int pcie_camera_frameStaStatistics(int ch, int* errNum);

/*
 * brief: release cameras
 *
 * */
int pcie_camera_destroy(void);

#ifdef __cplusplus
}
#endif
#endif

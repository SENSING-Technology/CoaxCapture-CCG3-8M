#ifndef __HAL_CAMERA_H
#define __HAL_CAMERA_H

#include "camera_common.h"
#include <stdint.h>

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
int hal_camera_init(camera_info* info);

/*
* brief: start pcie camera.
* input:   
*       board: 0 or 1
*       mode: 0 for hal_camera_read_timeout, 1 for hal_camera_poll_timeout 
* output:
*       return 0 is successed, return -1 is failed
**/
int hal_camera_start(int board, int mode);

/*
* brief: stop pcie camera.
* input:   
*       board: 0 or 1
* output:
*       return 0 is successed, return -1 is failed
**/
int hal_camera_stop(int board);

/*
* brief: get pcie card version.
* output:
*       return: (transfer none): the device version, NULL on error
**/
const char *hal_camera_GetVersion(int board);

/*
* brief: get current cameras status.
* input: 
* output: 
*       return current all cameras status, bit filed: 0 is camera have issue, 1 is init ok
**/
int hal_camera_getStatus(int board);

/*
*  brief: from which channel get image. use memcpy method.
*  input:
*       board:  0 or 1
*       ch:     number in 0~7
*       img: frameInfo struct for get yuv data.
*       timeout:  in ms units
*  output:
*       return 0: read successed, -1: read failed.
**/
int hal_camera_read_timeout(int board, int ch, frameInfo *img, int timeout);

/*
*  brief: from which channel get image. use memcpy method.
*  input:
*       board:  0 or 1       
*       img: frameInfo struct for get yuv data.
*       timeout:  in ms units
*  output:
*       ch:   number in 0~7  which channel get a picture
*  return:
*       0: read successed, -1: read failed.
**/
int hal_camera_poll_timeout(int board, frameInfo *img, int *chan, int timeout);


typedef void (*camera_data_callback)(int32_t msg_type, const frameInfo* img);
void hal_camera_data_callback(int board, int ch,camera_data_callback data_cb, void *user);

/*
 * brief: release cameras
 * input:   
 *       board: 0 or 1
 * */
int hal_camera_destroy(int board);

#ifdef __cplusplus
}
#endif
#endif

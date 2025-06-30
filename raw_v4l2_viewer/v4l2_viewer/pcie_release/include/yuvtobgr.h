#ifndef __YUVTOBGR_H
#define __YUVTOBGR_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
}

#ifdef __cplusplus
extern "C" {
#endif
int Pixel_Tra_FFmpeg(unsigned char* pSrc,unsigned char* pDst,int width,int height,AVPixelFormat pix_src,AVPixelFormat pix_dst);
int YV12ToBGR24_FFmpeg(unsigned char* pYUV,unsigned char* pBGR24,int width,int height);
//int YUYVToNV(unsigned char* pSrc,unsigned char* pDst,int width,int height);
//int GrayToNV12(unsigned char* pSrc,unsigned char* pDst,int width,int height);
//int GrayToUYVY(unsigned char* pSrc,unsigned char* pDst,int width,int height);
int Scale_Frame(unsigned char* pSrc,unsigned char* pDst,int src_width, int src_height,int dst_width,int dst_height,AVPixelFormat pix_src,AVPixelFormat pix_dst);
#ifdef __cplusplus
}
#endif
#endif




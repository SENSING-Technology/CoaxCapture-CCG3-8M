extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "yuvtobgr.h"

int Pixel_Tra_FFmpeg(unsigned char* pSrc,unsigned char* pDst,int width,int height,AVPixelFormat pix_src,AVPixelFormat pix_dst)
{
    if (width < 1 || height < 1 || pSrc == NULL || pDst == NULL)
        return -1;

    AVPicture pFrameSrc,pFrameDst;    
    avpicture_fill(&pFrameSrc,pSrc,pix_src,width,height);
    avpicture_fill(&pFrameDst,pDst,pix_dst,width,height);

    struct SwsContext* imgCtx = NULL;
    imgCtx = sws_getContext(width,height,pix_src,width,height,pix_dst,SWS_BILINEAR,0,0,0);
    
    if (imgCtx != NULL){
        sws_scale(imgCtx,pFrameSrc.data,pFrameSrc.linesize,0,height,pFrameDst.data,pFrameDst.linesize);
        if(imgCtx){
            sws_freeContext(imgCtx);
            imgCtx = NULL;
        }
        return 0;
    }
    else{
        sws_freeContext(imgCtx);
        imgCtx = NULL;
        return -1;
    }
}

int Scale_Frame(unsigned char* pSrc,unsigned char* pDst,int src_width, int src_height,int dst_width,int dst_height,AVPixelFormat pix_src,AVPixelFormat pix_dst)
{
    if (src_width < 1 || src_height < 1 || pSrc == NULL || pDst == NULL)
        return -1;

    //AVPixelFormat pix_src = AV_PIX_FMT_YUV422P;
    //AVPixelFormat pix_dst = AV_PIX_FMT_UYVY422;
    AVPicture pFrameSrc,pFrameDst;    
    avpicture_fill(&pFrameSrc,pSrc,pix_src,src_width,src_height);
    avpicture_fill(&pFrameDst,pDst,pix_dst,dst_width,dst_height);
    struct SwsContext* imgCtx = NULL;
    imgCtx = sws_getContext(src_width,src_height,pix_src,dst_width,dst_height,pix_dst,SWS_BICUBLIN,0,0,0);

    if (imgCtx != NULL){
        sws_scale(imgCtx,pFrameSrc.data,pFrameSrc.linesize,0,src_height,pFrameDst.data,pFrameDst.linesize);
        if(imgCtx){
            sws_freeContext(imgCtx);
            imgCtx = NULL;
        }
        return 0;
    }
    else{
        sws_freeContext(imgCtx);
        imgCtx = NULL;
        return -1;
    }
}

int GrayToNV12(unsigned char* pSrc,unsigned char* pDst,int width,int height)
{
    Pixel_Tra_FFmpeg(pSrc,pDst,width,height,AV_PIX_FMT_GRAY16LE,AV_PIX_FMT_NV12);
    return 0;
}

int GrayToUYVY(unsigned char* pSrc,unsigned char* pDst,int width,int height)
{
    Pixel_Tra_FFmpeg(pSrc,pDst,width,height,AV_PIX_FMT_YUYV422,AV_PIX_FMT_UYVY422);
    return 0;
}

int YV12ToBGR24_FFmpeg(unsigned char* pYUV,unsigned char* pBGR24,int width,int height)
{
    if (width < 1 || height < 1 || pYUV == NULL || pBGR24 == NULL)
        return -1;

    AVPicture pFrameYUV,pFrameBGR;
    
    avpicture_fill(&pFrameYUV,pYUV,AV_PIX_FMT_UYVY422,width,height);

    //U,V互换
    uint8_t * ptmp=pFrameYUV.data[1];
    pFrameYUV.data[1]=pFrameYUV.data[2];
    pFrameYUV.data [2]=ptmp;

    avpicture_fill(&pFrameBGR,pBGR24,AV_PIX_FMT_BGR24,width,height);

    struct SwsContext* imgCtx = NULL;
    imgCtx = sws_getContext(width,height,AV_PIX_FMT_UYVY422,width,height,AV_PIX_FMT_BGR24,SWS_BILINEAR,0,0,0);

    if (imgCtx != NULL){
        sws_scale(imgCtx,pFrameYUV.data,pFrameYUV.linesize,0,height,pFrameBGR.data,pFrameBGR.linesize);
        if(imgCtx){
            sws_freeContext(imgCtx);
            imgCtx = NULL;
        }
        return 0;
    }
    else{
        sws_freeContext(imgCtx);
        imgCtx = NULL;
        return -1;
    }
}

/*
int main(int argc, char* argv[]){
    struct timeval time1, time2;
    if(argc <2){
        printf("usage: ./yuvToBGR xxx.yuv \n");
        return -1;
    }

    FILE* fyuv = fopen(argv[1], "r+");
    if(fyuv > 0){
        unsigned char* pyuv = (unsigned char*)malloc(1280*720*2);
        fread(pyuv, 1, 1280*720*2, fyuv);
        fclose(fyuv);

        unsigned char* pbgr = (unsigned char*)malloc(1280*720*3);
        gettimeofday(&time1, NULL);
        YV12ToBGR24_FFmpeg(pyuv, pbgr, 1280, 720);
        gettimeofday(&time2, NULL);
        printf("timelost: %ld\n", time2.tv_usec - time1.tv_usec);
        FILE* fbgr = fopen("test.rgb", "w+");
        if(fbgr){
                fwrite(pbgr, 1, 1280*720*3, fbgr);
                fflush(fbgr);
                fclose(fbgr);
        }
        
        free(pbgr);
        free(pyuv);
    }
}
*/


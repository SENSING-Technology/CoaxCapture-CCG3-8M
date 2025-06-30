#ifndef __WATERMARK_H
#define __WATERMARK_H

#ifdef __cplusplus
extern "C" {
#endif
int waterMark(unsigned char *frame_buffer_in, unsigned char *frame_buffer_out,int w,int h,const char *str);
#ifdef __cplusplus
}
#endif
#endif
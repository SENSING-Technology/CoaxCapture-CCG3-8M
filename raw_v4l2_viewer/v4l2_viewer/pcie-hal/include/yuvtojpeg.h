#ifndef __YUVTOJPEG_H
#define __YUVTOJPEG_H

class YuvToJpeg
{
  public:
    YuvToJpeg(int width, int height, unsigned char* src, unsigned char* dsc);
    ~YuvToJpeg();
    int yuyvToyuv(void);
    int yuv422pToJpeg(char *filename, int quality);
    unsigned char *srcPtr;
    unsigned char *dscPtr; 

  private:
    int image_width;
    int image_height;
};

#endif
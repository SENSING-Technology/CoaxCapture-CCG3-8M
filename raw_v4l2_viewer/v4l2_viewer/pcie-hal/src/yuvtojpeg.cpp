#include "yuvtojpeg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jpeglib.h"
#include <sys/time.h>

YuvToJpeg::YuvToJpeg(int width, int height, unsigned char *src, unsigned char *dsc)
{
    image_width = width;
    image_height = height;
    srcPtr = src;
    dscPtr = dsc;
//    printf("%s: %d * %d\n", __func__, image_width, image_height);
}

YuvToJpeg::~YuvToJpeg(){
}
int YuvToJpeg::yuv422pToJpeg(char *filename, int quality)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1]; /* pointer to JSAMPLE row[s] */
    int row_stride;          /* physical row width in image buffer */
    JSAMPIMAGE buffer;
    int band, i, buf_width[3], buf_height[3], mem_size, max_line, counter;
    unsigned char *yuv[3];
    unsigned char *pSrc, *pDst;
    FILE *fp = NULL;

    yuv[0] = dscPtr;
    yuv[1] = yuv[0] + (image_width * image_height);
    yuv[2] = yuv[1] + (image_width * image_height) / 2;

    if (filename != NULL)
    {
        if ((fp = fopen(filename, "wb")) == NULL)
        {
            fprintf(stderr, "can't open %s\n", filename);
            return -1;
        }
    }
    else
    {
        return -1;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);

    cinfo.image_width = image_width; /* image width and height, in pixels */
    cinfo.image_height = image_height;
    cinfo.input_components = 3;     /* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB; /* colorspace of input image */

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    cinfo.raw_data_in = TRUE;
    cinfo.jpeg_color_space = JCS_YCbCr;
    cinfo.comp_info[0].h_samp_factor = 2;
    cinfo.comp_info[0].v_samp_factor = 1;

    jpeg_start_compress(&cinfo, TRUE);

    buffer = (JSAMPIMAGE)(*cinfo.mem->alloc_small)((j_common_ptr)&cinfo, JPOOL_IMAGE, 3 * sizeof(JSAMPARRAY));
    for (band = 0; band < 3; band++)
    {
        buf_width[band] = cinfo.comp_info[band].width_in_blocks * DCTSIZE;
        buf_height[band] = cinfo.comp_info[band].v_samp_factor * DCTSIZE;
        buffer[band] = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, buf_width[band], buf_height[band]);
    }
    max_line = cinfo.max_v_samp_factor * DCTSIZE;
    for (counter = 0; cinfo.next_scanline < cinfo.image_height; counter++)
    {
        //buffer image copy.
        for (band = 0; band < 3; band++)
        {
            mem_size = buf_width[band];
            pDst = (unsigned char *)buffer[band][0];
            pSrc = (unsigned char *)yuv[band] + counter * buf_height[band] * buf_width[band];

            for (i = 0; i < buf_height[band]; i++)
            {
                memcpy(pDst, pSrc, mem_size);
                pSrc += buf_width[band];
                pDst += buf_width[band];
            }
        }
        jpeg_write_raw_data(&cinfo, buffer, max_line);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(fp);

    return 0;
}

int YuvToJpeg::yuyvToyuv(void)
{
    int i, j;
    int offset = 0;
    unsigned char *py, *pv, *pu;

    if(dscPtr == NULL|| srcPtr == NULL){
        printf("%s, Warn: dscptr is null\n", __func__);
        return -1;
    }

    py = (unsigned char *)dscPtr;
    pu = py + image_width * image_height;
    pv = pu + image_width * image_height / 2;

    for (i = 0; i < image_height; i++)
    {
        for (j = 0; j < image_width * 2; j += 4)
        {
            offset = i * image_width * 2;
            *py++ = srcPtr[offset + j];
            *pu++ = srcPtr[offset + j + 1];
            *py++ = srcPtr[offset + j + 2];
            *pv++ = srcPtr[offset + j + 3];
        }
    }
}

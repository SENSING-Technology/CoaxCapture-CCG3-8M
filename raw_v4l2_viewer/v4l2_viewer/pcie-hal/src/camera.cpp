#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <asm/types.h>
#include <linux/videodev2.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include "camera.h"

#define FRAMEFIFODEPTH 6

// 1/20/1688 = 29.62us
#define IMX390_ONLINE_TIME 29620  //ns

//1/48 * 1464 = 30.5us
#define OV10640_ONLINE_TIME 30500 //ns

//#define YUV_DUMP

Camera::Camera(int channel, int fps, int width, int height)
{

    int i = 0;
    this->width = width;
    this->height = height;
    printf("camera 36\n");
    diffTimeValue = 1000000000UL/fps - 2000000;
//    printf("ch[%d], diffTimeValue:%lld\n", channel, diffTimeValue);
    pixSize = width * height * 2;
    buffer_count = 0;
    buffers = NULL;

    readIndex = 0;
    writeIndex = 0;
    oldTimestamp = 0;
    printf("camera 46\n");
    for (i = 0; i < FRAMEFIFODEPTH; i++)
    {
        buffer_t *buff = (buffer_t *)malloc(sizeof(buffer_t));
        buff->start = NULL; //(unsigned char *)malloc(this->pixSize);
        buff->length = this->pixSize;
        videoFifo.push_back(buff);
    }
     printf("camera 54 channel%d\n",channel);
    this->channel = channel;
    oldTimestamp = 0;
    lostCount = 0;

    pthread_mutex_init(&indexLock, NULL);
    printf("Create channel %02d succcessed\n", channel);
}

int Camera::xioctl(int fd, int request, void *arg)
{
    for (int i = 0; i < 100; i++)
    {
        int r = ioctl(fd, request, arg);
        if (r != -1 || errno != EINTR)
            return r;
    }
    return -1;
}

int Camera::CameraOpen(void)
{
    int ret;

    char namepath[128] = {0};
    memset(namepath, 0, sizeof(namepath));
    sprintf(namepath, "/dev/video%d", channel);

    if (access(namepath, F_OK) != 0)
    {
        printf("%s not axist!!!\n", namepath);
        return -1;
    }

    fd = open(namepath, O_RDWR | O_NONBLOCK, 0);
    if (fd == -1)
    {
        printf("open %s failed\n", namepath);
        return -1;
    }
    printf("open %s Successed, fd = %d\n", namepath, fd);

    struct v4l2_capability cap;
    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
        printf("VIDIOC_QUERYCAP\n");

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        printf("no capture\n");
    if (!(cap.capabilities & V4L2_CAP_STREAMING))
        printf("no streaming\n");

    //Enum all support formats
    struct v4l2_fmtdesc fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.index = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while ((ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmt)) == 0)
    {
        fmt.index++;
        printf("{ pixelformat = '%c%c%c%c', description = '%s' }\n",
               fmt.pixelformat & 0xFF, (fmt.pixelformat >> 8) & 0xFF,
               (fmt.pixelformat >> 16) & 0xFF, (fmt.pixelformat >> 24) & 0xFF,
               fmt.description);
    }

    printf("format: width: %d, height: %d,\n",width, height);
    //Try to set picture format, drvier can modify params
    struct v4l2_format format;
    memset(&format, 0, sizeof format);
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
    //format.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
    format.fmt.pix.field = V4L2_FIELD_ANY;
    if (xioctl(fd, VIDIOC_TRY_FMT, &format) == -1)
        printf("VIDIOC_TRY_FMT failed\n");

    printf("Try format: width: %d, height: %d,\n", format.fmt.pix.width, format.fmt.pix.height);
    printf("{ pixelformat = '%c%c%c%c'}\n",
           format.fmt.pix.pixelformat & 0xFF, (format.fmt.pix.pixelformat >> 8) & 0xFF,
           (format.fmt.pix.pixelformat >> 16) & 0xFF, (format.fmt.pix.pixelformat >> 24) & 0xFF);

    //Set picture format
    //format.fmt.pix.height+=1;
    printf("Try format: width: %d, height: %d,\n", format.fmt.pix.width, format.fmt.pix.height);
    if (xioctl(fd, VIDIOC_S_FMT, &format) == -1)
        printf("VIDIOC_S_FMT falied\n");

    width = format.fmt.pix.width;
    height = format.fmt.pix.height;

    //Require buffer numbers from driver
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof req);
    req.count = FRAMEFIFODEPTH;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1)
        printf("VIDIOC_REQBUFS\n");
    buffer_count = req.count;
    printf("mmap buffer count: %d\n", buffer_count);
    buffers = (buffer_t *)calloc(req.count, sizeof(buffer_t));

    //Require mmap buffers from driver
    size_t buf_max = 0;
    for (int i = 0; i < buffer_count; i++)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
            printf("VIDIOC_QUERYBUF\n");
        if (buf.length > buf_max)
            buf_max = buf.length;
        
        printf("querybuf buf.length:%d, offset:%d\n", buf.length, buf.m.offset);
        buffers[i].length = buf.length;
        buffers[i].start = (unsigned char *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                                        fd, buf.m.offset);
        if (buffers[i].start == MAP_FAILED)
            printf("mmap\n");
    }
    printf("mmap buffer end.\n");

    return 0;
}

int Camera::CameraStart(void)
{

    for (int i = 0; i < buffer_count; i++)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (xioctl(fd, VIDIOC_QBUF, &buf) == -1)
            printf("VIDIOC_QBUF\n");
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) == -1)
        printf("VIDIOC_STREAMON\n");

    return 0;
}

#if 0
struct embedTime{
    int tv_nsec;
    int tv_sec;
};
#endif

#define EXPOSURE_L ( width * (height-1) * 2 )
#define EXPOSURE_H ( width * (height-1) * 2 + 1 )


/* imx390
 * T0: isp vsync time   
 * T1: sensor vsync time, T1 = T0 - (3910-384) us
 * T1 - (Texp/2) + (1/20/1688) * 1000000 * 563  us
 * finally:
 * T0 - (Texp/2)  + 13150
 * 
 */

/* ov10640
 * T0: isp vsync time   
 * T1: sensor vsync time, T1 = T0 - 30 us
 * T1 - (Texp/2) + 30.5*480   us
 * Texp = (exp_L+4)*30.5 us
 * finally:
 * T0 - 15.25*exp_L + 14610
 */


#pragma pack(4)
struct embedTime{
    int ns;
    int idx;  // frame counter
    int second:8;
    int minitue:8;
    int hour:8;
    int reserved:8;

    int day:8;
    int month:8;
    int year:8;  // base from 2000
    int reserved1:8;

    char invalid[48];

    char exposure[6];  //for ov10640 exp_L_H,exp_L_L,exp_S_H,exp_S_L,exp_VS_H,exp_VS_L,
    char gain[6];      //for ov10640 gain_L_H,gain_L_L,gain_S_H,gain_S_L,gain_VS_H,gain_VS_L,
};

char bcd2char(char bcd)
{
    return bcd%16+(bcd>>4)*10;
}

time_t bcd2unix(struct embedTime* bcd)
{
    struct tm  dt;

    dt.tm_sec = bcd2char(bcd->second);
    dt.tm_min = bcd2char(bcd->minitue);
    dt.tm_hour = bcd2char(bcd->hour);

    dt.tm_year= bcd2char(bcd->year) + 100; //2000 - 1900;   based from 2000
    dt.tm_mon = bcd2char(bcd->month) - 1;  // struct tm: month start from 0
    dt.tm_mday = bcd2char(bcd->day);

    time_t t = mktime(&dt);

    return(t);
}

int Camera::CameraCapture(void)
{
    struct v4l2_buffer buf;
    int index, frameidx;
    unsigned int exposuretime;
    unsigned int exposuretime1;


    memset(&buf, 0, sizeof buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1){
		printf("debuf failed\n");
        return false;
	}

    //obtain timestamp
    long long timestamp;
    {
        struct embedTime* ptime = (struct embedTime*)buffers[buf.index].start;
        //timestamp = ptime->tv_nsec + (long long)ptime->tv_sec*1000000000;
        timestamp = bcd2unix(ptime)*1000000000L + ptime->ns;
        frameidx = ptime->idx;
		exposuretime  = ( (buffers[buf.index].start[EXPOSURE_H] & 0xFF)<<8 | (buffers[buf.index].start[EXPOSURE_L] & 0xFF) ) * IMX390_ONLINE_TIME;
		/*printf("0-timestamp:%lld exposure:%lld 0x%x 0x%x\n",
				timestamp,exposuretime,buffers[buf.index].start[EXPOSURE_H], buffers[buf.index].start[EXPOSURE_L]);*/
		exposuretime1 = ( (ptime->exposure[0]  & 0xFF)<<8 | (ptime->exposure[1]  & 0xFF) )  * OV10640_ONLINE_TIME;
		/*printf("1-timestamp:%lld exposure:%lld 0x%x 0x%x\n",
				timestamp,exposuretime1,buffers[buf.index].start[0x40],buffers[buf.index].start[0x41]);*/
	}

    //read frame data
    pthread_mutex_lock(&indexLock);
    index = writeIndex % FRAMEFIFODEPTH;
    videoFifo[index]->start = buffers[buf.index].start;
    videoFifo[index]->length = buf.bytesused;
    videoFifo[index]->timestamp = timestamp;
    videoFifo[index]->systime = buf.timestamp.tv_sec*1000000000L + buf.timestamp.tv_usec*1000L;
    videoFifo[index]->frameid = frameidx;
    videoFifo[index]->reserved = buf.sequence;
    videoFifo[index]->exposuretime  = exposuretime;
    videoFifo[index]->exposuretime1 = exposuretime1;
    writeIndex++;
    //printf("CameraCapture buf.seq:%d buf.bytesused:%d timestamp:%d systeme:%lld\n",
    //buf.sequence,buf.bytesused,buf.timestamp.tv_sec,videoFifo[index]->systime);
    pthread_mutex_unlock(&indexLock);


	if (xioctl(fd, VIDIOC_QBUF, &buf) == -1){
		printf("qbuf failed\n");
		return false;
	}
    return true;
}

int Camera::getFrameCount(int* errNum){

    if(errNum) *errNum = lostCount;

    return writeIndex;
}

int Camera::getYuvFrame(buffer_t *frame)
{
    int index, ret = -1;

    pthread_mutex_lock(&this->indexLock);
    //printf("readindex:%d writeindex:%d\n",readIndex, writeIndex);
    index = readIndex % FRAMEFIFODEPTH;
    if(readIndex < writeIndex - FRAMEFIFODEPTH + 1){
        printf("Warn: ch[%d]user have not obtain frame, count:%d\n", channel, writeIndex - readIndex - FRAMEFIFODEPTH);
        readIndex = writeIndex - FRAMEFIFODEPTH + 1;
    }
    if (readIndex < writeIndex)
    {
        frame->start = videoFifo[index]->start;
        frame->length = videoFifo[index]->length;
        frame->timestamp = videoFifo[index]->timestamp;
        frame->systime = videoFifo[index]->systime;
        frame->exposuretime = videoFifo[index]->exposuretime;
		/*printf("++++%lld\n", videoFifo[index]->exposuretime1);*/
        frame->exposuretime1 = videoFifo[index]->exposuretime1;
        frame->frameid = videoFifo[index]->frameid;
        frame->reserved = videoFifo[index]->reserved;
        readIndex++;
        ret = 0;
    }
    pthread_mutex_unlock(&this->indexLock);

    return ret;
}

int Camera::getFrameFormat(int& width,int&height)
{
    width = this->width;
    height = this->height;
    return 0;
}

int Camera::CameraStop(void)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMOFF, &type) == -1)
        printf("VIDIOC_STREAMOFF\n");
    return 0;
}

void Camera::CameraClose(void)
{
    int i;
    //Release mmap buffers.
    for (i = 0; i < buffer_count; i++)
    {
        munmap(buffers[i].start, buffers[i].length);
    }
    free(buffers);
    buffer_count = 0;
    buffers = NULL;

    //Free fifo buffers.
    for (i = 0; i < FRAMEFIFODEPTH; i++)
    {
        if (videoFifo[i])
            free(videoFifo[i]);
    }
    std::vector<buffer_t *>().swap(videoFifo);

    //Disable fd
    close(fd);

}

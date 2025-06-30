#ifndef __CAMERA_H
#define __CAMERA_H

#include <sys/types.h>
#include <string>
#include <pthread.h>
#include <iostream>
#include <vector>
#include <iostream>
#include <linux/videodev2.h>


typedef struct
{
    unsigned char *start;
    size_t length;
    int frameid;
    unsigned int exposuretime; // ns
    unsigned int exposuretime1; // ns
    long long timestamp; // picture timestamp: ns
    long long systime; // pc recv timestamp: ns
    int reserved;  // pc recv index
    int reserved1;
} buffer_t;

class Camera
{
  public:
    Camera(int channel, int fps, int width, int height);

    int CameraOpen(void);
    int CameraStart(void);
    int CameraCapture();
    int CameraStop(void);
    void CameraClose(void);

    int getFd(void){return fd;}
    int getChannel(void){return channel;}
    int getFrameCount(int* errNum);
    int getYuvFrame(buffer_t *frame);
    int getFrameFormat(int& width,int&height);



  private:
    int xioctl(int fd, int request, void *arg);

    int fd;
    int channel;
    int width;
    int height;
    int pixSize;

    int buffer_count;
    buffer_t *buffers;

    int readIndex;
    int writeIndex;
    int lostCount;
    long long oldTimestamp;
    long long diffTimeValue;
    pthread_mutex_t indexLock;

    std::vector<buffer_t *> videoFifo;

};

#endif //camera.h

#ifndef __CAMERA_ADAPTER_H
#define __CAMERA_ADAPTER_H

#include "camera.h"
#include <pthread.h>

#define SAVEFIFOSIZE 16

struct ImageFrame
{
    int width;
    int height;
    int imageSize;
    unsigned char *data;
};

struct SynchronizedFrames
{
    long long timeStamp;
    ImageFrame frames[4];
};

class CameraAdapter
{
  public:
    CameraAdapter(int cameraNum);
    void InitCamera();
    int findFourFrameValid(void);
    bool GetCurrentSynchronizedFrames(SynchronizedFrames *frames);
    bool CheckTimestamp(int fourFlags[]);
    void UinitCamera();

    int imageWidth;
    int imageHeight;
    int imageSize;
    int mCameraNum;
    long long localTimestamp;
    buffer_t framedata[4];

    // sync frame
    int writeIndex;
    int readIndex;
    SynchronizedFrames syncFrame[SAVEFIFOSIZE];
    pthread_t psaveId;
    pthread_mutex_t saveLock;
    bool threadExit;


  private:
    int loseFrameCount;
};

#endif

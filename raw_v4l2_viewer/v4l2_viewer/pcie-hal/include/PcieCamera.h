#ifndef __PCIECAMERA_H
#define __PCIECAMERA_H

#include <array>
#include <vector>
#include <map>
#include <memory>
#include "CameraGather.h"
#include "camera.h"
#include "camera_common.h"

#define MAX_CAMERAS   (8)

using namespace std;

class PcieCamera : public CameraImpl
{
  public:
    PcieCamera(int board, int channelmask, int w[],int h[],int fps[]);
    ~PcieCamera();

    int openCamera();
    int startCamera(int mode);

    int pollCamera(int *cameraFlags);
    int readFrameTimeOut(int channel, frameInfo *info, int timeout);
    int pollFrameTimeOut(frameInfo *info, int *chan, int timeout);
    int readCamera(int channel, frameInfo *info);
    int readFrameCount(int channel, int* ErrCount);

    int stopCamera();
    int closeCamera();

    int getChannels(){ return m_channels;};
    int getChmask(){return m_chmask;};
    int setDataMode(int mode){data_mode=mode;return 0;};
    int getDataMode(){return data_mode;};
  private:
	int m_boardNum;
	int m_channels;
	int m_chmask;
	int data_mode;
	array<int, MAX_CAMERAS> m_width;
    array<int, MAX_CAMERAS> m_height;
    array<int, MAX_CAMERAS> m_fps;
    fd_set m_fds;
    int m_pollFd;
    buffer_t m_buffer;
    struct timeval m_timeout;
    array<Camera*, MAX_CAMERAS> m_Camera;
    array<int, MAX_CAMERAS> m_efds;
    map<int, int> m_fd2chan;

  private:
    void init(int mode);
    int m_flag;
};

#endif

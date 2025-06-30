#ifndef __VIEW4_H
#define __VIEW4_H

#include <map>
#include <thread>
#include <vector>
#include <mutex>
#include <SDL/SDL.h>
#include <SDL/SDL_video.h>
//#include <SDL2/SDL.h>
//#include <SDL2/SDL_error.h>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include "hal_camera.h"
#include <iostream>

using namespace std;
namespace cameraview{
class View{
public:
    View(int w, int h);
    View(int w[], int h[]);
    ~View();

    int init();
    void deinit();
    void display();
    void sdl_init();
    //void sdl1_init();
    void opencv_init();
    void renderCameraView(char channel);
private:
    void wait();
    void cameraRun(int board);
    void setRegion(int chan,frameInfo frame);
    void scaleFrame(frameInfo srcFrame,frameInfo dstFrame);
public:
   static camera_info camInfo[MAX_BOARD];

   class Camera{
    public:
        Camera(int board, int channel, int w, int h):boardnum(board),chan(channel),width(w),height(h){};
        ~Camera(){};

        int boardnum;  //  board number
        int chan;
    private:
        int width;;
        int height;
        frameInfo frame;
        int error;
   };

private:
    int width[4];
    int height[4];
    int disp_w;
    int disp_h;
    vector< shared_ptr<Camera> > m_cameras;

    int status;
    int running;
    mutex mtx;
    array < shared_ptr<thread>, 4> m_pthread;
    map<int, int> m_chan2Regionid;
public:
    //SDL_Window *window;
    //SDL_Renderer *renderer;
    //SDL_Texture *textures[MAX_CHANS_PER_BOARD];
    //SDL_Rect cameraRects[MAX_CHANS_PER_BOARD];

    //SDL_Surface* screen;
    //SDL_Overlay* cameraOverlays[MAX_CHANS_PER_BOARD];
    //SDL_Rect cameraRects[MAX_CHANS_PER_BOARD];

    //cv::VideoCapture cameras[MAX_CHANS_PER_BOARD];
};
}
#endif
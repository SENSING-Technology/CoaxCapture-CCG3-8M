#include <QApplication>
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <iostream>

#include "TestWidget.h"
#include "../include/pcie_camera.h"

#define IMAGE_WIDTH 1280
#define IMAGE_HEIGHT 720

bool main_exit = false;
camera_info info;
frameInfo finfo;

static void
SigHandler(int signum)
{
    signal(SIGKILL, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGSTOP, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    main_exit = false;

    signal(SIGKILL, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGSTOP, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
}

static void
SigSetup(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = SigHandler;
    sigaction(SIGKILL, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
    sigaction(SIGSTOP, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
}

int main(int argc, char *argv[])
{
    int ret = -1;
    int i = 0;

    info.width = 1280;
    info.height = 720;
    info.fps = 25;
    info.channel_mask = 0xff;
    info.group = 0;

    QApplication a(argc, argv);
    TestWidget w;

    info.width = 1280;
    info.height = 720;
    info.fps = 25;
    info.channel_mask = 0xff;
    info.group = 0;

    memset(&finfo ,0 , sizeof(frameInfo));
    finfo.pdata = (unsigned char*)malloc(1280*720*3);
    memset(finfo.pdata, 0, 1280*720*3);
    finfo.isMemcpy = 0;


    SigSetup();

    /* initialize video display windows*/
    w.initVideoWall(IMAGE_WIDTH, IMAGE_HEIGHT);
    std::cout<<"initialize video display windows"<<std::endl;

    /* start display */
    for(i = 0; i < 8; i++){
          w.flashYuvData(i, finfo.pdata);
          usleep(10000);

          w.playVideoWall();
          w.show();
    }

    std::cout<<"w.playVideoWall"<<std::endl;
    w.playVideoWall();
    w.show();

    std::cout<<"while(main_exit == false)"<<std::endl;

    while(main_exit == false){
        usleep(1000);

        w.playVideoWall();
        w.show();
        // main_exit = true;
    }
    ret = pcie_camera_init(&info);
    if(ret < 0){
            printf("Initialize pcie camera failed\n");
            return -1;
    }

    printf("get pcie camera status: 0x%x\n", pcie_camera_getStatus());

    /* inter loop read thread, full yuv data to Opengl*/
    while (main_exit)
    {
        for(i = 0; i < 8; i++){
                ret = -1;
                ret = pcie_camera_readYUVData(i, &finfo);
                if(ret == 0){
                    w.flashYuvData(i, finfo.pdata);
                }
        }
        /* display */
        w.playVideoWall();
        usleep(400);
    }
    pcie_camera_destroy();
    if(finfo.pdata && (finfo.isMemcpy == 1)){
            free(finfo.pdata);
    }

    std::cout<<"main end"<<std::endl;
    return a.exec();
}

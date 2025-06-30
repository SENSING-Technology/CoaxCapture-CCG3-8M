#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include "pcie_camera.h"

#define SAVE_IMAGE

using namespace std;

int main_exit = 1;

static void
SigHandler(int signum)
{
        signal(SIGKILL, SIG_IGN);
        signal(SIGINT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGSTOP, SIG_IGN);
        signal(SIGHUP, SIG_IGN);

        main_exit = 0;
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

/*将大写字母转换成小写字母*/  
int tolower(int c)  
{  
    if (c >= 'A' && c <= 'Z')  
    {  
        return c + 'a' - 'A';  
    }  
    else  
    {  
        return c;  
    }  
}  

int _htoi(char s[])  
{  
    int i;  
    int n = 0;  
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))  
    {  
        i = 2;  
    }  
    else  
    {  
        i = 0;  
    }  
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i)  
    {  
        if (tolower(s[i]) > '9')  
        {  
            n = 16 * n + (10 + tolower(s[i]) - 'a');  
        }  
        else  
        {  
            n = 16 * n + (tolower(s[i]) - '0');  
        }  
    }  
    return n;  
}

int main(int argc, char **argv)
{
    int ret = -1;
    int i = 0;
    int imgCount[8] = {0};
    char namepath[128] = {0};
    int CheckframeCountPeriod = 0;
    camera_info info;
    frameInfo finfo;

    for(i=0; i<4; i++){
        info.width[i] = 1920;
        info.height[i] = 1080;
        info.fps[i] = 30;
    }
    for(i=4; i<8; i++){
        info.width[i] = 1920;
        info.height[i] = 1080;
        info.fps[i] = 30;
    }    
    info.channel_mask = 0xff;
    info.group = 0;

    printf("Usage: ./pcie_preview group ch_mask\n");
    if(argc > 1){
        sscanf(argv[1], "%d", &info.group);
    }
    if(argc > 2){
        info.channel_mask = _htoi(argv[2]);
    }
    SigSetup();

    memset(&finfo ,0 , sizeof(frameInfo));

    finfo.pdata = (unsigned char*)malloc(1920*1080*3);
    finfo.isMemcpy = 1;

    ret = pcie_camera_init(&info);
    if(ret < 0){
        printf("Initialize pcie camera failed\n");
        return -1;
    }

    printf("get pcie camera status: 0x%x\n", pcie_camera_getStatus());
#if 0    
    pcie_camera_destroy();
    if(finfo.pdata && (finfo.isMemcpy == 1)){
            free(finfo.pdata);
    }
    return 0;
#endif
    while(main_exit){
        for(i = 0; i < 8; i++){
//                pcie_camera_readYUVData(i, &finfo);
                ret = pcie_camera_readBGRData(i, &finfo);
                //printf("pcie_camera_readBGRData ret%d\n",ret);
#ifdef SAVE_IMAGE
                if(ret == 0){
                        printf("ch[%d], timestamp:%lld\n", i, finfo.timestamp);
                        memset(namepath, 0, sizeof(namepath));
                        sprintf(namepath, "ch_%d_%d.rgb", i, ++imgCount[i]);

                        FILE* fbgr = fopen(namepath, "w+");
                        if(fbgr){
                                fwrite(finfo.pdata, 1, finfo.width*finfo.height*3, fbgr);
                                fflush(fbgr);
                                fclose(fbgr);
                        }
                }
#endif
        }
        //printf("check pcie_camera_frameStaStatistics\n");
        if(++CheckframeCountPeriod%300 == 0){
             for(i = 0;i < 8; i++){
                  if(info.channel_mask & 1<<i){
                          int lostNum = 0;
                          printf("pcie_camera_frameStaStatistics\n");
                          ret = pcie_camera_frameStaStatistics(i, &lostNum);
                          printf("ch[%d]: frame[%d], lost[%d]\n", i, ret, lostNum);
                  }
             }
        }
        usleep(3000);
    }

    pcie_camera_destroy();
    if(finfo.pdata && (finfo.isMemcpy == 1)){
            free(finfo.pdata);
    }

    cout<<"main end"<<endl;
    return 0;
}

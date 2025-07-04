#include <stdio.h>
#include <string.h>
 
#include <SDL2/SDL.h>
#include "./yuvtobgr.h"
#include "./watermark.h"
 //g++ player.cpp -L/usr/local/lib/ -lSDL2
//自定义消息类型
#define REFRESH_EVENT   (SDL_USEREVENT + 1)     // 请求画面刷新事件
#define QUIT_EVENT      (SDL_USEREVENT + 2)     // 退出事件
 
//定义分辨率
// YUV像素分辨率
#define YUV_WIDTH   1920
#define YUV_HEIGHT  1080
//定义YUV格式
#define YUV_FORMAT  SDL_PIXELFORMAT_UYVY
 
int s_thread_exit = 0;  // 退出标志 = 1则退出

void save_image( uint8_t * buf, int length)
{
	char name[256]={0};
	FILE* fp;
    sprintf(name,"./picture.raw" );
    fp = fopen(name, "wb+");
    if(fp){
        fwrite(buf, 1, length, fp);
        fflush(fp);
        fclose(fp); }
}

int refresh_video_timer(void *data)
{
    while (!s_thread_exit)
    {
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(40);
    }
 
    s_thread_exit = 0;
 
    //push quit event
    SDL_Event event;
    event.type = QUIT_EVENT;
    SDL_PushEvent(&event);
 
    return 0;
}
#undef main
const char* watemake = "hello";
int main(int argc, char* argv[])
{
    //初始化 SDL
    if(SDL_Init(SDL_INIT_VIDEO))
       {
        fprintf( stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
 
    // SDL
    SDL_Event event;                            // 事件
    SDL_Rect rect;                              // 矩形
    SDL_Window *window = NULL;                  // 窗口
    SDL_Renderer *renderer = NULL;              // 渲染
    SDL_Texture *texture = NULL;                // 纹理
    SDL_Thread *timer_thread = NULL;            // 请求刷新线程
    uint32_t pixformat = YUV_FORMAT;            // YUV420P，即是SDL_PIXELFORMAT_IYUV
 
    // 分辨率
    // 1. YUV的分辨率
    int video_width = YUV_WIDTH;
    int video_height = YUV_HEIGHT;
    // 2.显示窗口的分辨率
    int win_width = 1920;
    int win_height = 1080;
 
    // YUV文件句柄
    FILE *video_fd = NULL;
    const char *yuv_path = argv[1];//"3840_2160.raw";
 
    size_t video_buff_len = 0;
 
    uint8_t *video_buf = NULL; //读取数据后先把放到buffer里面
    uint8_t *video_buf_dst = NULL;
 
    // 我们测试的文件是YUV420P格式
    //uint32_t y_frame_len = video_width * video_height;
    //uint32_t u_frame_len = video_width * video_height / 4;
    //uint32_t v_frame_len = video_width * video_height / 4;
    //uint32_t yuv_frame_len = y_frame_len + u_frame_len + v_frame_len;
    uint32_t yuv_frame_len =  video_width * video_height*2;
 
    //创建窗口
    window = SDL_CreateWindow("Simplest YUV Player",
                           SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED,
                           win_width, win_height,
                           SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if(!window)
    {
        fprintf(stderr, "SDL: could not create window, err:%s\n",SDL_GetError());
        goto _FAIL;
    }
    // 基于窗口创建渲染器
    renderer = SDL_CreateRenderer(window, -1, 0);
    // 基于渲染器创建纹理
    texture = SDL_CreateTexture(renderer,
                                pixformat,
                                SDL_TEXTUREACCESS_STREAMING,
                                win_width,
                                win_height);
 
    // 分配空间
    video_buf = (uint8_t*)malloc(yuv_frame_len);
    if(!video_buf)
    {
        fprintf(stderr, "Failed to alloce yuv frame space!\n");
        goto _FAIL;
    }
 
    // 打开YUV文件
    video_fd = fopen(yuv_path, "rb");
    if( !video_fd )
    {
        fprintf(stderr, "Failed to open yuv file\n");
        goto _FAIL;
    }
    // 创建请求刷新线程
    timer_thread = SDL_CreateThread(refresh_video_timer,
                                    NULL,
                                    NULL);
    
    while (1)
    {
        // 收取SDL系统里面的事件
        SDL_WaitEvent(&event);
 
        if(event.type == REFRESH_EVENT) // 画面刷新事件
        {
            //fseek(video_fd,0,SEEK_SET);
            video_buff_len = fread(video_buf, 1, yuv_frame_len, video_fd);
            if(video_buff_len <= 0)
            {
                //fprintf(stderr, "Failed to read data from yuv file!\n");
                continue;
                //goto _FAIL;
            }
            //
 #if  1          
        /*     video_buf_dst = (uint8_t*)malloc(yuv_frame_len);
            GrayToUYVY(video_buf,video_buf_dst,video_width,video_height);
            memcpy(video_buf,video_buf_dst,yuv_frame_len);
            free(video_buf_dst); 
            save_image(video_buf,yuv_frame_len); */

            video_buf_dst = (uint8_t*)malloc(yuv_frame_len);
            //Scale_Frame(video_buf,video_buf_dst,video_width,video_height,video_width/2,video_height/2);
            
            waterMark(video_buf,video_buf_dst,video_width,video_height,watemake);
            save_image(video_buf_dst,yuv_frame_len); 
            free(video_buf_dst); 
 #endif           
            // 设置纹理的数据 video_width = 320， plane
            SDL_UpdateTexture(texture, NULL, video_buf, video_width);
 
            // 显示区域，可以通过修改w和h进行缩放
            rect.x = 0;//win_width/32;
            rect.y = 0;//win_height/32;
           // float w_ratio = (win_width ) /(video_width);
            //float h_ratio = win_height * 1.0 /video_height;
            // 320x240 怎么保持原视频的宽高比例
            //rect.w = video_width * w_ratio;
            //rect.h = video_height * h_ratio;
            rect.w = video_width;//*15/16 ;
            rect.h = video_height;//*15/16 ;
 
            // 清除当前显示
            SDL_RenderClear(renderer);
            // 将纹理的数据拷贝给渲染器
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            // 显示
            SDL_RenderPresent(renderer);
        }
        else if(event.type == SDL_WINDOWEVENT)
        {
            //If Resize
            SDL_GetWindowSize(window, &win_width, &win_height);
            printf("SDL_WINDOWEVENT win_width:%d, win_height:%d\n",win_width,
                   win_height );
        }
        else if(event.type == SDL_QUIT) //退出事件
        {
            s_thread_exit = 1;
        }
        else if(event.type == QUIT_EVENT)
        {
            break;
        }
    }
 
_FAIL:
    s_thread_exit = 1;      // 保证线程能够退出
    // 释放资源
    if(timer_thread)
        SDL_WaitThread(timer_thread, NULL); // 等待线程退出
    if(video_buf)
        free(video_buf);
    if(video_fd)
        fclose(video_fd);
    if(texture)
        SDL_DestroyTexture(texture);
    if(renderer)
        SDL_DestroyRenderer(renderer);
    if(window)
        SDL_DestroyWindow(window);
 
    SDL_Quit();
 
    return 0;
 
}
 
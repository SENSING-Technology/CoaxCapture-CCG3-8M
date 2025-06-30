/***********************************************************
 *
 *
 * *********************************************************/
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
}
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <thread>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "view.h"
#include "fun.h"
#include "yuvtobgr.h"
#include "watermark.h"
#include "encoder_cuda.h"
#include <vector>
#include <thread>
#include <X11/Xlib.h>

static Uint32 SDL_VIDEO_Flags = SDL_ANYFORMAT | SDL_DOUBLEBUF | SDL_RESIZABLE | SDL_HWSURFACE;
static volatile int main_exit = 0;
uint8_t sdl_flag;

using namespace cameraview;

#define BYTE1 __attribute__((packed, aligned(1)))
#define UNUSED(x) (void)x

#define BOARDMAX 4
#define CHANELMAX 8
typedef struct _encoder{
    AVCodec *codec;
   AVCodecContext *codec_ctx;
}enCoder;
enCoder encoder;

typedef struct _enPara{
   AVFrame *srcframe;
   AVFrame *dstframe;
   SwsContext *sws_ctx;
   AVCodec *codec;
   AVCodecContext *codec_ctx;
   AVFormatContext *out_fmt_ctx;
   AVOutputFormat *out_fmt;
   AVStream *out_stream;
   AVPacket *pkt;
   int frame_count;
}EnPara;
EnPara  enPara[BOARDMAX][8];

int     creathreadfg[BOARDMAX][8];
int     encodeflag[BOARDMAX][8];
int     fileindex[BOARDMAX][8];
int     filefullflag[BOARDMAX][8];
long    filesize[BOARDMAX][8];
int     srcframecount[BOARDMAX][8];
int     enframecount[BOARDMAX][8];
FILE    *fp[BOARDMAX][8];
int wflag[BOARDMAX][8];
int wflag1[BOARDMAX][8];
int64_t start_time[BOARDMAX][8], end_time[BOARDMAX][8];
double duration[BOARDMAX][8], fps[BOARDMAX][8];


array <shared_ptr<thread>, 8> encodeThread[BOARDMAX];
std::queue<frameInfo> dataQueue[BOARDMAX][8];   
std::mutex mtx[BOARDMAX][8];                         
std::condition_variable condVarReady[BOARDMAX][8];             

array <shared_ptr<thread>, 8> f_enc_pthread;
std::queue<frameInfo> f_dataQueue[8];   
std::mutex f_mtx[8];                         
std::condition_variable f_cv[8];     

typedef struct _GlobalVar{
    int width[BOARDMAX];
    int height[BOARDMAX];
    unsigned int chan_mask[BOARDMAX];  // 0x1 bitset 1 indicate a camera
    unsigned int trigger_mode;  // 0: freerun   1: senconds-justified
    unsigned int is_save_buf;
    unsigned int save_h264;
    unsigned int print_time_stamp;
    unsigned int pixel_format;//0:YUYV 1:Gray16le
    unsigned int region_x; //1~4
    unsigned int region_y; //1~4
    unsigned int sdl_chan;
    unsigned int multi_chan;
    unsigned int playMode;
    unsigned int show;
}GlobalVar;

GlobalVar globalset;

typedef struct _AncillaryData
{
    unsigned char header[4];//4
    unsigned char payloadMajorVersion;
    unsigned char payloadMinorVersion; //6
    unsigned short payloadNumBytes; //8
    unsigned int gw5State;//12
    float gw5Temperature;//16
    unsigned int inputFrameErrorCount;//20
    unsigned int inputFrameCount;//24
    unsigned int inputFrameCrc; //28
    long long inputFrameTimestampMs; //36
    unsigned int outputFrameCount;//40
    unsigned int outputFrameCrc; //44
    long long outputFrameTimestampMs;//52
    float totalGain[4];//68
    float sensorGain[4];//84
    float ispGain[4];//100
    float unclippedEv[4];//116
    float unclippedEvAvg;//120
    unsigned short intergrationLines[4];//128
    unsigned short vts;//130
    unsigned char padAe[2];//132
    float frameRate;//136
 }BYTE1 AncillaryData;
AncillaryData g_anc_illary_date;
typedef struct  _AncDate
{
    /* data */
    unsigned int frame_count;
    unsigned int frame_trig_sync_s;
    unsigned int frame_trig_sync_ns;
    unsigned int frame_rev_sync_s;
    unsigned int frame_rev_sync_ns;
    unsigned int frame_cache_irq_s;
    unsigned int frame_cache_irq_ns;
    unsigned int frame_pc_ack;
    unsigned int frame_dma_trans;
}AncData;
AncData g_anc_date;

static int image_index[BOARDMAX][CHANELMAX] = {0};
static int framecount[BOARDMAX][CHANELMAX] = {0};

void save_image(int board, int channel, frameInfo* frame);
void save_image_8(int board, int channel, frameInfo* frame);
void save_image_16(int board, int channel, frameInfo* frame);
void save_image_16_t(int board, int channel, frameInfo* frame);

int sw_encodetoh264(EnPara enpara[][8], int board, int channel, frameInfo* frame);
int sw_encodetoh264init(EnPara enpara[][8], int board, int channel, frameInfo* frame);
int sw_encodetoh264uninit(EnPara enpara[][8], int board, int channel);
int frame_encode_cuda(int board, int channel, frameInfo *frame);
void getRealTimeData(int board, int channel, frameInfo *frame);
void encCudaThread(int board, int channel);
void getRealTimeDataFfmpeg(int board, int channel, frameInfo *frame);
void encCudaThreadFfmpeg(EnPara enpara[][8], int board, int channel);
void getRealTimeSdlData(int board, int channel, frameInfo *frame);

camera_info View::camInfo[MAX_BOARD] = {
    {
        .width  = {1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920},
        .height = {1080, 1080, 1080, 1080, 1080, 1080, 1080, 1080},
        .fps    = {20, 20, 20, 20, 20, 20,20,20},
        .defertime = {0},
        .trigger_mode = {0, 0, 0, 0, 0, 0, 0, 0},
        .channel_mask = MAX_CHANS_MASK,
        .group = 0,
        .force_init = 1,
        .cam_otaselect = {0, 0, 0, 0, 0, 0, 0, 0},
        .card_mcureset = 0,
    },
    {
        .width  = {1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920},
        .height = {1080, 1080, 1080, 1080, 1080, 1080, 1080, 1080},
        .fps    = {20, 20, 20, 20, 20, 20,20,20},
        .defertime = {0},
        .trigger_mode = {0, 0, 0, 0, 0, 0, 0, 0},
        .channel_mask = MAX_CHANS_MASK,
        .group = 1,
        .force_init = 1,
        .cam_otaselect = {0, 0, 0, 0, 0, 0, 0, 0},
        .card_mcureset = 0,
    },
        {
        .width  = {1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920},
        .height = {1080, 1080, 1080, 1080, 1080, 1080, 1080, 1080},
        .fps    = {20, 20, 20, 20, 20, 20,20,20},
        .defertime = {0},
        .trigger_mode = {0, 0, 0, 0, 0, 0, 0, 0},
        .channel_mask = MAX_CHANS_MASK,
        .group = 2,
        .force_init = 1,
        .cam_otaselect = {0, 0, 0, 0, 0, 0, 0, 0},
        .card_mcureset = 0,
    },
        {
        .width  = {1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920},
        .height = {1080, 1080, 1080, 1080, 1080, 1080, 1080, 1080},
        .fps    = {20, 20, 20, 20, 20, 20,20,20},
        .defertime = {0},
        .trigger_mode = {0, 0, 0, 0, 0, 0, 0, 0},
        .channel_mask = MAX_CHANS_MASK,
        .group = 3,
        .force_init = 1,
        .cam_otaselect = {0, 0, 0, 0, 0, 0, 0, 0},
        .card_mcureset = 0,
    },
};


View::View(int w[], int h[])
{
    width[0] = w[0];
    width[1] = w[1];
    width[2] = w[2];
    width[3] = w[3];
    height[0] =h[0];
    height[1] =h[1];
    height[2] =h[0];
    height[3] =h[1];
    m_pthread.fill(NULL);
}

View::~View()
{
}

inline void sleep_ms(int ms)
{
    this_thread::sleep_for(chrono::milliseconds(ms));
}

void View::wait()
{
    for(auto iter = m_pthread.begin(); iter != m_pthread.end(); iter++){
        if(*iter) (*iter)->join();
    }
}

void convert_image(frameInfo* frame)
{
    unsigned char temp[2] = {0x0,0x0};
    for(int i =0; i< frame->length; i+=2)
    {
        temp[0] = *(frame->pdata+i);
        temp[1] = *(frame->pdata+i+1);
        *(frame->pdata+i) = ((temp[0]<<4)&0xf0);
        *(frame->pdata+i+1) = ((temp[1]<<4)&0xf0) + ((temp[0]>>4)&0xf);
    }   
}

void convert_image(frameInfo* frame, uint8_t *video_buf_frame)
{
    //unsigned char temp[2] = {0x0,0x0};
    for(int i =0; i< frame->length; i+=2)
    {
        //temp[0] = *(frame->pdata+i);
        //temp[1] = *(frame->pdata+i+1);
        *(video_buf_frame+i) = ( *(frame->pdata+i)<<4)&0xf0;
        *(video_buf_frame+i+1) = ((*(frame->pdata+i+1)<<4)&0xf0) + ((*(frame->pdata+i)>>4)&0xf);
    }   
}

char sys_time[256] = {0};
char* time()
{
#if 0    
	time_t sec,sec2;
    sec=time(NULL);
    if(sec!=sec2)
    {
        sec2=sec;
        struct tm* today = localtime(&sec2);	
        strftime(sys_time, sizeof(sys_time), "%Y/%m/%d %H\\:%M\\:%S", today);    
    }
#endif
    //sprintf(sys_time,"a dev/video%d frame:%d timestamp:%lld.%09lld(s)\n", 
    //            chan,frame.reserved,( frame.systime/1000000000),(frame.systime%1000000000));     
    return sys_time;
}
 
void View::cameraRun(int board)
{
    frameInfo frame,frameWater,frameScale;
    frameInfo *pFrame;
    int ret;
    int chan, cam_chan, iCamType;
    unsigned char *p;
    uint8_t *video_buf[32];
    long long systime_before[32];
    frameWater.pdata = (uint8_t*)malloc(3840*2160*2);
    frameWater.width = 1920;
    frameWater.height = 1080;
    frameScale.pdata = (uint8_t*)malloc(1920*1080*2);
    frameScale.width = 1920;
    frameScale.height = 1080;
    for(int i=0; i<32;i++)
    {
        video_buf[i] =  (uint8_t*)malloc(3840*2160*2);
    }
    frame.pdata = (uint8_t*)malloc(3840*2160*2);
    frame.isMemcpy = 1;
    while(main_exit != 1){
        //memset(&frame, 0, sizeof(frame));
        //chan = -1;
        ret = hal_camera_poll_timeout(board, &frame, &chan, 100);
        if(ret == 0){
            if ( globalset.is_save_buf ){
                //if(1 == (frame.reserved%300))
                {
                    switch(globalset.is_save_buf)
                    {
                        case 8:
                            save_image_8(board, chan, &frame);
                            break;
                        case 12:
                            save_image(board, chan, &frame);
                            break;
                        case 16:
                            //save_image(board, chan, &frame);
                            save_image_16(board, chan, &frame);
                            break;
                        case 100:
                            save_image_8(board, chan, &frame);
                            save_image_16(board, chan, &frame);
                            save_image(board, chan, &frame);
                            break;
                        default:
                            save_image(board, chan, &frame);
                            break;
                    }
                }
            }
#if 0
           if ( globalset.is_save_buf ){
                if(encodeflag[board][chan] == 0){
                    int ret = sw_encodetoh264init(enPara,board, chan, &frame);
                    if(ret == 0)
                        encodeflag[board][chan] = 1;
                    
                }
                if((encodeflag[board][chan] == 1) && (main_exit != 1))
                    sw_encodetoh264(enPara,board, chan, &frame);
           }
           if(main_exit == 1 || filefullflag[board][chan] == 1){
                encodeflag[board][chan] = 0;
                filefullflag[board][chan] = 0;
                filesize[board][chan] = 0;
                sw_encodetoh264uninit(enPara, board, chan);
            }
#endif

            if(globalset.show == 1){
                getRealTimeSdlData(board, chan, &frame); 
            }
        }
        ret = -1;
    }
}

 void View::scaleFrame(frameInfo srcFrame,frameInfo dstFrame)
 {
    Scale_Frame(srcFrame.pdata,dstFrame.pdata,srcFrame.width,srcFrame.height,dstFrame.width,dstFrame.height,AV_PIX_FMT_YUV422P,AV_PIX_FMT_UYVY422);
 }

void View::display()
{
    running = true;

    if ( 0 != globalset.chan_mask[0] )
    {
        m_pthread[0] = make_shared<thread>(&View::cameraRun, this, 0);
        hal_camera_start(0, 1);
    }

    if ( 0 != globalset.chan_mask[1] )
    {
        m_pthread[1] = make_shared<thread>(&View::cameraRun, this, 1);
        hal_camera_start(1, 1);
    }

    if ( 0 != globalset.chan_mask[2] )
    {
        m_pthread[2] = make_shared<thread>(&View::cameraRun, this, 2);
        hal_camera_start(2, 1);
    }

    if ( 0 != globalset.chan_mask[3] )
    {
        m_pthread[3] = make_shared<thread>(&View::cameraRun, this, 3);
        hal_camera_start(3, 1);
    }
}

void View::deinit()
{
    running = false;
    wait();

    if ( 0 != globalset.chan_mask[0] )
    {
        hal_camera_stop(0);
        hal_camera_destroy(0);
    }

    if ( 0 != globalset.chan_mask[1] )
    {
        hal_camera_stop(1);
        hal_camera_destroy(1);
    }

    if ( 0 != globalset.chan_mask[2] )
    {
        hal_camera_stop(2);
        hal_camera_destroy(2);
    }


    if ( 0 != globalset.chan_mask[3] )
    {
        hal_camera_stop(3);
        hal_camera_destroy(3);
    }
}

int View::init()
{
    int status, board;
    for(board=0; board<MAX_BOARD; board++){
        if (0 == globalset.chan_mask[board] )
            continue;
        View::camInfo[board].channel_mask = globalset.chan_mask[board];
        for(int i=0; i<MAX_CHANS_PER_BOARD; i++){
            View::camInfo[board].trigger_mode[i] = globalset.trigger_mode;
        }

        status = hal_camera_init(&View::camInfo[board]);
        if(status != 0){
            cout << "board " << board << " pcie_camera_init failed!" << endl;
            return -1;
        }
        printf("hal_camera_init_ok\n");
        sleep_ms(2000);
#if 0    
        status = hal_camera_getStatus(board);
        if(status == 0){
            hal_camera_destroy(board);
            cout << "board " << board << " status is: " << status << endl;
            if((board == 1) && (0 != globalset.chan_mask[0]))
                hal_camera_destroy(0);
            return -1;
        }
#endif
    }
    return 0;
}


void getRealTimeSdlData(int board, int channel, frameInfo *frame)
{   
    frameInfo enframe;
    std::lock_guard<std::mutex> lock(mtx[board][channel]);
    memset(&enframe, 0, sizeof(enframe));
    memcpy(&enframe,  frame, sizeof(frameInfo));
    enframe.timestamp = frame->timestamp;
    enframe.pdata = (uint8_t*)malloc(3840*2160*2);
    memset(enframe.pdata, 0, sizeof(enframe.pdata));
    memcpy(enframe.pdata,  frame->pdata, frame->length);

    dataQueue[board][channel].push(enframe); 
    condVarReady[board][channel].notify_one();   
}

#if 1
void convert_image(unsigned char* pdata, int length) 
{
    unsigned char temp[2] = {0x0, 0x0};
    for (int i = 0; i < length; i += 2) {
        temp[0] = *(pdata + i);
        temp[1] = *(pdata + i + 1);
        *(pdata + i) = ((temp[0] << 4) & 0xf0);
        *(pdata + i + 1) = ((temp[1] << 4) & 0xf0) + ((temp[0] >> 4) & 0xf);
    }
}

bool shouldExit = false;
void processCameraImage(int board, int channel, int displayWidth, int canvasX, int canvasY, float scale, cv::Mat& canvas)
{
    cv::Mat cameraImages;
    while(!shouldExit){
            std::unique_lock<std::mutex> lock(mtx[board][channel]);
            condVarReady[board][channel].wait(lock, [board, channel] { return !dataQueue[board][channel].empty(); });  
            frameInfo frame = dataQueue[board][channel].front();
            dataQueue[board][channel].pop();
            lock.unlock();
            // 从内存中创建Mat对象，不进行数据的复制
            cameraImages = cv::Mat(frame.height, frame.width, CV_8UC2, frame.pdata);
            // 将YUYV422格式转换为BGR格式
            cv::cvtColor(cameraImages, cameraImages, cv::COLOR_YUV2BGR_YUYV);
            // 将摄像头图像放置在画布上的不同位置
            cv::Mat resizedImage;
            if(frame.width == 3840 && globalset.playMode == 2){
                cv::resize(cameraImages, resizedImage, cv::Size(), scale / 2, scale / 2);
            }
            else
                cv::resize(cameraImages, resizedImage, cv::Size(), scale, scale);
            resizedImage.copyTo(canvas(cv::Rect(canvasX, canvasY, resizedImage.cols, resizedImage.rows)));
            delete[] frame.pdata;
    }
    return;
}

void processCameraImage3(int board, int channel, int displayWidth, int canvasX, int canvasY, float scale, cv::Mat& canvas)
{
     // 计算每个摄像头显示的宽高
    int display_width_per_camera = 1920 * scale;
    int display_height_per_camera = 1080 * scale;
    double scale1 = std::min((double)display_width_per_camera / 1920, (double)display_height_per_camera / 1536);
    double scale2 = std::min((double)display_width_per_camera / 3840, (double)display_height_per_camera / 2160);

    cv::Mat cameraImages;
    cv::Mat rawImage;
    int canvasX1 = 0;
    while(!shouldExit){
            std::unique_lock<std::mutex> lock(mtx[board][channel]);
            condVarReady[board][channel].wait(lock, [board, channel] { return !dataQueue[board][channel].empty(); });  
            frameInfo frame = dataQueue[board][channel].front();
            dataQueue[board][channel].pop();
            lock.unlock();
            if(globalset.pixel_format == 0){
                // 从内存中创建Mat对象，不进行数据的复制
                cameraImages = cv::Mat(frame.height, frame.width, CV_8UC2, frame.pdata);
                // 将YUYV422格式转换为BGR格式
                cv::cvtColor(cameraImages, cameraImages, cv::COLOR_YUV2BGR_YUYV);
            }
            else{
                convert_image(frame.pdata, frame.length); 
                rawImage = cv::Mat(frame.height, frame.width, CV_16UC1, frame.pdata);
                // 将16位raw数据缩放到8位范围内
                rawImage.convertTo(cameraImages, CV_8UC1, 1.0 / 256.0); 
                cv::cvtColor(cameraImages, cameraImages, cv::COLOR_GRAY2BGR);
            }

            // 将摄像头图像放置在画布上的不同位置
            cv::Mat resizedImage;
            if(frame.width == 3840){
                cv::resize(cameraImages, resizedImage, cv::Size(), scale2, scale2);
                resizedImage.copyTo(canvas(cv::Rect(canvasX, canvasY, resizedImage.cols, resizedImage.rows)));
            }
            else if(frame.height == 1536){
                cv::resize(cameraImages, resizedImage, cv::Size(), scale1, scale1);
                canvasX1 +=  canvasX+(display_width_per_camera-resizedImage.cols)/2;
                resizedImage.copyTo(canvas(cv::Rect(canvasX1, canvasY, resizedImage.cols, resizedImage.rows)));
                canvasX1 = 0;
            }
            else{
                cv::resize(cameraImages, resizedImage, cv::Size(), scale, scale);
                resizedImage.copyTo(canvas(cv::Rect(canvasX, canvasY, resizedImage.cols, resizedImage.rows)));
            }

            delete[] frame.pdata;
    }
    return;
}

void opencv_multi_channel_16_play()
{
    const int boardCount = 1;
    const int cameraCount = 8;

    const int displayWidth = 1920;
    const int displayHeight = 1080;
    const int cameraWidth = 1920;
    const int cameraHeight = 1080;

    int canvasX = 0;
    int canvasY = 0;

    cv::namedWindow("Camera Display", cv::WINDOW_NORMAL);
    cv::resizeWindow("Camera Display", displayWidth, displayHeight);

    cv::Mat cameraImages[cameraCount*2]; // 摄像头图像的Mat对象数组
    // 创建一个画布，用于将八个摄像头图像组合在一起
    cv::Mat canvas(displayHeight, displayWidth, CV_8UC3, cv::Scalar(0, 0, 0));
    // 计算每个摄像头图像在画布上的缩放比例
    float scale = std::min((float)displayWidth / 3840, (float)displayHeight / 4320);
    std::vector<std::thread> threads(cameraCount*boardCount);

    for(int i = 0; i < boardCount; i++){
        for (int j = 0; j < cameraCount; j++) {
            if(globalset.chan_mask[i] & (1 << j)){
                if(!dataQueue[i][j].empty()){
                    //continue;
                    std::unique_lock<std::mutex> lock(mtx[i][j]);
                    condVarReady[i][j].wait(lock, [i, j] { return !dataQueue[i][j].empty(); });  
                    frameInfo frame = dataQueue[i][j].front();
                    dataQueue[i][j].pop();
                    lock.unlock();

                    // 从内存中创建Mat对象，不进行数据的复制
                    cameraImages[i*cameraCount+j] = cv::Mat(frame.height, frame.width, CV_8UC2, frame.pdata);
                    // 将YUYV422格式转换为BGR格式
                    cv::cvtColor(cameraImages[i*cameraCount+j], cameraImages[i*cameraCount+j], cv::COLOR_YUV2BGR_YUYV);
                    // 将摄像头图像放置在画布上的不同位置
                    cv::Mat resizedImage;
                    if(frame.width == 3840 && globalset.playMode == 2){
                        cv::resize(cameraImages[i*cameraCount+j], resizedImage, cv::Size(), scale / 2 , scale / 2);
                    }
                    else
                        cv::resize(cameraImages[i*cameraCount+j], resizedImage, cv::Size(), scale, scale);
                    //printf("%d-%d,X:%d,Y:%d,W:%d,H:%d\n", i, j, canvasX, canvasY, resizedImage.cols, resizedImage.rows);
                    resizedImage.copyTo(canvas(cv::Rect(canvasX, canvasY, resizedImage.cols, resizedImage.rows)));
                    threads[i*cameraCount+j] = thread(processCameraImage, i, j, displayWidth, canvasX, canvasY, scale, ref(canvas));
                    canvasX += resizedImage.cols;
                    if (canvasX >= displayWidth) {
                        canvasX = 0;
                        canvasY += resizedImage.rows;
                    }
                    delete[] frame.pdata;
                }
                else {
                    // 从内存中创建Mat对象，不进行数据的复制
                    cameraImages[i*cameraCount+j] = cv::Mat(cameraHeight, cameraWidth, CV_8UC3, cv::Scalar(0,0,0));
                    // 将摄像头图像放置在画布上的不同位置
                    cv::Mat resizedImage;
                    if(cameraWidth == 3840 && globalset.playMode == 2){
                        cv::resize(cameraImages[i*cameraCount+j], resizedImage, cv::Size(), scale / 2 , scale / 2);
                    }
                    else
                        cv::resize(cameraImages[i*cameraCount+j], resizedImage, cv::Size(), scale, scale);
                    //printf("%d-%d,X:%d,Y:%d,W:%d,H:%d\n", i, j, canvasX, canvasY, resizedImage.cols, resizedImage.rows);
                    resizedImage.copyTo(canvas(cv::Rect(canvasX, canvasY, resizedImage.cols, resizedImage.rows)));
                    threads[i*cameraCount+j] = thread(processCameraImage, i, j, displayWidth, canvasX, canvasY, scale, ref(canvas));
                    canvasX += resizedImage.cols;
                    if (canvasX >= displayWidth) {
                        canvasX = 0;
                        canvasY += resizedImage.rows;
                    }
                }
            }
            
        }
    }

    while (true) {
        // 在窗口中显示画布
        cv::imshow("Camera Display", canvas);

        if(cv::getWindowProperty("Camera Display", cv::WND_PROP_AUTOSIZE) != 0){
            shouldExit = true;
            break;
        }
        if (cv::waitKey(1) == 27) {
            shouldExit = true;
            break; 
        }
    }
    for(int i = 0; i < cameraCount * boardCount; i++){
        if(threads[i].joinable()){
            threads[i].join();
        }
    }
    main_exit = 1;
    cv::destroyAllWindows();
    return;
}

void opencv_multi_channel_8_play()
{
    const int boardCount = 1;
    const int cameraCount = 8;

    int canvasX = 0;
    int canvasY = 0;

    cv::Mat cameraImages[cameraCount*2]; // 摄像头图像的Mat对象数组
    cv::Mat rawImage;
    const int displayWidth = 1920;
    const int displayHeight = 1080;
    const int cameraWidth = 1920;
    const int cameraHeight = 1080;

    const int cameraWidth1 = 1536;
    const int cameraWidth2 = 3840;

    cv::namedWindow("Camera Display", cv::WINDOW_NORMAL);
    cv::resizeWindow("Camera Display", displayWidth, displayHeight);

    int numRows = 2;
    int numCols = 4;

    // 计算每个摄像头图像在画布上的缩放比例
    double scale = std::min((double)displayWidth / (cameraWidth * numCols),(double)displayHeight/ (cameraHeight* numRows));

    // 计算每个摄像头显示的宽高
    int display_width_per_camera = cameraWidth * scale;
    int display_height_per_camera = cameraHeight * scale;

    double scale1 = std::min((double)display_width_per_camera / 1920, (double)display_height_per_camera / 1536);
    double scale2 = std::min((double)display_width_per_camera / 3840, (double)display_height_per_camera / 2160);

    // 创建一个画布，用于将八个摄像头图像组合在一起
    cv::Mat canvas(display_height_per_camera * numRows, display_width_per_camera * numCols, CV_8UC3, cv::Scalar(0, 0, 0));

    std::vector<std::thread> threads(cameraCount*boardCount);
    int canvasX1 = 0;
    int cancols = 0;
    for(int i = 0; i < boardCount; i++){
        for (int j = 0; j < cameraCount; j++) {
            if(globalset.chan_mask[i] & (1 << j)){
                if(!dataQueue[i][j].empty()){
                    //continue;
                    std::unique_lock<std::mutex> lock(mtx[i][j]);
                    condVarReady[i][j].wait(lock, [i, j] { return !dataQueue[i][j].empty(); });  
                    frameInfo frame = dataQueue[i][j].front();
                    dataQueue[i][j].pop();
                    lock.unlock();
                    if(globalset.pixel_format == 0){
                        // 从内存中创建Mat对象，不进行数据的复制
                        cameraImages[i*cameraCount+j] = cv::Mat(frame.height, frame.width, CV_8UC2, frame.pdata);
                        // 将YUYV422格式转换为BGR格式
                        cv::cvtColor(cameraImages[i*cameraCount+j], cameraImages[i*cameraCount+j], cv::COLOR_YUV2BGR_YUYV);
                    }
                    else {
                        convert_image(frame.pdata, frame.length); 
                        rawImage = cv::Mat(frame.height, frame.width, CV_16UC1, frame.pdata);
                        // 将16位raw数据缩放到8位范围内
                        rawImage.convertTo(cameraImages[i*cameraCount+j], CV_8UC1, 1.0 / 256.0); 
                        cv::cvtColor(cameraImages[i*cameraCount+j], cameraImages[i*cameraCount+j], cv::COLOR_GRAY2BGR);
                    }
                    // 将摄像头图像放置在画布上的不同位置
                    cv::Mat resizedImage;
                    if(frame.width == 3840){
                        cv::resize(cameraImages[i*cameraCount+j], resizedImage, cv::Size(), scale2 , scale2);
                        resizedImage.copyTo(canvas(cv::Rect(canvasX, canvasY, resizedImage.cols, resizedImage.rows)));
                    }
                    else if(frame.height == 1536){
                        cv::resize(cameraImages[i*cameraCount+j], resizedImage, cv::Size(), scale1, scale1);
                        canvasX1 +=  canvasX + (display_width_per_camera-resizedImage.cols)/2;
                        resizedImage.copyTo(canvas(cv::Rect(canvasX1, canvasY, resizedImage.cols, resizedImage.rows)));
                        canvasX1 = 0;
                    }
                    else{
                        cv::resize(cameraImages[i*cameraCount+j], resizedImage, cv::Size(), scale, scale);
                        cancols = resizedImage.cols;
                        resizedImage.copyTo(canvas(cv::Rect(canvasX, canvasY, resizedImage.cols, resizedImage.rows)));
                    }

                    printf("0:%d-%d,X:%d,Y:%d,W:%d,H:%d\n", i, j, canvasX, canvasY, resizedImage.cols, resizedImage.rows);

                    //resizedImage.copyTo(canvas(cv::Rect(canvasX, canvasY, resizedImage.cols, resizedImage.rows)));

                    threads[i*cameraCount+j] = thread(processCameraImage3, i, j, displayWidth, canvasX, canvasY, scale, ref(canvas));
                    canvasX += cancols;
                    if (canvasX >= displayWidth) {
                        canvasX = 0;
                        canvasY += resizedImage.rows;
                    }
                    delete[] frame.pdata;
                }
                else {
                    // 从内存中创建Mat对象，不进行数据的复制
                    cameraImages[i*cameraCount+j] = cv::Mat(cameraHeight, cameraWidth, CV_8UC3, cv::Scalar(0,0,0));
                    // 将摄像头图像放置在画布上的不同位置
                    cv::Mat resizedImage;
                    cv::resize(cameraImages[i*cameraCount+j], resizedImage, cv::Size(), scale, scale);
                    
                    printf("1:%d-%d,X:%d,Y:%d,W:%d,H:%d\n", i, j, canvasX, canvasY, resizedImage.cols, resizedImage.rows);

                    resizedImage.copyTo(canvas(cv::Rect(canvasX, canvasY, resizedImage.cols, resizedImage.rows)));
                    threads[i*cameraCount+j] = thread(processCameraImage3, i, j, displayWidth, canvasX, canvasY, scale, ref(canvas));
                    canvasX += resizedImage.cols;
                    if (canvasX >= displayWidth) {
                        canvasX = 0;
                        canvasY += resizedImage.rows;
                    }
                }
            }
            
        }
    }

    while (true) {
        // 在窗口中显示画布
        cv::imshow("Camera Display", canvas);

        if(cv::getWindowProperty("Camera Display", cv::WND_PROP_AUTOSIZE) != 0){
            shouldExit = true;
            break;
        }
        if (cv::waitKey(1) == 27) {
            shouldExit = true;
            break; 
        }
    }

    main_exit = 1;
    system("../kill_view.sh");
    for(int i = 0; i < cameraCount * boardCount; i++){
        if(threads[i].joinable()){
            threads[i].join();
        }
    }

    cv::destroyAllWindows();
    return;
}

void opencv_single_channel_play(int board, int channel) 
{
    const int displayWidth = 1920;
    const int displayHeight = 1080;

    cv::namedWindow("Camera Display", cv::WINDOW_NORMAL);
    cv::resizeWindow("Camera Display", displayWidth, displayHeight);

    cv::Mat cameraImage; // 摄像头图像的Mat对象
    cv::Mat rawImage;
    while (true) {
        // 从内存中创建Mat对象，不进行数据的复制
        std::unique_lock<std::mutex> lock(mtx[board][channel]);
        condVarReady[board][channel].wait(lock, [board, channel] { return !dataQueue[board][channel].empty(); });  
        frameInfo frame = dataQueue[board][channel].front();
        dataQueue[board][channel].pop();
        lock.unlock();

        if(globalset.pixel_format == 0){
            // YUYV422 or UYVY422
            cameraImage = cv::Mat(frame.height, frame.width, CV_8UC2, frame.pdata);
            cv::cvtColor(cameraImage, cameraImage, cv::COLOR_YUV2BGR_YUYV);
            //cv::cvtColor(cameraImage, cameraImage, cv::COLOR_YUV2BGR_UYVY);
        }
        else{
#if 0
            // raw
            cameraImage = cv::Mat(frame.height, frame.width, CV_8UC1, frame.pdata);
            cv::cvtColor(cameraImage, cameraImage, cv::COLOR_GRAY2BGR); 
#endif

#if 1
            // raw16
            // 转换为16位灰度图像数据
            convert_image(frame.pdata, frame.length); 
            rawImage = cv::Mat(frame.height, frame.width, CV_16UC1, frame.pdata);
            // 将16位raw数据缩放到8位范围内
            //rawImage.convertTo(cameraImage, CV_8UC1, 1.0 / 16.0);
            //rawImage.convertTo(cameraImage, CV_8UC1, 1.0 / 32.0);
            //rawImage.convertTo(cameraImage, CV_8UC1, 1.0 / 64.0);
            //rawImage.convertTo(cameraImage, CV_8UC1, 1.0 / 128.0); 
            rawImage.convertTo(cameraImage, CV_8UC1, 1.0 / 256.0); 
            cv::cvtColor(cameraImage, cameraImage, cv::COLOR_GRAY2BGR);
#endif

#if 0
            // raw12
            unsigned short* rawData12bit = reinterpret_cast<unsigned short*>(frame.pdata);
            // 将12位raw数据转换为16位数据
            cv::Mat rawImage12bit(frame.height, frame.width, CV_16UC1);
            for (int i = 0; i < frame.height * frame.width; i++) {
                unsigned short highByte = (rawData12bit[i] >> 4)& 0x0FF0;
                unsigned short lowByte = (rawData12bit[i] << 8) & 0x0FF0;
                rawImage12bit.at<unsigned short>(i) = highByte | lowByte;
            }
            // 将16位raw数据缩放到8位范围内
            rawImage12bit.convertTo(cameraImage, CV_8UC1, 1.0 / 16.0 ); 
            //rawImage12bit.convertTo(cameraImage, CV_8UC1, 1.0 / 64.0 );
            //rawImage12bit.convertTo(cameraImage, CV_8UC1, 1.0 / 128.0 );
            // 创建一个cv::Mat对象，将16位raw数据转换为BGR格式
            cv::cvtColor(cameraImage, cameraImage, cv::COLOR_GRAY2BGR);
#endif
        }


        delete[] frame.pdata;
        // 创建一个画布，用于显示摄像头图像
        cv::Mat canvas(displayHeight, displayWidth, CV_8UC3, cv::Scalar(0, 0, 0));

        // 将摄像头图像缩放到画布大小
        cv::Mat resizedImage;
        cv::resize(cameraImage, resizedImage, cv::Size(displayWidth, displayHeight));

        // 在画布中心显示缩放后的图像
        int canvasX = (displayWidth - resizedImage.cols) / 2;
        int canvasY = (displayHeight - resizedImage.rows) / 2;
        resizedImage.copyTo(canvas(cv::Rect(canvasX, canvasY, resizedImage.cols, resizedImage.rows)));

        // 将摄像头图像放置在画布中心
        /*int canvasX = (displayWidth - cameraWidth) / 2;
        int canvasY = (displayHeight - cameraHeight) / 2;
        cv::Mat resizedImage;
        cameraImage.copyTo(canvas(cv::Rect(canvasX, canvasY, cameraImage.cols, cameraImage.rows)));*/

        // 在窗口中显示画布
        cv::imshow("Camera Display", canvas);

        if(cv::getWindowProperty("Camera Display", cv::WND_PROP_AUTOSIZE) != 0){
            break;
        }

        if (cv::waitKey(1) == 27) {
            break; // 按下Esc键退出循环
        }
    }

    main_exit = 1;
    cv::destroyAllWindows();
    return;
}
#endif

static void my_handler(int signum)
{
    UNUSED(signum);
    main_exit = 1;
}

static char const *options = "VC:c:G:g:W:H:w:h:M:s:TP:X:Y:R:o:l:e:";

static void usage()
{
	printf("usage:\n"
           "-c:	   channels mask on board 0(0x3F default)\n"
           "       0x1~0x3F   bitset 1 indicates a camera link\n"
           "-C:	   channels mask on board 1(0x3F default)\n"
           "       0x1~0x3F   bitset 1 indicates a camera link\n"
           "-g:	   channels mask on board 0(0x3F default)\n"
           "       0x1~0x3F   bitset 1 indicates a camera link\n"
           "-G:	   channels mask on board 1(0x3F default)\n"
           "       0x1~0x3F   bitset 1 indicates a camera link\n"
		   "-M:	   trigger mode   cmdnum\n"
		   "       freerun              0\n"
		   "       seconds-justified    1\n"
           "-w     board 0 width(1920 default)\n"
           "-h     board 0 height(1080 default)\n"
           "-W     board 1 width(1280 default)\n"
           "-H     board 1 height(960 default)\n"
           "-X     view display region X value(1 default)\n"
           "-Y     view display region Y value(1 default)\n"
           "-s:    save image type(8 12 16 100 raw)\n"
           "-T:    print time stamp\n"
           "-P:    input frame pixel format (0:UYVY 1:Gray16le ) \n");
}

static unsigned int parse_input(int argc, char *argv[])
{
	int status = 0, opt;

	do {
		opt = getopt(argc, argv, options);
		switch (opt)
		{
        case 'C':
			globalset.chan_mask[1] = atoi(optarg) & MAX_CHANS_MASK;
			break;
		case 'c':
			globalset.chan_mask[0]  = atoi(optarg) & MAX_CHANS_MASK;
            break;
        case 'G':
			globalset.chan_mask[3] = atoi(optarg) & MAX_CHANS_MASK;
			break;
		case 'g':
			globalset.chan_mask[2]  = atoi(optarg) & MAX_CHANS_MASK;
            break;
        case 'W':
            globalset.width[1] = atoi(optarg);
            break;
        case 'H':
            globalset.height[1] = atoi(optarg);
            break;
        case 'w':
            globalset.width[0] = atoi(optarg);
            break;
        case 'h':
            globalset.height[0] = atoi(optarg);
            break;
		case 'M':
			globalset.trigger_mode = atoi(optarg);
			break;
		case 's':
			globalset.is_save_buf = atoi(optarg);
			break;
        case 'T':
            globalset.print_time_stamp=1;
            break;
        case 'P':
            globalset.pixel_format=atoi(optarg);
            break;
        case 'X':
            globalset.region_x=atoi(optarg);
            break;
        case 'Y':
            globalset.region_y=atoi(optarg);
            break;
        case 'R':
            globalset.sdl_chan=atoi(optarg);
            break;
        case 'l':
            globalset.playMode=atoi(optarg);
            break;
        case 'o':
            globalset.show=atoi(optarg);
            break;
        case 'e':
            globalset.save_h264=atoi(optarg);
            break;
		case -1:
			break;
		case 'V':
			usage();
			status = -1;
            break;
		default:
			status = -1;
			usage();
			break;
		}
	} while ((opt != -1) && (status == 0));

	optind = 1;
    if(0 == status)
    {
        printf("status :%d mask bord0:%x bord1:%x\n",status, globalset.chan_mask[0],globalset.chan_mask[1]);
        printf("WH bord0:%d*%d bord1:%d*%d\n", globalset.width[0], globalset.height[0],globalset.width[1], globalset.height[1]);
    }
	return status;
}

void save_anc(int board, int channel, frameInfo* frame)
{
	char name[256]={0};
	FILE* fp;

    //sprintf(name,"%s/%d_%d_%lld.yuv", ".", board,channel, frame->timestamp);
    printf(">>>framelen=%d\n", frame->length);
    //sprintf(name,"./%d_%d_%d_%d.anc", board,channel, frame->systime>>32,(frame->timestamp&0xffffffff));
    sprintf(name,"./%d_%d_%lld(s)_%lld(ns).anc", board,channel,( frame->systime/1000000000),(frame->systime%1000000000));
    fp = fopen(name, "wb+");
    if(fp){
        /*ret = fwrite(frame->yuvdata, 1, frame->width*frame->height*2, fp);*/
        memcpy(&g_anc_illary_date,frame->pdata+(frame->length-1920*2),sizeof(g_anc_illary_date));
        printf("head%02x%02x datalenght:%ld\n",g_anc_illary_date.header[0],g_anc_illary_date.header[1],sizeof(g_anc_illary_date));
        printf("t1:%d t2:%d t3:%d t4:%d\n",g_anc_illary_date.intergrationLines[0],g_anc_illary_date.intergrationLines[1],
        g_anc_illary_date.intergrationLines[2],g_anc_illary_date.intergrationLines[3]);
        //printf("fps:%d\n",g_anc_illary_date.frameRate);
        memcpy(&g_anc_date,frame->pdata+(frame->length-3640),sizeof(g_anc_date));
        //fprintf(fp,"frame_count:%d\n",g_anc_date.frame_count);
        //fprintf(fp,"frame_trig_sync_s:%d\n",g_anc_date.frame_trig_sync_s);
        //fprintf(fp,"frame_trig_sync_ns:%d\n",g_anc_date.frame_trig_sync_ns);
        //fprintf(fp,"frame_rev_sync_s:%d\n",g_anc_date.frame_rev_sync_s);
        //fprintf(fp,"frame_rev_sync_ns:%d\n",g_anc_date.frame_rev_sync_ns);
        //fprintf(fp,"frame_cache_irq_s:%d\n",g_anc_date.frame_cache_irq_s);
        //fprintf(fp,"frame_cache_irq_ns:%d\n",g_anc_date.frame_cache_irq_ns);
        //fprintf(fp,"frame_pc_ack:%d\n",g_anc_date.frame_pc_ack);
        //fprintf(fp,"frame_dma_trans:%d\n",g_anc_date.frame_dma_trans);
        //fprintf(fp,"framecout:%d",g_anc_date.frame_count);
        fwrite(frame->pdata+(frame->length-1920*2), 1, 1920*2, fp);
        fflush(fp);
        fclose(fp);
        //printf("%s saved!\n", name);
    }
}
//eg: 0xff03 save as 0x3f
void save_image_8(int board, int channel, frameInfo* frame)
{
	char name[256]={0};
	FILE* fp;
    unsigned char temp = 0;
    sprintf(name,"./%d_%d_%lld(s)_%09lld(ns)_8.raw", board,channel,( frame->systime/1000000000),(frame->systime%1000000000));
    fp = fopen(name, "wb+");
    if(fp){
        for(int i =0; i< frame->length; i+=2)
        {
            temp =  ((*(frame->pdata+i+1)<<4)&0xf0) + ((*(frame->pdata+i)>>4)&0xf); 
            fwrite(&temp, 1, 1, fp);
        }
        
        fflush(fp);
        fclose(fp);
    }
}
//eg: 0xff03 save as 0x3ff0
void save_image_16(int board, int channel, frameInfo* frame)
{
    image_index[board][channel]+=1;
	char name[256]={0};
	FILE* fp;
    unsigned char src[2] = {0x0,0x0};
    unsigned char dst[2] = {0x0,0x0};
    //sprintf(name,"./%d_%d_%lld(s)_%09lld(ns)_16.raw", board,channel,( frame->systime/1000000000),(frame->systime%1000000000));
    sprintf(name,"../image/%d_%d_%d_%llds_%09lldns_16.raw", board,channel,image_index[board][channel],( frame->systime/1000000000),(frame->systime%1000000000));
    fp = fopen(name, "wb+");
    if(fp){
        for(int i =0; i< frame->length; i+=2)
        {
            src[0] = *(frame->pdata+i);
            src[1] = *(frame->pdata+i+1);
            dst[0] = ((src[0]<<4)&0xf0);
            dst[1] = ((src[1]<<4)&0xf0) + ((src[0]>>4)&0xf);
            fwrite(dst, 1, 2, fp);
        }   
        fflush(fp);
        fclose(fp);
    }
}

void save_image_16_t(int board, int channel, frameInfo* frame)
{
    image_index[board][channel]+=1;
	char name[256]={0};
	FILE* fp;
    unsigned char src[2] = {0x0,0x0};
    unsigned char dst[2] = {0x0,0x0};
    //sprintf(name,"./%d_%d_%lld(s)_%09lld(ns)_16.raw", board,channel,( frame->systime/1000000000),(frame->systime%1000000000));
    sprintf(name,"../image/%d_%d_%d_%llds_%09lldns_16.raw", board,channel,image_index[board][channel],( frame->systime/1000000000),(frame->systime%1000000000));
    fp = fopen(name, "wb+");
    if(fp){
        unsigned char temp[2] = {0x0,0x0};
        for(int i =0; i<frame->length; i+=2)
        {
            temp[0] = *(frame->pdata+i);
            temp[1] = *(frame->pdata+i+1);
            *(frame->pdata+i) = ((temp[0]<<4)&0xf0);
            *(frame->pdata+i+1) = ((temp[1]<<4)&0xf0) + ((temp[0]>>4)&0xf);
        }
        fwrite(frame->pdata, frame->length,1,fp);
        fclose(fp);
    }
}


void save_image_16_1(int board, int channel, frameInfo* frame)
{
    image_index[board][channel]+=1;
	char name[256]={0};
	FILE* fp;
    unsigned char src[2] = {0x0,0x0};
    unsigned char dst[2] = {0x0,0x0};
    unsigned char byteL;
    unsigned char byteM;
    unsigned char byteH; 
    //sprintf(name,"./%d_%d_%lld(s)_%09lld(ns)_16.raw", board,channel,( frame->systime/1000000000),(frame->systime%1000000000));
    sprintf(name,"../image/%d_%d_%d_%llds_%09lldns_16.raw", board,channel,image_index[board][channel],( frame->systime/1000000000),(frame->systime%1000000000));
    fp = fopen(name, "wb+");
    /*if(fp){
        for(int i =0; i< frame->length; i+=2)
        {
            src[0] = *(frame->pdata+i);
            src[1] = *(frame->pdata+i+1);
            dst[0] = ((src[0]<<4)&0xf0);
            dst[1] = ((src[1]<<4)&0xf0) + ((src[0]>>4)&0xf);
            fwrite(dst, 1, 2, fp);
        }   
        fflush(fp);
        fclose(fp);
    }*/

    //uint8_t* tmpDataRaw = (uint8_t*)pFrameRaw[rawPingpang]->data[0];
    unsigned char tmpDataRaw[4] = {0};
    for (int i = 0; i < frame->length; i+=3)	
    {
        byteL = frame->pdata[i + 0]; 
        byteM = frame->pdata[i + 1]; 
        byteH = frame->pdata[i + 2];
        
        // 这个验证了多次，目前UVC以RGB888采集的情况下这个顺序是正确的
        uint16_t value1 = ((((uint32_t)byteL) << 4) & 0x0FF0) | ((byteH >> 0) & 0x0F);
        uint16_t value2 = ((((uint32_t)byteM) << 4) & 0x0FF0) | ((byteH >> 4) & 0x0F);	
        // raw - BAYER RGGB LE 数据格式组合
        tmpDataRaw[0] = (value1 << 0) & 0xFF; 
        tmpDataRaw[1] = (value1 >> 8) & 0xFF; 
        tmpDataRaw[2] = (value2 << 0) & 0xFF; 
        tmpDataRaw[3] = (value2 >> 8) & 0xFF;

        fwrite(tmpDataRaw, 1, 4, fp);
        
        // display - BAYER RGGB LE 数据格式组合 index -= 4;
        /*value1 = (uint32_t)((double)value1 * ((double)pow(2，16) / (double)pow(2，12))); 
        value2 = (uint32_t)((double)value2 * ((double)pow(2，16) / (double)pow(2，12))); 
        tmpData[index++] = (value1 << 0) & 0xFF; tmpData[index++]= (value1 >> 8) & 0xFF; 
        tmpData[index++] = (value2 << 0) & 0xFF; tmpData[index++]= (value2 >> 8) & 0xFF;*/
    }
    fflush(fp);
    fclose(fp);
}

void save_image_16_ex(int board, int channel, frameInfo* frame,int pixel_seq)
{
	char name[256]={0};
	FILE* fp;
    unsigned char src12[3] = {0x0,0x0,0x0};
    unsigned char dst[4] = {0x0,0x0,0x0,0x0};
    unsigned int comb_values[24]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    sprintf(name,"./%d_%d_%lld(s)_%09lld(ns).raw16.pix%d", 
    board,channel,( frame->systime/1000000000),(frame->systime%1000000000),pixel_seq);
    fp = fopen(name, "wb+");
    if(fp){
        for(int i =0; i< frame->length; i+=4)
        {
            convert16to12_seqx(frame->pdata+i,comb_values);
            src12[0] = (comb_values[pixel_seq] & 0xff);
            src12[1] = ((comb_values[pixel_seq]>>8) & 0xff);
            src12[2] = ((comb_values[pixel_seq]>>16) & 0xff);
            convert12to16(src12,dst);
            fwrite(dst, 1, 4, fp); 
        }
        fflush(fp);
        fclose(fp);
    }
}

void save_image_12_ex(int board, int channel, frameInfo* frame,int pixel_seq)
{
	char name[256]={0};
	FILE* fp;
    unsigned char src12[3] = {0x0,0x0,0x0};
    unsigned int comb_values[24]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    sprintf(name,"./%d_%d_%lld(s)_%09lld(ns).raw12.pix%d", 
    board,channel,( frame->systime/1000000000),(frame->systime%1000000000),pixel_seq);
    fp = fopen(name, "wb+");
    if(fp){
        for(int i =0; i< frame->length; i+=4)
        {
            convert16to12_seqx(frame->pdata+i,comb_values);
            src12[0] = (comb_values[pixel_seq] & 0xff);
            src12[1] = (comb_values[pixel_seq]>>8 & 0xff);
            src12[2] = (comb_values[pixel_seq]>>16 & 0xff);
            fwrite(src12, 1, 3, fp);
        }
        fflush(fp);
        fclose(fp);
    }
}
//eg: save 0xff03 as 0x03ff
void save_image_01_10(int board, int channel, frameInfo* frame)
{
	char name[256]={0};
	FILE* fp;
    sprintf(name,"./%d_%d_%lld(s)_%09lld(ns)_0110.raw", 
    board,channel,( frame->systime/1000000000),(frame->systime%1000000000));
    fp = fopen(name, "wb+");
    if(fp){
        for(int i =0; i< frame->length; i+=2)
        {
            fwrite(frame->pdata+i+1, 1, 1,fp);
            fwrite(frame->pdata+i, 1,1, fp);
        }
        fflush(fp);
        fclose(fp);
    }
}

void save_image(int board, int channel, frameInfo* frame)
{
	char name[256]={0};
	FILE* fp;
 #if 1
    framecount[board][channel]++;
    //printf("save_image in\n");
    image_index[board][channel]+=1;
    if(globalset.pixel_format == 0){
        sprintf(name,"../image/%d_%d_%d_%llds_%09lldns.yuv", board,channel,image_index[board][channel],( frame->systime/1000000000),(frame->systime%1000000000));
    }
    else {
        sprintf(name,"../image/%d_%d_%d_%llds_%09lldns.raw", board,channel,image_index[board][channel],( frame->systime/1000000000),(frame->systime%1000000000));
    }
    //printf("video%d: framecount=%02d timestamp = %llu.%09llu(s)\n", channel, framecount[board][channel],(frame->systime/1000000000),(frame->systime%1000000000));
    fp = fopen(name, "wb+");
    //printf("========================frame->length=%d\n", frame->length);
    if(fp){
        fwrite(frame->pdata, 1, frame->length, fp);
        fflush(fp);
        fclose(fp);
    }
#endif  
#if 1
    //save_image_12(board, channel, frame);
    //save_image_16(board, channel, frame);
    //save_image_16_1(board, channel, frame);
 #else  
    save_image_01_10(board, channel, frame);
    //for(int i=7; i<8;i++)
    {
        save_image_12_ex(board, channel, frame,(frame->reserved/300)%24);
        save_image_16_ex(board, channel, frame,(frame->reserved/300)%24);
    }
#endif  
    //save_anc(board, channel, frame);
}

#if 1
int sw_encodetoh264init(EnPara enpara[][8], int board, int channel, frameInfo* frame)
{
    char outPutfile[128] = {0};

#if 1
    switch(channel){
        case 0:
            fp[board][channel] = fopen("chan_0_log.txt", "w+");
            break;
        case 1:
            fp[board][channel] = fopen("chan_1_log.txt", "w+");
            break;
        case 2:
            fp[board][channel] = fopen("chan_2_log.txt", "w+");
            break;
        case 3:
            fp[board][channel] = fopen("chan_3_log.txt", "w+");
            break;
        case 4:
            fp[board][channel] = fopen("chan_4_log.txt", "w+");
            break;
        case 5:
            fp[board][channel] = fopen("chan_5_log.txt", "w+");
            break;
        case 6:
            fp[board][channel] = fopen("chan_6_log.txt", "w+");
            break;
        case 7:
            fp[board][channel] = fopen("chan_7_log.txt", "w+");
            break;
        default:
            break;
    }
#endif

    // 设置视频参数
    enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_YUYV422;
    enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_YUV420P;

#if 1
    // 获取软件编码器
    enpara[board][channel].codec = const_cast<AVCodec*>(avcodec_find_encoder(AV_CODEC_ID_H264));
    if(!enpara[board][channel].codec){
        fprintf(stderr, "Error finding libx264 encoder\n");
    }
#else
    // 获取硬件编码器
    enpara[board][channel].codec = avcodec_find_encoder_by_name("h264_nvenc");
    if(!enpara[board][channel].codec){
        fprintf(stderr, "Error finding h264_nvenc encoder\n");
        return 1;
    }
   
#endif 

    // 创建编码器上下文
    enpara[board][channel].codec_ctx = avcodec_alloc_context3(enpara[board][channel].codec);
    if (!enpara[board][channel].codec_ctx) {
        fprintf(stderr, "Failed to allocate codec context\n");
        av_frame_free(&enpara[board][channel].srcframe);
        av_frame_free(&enpara[board][channel].dstframe);
        sws_freeContext(enpara[board][channel].sws_ctx);
        return 1;
    }

    // 设置编码参数
    enpara[board][channel].codec_ctx->codec_id = AV_CODEC_ID_H264;
    enpara[board][channel].codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    enpara[board][channel].codec_ctx->width = frame->width;
    enpara[board][channel].codec_ctx->height = frame->height;
    enpara[board][channel].codec_ctx->time_base = (AVRational){1, 25};
    enpara[board][channel].codec_ctx->framerate = (AVRational){25, 1};
    enpara[board][channel].codec_ctx->pix_fmt = dst_pix_fmt;
    enpara[board][channel].codec_ctx->bit_rate = 4000000;
    enpara[board][channel].codec_ctx->gop_size = 10;
    enpara[board][channel].codec_ctx->max_b_frames = 1;
    enpara[board][channel].codec_ctx->qmin = 10;
    enpara[board][channel].codec_ctx->qmax = 51;
    enpara[board][channel].codec_ctx->qcompress = 0.6;
    enpara[board][channel].codec_ctx->profile = FF_PROFILE_H264_MAIN;
    //printf("================width=%d\n", enpara[board][channel].codec_ctx->width);
    //printf("================height=%d\n", enpara[board][channel].codec_ctx->height);

    // 检查是否支持硬件加速
    /*if(!(enpara[board][channel]->codec->capabilities & AV_CODEC_CAP_HARDWARE)){
        fprintf(stderr, "Codec %s does not support hardware acceleration\n", enpara[board][channel]->codec->name);
        return 1;
    }
    enpara[board][channel]->codec_ctx->hw_frames_ctx = av_hwframe_ctx_alloc(enpara[board][channel]->codec_ctx->hw_device_ctx);
    if(!enpara[board][channel]->codec_ctx->hw_frames_ctx){
        fprintf(stderr, "Error creating hardware frame context\n");
    }*/

    // 打开编码器
    int ret = avcodec_open2(enpara[board][channel].codec_ctx, enpara[board][channel].codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to open codec: %d\n", ret);
        avcodec_free_context(&enpara[board][channel].codec_ctx);
        av_frame_free(&enpara[board][channel].srcframe);
        av_frame_free(&enpara[board][channel].dstframe);
        sws_freeContext(enpara[board][channel].sws_ctx);
        return 1;
    }

    // 创建输出格式上下文
    sprintf(outPutfile, "../image/output_%d_%d_%d.h264", board, channel, fileindex[board][channel]);
    enpara[board][channel].out_fmt_ctx = NULL;
    enpara[board][channel].out_fmt = const_cast<AVOutputFormat*>(av_guess_format(NULL, outPutfile, NULL));
    avformat_alloc_output_context2(&enpara[board][channel].out_fmt_ctx, enpara[board][channel].out_fmt, NULL, NULL);
    if (!enpara[board][channel].out_fmt_ctx) {
        fprintf(stderr, "Failed to allocate output format context\n");
        avcodec_free_context(&enpara[board][channel].codec_ctx);
        av_frame_free(&enpara[board][channel].srcframe);
        av_frame_free(&enpara[board][channel].dstframe);
        sws_freeContext(enpara[board][channel].sws_ctx);
        return 1;
    }

    // 创建输出流
    enpara[board][channel].out_stream = avformat_new_stream(enpara[board][channel].out_fmt_ctx, enpara[board][channel].codec);
    if (!enpara[board][channel].out_stream) {
        fprintf(stderr, "Failed to create output stream\n");
        avformat_free_context(enpara[board][channel].out_fmt_ctx);
        avcodec_free_context(&enpara[board][channel].codec_ctx);
        av_frame_free(&enpara[board][channel].srcframe);
        av_frame_free(&enpara[board][channel].dstframe);
        sws_freeContext(enpara[board][channel].sws_ctx);
        return 1;
    }

    //初始化输出流
    enpara[board][channel].out_stream->codecpar->codec_id = AV_CODEC_ID_H264;
    enpara[board][channel].out_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    enpara[board][channel].out_stream->codecpar->width = frame->width;
    enpara[board][channel].out_stream->codecpar->height = frame->height;
    enpara[board][channel].out_stream->codecpar->format = enpara[board][channel].codec_ctx->pix_fmt;
    enpara[board][channel].out_stream->codecpar->bit_rate = 4000000;
    ret = avcodec_parameters_from_context(enpara[board][channel].out_stream->codecpar, enpara[board][channel].codec_ctx);
    if (ret < 0) {
        fprintf(stderr, "Failed to copy codec parameters: %d\n", ret);
        avformat_free_context(enpara[board][channel].out_fmt_ctx);
        avcodec_free_context(&enpara[board][channel].codec_ctx);
        av_frame_free(&enpara[board][channel].srcframe);
        av_frame_free(&enpara[board][channel].dstframe);
        sws_freeContext(enpara[board][channel].sws_ctx);
        return 1;
    }
 
    // 打开输出文件
    ret = avio_open(&enpara[board][channel].out_fmt_ctx->pb, outPutfile, AVIO_FLAG_WRITE);
    if (ret < 0) {
        fprintf(stderr, "Failed to open output file: %d\n", ret);
        avformat_free_context(enpara[board][channel].out_fmt_ctx);
        avcodec_free_context(&enpara[board][channel].codec_ctx);
        av_frame_free(&enpara[board][channel].srcframe);
        av_frame_free(&enpara[board][channel].dstframe);
        sws_freeContext(enpara[board][channel].sws_ctx);
        return 1;
    }

    // 写入文件头
    ret = avformat_write_header(enpara[board][channel].out_fmt_ctx, NULL);
    if(ret < 0){
        fprintf(stderr, "Error writing header: %d\n", ret);
        return 1;
    }

    // 创建转换上下文
    enpara[board][channel].sws_ctx = sws_getContext(frame->width, frame->height, src_pix_fmt, frame->width, frame->height, dst_pix_fmt, 0, NULL, NULL, NULL);
    if(!enpara[board][channel].sws_ctx){
        fprintf(stderr, "Error creating conversion context\n");
        return 1;
    }

    // 创建AVFrame对象
    enpara[board][channel].srcframe = av_frame_alloc();
    if (!enpara[board][channel].srcframe) {
        fprintf(stderr, "Failed to allocate frame\n");
        return 1;
    }
    enpara[board][channel].srcframe->width = frame->width;
    enpara[board][channel].srcframe->height = frame->height;
    enpara[board][channel].srcframe->format = src_pix_fmt;//dst_pix_fmt;
    printf("src:frame->width=%d, frame->height=%d\n", frame->width, frame->height);
    ret = av_frame_get_buffer(enpara[board][channel].srcframe, 32);
    if (ret < 0) {
        fprintf(stderr, "Failed to allocate frame buffer\n");
        av_frame_free(&enpara[board][channel].srcframe);
        return 1;
    }

    // 创建AVFrame对象
    enpara[board][channel].dstframe = av_frame_alloc();
    if (!enpara[board][channel].dstframe) {
        fprintf(stderr, "Failed to allocate frame\n");
        return 1;
    }
    enpara[board][channel].dstframe->width = frame->width;
    enpara[board][channel].dstframe->height = frame->height;
    enpara[board][channel].dstframe->format = dst_pix_fmt;
    printf("dst:frame->width=%d, frame->height=%d\n", frame->width, frame->height);
    ret = av_frame_get_buffer(enpara[board][channel].dstframe, 32);
    if (ret < 0) {
        fprintf(stderr, "Failed to allocate frame buffer\n");
        av_frame_free(&enpara[board][channel].dstframe);
        return 1;
    }

    // 创建AVPacket
    enpara[board][channel].pkt = av_packet_alloc();
    if(!enpara[board][channel].pkt){
        fprintf(stderr, "Error allocating packet\n");
        return 1;
    }

    enpara[board][channel].frame_count = 0;

    return 0;
}

int sw_encodetoh264uninit(EnPara enpara[][8], int board, int channel)
{
    printf("结束编码并释放资源\n");
    // 结束编码并释放资源
    int ret = avcodec_send_frame(enpara[board][channel].codec_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error sending end-of-stream to encoder: %d\n", ret);
    }
    while (ret >= 0) {
        ret = avcodec_receive_packet(enpara[board][channel].codec_ctx, enpara[board][channel].pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            fprintf(stderr, "Error receiving packet from encoder: %d\n", ret);
            break;
        }
        av_packet_rescale_ts(enpara[board][channel].pkt, enpara[board][channel].codec_ctx->time_base, enpara[board][channel].out_stream->time_base);
        enpara[board][channel].pkt->stream_index = enpara[board][channel].out_stream->index;
        ret = av_interleaved_write_frame(enpara[board][channel].out_fmt_ctx, enpara[board][channel].pkt);
        if (ret < 0) {
            fprintf(stderr, "Error writing frame to output file: %d\n", ret);
            break;
        }
        av_packet_unref(enpara[board][channel].pkt);
    }

    av_write_trailer(enpara[board][channel].out_fmt_ctx);
    avio_close(enpara[board][channel].out_fmt_ctx->pb);
    avformat_free_context(enpara[board][channel].out_fmt_ctx);
    avcodec_free_context(&enpara[board][channel].codec_ctx);
    av_frame_free(&enpara[board][channel].srcframe);
    av_frame_free(&enpara[board][channel].dstframe);
    sws_freeContext(enpara[board][channel].sws_ctx);

    return 0;
}

int sw_encodetoh264(EnPara enpara[][8], int board, int channel, frameInfo* frame) 
{
    // 获取逐帧读取的YUV文件并编码输出到H.264文件
    memcpy(enpara[board][channel].srcframe->data[0], frame->pdata, frame->length);
    
#if 1 //test
    if((wflag1[board][channel] == 0) && (srcframecount[board][channel] == 0)){
        wflag1[board][channel] = 1;
        start_time[board][channel] = av_gettime();
    } 
    srcframecount[board][channel]++;   
#endif

    // 将YUV数据转换成编码器所需的格式
    sws_scale(enpara[board][channel].sws_ctx, enpara[board][channel].srcframe->data, enpara[board][channel].srcframe->linesize, 0, enpara[board][channel].srcframe->height, enpara[board][channel].dstframe->data, enpara[board][channel].dstframe->linesize);
    
    // 设置时间戳和类型
    enpara[board][channel].dstframe->pts = enpara[board][channel].frame_count * enpara[board][channel].codec_ctx->time_base.num;
    enpara[board][channel].dstframe->pict_type = AV_PICTURE_TYPE_NONE;

    // 编码帧
    int ret = avcodec_send_frame(enpara[board][channel].codec_ctx, enpara[board][channel].dstframe);
    if (ret < 0) {
        fprintf(stderr, "Error sending frame to encoder: %d\n", ret);
        return 1;
    }

    while (ret >= 0) {      
        ret = avcodec_receive_packet(enpara[board][channel].codec_ctx, enpara[board][channel].pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if(ret < 0){
            fprintf(stderr, "Error receiving packet from encoder: %d\n", ret);
            return 1;
        }

        // 写入输出文件
        enpara[board][channel].pkt->stream_index = enpara[board][channel].out_stream->index;
        av_packet_rescale_ts(enpara[board][channel].pkt, enpara[board][channel].codec_ctx->time_base, enpara[board][channel].out_stream->time_base);
        ret = av_interleaved_write_frame(enpara[board][channel].out_fmt_ctx, enpara[board][channel].pkt);
        if (ret < 0) {
            fprintf(stderr, "Error writing frame to output file: %d\n", ret);
            return 1;
        }

#if 1   // 当写入的文件大于1G时，写下一个文件
        filesize[board][channel]+=enpara[board][channel].pkt->size;
        if(filesize[board][channel] > 1024*1024*1024*1){ 
            fileindex[board][channel]++;
            filefullflag[board][channel] = 1;
        }
#endif

        av_packet_unref(enpara[board][channel].pkt);

        enframecount[board][channel]++;
    }

#if 1 // test
    if(wflag[board][channel] == 0){
        if(srcframecount[board][channel] == 1800){
            end_time[board][channel] = av_gettime();
            duration[board][channel] = (end_time[board][channel] - start_time[board][channel]) / 1000000.0;
            fps[board][channel] = enframecount[board][channel] / duration[board][channel];
            fprintf(fp[board][channel],"board %d :channel %d :Encode %d frames in %g seconds (%g fps)\n",board, channel, enframecount[board][channel], duration[board][channel], fps[board][channel]);

            //char buff[256] = {0};
           // sprintf(buff, "channel %d :Encode %d frames in %g seconds (%g fps)\n", channel, enframecount[channel], duration[channel], fps[channel]);
           // printf("channel %d :Encode %d frames in %g seconds (%g fps)\n", channel, enframecount[channel], duration[channel], fps[channel]);

            wflag[board][channel] = 1;
        }
    }
#endif

    enpara[board][channel].frame_count++;
    //printf("编码第 %d 帧结束\n", enpara[board][channel].frame_count);
    return 0;
}
#endif


int main(int argc, char *argv[])
{
    struct sigaction action;
    int status;

    memset(&action, 0, sizeof(action));
    action.sa_handler = my_handler;
    sigaction(SIGINT, &action, NULL);

    memset(&globalset, 0, sizeof(globalset));
    for(int i = 0; i < MAX_BOARD; i++){
         globalset.chan_mask[i] = 0;
         globalset.width[i] = 1920;
         globalset.height[i] = 1080;
    }

    globalset.trigger_mode = 0;
    globalset.is_save_buf = 0;
    globalset.save_h264 = 0;
    globalset.print_time_stamp = 0;
    globalset.pixel_format = 0;
    globalset.region_x = 1;
    globalset.region_y = 1;
    globalset.show = 0;
    if (0 != parse_input(argc, argv)) {
        return EXIT_FAILURE;
    }

    View *pView = new View(globalset.width, globalset.height);
    status = pView->init();
    if(status != 0){
        return 0;
    }
    pView->display();
    if(globalset.show == 1){
        if(globalset.playMode > 0){
            //opencv_multi_channel_16_play();
            opencv_multi_channel_8_play();
        }
        else{
            for(int i = 0; i < BOARDMAX; i++){
                for(int j = 0; j < CHANELMAX; j++) {
                    if(globalset.chan_mask[i] & (1 << j)){
                        opencv_single_channel_play(i, j);
                        goto end;
                    }
                }
            }
        }
    }
end:        
    while(main_exit == 0){
        sleep_ms(500);
    }
    
    pView->deinit();

    delete pView;
    system("../kill_view.sh");
}
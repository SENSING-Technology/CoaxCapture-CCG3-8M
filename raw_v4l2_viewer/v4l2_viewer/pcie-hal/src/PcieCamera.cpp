#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "PcieCamera.h"
#include "camera.h"
#include "camera_common.h"

/* PCIE Camera realization */
PcieCamera::PcieCamera(int board, int channelmask, int w[],int h[],int fps[])
{
   int index, i;
   printf("PcieCamera\n");
   for(i = 0;i < MAX_CAMERAS; i++){
      m_width[i] = w[i];
      m_height[i] = h[i];
      m_fps[i] = fps[i];
      m_Camera[i] = NULL;
   }
   printf("PcieCamera 19\n");
   m_channels = MAX_CAMERAS;
   if(board == 0 || board == 1||board==2||board==3){
      m_boardNum = board;
   }else{
      m_boardNum = 0;
   }
printf("PcieCamera 26\n");
   m_chmask = channelmask;

   for (i = 0; i < m_channels; i++)
   {
      if(m_chmask & (1<<i)){
         index = i + m_boardNum*MAX_CAMERAS;
         printf("PcieCamera 33  m_fps:%d\n",m_fps);
         Camera *camera = new Camera(index, m_fps[i], m_width[i], m_height[i]);
         printf("PcieCamera 35\n");
         m_Camera[i] = camera;
      }
   }
   printf("PcieCamera 37\n");
   m_flag = 0;
}

PcieCamera::~PcieCamera(){
   std::cout<<"PcieCamera::~PcieCamera"<<std::endl;

   for (int i = 0; i < m_channels; i++)
   {
      if(m_chmask & (1<<i)){
         delete m_Camera[i];
         m_Camera[i] = NULL;
      }
   }
}

int PcieCamera::openCamera()
{
   int i = 0;
   /* open cameras*/
   for (i = 0; i < m_channels; i++)
   {
      if(m_chmask & (1<<i)){
         if (m_Camera[i]->CameraOpen() < 0)
         {
            std::cout << "mCamera[" << i << "]->CameraOpen() failed" << std::endl;
            return -1;
         }
      }
   }

   return 0;
}

int PcieCamera::startCamera(int mode)
{
   /* start cameras*/
   for (int i = 0; i < m_channels; i++)
   {
      if(m_chmask & (1<<i)){
         if (m_Camera[i]->CameraStart() < 0)
         {
            std::cout << "mCamera[" << i << "]->CameraStart() failed" << std::endl;
            return -1;
         }
      }
   }

   init(mode);

   return 0;
}

int PcieCamera::stopCamera()
{
   for (int i = 0; i <m_channels; i++)
   {
      if(m_chmask & (1<<i)){
         if (m_Camera[i])
         {
            m_Camera[i]->CameraStop();
         }
      }
   }
   return 0;
}

int PcieCamera::pollCamera(int *cameraFlags)
{
   int i = 0;
   /* Select 8 channels camera */
   /* set select fd sets*/
   FD_ZERO(&m_fds);
   for (i = 0; i < m_channels; i++)
   {
      if(m_chmask & (1<<i)){
         int curfd = m_Camera[i]->getFd();

         FD_SET(curfd, &m_fds);

         if (curfd > m_pollFd)  m_pollFd = curfd;
      }
   }

   m_timeout.tv_sec = 3;
   m_timeout.tv_usec = 10000;
   *cameraFlags = 0;
   int r = select(m_pollFd + 1, &m_fds, 0, 0, &m_timeout);
   if (r == -1)
   {
      std::cout << "select failed\n"
                << std::endl;
      return -1;
   }
   else if (r == 0)
   {
      std::cout << "select timeout" << std::endl;
      return -2;
   }
   else
   {
      /* read channel data */
      for (i = 0; i < m_channels; i++)
      {
         if(m_chmask & (1<<i)){
            if (FD_ISSET(m_Camera[i]->getFd(), &m_fds))
            {
               m_Camera[i]->CameraCapture();
               *cameraFlags |= 1<<i;
            }
         }
      }
   }
   return 0;
}

void PcieCamera::init(int mode)
{
   if(m_flag) return;

   m_flag = 1;

   if(mode == 0){
      /* start cameras*/
      for (int i = 0; i < m_channels; i++)
      {
         if(m_chmask & (1<<i)){
            m_efds[i] = epoll_create(1);

            struct epoll_event ev;
            ev.data.fd = m_Camera[i]->getFd();;
            ev.events = EPOLLIN | EPOLLERR;
            epoll_ctl(m_efds[i], EPOLL_CTL_ADD, m_Camera[i]->getFd(), &ev);
         }
      }
   }

   if(mode == 1){
      m_efds[0] = epoll_create(m_channels);

      struct epoll_event ev;

      ev.events = EPOLLIN | EPOLLERR;

      for (int i = 0; i < m_channels; i++)
      {
         if(m_chmask & (1<<i)){
            ev.data.fd = m_Camera[i]->getFd();
            m_fd2chan[ev.data.fd] = i;
            epoll_ctl(m_efds[0], EPOLL_CTL_ADD, m_Camera[i]->getFd(), &ev);
         }
      }
   }
}

int PcieCamera::readFrameTimeOut(int channel, frameInfo *info, int timeout)
{

   buffer_t buffer;
#if  0
   /* Select 8 channels camera */
   /* set select fd sets*/
   FD_ZERO(&m_fds);

   int curfd = m_Camera[channel]->getFd();

   FD_SET(curfd, &m_fds);

   m_pollFd = curfd;

   m_timeout.tv_sec = timeout/1000;
   m_timeout.tv_usec = timeout*1000;

   int r = select(m_pollFd + 1, &m_fds, 0, 0, &m_timeout);
   if (r == -1)
   {
      std::cout << "select failed\n"
                << std::endl;
      return -1;
   }
   else if (r == 0)
   {
      std::cout << "select timeout" << std::endl;
      return -2;
   }
   else
   {
      if (FD_ISSET(m_Camera[channel]->getFd(), &m_fds))
      {
         m_Camera[channel]->CameraCapture();
         if(0 == m_Camera[channel]->getYuvFrame(&buffer))
         {
            if(info->isMemcpy)
            {
               memcpy(info->pdata, buffer.start, buffer.length);
            }else{
               info->pdata = buffer.start;
            }
            info->length = buffer.length;
            info->exposuretime = buffer.exposuretime;
            info->timestamp = buffer.timestamp;
            info->index = buffer.frameid;
            info->systime = buffer.systime;
            info->reserved = buffer.reserved;
            return 0;
         }
         else
            return -3;
      }
   }
   return 0;
#else

   struct epoll_event epEvents[1] = {};

   int eventNum = epoll_wait(m_efds[channel], epEvents, 1, timeout);
   if(eventNum <= 0) {
      //perror("epoll failure");
      return -1;
   }
   for(int i = 0; i < eventNum; ++i) {
      int tmpFd = epEvents[i].data.fd;
      if ((epEvents[i].events & EPOLLERR) || (epEvents[i].events & EPOLLRDHUP)) {
         if(tmpFd == m_Camera[channel]->getFd()) {
            return -1;
         }
      }
      else if (epEvents[i].events & EPOLLIN) {
         if(tmpFd == m_Camera[channel]->getFd()) {//client connect
            m_Camera[channel]->CameraCapture();
            if(0 == m_Camera[channel]->getYuvFrame(&buffer))
            {
               if(info->isMemcpy)
               {
                  memcpy(info->pdata, buffer.start, buffer.length);
               }else{
                  info->pdata = buffer.start;
               }
               info->length = buffer.length;
			   if(1920 == m_width[channel]){
				   info->exposuretime = buffer.exposuretime; //for imx390
				   info->timestamp = buffer.timestamp - info->exposuretime/2 +13150*1000;
				   /*printf("[%d] %lld %lld %lld %u \n",m_boardNum,buffer.timestamp, info->timestamp, buffer.timestamp-info->timestamp,info->exposuretime);*/
			   }else{
				   info->exposuretime = buffer.exposuretime1; //for ov10640
				   info->timestamp = buffer.timestamp - info->exposuretime/2 + 14579*1000;
				   /*printf("[%d] %lld %lld %lld %u \n",m_boardNum, buffer.timestamp, info->timestamp, buffer.timestamp-info->timestamp,info->exposuretime);*/
			   }
			   info->reserved1 = buffer.timestamp;
               info->index = buffer.frameid;
               info->systime = buffer.systime;
               info->reserved = buffer.reserved;
               return 0;
            }
            else
               return -3;
         }
         else {  //client msg

         }
      }
      else {
         cout<< "!!UNKNOWN events"<<endl;
      }
   }
   return 0;
#endif
}

int PcieCamera::pollFrameTimeOut(frameInfo *info, int *chan, int timeout)
{
   buffer_t buffer;
   int channel;
   struct epoll_event epEvents[1] = {};

   int eventNum = epoll_wait(m_efds[0], epEvents, 1, timeout);
   if(eventNum <= 0) {
	  perror("epoll failure");
      return -1;
   }
   //printf("=============eventNum=%d\n", eventNum);
   for(int i = 0; i < eventNum; ++i) {
      int tmpFd = epEvents[i].data.fd;

      channel = m_fd2chan[tmpFd];
      if ((epEvents[i].events & EPOLLERR) || (epEvents[i].events & EPOLLRDHUP)) {
         if(channel < m_channels) {
            *chan = channel;
            //return -1;
         }
         return -1;
      }
      else if (epEvents[i].events & EPOLLIN) {
         if(channel < m_channels) {//client connect
            *chan = channel;
			/*printf("channel:%d\n",channel);*/
            m_Camera[channel]->CameraCapture();
            if(0 == m_Camera[channel]->getYuvFrame(&buffer))
            {
               m_Camera[channel]->getFrameFormat(info->width,info->height);
               if(info->isMemcpy)
               {
				   memcpy(info->pdata, buffer.start, buffer.length);
               }else{
                  info->pdata = buffer.start;
               }
               info->length = buffer.length;
               //printf("pollFrameTimeOut lenght:%d w:%d h:%d time:%lld frameid:%d\n",info->length,info->width,info->height, buffer.systime,buffer.frameid);

               if(1920 == m_width[channel]){
                  info->exposuretime = buffer.exposuretime; //for imx390
                  /*printf("%lld - %d + 13150000\n",buffer.timestamp,info->exposuretime/2);*/
                  info->timestamp = buffer.timestamp - info->exposuretime/2 + 13150*1000;
               }else{
                  info->exposuretime = buffer.exposuretime1; //for ov10640
                  info->timestamp = buffer.timestamp - info->exposuretime/2 + 14579*1000;
               }
               info->reserved1 = buffer.timestamp;
                  info->index = buffer.frameid;
                  info->systime = buffer.systime;
                  info->reserved = buffer.reserved;
               return 0;
            }
            else
               return -3;
         }
         else {  //client msg
            return -1;
         }
      }
      else {
         cout<< "!!UNKNOWN events"<<endl;
         return -1;
      }
   }
	printf("epoll successed  %d\n",__LINE__);

   return 0;

}

int PcieCamera::readCamera(int channel, frameInfo *info)
{
   buffer_t buffer;
   if(m_chmask & (1<<channel))
   {
      //printf("chmask ok/n");
      if(m_Camera[channel])
      {
         //printf("m_Camera ok/n");
         if(0 == m_Camera[channel]->getYuvFrame(&buffer))
         {
            if(info->isMemcpy)
            {
               memcpy(info->pdata, buffer.start, buffer.length);
            }else{
               info->pdata = buffer.start;
            }
            info->length = buffer.length;
            info->exposuretime = buffer.exposuretime;
            info->timestamp = buffer.timestamp;
            info->index = buffer.frameid;
            info->systime = buffer.systime;
            info->reserved = buffer.reserved;
            return 0;
         }
      }
   }

   return -1;
}

int PcieCamera::readFrameCount(int channel, int* errNum)
{
    if(channel < 0 || channel >= m_channels)
      return -1;
    if(m_Camera[channel]){
      return m_Camera[channel]->getFrameCount(errNum);
    }

    return -2;
}

int PcieCamera::closeCamera()
{

   std::cout<<"Free card"<<m_boardNum<< " mCamera buffers"<<std::endl;
   //Free mCamera buffers.
   for (int i = 0; i <m_channels; i++)
   {
      if(m_chmask & (1<<i)){
         if (m_Camera[i])
         {
            std::cout << "free " << m_boardNum <<"-"<< i << " buffers" << endl;
            //m_Camera[i]->CameraStop();
            m_Camera[i]->CameraClose();
         }
         if(m_efds[i]){
            close(m_efds[i]);
            m_efds[i] = 0;
         }
      }
   }
   return 0;
}

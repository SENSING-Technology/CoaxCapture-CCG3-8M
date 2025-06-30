#define __STDC_CONSTANT_MACROS
extern "C"
{
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
    #include <libavutil/avutil.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
}
#include <stdio.h>
#include <time.h>
#include "watermark.h"
enum AVPixelFormat pixFmt = AV_PIX_FMT_UYVY422;
//int in_width=1920;
//int in_height=1080;

int waterMarkFrame(AVFrame *frame_in,AVFrame *frame_out,int w,int h,const char *str)
{
	int ret;
	/*根据名字获取ffmegding定义的filter*/
	const AVFilter *buffersrc=avfilter_get_by_name("buffer");//原始数据
	const AVFilter *buffersink=avfilter_get_by_name("buffersink");//处理后的数据
	/*动态分配AVFilterInOut空间*/
	AVFilterInOut *outputs=avfilter_inout_alloc();
	AVFilterInOut *inputs=avfilter_inout_alloc();	
	/*创建AVFilterGraph,分配空间*/
	AVFilterGraph *filter_graph;//对filters系统的整体管理结构体
	filter_graph = avfilter_graph_alloc();
	enum AVPixelFormat pix_fmts[]={pixFmt, AV_PIX_FMT_NONE};//设置格式
	/*过滤器参数：解码器的解码帧将被插入这里。*/
	char args[256];
	snprintf(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		w,h,pixFmt,1,25,1,1);//图像宽高，格式，帧率，画面横纵比
	/*创建过滤器上下文,源数据AVFilterContext*/
	AVFilterContext *buffersrc_ctx;
	ret=avfilter_graph_create_filter(&buffersrc_ctx,buffersrc,"in",args,NULL,filter_graph);
	if(ret<0)
	{
		printf("创建src过滤器上下文失败AVFilterContext\n");
		return -1;
	}		
	/*创建过滤器上下文，处理后数据buffersink_params*/
	AVBufferSinkParams *buffersink_params;
	buffersink_params=av_buffersink_params_alloc();
	buffersink_params->pixel_fmts=pix_fmts;//设置格式
	AVFilterContext *buffersink_ctx;
	ret=avfilter_graph_create_filter(&buffersink_ctx,buffersink,"out",NULL,buffersink_params,filter_graph);
	av_free(buffersink_params);
	if(ret<0)
	{
		printf("创建sink过滤器上下文失败AVFilterContext\n");
		return -2;
	}	
	/*过滤器链输入/输出链接列表*/
	outputs->name       =av_strdup("in");
	outputs->filter_ctx =buffersrc_ctx;
	outputs->pad_idx    =0;
	outputs->next		=NULL;

	inputs->name		=av_strdup("out");
	inputs->filter_ctx	=buffersink_ctx;
	inputs->pad_idx    =0;
	inputs->next		=NULL;
	char filter_desrc[1000]={0};//要添加的水印数据
	printf("%s\n",str);
	snprintf(filter_desrc,sizeof(filter_desrc),"drawtext=fontfile=msyhbd.ttc:fontcolor=green:fontsize=50:x=100:y=100:text='%s\nSENSING'",str);
	if(avfilter_graph_parse_ptr(filter_graph,filter_desrc,&inputs,&outputs, NULL)<0)//设置过滤器数据内容
	{
		printf("添加字符串信息失败\n");
		return -3;
	}
	/*检测配置信息是否正常*/
	if(avfilter_graph_config(filter_graph,NULL)<0)
	{
		printf("配置信息有误\n");
		return -4;
	}	
	#if 0
	/*
		查找要在使用的过滤器，将要触处理的数据添加到过滤器
		注意：时间若从外面传入(即144行数据已完整)，则此处不需要查找，直接添加即可，否则需要添加下面代码
	*/
	AVFilterContext* filter_ctx;//上下文
	int parsed_drawtext_0_index = -1;
	 for(int i=0;i<filter_graph->nb_filters;i++)//查找使用的过滤器
	 {
		 AVFilterContext *filter_ctxn=filter_graph->filters[i];
		 printf("[%s %d]:filter_ctxn_name=%s\n",__FUNCTION__,__LINE__,filter_ctxn->name);
		 if(!strcmp(filter_ctxn->name,"Parsed_drawtext_0"))
		 {
			parsed_drawtext_0_index=i;
		 }
	 }
	 if(parsed_drawtext_0_index==-1)
	 {
		printf("[%s %d]:no Parsed_drawtext_0\n",__FUNCTION__,__LINE__);//没有找到过滤器
	 }
	 filter_ctx=filter_graph->filters[parsed_drawtext_0_index];//保存找到的过滤器
	 
		/*获取系统时间，将时间加入到过滤器*/
		char sys_time[64];
		time_t sec,sec2;	 
		sec=time(NULL);
		if(sec!=sec2)
		{
			sec2=sec;
			struct tm* today = localtime(&sec2);	
			strftime(sys_time, sizeof(sys_time), "%Y/%m/%d %H\\:%M\\:%S", today);       //24小时制
		}
		av_opt_set(filter_ctx->priv, "text", sys_time, 0 );  //设置text到过滤器
	 #endif
	/*往源滤波器buffer中输入待处理数据*/
	 if(av_buffersrc_add_frame(buffersrc_ctx,frame_in)<0)
	 {
		return -5;
	 }
	 /*从滤波器中输出处理数据*/
	 if(av_buffersink_get_frame(buffersink_ctx, frame_out)<0)
	 {
		return -6;
	 }
	avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
    avfilter_graph_free(&filter_graph);
	return 0;
}

int waterMark(unsigned char *frame_buffer_in, unsigned char *frame_buffer_out,int w,int h,const char *str)
{
	int in_width=w;
	int in_height=h;

	AVFrame *frame_in=av_frame_alloc();
	//unsigned char *frame_buffer_in;
	//frame_buffer_in=(unsigned char *)av_malloc(av_image_get_buffer_size(pixFmt,in_width,in_height,4));
	/*根据图像设置图像指针和内存对齐方式*/
	av_image_fill_arrays(frame_in->data,frame_in->linesize,frame_buffer_in,pixFmt,in_width,in_height,4);

	AVFrame *frame_out=av_frame_alloc();
	//unsigned char *frame_buffer_out;
	//frame_buffer_out=(unsigned char *)av_malloc(av_image_get_buffer_size(pixFmt,in_width,in_height,4));
	av_image_fill_arrays(frame_out->data,frame_out->linesize,frame_buffer_out,pixFmt,in_width,in_height,4);

	frame_in->width=in_width;
	frame_in->height=in_height;
	frame_in->format=pixFmt;

	frame_out->width=in_width;
	frame_out->height=in_height;
	frame_out->format=pixFmt;


/*     char sys_time[64];
	time_t sec,sec2;
    sec=time(NULL);
    if(sec!=sec2)
    {
        sec2=sec;
        struct tm* today = localtime(&sec2);	
        strftime(sys_time, sizeof(sys_time), "%Y/%m/%d %H\\:%M\\:%S", today);    
    } */
    
    waterMarkFrame(frame_in,frame_out,in_width,in_height,str);//添加水印

	av_image_copy_to_buffer(frame_buffer_out,in_width*in_height*2,frame_out->data,frame_out->linesize,AV_PIX_FMT_YUV422P,frame_out->width,frame_out->height,4);
	av_frame_free(&frame_in);
	av_frame_free(&frame_out);
    return 0;	
}

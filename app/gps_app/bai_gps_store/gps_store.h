#ifndef _H_GPS_STORE_IO_
#define _H_GPS_STORE_IO_
/*********************************************************************************
  * @文件名称 :gps_store.h
  * @功能描述 :所有和存储管理及和应用程序相关的函数都在这里
  * @作	   者 :白养民
  * @创建日期 :2015-6-19
  * @文件版本 :0.01
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-19	创建函数
*********************************************************************************/
#ifdef __cplusplus
#if __cplusplus
	extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include "hi_common.h"
#include "hi_comm_video.h"
#include <time.h>

#define DF_STORE_AV_PACK_SIZE			0x10000		///每次处理的音视频包的大小
#define DF_STORE_AV_PACK_SIZE_X2		0x20000		///每次处理的音视频包的大小 * 2
#define DF_STORE_CHANNEL_MAX			5			///定义的总通道数量
#define DF_STORE_AUDIO_FRAME_SIZE		168			///音频数据每包的大小
#define DF_STORE_AUDIO_FRAME_MAX		8			///音频数据最大帧数，该值最小为6
#define DF_STORE_WRITE_INDEX_START		1024		///录像数据开始写入的位置，单位为64K
#define DF_STORE_FOLDER_LIST_LEN		16			///文件夹列表长度
#define DF_STORE_FILE_LIST_HEAD_LEN		64			///文件列表头长度
#define DF_STORE_FILE_LIST_LEN			128			///文件列表长度
#define DF_STORE_NVR_LIST_LEN			64			///nvr文件列表信息长度
#define DF_STORE_NVR_LIST_HEAD_LEN		64			///nvr文件列表信息头长度

#define DF_STORE_MAGIC_FOLDER_LIST		"GP01"
#define DF_STORE_MAGIC_FILE_LIST_HEAD	"FILELIST01"
#define DF_STORE_MAGIC_FILE_LIST		"LIST0001"
#define DF_STORE_MAGIC_NVR_LIST_HEAD	"NVR001"
#define DF_STORE_MAGIC_NVR_LIST			"nvr001"



#define DF_STORE_FIFO_NAME				"/tmp/cmd_pipe"
#define DF_STORE_HDISK_AV				"/disk1"	///录像信息所在的文件目录
#define DF_STORE_FOLDER_LIST_NAME		"folder_list"
#define DF_STORE_FILE_LIST_NAME			"file_list"
#define DF_STORE_HDISK_DIR_NAME			"dev/sda4"	///录像信息所在的文件目录


/*********************************************************************************
  *枚举类型:en_store_style
  *功能描述:存储的数据类型
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-23	创建
*********************************************************************************/
typedef enum _en_store_style
{
	STORE_STYLE_VIDEO			=0,			///存储的是视频
	STORE_STYLE_AUDIO,						///存储的是音频
	STORE_STYLE_VIDEO_AUDIO,				///存储的是音视频混合数据
}en_store_style;


/*********************************************************************************
  *枚举类型:en_trige_style
  *功能描述:存储数据的触发方式
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-23	创建
*********************************************************************************/
typedef enum _en_trige_style
{
	STORE_TRIGE_NORMA			=0,		///正常触发，默认值
	STORE_TRIGE_ALRAM,					///报警触发
	STORE_TRIGE_EVENT,					///外部事件触发
	STORE_TRIGE_TIMER,					///定时器触发
	STORE_TRIGE_HUMAN,					///手动触发
}en_trige_style;

/*********************************************************************************
  *枚举类型:en_hdisk_state
  *功能描述:硬盘状态
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-13	创建
*********************************************************************************/
typedef enum _en_hdisk_state
{
	STORE_HDISK_NONE			=0,		///没有检测到硬盘
	STORE_HDISK_FORMATING,				///硬盘正在格式化
	STORE_HDISK_FORMATED,				///硬盘格式化完成
	STORE_HDISK_MOUNTED,				///硬盘挂载文件系统OK
	STORE_HDISK_ERROR,					///硬盘错误
}en_hdisk_state;


/*********************************************************************************
  *结 构 体:st_store_video
  *功能描述:存储的视频参数
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-23	创建
*********************************************************************************/
typedef struct _st_store_video_param
{
	unsigned char				channel;			///通道号
	PIC_SIZE_E					e_pic_size;			///视频分辨率大小
	VIDEO_NORM_E				e_video_norm;		///视频制式，PAL or NTSC
	PAYLOAD_TYPE_E				e_payload_type;		///要加载的文件格式
	en_trige_style				e_trige_type;		///触发方式
}st_store_video_param;


/*********************************************************************************
  *结 构 体:st_store_video
  *功能描述:存储的视频参数
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-23	创建
*********************************************************************************/
typedef struct _st_store_audio_param
{
	unsigned char				channel;			///通道号
	PAYLOAD_TYPE_E				e_payload_type;		///要加载的文件格式
	en_trige_style				e_trige_type;		///触发方式
}st_store_audio_param;

#pragma pack(1)

/*********************************************************************************
  *结 构 体:st_store_video
  *功能描述:存储的视频buf信息
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-23	创建
*********************************************************************************/
typedef struct _st_store_av_data
{
	unsigned char 				av_buf[DF_STORE_AV_PACK_SIZE_X2];	///定义的进行音视频数据存取的buf
	//unsigned char 				a_buf[DF_STORE_AUDIO_FRAME_MAX][DF_STORE_AUDIO_FRAME_SIZE];		///定义的进行音频数据存取的buf,每次存入168个字节的数据
	unsigned char 				v_buf[512];			///定义的进行视频数其他类型帧(参数帧，非ipb帧)存储的buf
	unsigned long				av_buf_wr;			///往buf里面写入的位置
	unsigned long				av_buf_rd;			///从buf里面读取的位置
	unsigned long				av_len;				///buf里面的有效数据
	//unsigned long				a_buf_wr;			///往buf里面写入的位置，二维数组位置，最大为8
	//unsigned long				a_buf_rd;			///从buf里面读取的位置，二维数组位置，最大为8
	//unsigned long				a_len;				///buf里面的有效数据，二维数组长度，最大为8
	unsigned long				v_buf_wr;			///往buf里面写入的位置
	unsigned long				v_buf_rd;			///从buf里面读取的位置
	unsigned long				v_len;				///buf里面的有效数据
	unsigned char				iswrite;			///正在进行写操作
	utc_time					time[2];			///要存储的数据生成的时间,分别表示前后两个64k区域的数据生成时间
	unsigned char				frame_style[2];		///表示要存储的帧类型，BIT(1)表示该64k数据中有I帧，BIT(7)表示该区域存储已满
}st_store_av_data;
 
 
 /*********************************************************************************
   *结 构 体:st_store_video
   *功能描述:存储的音频buf信息
   *---------------------------------------------------------------------------------
   * @修改人	 修改时间	 修改内容
   * 白养民 	 2015-06-23  创建
 *********************************************************************************/
 typedef struct _st_store_a_data
 {
	 unsigned char				 a_buf[DF_STORE_AUDIO_FRAME_MAX][DF_STORE_AUDIO_FRAME_SIZE];	 ///定义的进行音频数据存取的buf,每次存入168个字节的数据
	 unsigned long				 a_buf_wr;			 ///往buf里面写入的位置，二维数组位置，最大为8
	 unsigned long				 a_buf_rd;			 ///从buf里面读取的位置，二维数组位置，最大为8
	 unsigned long				 a_len; 			 ///buf里面的有效数据，二维数组长度，最大为8
}st_store_a_data;



/*********************************************************************************
  *结 构 体:st_store_video
  *功能描述:存储的视频数据及参数信息
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-23	创建
*********************************************************************************/
typedef struct _st_store_av_manage
{
	en_store_style				style;				///要存储的数据类型
	st_store_video_param		param_video;		///存储的视频参数
	st_store_audio_param		param_audio;		///音频参数
	utc_time					time;				///要存储的数据生成的时间
	unsigned long				tick;				///要存储的数据生成的tick值，单位为10ms
	unsigned long				tick_validity;		///要存储的数据的有效时间长度，单位为10ms，
	st_store_av_data			data_av;			///要存储的录像数据
	st_store_a_data				data_a;				///音频数据暂存buf
}st_store_av_manage;


/*********************************************************************************
  *结 构 体:st_store_folder_list
  *功能描述:存储录像数据的文件夹列表信息,固定长度为16字节
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-13	创建
*********************************************************************************/
typedef struct _st_store_folder_list
{
	s8						head[4];			///8字节（固定 "GP01"）
	u32						start_index;		///开始写入裸盘中的位置
	u16						folder_ID;			///文件夹ID
	u8						time_bcd[6];		///文件夹日期
}st_store_folder_list;



/*********************************************************************************
  *结 构 体:st_store_file_list_head
  *功能描述:存储录像数据的文件列表信息头,固定长度为64字节
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-13	创建
*********************************************************************************/
typedef struct _st_store_file_list_head
{
	s8						head[10];			///8字节（固定 "FILELIST01"）
	u8						time_start_bcd[6];	///文件开始时间	6字节BCD码（年月日时分秒）
	u8						time_end_bcd[6];	///文件结束时间	6字节BCD码（年月日时分秒）
	u16						file_num;			///总文件数	2字节（先低后高）
	u32						filelist_size;		///文件总大小	4字节（先低后高），单位为64K
	u32						write_index_start;	///当天数据写入的开始index位置
	u32						write_index_end;	///当天数据写入的末尾index位置
}st_store_file_list_head;

/*********************************************************************************
  *结 构 体:st_store_file_list
  *功能描述:存储录像数据的文件列表信息,固定长度为128字节
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-13	创建
*********************************************************************************/
typedef struct _st_store_file_list
{
	s8						head[8];			///8字节（固定 "LIST0001"）
	s8						file_name[64];		///文件名称	96字节，不足96字节后面补齐0
	u16						file_len;			///文件长度 2字节（先低后高），单位为64K
	u8						time_start_bcd[6];	///文件开始时间	6字节BCD码（年月日时分秒）
	u8						time_end_bcd[6];	///文件结束时间	6字节BCD码（年月日时分秒）
	u32						write_index_start;	///当天数据写入的开始index位置
	u32						write_index_end;	///当天数据写入的末尾index位置
	u8						channel;			///录像通道	1字节（0表示为1号通道）
	u8						t_data[43];			///空33个字节多几个字节是为了不溢出
}st_store_file_list;


/*********************************************************************************
  *结 构 体:st_store_channel_param
  *功能描述:银盘存储中单个通道的参数
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-13	创建
*********************************************************************************/
typedef struct _st_store_channel_param
{
	en_store_style				style;				///要存储的数据类型
	st_store_video_param		param_video;		///存储的视频参数
	st_store_audio_param		param_audio;		///音频参数
	char						write_file[128];	///正在写入的文件的名称
	FILE *						write_file_fd;		///正在写入的文件的句柄
	unsigned long				file_len;			///该文件对应的数据的总长度，单位为64K
	utc_time					time_start;			///写入开始时间
	utc_time					time_end;			///写入结束时间
	u32							write_index_start;	///天数据写入的开始index位置
	u32							write_index_end;	///天数据写入的末尾index位置
	unsigned char				write_frame_style;	///正在存储处理的帧类型，BIT(1)表示该64k数据中有I帧，BIT(7)表示该区域存储已满
	utc_time					write_time;				///正在存储处理的数据时间
}st_store_channel_param;


/*********************************************************************************
  *结 构 体:st_store_hdisk
  *功能描述:硬盘存储相关参数
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-13	创建
*********************************************************************************/
typedef struct _st_store_hdisk
{
	en_hdisk_state				disk_state;				///硬盘当前的状态
	unsigned long				disk_size_av;			///硬盘可以存储的录像盘的大小，单位为65536字节，及64K
	unsigned long				write_index;			///硬盘当前可以写入的位置，单位为64K
	unsigned long				read_index;				///硬盘当前可以写入的位置，单位为64K
	unsigned char				write_limit_mode;		///硬盘存储数据限制的模式0表示按照时间长度限制存储，1表示按照大小限制存储
	unsigned long				write_size_max;			///硬盘存储录像单个文件的最大值，单位为64K
	unsigned long				write_time_max;			///硬盘存储录像单个文件的时间最长值，单位为秒s
	int 						write_hdisk_fd;			///裸盘写入的句柄
	FILE * 						write_folder_list_fd;	///文件信息文件夹列表对应的文件句柄
	FILE *						write_file_list_fd;		///正在写入的文件列表的句柄
	utc_time					write_folder_day;		///当前正在写入的文件夹对应的时间，该时间小时，分钟，秒均为0
	char						write_folder[32];		///当前正在写入的文件夹位置
	st_store_file_list_head		file_list_head;			///当天存储的数据的总信息
	st_store_channel_param		channel_param[DF_STORE_CHANNEL_MAX];	///存储的每个通道的参数
}st_store_hdisk;
#pragma pack()



extern int					gt_store_fifo_fd;
extern st_store_hdisk		gt_store_hdisk;			///硬盘状态信息和相关硬盘参数


int store_fifo_write( char *p);
int store_init(void);
int store_end(void);
int store_save_audio( st_store_audio_param *pt_audio_param, u8 * p_data,u32 len );
int store_save_video( st_store_video_param *tp_video_param, u8 * p_data,u32 len );


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef _H_GPS_STORE_IO_ */

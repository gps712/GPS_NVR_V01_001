/*********************************************************************************
  * @文件名称 :gps_store.c
  * @功能描述 :所有和存储管理及和应用程序相关的函数都在这里
  * @作	   者 :白养民
  * @创建日期 :2015-6-19
  * @文件版本 :0.01
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-19	创建
*********************************************************************************/
#ifdef __cplusplus
#if __cplusplus
	extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
//#include "linux/time.h"
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "gps_typedef.h"
#include "common_func.h"
#include "gps_disk_io.h"
#include "gps_param.h"
#include "gps_store.h"
#include "gps_hi_rtc.h"


#define DF_STORE_AUDIO_DOUBLE		3			///当buf中的音频包数大于该值时，需要两包发送


st_store_av_manage gt_store_av_manage[DF_STORE_CHANNEL_MAX];			///定义的进行音视频数据存取管理的结构体
static pthread_t gt_store_thread_pid;					///录像数据存储线程PID
int		gt_store_fifo_fd = 0;							///录像数据存储有名管道的句柄
static st_gps_thread_param	gt_store_thread_para;		///调试接口线程参数
st_store_hdisk		gt_store_hdisk;						///硬盘状态信息和相关硬盘参数
static utc_time		sgt_utc_now;
static en_hdisk_state		sgt_disk_status = STORE_HDISK_NONE;
static utc_time				sgt_disk_size	= 6250000;


/*********************************************************************************
  *函数名称:static int store_create_file_list_head( char *p_buf )
  *功能描述:生成文件列表头信息
  *输	入: p_buf	:要存储的buf
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-22	创建
*********************************************************************************/
static int store_create_file_list_head( char *p_buf )
{
	int len = 0;
	memset(p_buf, 0 ,DF_STORE_FILE_LIST_HEAD_LEN);
	strcpy(gt_store_hdisk.file_list_head.head,DF_STORE_MAGIC_FILE_LIST_HEAD);
	memcpy(p_buf, (u8 *)&gt_store_hdisk.file_list_head ,sizeof(st_store_file_list_head));
	return 0;
}



/*********************************************************************************
  *函数名称:static int store_create_file_list( st_store_channel_param * pt_store_channel,char *p_buf )
  *功能描述:生成文件列表信息
  *输	入: p_buf	:要存储的buf
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-22	创建
*********************************************************************************/
static int store_create_file_list( st_store_channel_param * pt_store_channel,char *p_buf )
{
	int len = 0;
	st_store_file_list * pt_file_list;
	pt_file_list =(st_store_file_list * )p_buf;

	memset(pt_file_list,0,DF_STORE_FILE_LIST_LEN);
	strcpy(pt_file_list->head,DF_STORE_MAGIC_FILE_LIST);
	strcpy(pt_file_list->file_name, pt_store_channel->write_file );
	pt_file_list->file_len = pt_store_channel->file_len;
	mytime_to_bcd(pt_file_list->time_start_bcd,utc_to_mytime( pt_store_channel->time_start));
	mytime_to_bcd(pt_file_list->time_end_bcd,utc_to_mytime( pt_store_channel->time_end));
	pt_file_list->.write_index_start = pt_store_channel->write_index_start;
	pt_file_list->write_index_end	= pt_store_channel->write_index_end;
	pt_file_list->channel 			= pt_store_channel->param_video.channel;
	
	return 0;
}





/*********************************************************************************
  *函数名称:static int store_create_nvr_list_head( u8 channel,char *p_buf )
  *功能描述:生成nvr列表头信息
  *输	入: channel	:对应的录像通道，0表示通道1
  			p_buf	:要存储的buf
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-22	创建
*********************************************************************************/
static int store_create_nvr_list_head( u8 channel,char *p_buf )
{
	int len = 0;

	memset(p_buf,0,DF_STORE_NVR_LIST_HEAD_LEN);

	///数据头固定为"NVR001"
	strcpy(p_buf,DF_STORE_MAGIC_NVR_LIST_HEAD);
	len += 6;
	///文件开始时间	6字节BCD码（年月日时分秒）
	mytime_to_bcd(p_buf+len,utc_to_mytime( gt_store_hdisk.channel_param[channel].time_start ));
	len += 6;
	///文件结束时间	6字节BCD码（年月日时分秒）
	mytime_to_bcd(p_buf+len,utc_to_mytime( gt_store_hdisk.channel_param[channel].time_end ));
	len += 6;
	///文件总大小	4字节（先低后高），单位为64K
	memcpy(p_buf+len,(u8 *)&(gt_store_hdisk.channel_param[channel].file_len),4);
	len += 4;
	///文件信息对应写入裸盘开始位置	4字节
	memcpy(p_buf+len,(u8 *)&(gt_store_hdisk.channel_param[channel].write_index_start),4);
	len += 4;
	///文件信息对应写入裸盘结束位置	4字节
	memcpy(p_buf+len,(u8 *)&(gt_store_hdisk.channel_param[channel].write_index_end),4);
	len += 4;
	///录像通道	1字节（0表示为1号通道）
	p_buf[len] = channel;
	len += 1;
	///空33字节
	len += 33;
	return 0;
}



/*********************************************************************************
  *函数名称:static int store_create_nvr_list( u8 channel,char *p_buf )
  *功能描述:生成nvr列表信息
  *输	入: channel	:对应的录像通道，0表示通道1
  			p_buf	:要存储的buf
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-22	创建
*********************************************************************************/
static int store_create_nvr_list( u8 channel,char *p_buf )
{
	int len = 0;

	memset(p_buf,0,DF_STORE_NVR_LIST_LEN);

	///数据头固定为"nvr001"
	strcpy(p_buf,DF_STORE_MAGIC_NVR_LIST);
	len += 6;
	///数据的时间
	mytime_to_bcd(p_buf+len,utc_to_mytime( gt_store_hdisk.channel_param[channel].write_time));
	len += 6;
	///写入裸盘的位置
	memcpy(p_buf+len,(u8 *)&(gt_store_hdisk.channel_param[channel].write_index_end),4);
	len += 4;
	///录像通道	1字节（0表示为1号通道）
	p_buf[len] = channel;
	len += 1;
	///是否有I帧 1字节（0表示没有，1表示有）
	if(gt_store_hdisk.channel_param[channel].write_frame_style & (BIT(1)) )
	{
		p_buf[len] = 1;
	}
	len += 1;
	///空46字节
	len += 46;
	return 0;
}

/*********************************************************************************
  *函数名称:static int store_create_nvr_name(st_store_channel_param * pt_store_channel,char *p_buf )
  *功能描述:生成要存储的信息文件的的文件名称
  *输	入: pt_store_channel	:
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-22	创建
*********************************************************************************/
static int store_create_nvr_name(st_store_channel_param * pt_store_channel,char *p_buf )
{
	u8 i;
	MYTIME	my_time1,my_time2;				///
	
	my_time1 = utc_to_mytime(pt_store_channel->time_start );
	my_time2 = utc_to_mytime(pt_store_channel->time_end );
	sprintf(p_buf,"%s\0000-%02d%02d%02d-%02d%02d%02d-%02d%02d%02d-%d-%d.nvr",
		gt_store_hdisk.write_folder,
		YEAR(my_time1),MONTH(my_time1),DAY(my_time1),
		HOUR(my_time1),MINUTE(my_time1),SEC(my_time1),
		HOUR(my_time2),MINUTE(my_time2),SEC(my_time2),
		pt_store_channel->param_video.e_trige_type,
		pt_store_channel->param_video.channel									
		);
	return 0;
}


/*********************************************************************************
  *函数名称:static int store_create_foldernvr_name(char *p_buf, utc_time time1 )
  *功能描述:生成要存储的信息文件夹的名称
  *输	入: pt_store_channel	:
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-22	创建
*********************************************************************************/
static int store_create_foldernvr_name(char *p_buf, utc_time time1 )
{
	u8 i;
	MYTIME	my_time1,my_time2;				///
	char c_buf[16];
	
	///生成文件夹名称
	utc_to_bcd(c_buf, time1);
	sprintf(p_buf,"%s/%04d-%02d-%02d",DF_STORE_HDISK_AV,c_buf[0]+2000,c_buf[1],c_buf[2]);
	return 0;
}


/*********************************************************************************
  *函数名称:static int store_remove_oldest_folder(void)
  *功能描述:录像存储满了，需要删除最老的数据和相关信息时调用该函数
  *输	入: none
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
static int store_remove_oldest_folder(void)
{
	int	i,j,k;
	int max_id,min_id;
	FILE *	floder_fd,floder_fd_temp;
	s8	 	c_buf[128];
	s8	 	c_buf_name1[64];
	s8	 	c_buf_name2[64];
	u8		uc_file_buf[DF_STORE_FOLDER_LIST_LEN*128];		///该buf长度必须为 DF_STORE_FOLDER_LIST_LEN 的整数倍
	int	i_init_folder;
	int	oldest_folder_is_find = 0;
	int filesize,len,read_len;
	st_store_folder_list t_folder_list;
	st_store_folder_list* pt_folder_list;

	if(sgt_disk_status != STORE_HDISK_MOUNTED )
		return -1;

	min_id = 65535;
	max_id = 0;

	///创建文件，如果文件不存在就创建，存在就打开
	sprintf(c_buf_name1,"%s/%s",DF_STORE_HDISK_AV,DF_STORE_FOLDER_LIST_NAME);
	floder_fd = fopen(c_buf_name1, "a+");
	if( floder_fd == 0 )
	{
		gt_store_hdisk.disk_state = STORE_HDISK_ERROR;
		SAMPLE_PRT("open file err\n");
		return -1;
	}
	
	///创建临时文件
	sprintf(c_buf_name2,"%s/%s_temp",DF_STORE_HDISK_AV,DF_STORE_FOLDER_LIST_NAME);
	floder_fd_temp= fopen(c_buf_name2, "w+");
	if( floder_fd_temp == 0 )
	{
		gt_store_hdisk.disk_state = STORE_HDISK_ERROR;
		SAMPLE_PRT("open file err\n");
		return -1;
	}
	
	i_init_folder = 1;
	///查看文件长度
	fseek(floder_fd, 0, SEEK_END);
	filesize = ftell(floder_fd);
	if( filesize >= DF_STORE_FOLDER_LIST_LEN )
	{
		/////////////////////////////////////////////////////////////////////////////
		fseek(floder_fd, 0, SEEK_SET);
		len = filesize;
		read_len= 0;

		///从folderlist中找到数据的开始位置和结束位置，并找到最后的日期文件夹
		for( i = 0; i < filesize/DF_STORE_FOLDER_LIST_LEN; i++)
		{
			if(read_len == 0)
			{
				if( len > sizeof(uc_file_buf) )
				{
					read_len = sizeof(uc_file_buf);
				}
				else
				{
					read_len = len;
				}
				len -= read_len;
				fread(uc_file_buf,read_len,1,floder_fd);

				///查找最早的文件夹，并将它删除
				j = 0;
				if(oldest_folder_is_find < 2)
				{
					for(j=0; j<read_len; j+=DF_STORE_FOLDER_LIST_LEN )
					{
						pt_folder_list = (st_store_folder_list*)&uc_file_buf[j];
						if( memcmp( pt_folder_list->head, DF_STORE_MAGIC_FOLDER_LIST, sizeof(DF_STORE_MAGIC_FOLDER_LIST) ) == 0 )
						{
							++oldest_folder_is_find;
							///找到的第一个文件夹删除掉
							if(oldest_folder_is_find == 1)
							{
								store_create_foldernvr_name(c_buf,utc_from_bcd(pt_folder_list->time_bcd));
								disk_remove_dir(c_buf);
							}
							///第二个文件夹的索引为最后的索引
							else if(oldest_folder_is_find == 2)
							{
								gt_store_hdisk.read_index = pt_folder_list->start_index;
								break;
							}
						}
					}
				}
				if(oldest_folder_is_find >= 2)
				{
					fwrite(uc_file_buf+j,read_len-j,1,floder_fd_temp);
				}
			}
		}
		/////////////////////////////////////////////////////////////////////////////
	}
	if( oldest_folder_is_find < 2 )
	{
		gt_store_hdisk.read_index = DF_STORE_WRITE_INDEX_START;
		gt_store_hdisk.write_index= DF_STORE_WRITE_INDEX_START;
	}
	fclose(floder_fd);
	fclose(floder_fd_temp);
	remove(c_buf_name1);
	///重命名文件名称
	rename( c_buf_name2, c_buf_name1 );
	return 0;
}


/*********************************************************************************
  *函数名称:static int store_close_file_list(void)
  *功能描述:将file_list文件信息进行保存和关闭，同时更新文件列表文件
  *输	入: close_channel	，需要保存测通道编号，bit0表示通道0，bit1表示通道1，........
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
static int store_close_file_list(void)
{
	u8	 uc_buf[128];
	
	if(gt_store_hdisk.write_file_list_fd)
	{
		///生产nvr文件list头		
		store_create_file_list_head((char *)uc_buf);
		fseek(gt_store_hdisk.write_file_list_fd,0,SEEK_SET);
		fwrite(uc_buf,DF_STORE_FILE_LIST_HEAD_LEN,1,gt_store_hdisk.write_file_list_fd);
		fclose(gt_store_hdisk.write_file_list_fd);
		gt_store_hdisk.write_file_list_fd = 0;
		return 1;
	}
	return 0;
}



/*********************************************************************************
  *函数名称:static int store_close_channel_nvr(u16 close_channel)
  *功能描述:将指定通道的录像文件信息进行保存和关闭，同时更新文件列表文件
  *输	入: close_channel	，需要保存测通道编号，bit0表示通道0，bit1表示通道1，........
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
static int store_close_channel_nvr(u16 close_channel)
{
	u8	i;
	s8	c_buf[128];
	u8	uc_buf[128];
	u32	ul_len,ul_temp1;
	
	for(i=0; i<DF_STORE_CHANNEL_MAX; i++)
	{
		if( close_channel & (BIT( i )) )
		{
			if( gt_store_hdisk.channel_param[i].file_len )
			{
				if(( gt_store_hdisk.channel_param[i].write_file_fd ) && (gt_store_hdisk.write_file_list_fd))
				{
					///生产nvr文件list头
					store_create_nvr_list_head(i,uc_buf);

					///写入头信息
					fseek(gt_store_hdisk.channel_param[i].write_file_fd,0,SEEK_SET);
					fwrite(uc_buf,DF_STORE_NVR_LIST_HEAD_LEN,1,gt_store_hdisk.channel_param[i].write_file_fd);
					///关闭文件，并清空句柄
					fclose( gt_store_hdisk.channel_param[i].write_file_fd );
					gt_store_hdisk.channel_param[i].write_file_fd = 0;
					
					///新文件名称生成
					store_create_nvr_name(&gt_store_hdisk.channel_param[i],c_buf);
					///重命名文件名称
					rename( gt_store_hdisk.channel_param[i].write_file, c_buf );
					strcpy( gt_store_hdisk.channel_param[i].write_file, c_buf );

					///填充文件列表内容
					store_create_file_list(&gt_store_hdisk.channel_param[i],uc_buf);
					
					///将文件信息写入列表文件中
					//ul_temp1 = gt_store_hdisk.file_list_head.file_num;
					//ul_temp1 = DF_STORE_FILE_LIST_LEN * ul_temp1 + DF_STORE_FILE_LIST_HEAD_LEN;
					//disk_write(gt_store_hdisk.write_file_list_fd,ul_temp1,uc_buf,DF_STORE_FILE_LIST_LEN);
					fwrite(uc_buf,DF_STORE_FILE_LIST_LEN,1,gt_store_hdisk.write_file_list_fd);
					///改变文件列表头信息的内容
					gt_store_hdisk.file_list_head.file_num++;
					gt_store_hdisk.file_list_head.filelist_size += gt_store_hdisk.channel_param[i].file_len;
					if(utc_from_bcd(gt_store_hdisk.file_list_head.time_end_bcd) < gt_store_hdisk.channel_param[i].time_end)
						utc_to_bcd(gt_store_hdisk.file_list_head.time_end_bcd ,gt_store_hdisk.channel_param[i].time_end);
					if(utc_from_bcd(gt_store_hdisk.file_list_head.time_start_bcd) == 0)
						utc_to_bcd(gt_store_hdisk.file_list_head.time_start_bcd ,gt_store_hdisk.channel_param[i].time_start);
				}
			}
		}
	}
	return 0;
}


/*********************************************************************************
  *函数名称:static FILE * store_search_file_list_head( char * list_name,st_store_file_list_head * p_list_head )
  *功能描述:查找指定的路径文件下的file_list数据头信息，并填充p_list_head中的内容
  *输	入: list_name	，file_list文件路径名称
  			p_list_head	，要创建测数据包头指针
  *输	出: none
  *返 回 值:0，表示失败，非0表示成功创建了file_list文件
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-24	创建
*********************************************************************************/
static FILE * store_search_file_list_head( char * list_name,st_store_file_list_head * p_list_head )
{
	u8 		i;
	u32 	ul_len,ul_temp1;
	u32		filesize;
	int		i_init_file_list;
	st_store_av_manage *p_av_manage;
	st_store_file_list	ft_file_list;
	FILE *	file_list_fd = 0;
	
	file_list_fd = fopen(list_name, "a+");
	if( file_list_fd == 0 )
	{
		SAMPLE_PRT("open file err\n");
		return 0;
	}
	memset(p_list_head,0,sizeof(st_store_file_list_head));
	
	i_init_file_list = 1;
	///查看文件长度，超过filelist头部分则读取头
	fseek( file_list_fd, 0, SEEK_END );  
	filesize = ftell( file_list_fd );
	if( filesize > DF_STORE_FILE_LIST_HEAD_LEN )
	{
		fseek( file_list_fd, 0, SEEK_SET );
		fread( (u8 *)p_list_head, DF_STORE_FILE_LIST_HEAD_LEN, 1, file_list_fd );
		///如果数据包头符合要求，则不需要初始化该文件，后面的操作直接续存即可
		if(memcmp(p_list_head->head,DF_STORE_MAGIC_FILE_LIST_HEAD,sizeof(DF_STORE_MAGIC_FILE_LIST_HEAD)) == 0)
		{
			if( p_list_head->file_num != ( filesize - DF_STORE_FILE_LIST_HEAD_LEN ) / DF_STORE_FILE_LIST_LEN )
			{
				if( (filesize-DF_STORE_FILE_LIST_HEAD_LEN)%DF_STORE_FILE_LIST_LEN == 0 )
				{
					fseek(file_list_fd, filesize-DF_STORE_FILE_LIST_LEN, SEEK_SET);
					fread( (u8 *)&ft_file_list, DF_STORE_FILE_LIST_LEN, 1, file_list_fd );
					if(memcmp(ft_file_list.head,DF_STORE_MAGIC_FILE_LIST,sizeof(DF_STORE_MAGIC_FILE_LIST)) == 0)
					{
						p_list_head->write_index_end 	= ft_file_list.write_index_end;
						p_list_head->filelist_size 		= ( p_list_head->write_index_end + gt_store_hdisk.disk_size_av - DF_STORE_WRITE_INDEX_START- p_list_head->write_index_start + 1)%(gt_store_hdisk.disk_size_av - DF_STORE_WRITE_INDEX_START);
						p_list_head->file_num			= ( filesize - DF_STORE_FILE_LIST_HEAD_LEN ) / DF_STORE_FILE_LIST_LEN;
						memcpy(p_list_head->time_end_bcd,ft_file_list.time_end_bcd,6);
					}
				}
			}
			i_init_file_list = 0;
		}
	}
	fseek(file_list_fd, 0, SEEK_END); 
	strcpy(p_list_head->head,DF_STORE_MAGIC_FILE_LIST_HEAD);
	return file_list_fd;
}


/*********************************************************************************
  *函数名称:static int store_save_channel_nvr(u16 save_channel)
  *功能描述:将指定通道的录像文件进行保存
  *输	入: save_channel	，需要保存测通道编号，bit0表示通道0，bit1表示通道1，........
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
static int store_save_channel_nvr(u16 save_channel)
{
	u8 		i;
	u8	 	uc_buf[128];
	s8	 	c_buf[128];
	u32 	ul_len,ul_temp1;
	u32		filesize;
	int		i_init_file_list;
	st_store_av_manage *p_av_manage;

	///当前文件夹已经创建，否则返回错误
	if( strlen(gt_store_hdisk.write_folder) == 0 )
	{
		SAMPLE_PRT("folder err\n");
		return -1;
	}
	for(i=0; i<DF_STORE_CHANNEL_MAX; i++)
	{
		if( save_channel & (BIT( i )) )
		{
			p_av_manage = &gt_store_av_manage[i];
			///如果写入nvr数据的文件不存在，则需要创建该文件
			if( gt_store_hdisk.channel_param[i].write_file_fd == 0 )
			{
				///填充数据
				memset((u8 *)&gt_store_hdisk.channel_param[i],0,sizeof(st_store_channel_param));
				memcpy((u8 *)&gt_store_hdisk.channel_param[i].style, (u8 *)&p_av_manage->style,sizeof(en_store_style)+sizeof(st_store_video_param)+sizeof(st_store_audio_param));
				gt_store_hdisk.channel_param[i].time_start 	= gt_store_hdisk.channel_param[i].write_time;
				gt_store_hdisk.channel_param[i].time_end	= gt_store_hdisk.channel_param[i].write_time;
				gt_store_hdisk.channel_param[i].write_index_start 	= gt_store_hdisk.write_index;
				gt_store_hdisk.channel_param[i].write_index_end		= gt_store_hdisk.write_index;
				///生成文件名称
				store_create_nvr_name( &gt_store_hdisk.channel_param[i], gt_store_hdisk.channel_param[i].write_file );

				///创建文件
				gt_store_hdisk.channel_param[i].write_file_fd = fopen(gt_store_hdisk.channel_param[i].write_file, "w+");
				if( gt_store_hdisk.channel_param[i].write_file_fd == 0 )
				{
					gt_store_hdisk.disk_state = STORE_HDISK_ERROR;
					SAMPLE_PRT("open file err\n");
					return -1;
				}
				///写入nvr文件列表头信息
				store_create_nvr_list_head(i,uc_buf);
				fwrite(uc_buf,DF_STORE_NVR_LIST_HEAD_LEN,1,gt_store_hdisk.channel_param[i].write_file_fd);
			}
			///如果写入filelist的文件不存在，需要创建并初始化该文件
			if( gt_store_hdisk.write_file_list_fd == 0 )
			{
				///创建文件，如果文件不存在就创建，存在就打开
				sprintf(c_buf,"%s/%s",gt_store_hdisk.write_folder,DF_STORE_FILE_LIST_NAME);
				gt_store_hdisk.write_file_list_fd = store_search_file_list_head(c_buf,&gt_store_hdisk.file_list_head);
				if(gt_store_hdisk.write_file_list_fd == 0)
				{
					gt_store_hdisk.disk_state = STORE_HDISK_ERROR;
					SAMPLE_PRT("open file err\n");
					return -1;
				}
				///填充写入的列表头数据
				if(gt_store_hdisk.file_list_head.write_index_start == 0)
				{
					utc_to_bcd(gt_store_hdisk.file_list_head.time_start_bcd, gt_store_hdisk.channel_param[i].time_start);
					utc_to_bcd(gt_store_hdisk.file_list_head.time_end_bcd, gt_store_hdisk.channel_param[i].time_end);
					gt_store_hdisk.file_list_head.write_index_start = gt_store_hdisk.channel_param[i].write_index_start;
					gt_store_hdisk.file_list_head.write_index_end	= gt_store_hdisk.channel_param[i].write_index_end;
					store_create_file_list_head((char *)uc_buf);
					fwrite(uc_buf,DF_STORE_FILE_LIST_HEAD_LEN,1,gt_store_hdisk.write_file_list_fd);
				}
			}
			///填充要写入的nvr列表信息
			gt_store_hdisk.channel_param[i].file_len++;
			gt_store_hdisk.channel_param[i].time_end			= gt_store_hdisk.channel_param[i].write_time;
			gt_store_hdisk.channel_param[i].write_index_end		= gt_store_hdisk.write_index;
			store_create_nvr_list(i,uc_buf);
			fwrite(uc_buf,DF_STORE_NVR_LIST_LEN,1,gt_store_hdisk.channel_param[i].write_file_fd);
		}
	}
	return 0;
}



/*********************************************************************************
  *函数名称:int store_av_init(void)
  *功能描述:初始化硬盘存储相关参数
  *输	入: none
  *输	出: none
  *返 回 值:	0:OK  非0:错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
int store_nvr_init(void)
{
	int	i,j,k,i_index;
	int max_id,min_id;
	FILE *	floder_fd;
	s8	 	c_buf[128];
	u8		uc_file_buf[DF_STORE_FOLDER_LIST_LEN*128];		///该buf长度必须为 DF_STORE_FOLDER_LIST_LEN 的整数倍
	int	i_init_folder;
	int filesize,len,read_len;
	en_hdisk_state		disk_state;
	st_store_folder_list t_folder_list;
	st_store_folder_list* pt_folder_list;
	
	if(gt_store_hdisk.disk_state != STORE_HDISK_MOUNTED )
		return -1;
	disk_state = gt_store_hdisk.disk_state;
	memset(gt_store_hdisk, 0, sizeof(gt_store_hdisk));
	gt_store_hdisk.disk_state = disk_state;
	gt_store_hdisk.disk_size_av = sgt_disk_size;

	min_id = 65535;
	max_id = 0;

	///创建文件，如果文件不存在就创建，存在就打开
	sprintf(c_buf,"%s/%s",DF_STORE_HDISK_AV,DF_STORE_FOLDER_LIST_NAME);
	floder_fd = fopen(c_buf, "a+");
	if( floder_fd == 0 )
	{
		gt_store_hdisk.disk_state = STORE_HDISK_ERROR;
		SAMPLE_PRT("open file err\n");
		return -1;
	}
	
	i_init_folder = 1;
	///查看文件长度
	fseek(floder_fd, 0, SEEK_END);
	filesize = ftell(floder_fd);
	if( filesize >= DF_STORE_FOLDER_LIST_LEN )
	{
		i_init_folder = 0;
		/*
		///从最早的folderlist中找到数据的开始位置
		fseek(floder_fd, 0, SEEK_SET);
		fread((u8 *)&t_folder_list,DF_STORE_FOLDER_LIST_LEN,1,floder_fd);
		if(memcmp(t_folder_list.head,"GP01",4) == 0)
		{
			if(t_folder_list.start_index == DF_STORE_WRITE_INDEX_START)
				gt_store_hdisk.read_index = gt_store_hdisk.disk_size_av;
			else
				gt_store_hdisk.read_index--;
		}
		///从最后的folderlist中找到数据的结束位置，和最后的日期文件夹
		fseek(floder_fd, filesize - DF_STORE_FOLDER_LIST_LEN, SEEK_SET);
		fread((u8 *)&t_folder_list,DF_STORE_FOLDER_LIST_LEN,1,floder_fd);
		if(memcmp(t_folder_list.head,"GP01",4) == 0)
		{
			gt_store_hdisk.write_index = t_folder_list.start_index;
			gt_store_hdisk.write_folder_day = utc_from_bcd(t_folder_list.time_bcd);
		}
		*/
		/////start 这段代码和上面屏蔽的相同功能，只不过多了便利，这样能检测到文件出错
		fseek(floder_fd, 0, SEEK_SET);
		
		len = filesize;
		read_len= 0;

		///从folderlist中找到数据的开始位置和结束位置，并找到最后的日期文件夹
		for( i = 0; i < filesize/DF_STORE_FOLDER_LIST_LEN; i++)
		{
			if(read_len == 0)
			{
				if( len > sizeof(uc_file_buf) )
				{
					read_len = sizeof(uc_file_buf);
				}
				else
				{
					read_len = len;
				}
				len -= read_len;
				fread(uc_file_buf,read_len,1,floder_fd);
				i_index = 0;
			}
			if( read_len )
			{
				pt_folder_list = (st_store_folder_list*)&uc_file_buf[i_index*DF_STORE_FOLDER_LIST_LEN];
				if( memcmp( pt_folder_list->head, DF_STORE_MAGIC_FOLDER_LIST, sizeof(DF_STORE_MAGIC_FOLDER_LIST) ) == 0 )
				{
					if(pt_folder_list->folder_ID < min_id)
						min_id = pt_folder_list->folder_ID;
					if(pt_folder_list->folder_ID > max_id)
					{
						max_id = pt_folder_list->folder_ID;
						gt_store_hdisk.write_folder_day = utc_from_bcd(pt_folder_list->time_bcd);
					}
					if( i == 0 )
					{
						gt_store_hdisk.read_index = pt_folder_list->start_index;
					}
					gt_store_hdisk.write_index = pt_folder_list->start_index;
				}
				else
				{
					gt_store_hdisk.disk_state = STORE_HDISK_ERROR;
					SAMPLE_PRT("folder_list_error!\n");
					return -1;
				}
				read_len -= DF_STORE_FOLDER_LIST_LEN;
				i_index++;
			}
		}
		//////end

		///如果找到了最后一天，则根据最后一天的数据找到最后写入的index位置
		if( gt_store_hdisk.write_folder_day )
		{
			///生成文件夹名称
			store_create_foldernvr_name(gt_store_hdisk.write_folder,gt_store_hdisk.write_folder_day);
			
			///生成文件夹内部的file_list文件名称，然后打开并检索信息，最后生成file_list头结构体信息
			sprintf(c_buf,"%s/%s",gt_store_hdisk.write_folder,DF_STORE_FILE_LIST_NAME);
			gt_store_hdisk.write_file_list_fd = store_search_file_list_head(c_buf,&gt_store_hdisk.file_list_head);
			if(gt_store_hdisk.write_file_list_fd == 0)
			{
				gt_store_hdisk.disk_state = STORE_HDISK_ERROR;
				SAMPLE_PRT("open file err\n");
				return -1;
			}
			///找到了最后的索引位置，记录到 gt_store_hdisk.write_index 
			if( gt_store_hdisk.file_list_head.write_index_end )
			{
				gt_store_hdisk.write_index = gt_store_hdisk.file_list_head.write_index_end;
				gt_store_hdisk.write_index++;
				if(gt_store_hdisk.write_index > gt_store_hdisk.disk_size_av)
					gt_store_hdisk.write_index = DF_STORE_WRITE_INDEX_START;
			}
		}
	}
	///填充写入的列表头数据
	if(i_init_folder)
	{
		gt_store_hdisk.write_index = DF_STORE_WRITE_INDEX_START;
		gt_store_hdisk.read_index  = DF_STORE_WRITE_INDEX_START;
		gt_store_hdisk.write_folder_day = sgt_utc_now / 86400 * 86400;
		store_create_foldernvr_name(gt_store_hdisk.write_folder,gt_store_hdisk.write_folder_day);
		disk_create_dir(gt_store_hdisk.write_folder);
	}
	fclose(floder_fd);

	///打开裸盘
	if(0 == gt_store_hdisk.write_hdisk_fd)
	{
		gt_store_hdisk.write_hdisk_fd = open(DF_STORE_HDISK_DIR_NAME, O_RDWR );
		if(gt_store_hdisk.write_hdisk_fd == 0)
		{
			gt_store_hdisk.disk_state = STORE_HDISK_ERROR;
			SAMPLE_PRT("open file err\n");
			return -1;
		}
	}
	return 0;
}


/*********************************************************************************
  *函数名称:static int store_save_proc(void)
  *功能描述:数据存储函数，该函数实现硬盘数据的存储。
  *输	入: none
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
static int store_save_proc(void)
{
	int i,j,k;
	st_store_av_manage *p_av_manage;
	s64	s64_write_addr;
	int	new_file;
	int	save_channel_file;
	int	close_file;
	u8	 uc_buf[128];
	u32 	ul_len,ul_temp1;
	
	if(gt_store_hdisk.disk_state != STORE_HDISK_MOUNTED )
	{
		return -1;
	}

	if(gt_store_hdisk.write_folder_day == 0)
	{
		if(store_nvr_init() != 0)
			return -1;
	}
	if(gt_store_hdisk.write_folder_day != sgt_utc_now / 86400 * 86400)
	{
		store_close_channel_nvr( 0xFFFF );
		store_close_file_list();
		gt_store_hdisk.write_folder_day = sgt_utc_now / 86400 * 86400;
		store_create_foldernvr_name(gt_store_hdisk.write_folder,gt_store_hdisk.write_folder_day);
		disk_create_dir(gt_store_hdisk.write_folder);
	}
	for(i=0;i<DF_STORE_CHANNEL_MAX;i++)
	{
		p_av_manage = &gt_store_av_manage[i];
		save_channel_file  = 0;
		close_file = 0;
		///查询是否有需要写入的数据，有的话直接先写入
		for(j=0;j<2;j++)
		{
			if( p_av_manage->data_av.frame_style[j] & (BIT(7)) )
			{
				p_av_manage->data_av.frame_style[j] &= ~(BIT(7));
				p_av_manage->data_av.av_buf_rd = DF_STORE_AV_PACK_SIZE * ( 1 - j );
				p_av_manage->data_av.av_len %= DF_STORE_AV_PACK_SIZE;
				gt_store_hdisk.channel_param[i].write_frame_style = p_av_manage->data_av.frame_style[j];
				gt_store_hdisk.channel_param[i].write_time = p_av_manage->data_av.time[j];
				if(gt_store_hdisk.write_hdisk_fd)
				{
					///写入硬盘数据，并更改写入位置信息
					s64_write_addr = gt_store_hdisk.write_index;
					s64_write_addr *= DF_STORE_AV_PACK_SIZE;
					disk_write(gt_store_hdisk.write_hdisk_fd, s64_write_addr, p_av_manage->data_av.av_buf + j * DF_STORE_AV_PACK_SIZE, DF_STORE_AV_PACK_SIZE);
					
					///如果当前存入的和之前的录像格式不相同，则保存老格式信息
					if( memcmp((u8 *)&gt_store_hdisk.channel_param[i].style, (u8 *)&p_av_manage->style,sizeof(en_store_style)+sizeof(st_store_video_param)+sizeof(st_store_audio_param)) )
					{
						///保存老格式录像信息
						store_close_channel_nvr( BIT(i) );
					}

					///检查并写入新的存储信息
					store_save_channel_nvr( BIT(i) );

					///检查nvr数据是否已满(单个文件的时间，或大小限制)，如果满了，则保存nvr文件，并更新file_list文件
					if(gt_store_hdisk.write_limit_mode)
					{
						if( gt_store_hdisk.channel_param[i].file_len >= gt_store_hdisk.write_size_max )
						{
							store_close_channel_nvr( BIT(i) );
						}
					}
					else
					{
						if( gt_store_hdisk.channel_param[i].time_end - gt_store_hdisk.channel_param[i].time_start >= gt_store_hdisk.write_time_max )
						{
							store_close_channel_nvr( BIT(i) );
						}
					}

					///裸盘写入位置加1，然后判断是否越界
					gt_store_hdisk.write_index++;
					if(	gt_store_hdisk.write_index > gt_store_hdisk.disk_size_av )
					{
						gt_store_hdisk.write_index = DF_STORE_WRITE_INDEX_START;
					}
					if(	gt_store_hdisk.write_index == gt_store_hdisk.read_index)
					{
						store_remove_oldest_folder();
					}
					break;
				}
			}
		}
	}
	
	return 0;
}


/*********************************************************************************
  *函数名称:void* store_thread_proc(void *p)
  *功能描述:数据存储线程，该线程实现硬盘数据的存储。
  *输	入: p		:传递的参数，该参数的类型必须为	st_gps_thread_param
  *输	出: none
  *返 回 值:固定返回为	NULL
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
void* store_thread_proc(void *p)
{
	fd_set			readfds, testfds;
    int				i;
    int				pipe_fd;
    int				res;
	char 			buffer[64];
	static 	int		bytes_read = 0;
    struct timeval TimeoutVal;
	
    st_gps_thread_param	*pt_para;
	
	pt_para = (st_gps_thread_param*)p;

	pipe_fd = gt_store_fifo_fd;
	
    FD_ZERO(&readfds);
    FD_SET(pipe_fd, &readfds);

    while( pt_para->thread_start)
	{
        testfds = readfds;
		///TimeoutVal.tv_usec	= 0;
		TimeoutVal.tv_usec	= 100000;
		TimeoutVal.tv_sec	= 0;
		res = select(pipe_fd+1, &testfds, (fd_set *)0, (fd_set *)0, &TimeoutVal);
		//printf( "S" );
		//fflush(stdout);
        if(res < 0)
        {
            perror("server5");
            return;
        }
        sgt_utc_now = rtc_get_time(0);
        if( FD_ISSET(pipe_fd,&testfds) )
        {
            do
            {
            	memset(buffer,0,sizeof(buffer));
                res = read(pipe_fd, buffer, sizeof(buffer)-1);
                bytes_read += res;
				if( res > 0 )
	                printf("FIFO:%s\n", buffer );
            }
            while(res > 0);
        }
    }
}


/*********************************************************************************
  *函数名称:int store_thread_Start(char c_val)
  *功能描述:开启存储录像线程
  *输	入: none
  *输	出: none
  *返 回 值:	0:OK  非0:错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
int store_thread_Start(char c_val)
{
	s32 i_ret;
	
	gt_store_thread_para.thread_start = TRUE;
	gt_store_thread_para.thread_param = c_val;
    i_ret = pthread_create(&gt_store_thread_pid, 0, store_thread_proc, (void*)&gt_store_thread_para);
	return i_ret;
}


/*********************************************************************************
  *函数名称:int store_fifo_write( char *p)
  *功能描述:向线程store的有名管道发送字符串
  *输	入: p		:传递的字符串
  *输	出: none
  *返 回 值:	0:OK  非0:错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
int store_fifo_write( char *p)
{
    int i_size;
	//pt_para->thread_start = FALSE;
	if(gt_store_fifo_fd>0)
	{
		i_size=write(gt_store_fifo_fd,p,strlen(p));
		if(i_size < 0)
		{
			//printf("%s: ERROR1!\n" __FUNCTION__);
			printf("%s: ERROR1!\n", "store_fifo_write");
	        return -1;
		}
		//printf("%s:send->%s\n" __FUNCTION__,p);
		printf("%s:send->%s\n", "store_fifo_write",p);
		return 0;
	}
	//printf("%s: ERROR2!\n" __FUNCTION__);
	printf("%s: ERROR2!\n", "store_fifo_write");
	return 1;
}


/*********************************************************************************
  *函数名称:int store_creat_fifo(void)
  *功能描述:创建一个有名管道，并且启动录像存储线程
  *输	入: none
  *输	出: none
  *返 回 值:	0:OK  非0:错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
int store_creat_fifo(void)
{
	int ret;
    int                pipe_fd;
    //ret = mkfifo( FIFO_NAME, S_IFIFO | 0666 );
    remove(DF_STORE_FIFO_NAME);
    ret = mkfifo( DF_STORE_FIFO_NAME, 0666 );
    if (ret == 0)
	{
		printf("成功创建管道!\n");
		pipe_fd = open(DF_STORE_FIFO_NAME, O_NONBLOCK | O_RDWR);
		
		printf("open=%d\n",pipe_fd);
	    if( pipe_fd==-1 )
	    {
	        printf("open pipe erro!");
	        return 2;
	    }
		else
		{
			printf("open pipe ok!");
		}
		gt_store_fifo_fd = pipe_fd;
		return store_thread_Start(1);
	} 
	else
    {
		printf("创建管道失败!\n");
		gt_store_fifo_fd = 0;
		return 1;
    }
}


/*********************************************************************************
  *函数名称:int store_creat_fifo(void)
  *功能描述:销毁录像存储的有名管道，并且结束录像存储线程，并且删除管道名称
  *输	入: none
  *输	出: none
  *返 回 值:	0:OK  非0:错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
int store_destroy_fifo(void)
{
	int i_ret = 0;
	if(gt_store_fifo_fd)
	{
		gt_store_thread_para.thread_start = FALSE;
		usleep(10000);
	    pthread_join(gt_store_thread_pid, 0);
		close( gt_store_fifo_fd );
		gt_store_fifo_fd = 0;
		printf(" 线程结束了_store!\n");
	}
	remove(DF_STORE_FIFO_NAME);
	return i_ret;
}


/*********************************************************************************
  *函数名称:int store_av_init(void)
  *功能描述:初始化录像相关的参数和数据
  *输	入: none
  *输	出: none
  *返 回 值:	0:OK  非0:错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
int store_av_init(void)
{
	memset(gt_store_av_manage, 0, sizeof(gt_store_av_manage));
}


/*********************************************************************************
  *函数名称:int store_init(void)
  *功能描述:初始化录像部分，开始录像必须要调用该函数
  *输	入: none
  *输	出: none
  *返 回 值:	0:OK  非0:错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
int store_init(void)
{
	store_av_init( );
	memset(gt_store_hdisk, 0, sizeof(gt_store_hdisk));
	store_creat_fifo( );
}


/*********************************************************************************
  *函数名称:int store_end(void)
  *功能描述:结束录像，程序退出时必须要调用该函数
  *输	入: none
  *输	出: none
  *返 回 值:	0:OK  非0:错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
int store_end(void)
{
	store_av_init( );
	store_destroy_fifo( );
}



/*********************************************************************************
  *函数名称:static int store_save_av_data( st_store_buf *pt_store_buf, u8 * p_data,uint32_t len )
  *功能描述:存储数据到音视频buf中，该函数只能在store_save_video函数内部调用
  *输	入: pt_store_buf:写入的目的buf
  			p		:写入的数据
  			len		:写入的长度
  *输	出: none
  *返 回 值:	0:正常写入， 1:表示写入buf已满，从头重新写入，2:从前半部分写入到了后半部分
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建函数
*********************************************************************************/
static int store_save_av_data( st_store_av_data *pt_store_buf, u8 * p_data,uint32_t len )
{
	u32	i;
	u32 ul_len;
	int i_ret = 0;
	u32	start_wr;

	if((!len)||(len>sizeof(pt_store_buf->av_buf)))
	{
		//printf("ERROR:%s ,len = %d", __FUNCTION__ ,len );
		printf("ERROR:%s ,len = %d", __func__ ,len );
		return 0;
	}
	start_wr = pt_store_buf->av_buf_wr;

	///如果超过了长度，则需要先写入一部分，然后从buf头部重新写入。
	if(pt_store_buf->av_buf_wr + len > sizeof(pt_store_buf->av_buf))
	{
		ul_len = sizeof(pt_store_buf->av_buf) - pt_store_buf->av_buf_wr;
		memcpy( &( pt_store_buf->av_buf[pt_store_buf->av_buf_wr]), p_data, ul_len );
		len -= ul_len;
		p_data += ul_len;
		pt_store_buf->av_len	+= ul_len;
		pt_store_buf->av_buf_wr = 0;
	}
	///
	if(len)
	{
		memcpy(&(pt_store_buf->av_buf[pt_store_buf->av_buf_wr]),p_data,len);
		pt_store_buf->av_len	+= len;
		pt_store_buf->av_buf_wr += len;
	}
	///检查是否写入新的64K区域，前半段写入
	if((start_wr >= DF_STORE_AV_PACK_SIZE)&&(pt_store_buf->av_buf_wr < DF_STORE_AV_PACK_SIZE))
	{
		pt_store_buf->time[0] = 0;
		pt_store_buf->frame_style[0] = 0;
		///如果没有检测到I帧(写入I帧后，该值frame_style为2)，则认为都是p b帧
		pt_store_buf->frame_style[1] |= BIT(7) + BIT(0);
		return 1;
	}
	///检查是否写入新的64K区域，后半段写入
	else if((start_wr < DF_STORE_AV_PACK_SIZE)&&(pt_store_buf->av_buf_wr >= DF_STORE_AV_PACK_SIZE))
	{
		pt_store_buf->time[1] = 0;
		pt_store_buf->frame_style[1] = 0;
		///如果没有检测到I帧(写入I帧后，该值frame_style为2)，则认为都是p b帧
		pt_store_buf->frame_style[0] |= BIT(7) + BIT(0);
		return 2;
	}
	else
		return 0;
}


/*********************************************************************************
  *函数名称:int store_save_audio( st_store_audio_param *t_audio_param, u8 * p_data,uint32_t len )
  *功能描述:存储音频数据接口函数
  *输	入: pt_audio_param:音频数据参数
  			p_data		:数据指针buf
  			len			:数据长度
  *输	出: none
  *返 回 值:	0:OK  非0:错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建函数
*********************************************************************************/
int store_save_audio( st_store_audio_param *pt_audio_param, u8 * p_data,u32 len )
{
	st_store_audio_param  *pt_a_param;
	st_store_a_data	*pt_store_data;
	u32	i;
	u8	uc_channel;
	u32 ul_len;

	
	uc_channel = pt_audio_param->channel;
	uc_channel %= DF_STORE_CHANNEL_MAX;

	pt_a_param = &(gt_store_av_manage[uc_channel].param_audio);

	if(memcmp(pt_a_param,pt_audio_param,sizeof(st_store_audio_param)) != 0)
	{
		memset((u8 *)&(gt_store_av_manage[uc_channel].data_a),0,sizeof(st_store_a_data));
		memcpy(pt_a_param,pt_audio_param,sizeof(st_store_audio_param));
	}
	
	
	if((p_data[0] == 0x00) && (p_data[1] == 0x01) && (p_data[2] == 0x52) && (p_data[3] == 0x00) && ( len == 168 ))
	{
		pt_store_data = &(gt_store_av_manage[uc_channel].data_a);
		
		memcpy(pt_store_data->a_buf[pt_store_data->a_buf_wr], p_data, DF_STORE_AUDIO_FRAME_SIZE);
		pt_store_data->a_buf_wr++;
		pt_store_data->a_buf_wr = (pt_store_data->a_buf_wr + 1) % DF_STORE_AUDIO_FRAME_MAX;
		///当前总的音频包数量
		pt_store_data->a_len = ( pt_store_data->a_buf_wr + DF_STORE_AUDIO_FRAME_MAX - pt_store_data->a_buf_rd ) % DF_STORE_AUDIO_FRAME_MAX;
		return 0;
	}
	
	return 1;
}

/*********************************************************************************
  *函数名称:int store_save_video( st_store_video_param *tp_video_param, u8 * p_data,u32 len )
  *功能描述:存储视频数据接口函数
  *输	入: tp_video_param:视频数据参数
  			p_data		:数据指针buf
  			len			:数据长度
  *输	出: none
  *返 回 值:	0:正常返回,	1:表示64K存储已满，可以将数据存入硬盘中	2:本次存入的数据格式和上次的不相同
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建函数
*********************************************************************************/
int store_save_video( st_store_video_param *tp_video_param, u8 * p_data,u32 len )
{
	st_store_video_param  *pt_v_param;
	st_store_av_data	*pt_store_data;
	st_store_a_data		*pt_store_a_data;
	u32	i;
	u8  uc_ip_frame = 0;
	u8	uc_channel;
	u8	uc_proc_index = 0;
	static u8	uc_buf[256];
	u32 ul_av_len,ul_a_len,ul_v_len,ul_len;
	u32 ul_val;
	int i_ret = 0;

	
	uc_channel = tp_video_param->channel;
	uc_channel %= DF_STORE_CHANNEL_MAX;

	pt_v_param = &(gt_store_av_manage[uc_channel].param_video);

	if(memcmp(pt_v_param,tp_video_param,sizeof(st_store_video_param)) != 0)
	{
		memset((u8 *)&(gt_store_av_manage[uc_channel].data_av),0,sizeof(st_store_av_data));
		memcpy(pt_v_param,tp_video_param,sizeof(st_store_video_param));
		i_ret += 100;
	}
	
	if((p_data[0] == 0x00) && (p_data[1] == 0x00) && (p_data[2] == 0x00) && (p_data[3] == 0x01))
	{
		if( ( p_data[4] & 0x31 ) == 0x05 )
		{
			///找到了p帧
			uc_ip_frame = 1;
		}
		else if(( p_data[4] & 0x31) == 0x01 )
		{
			///找到了I帧
			uc_ip_frame = 2;
		}
		else
		{
			///找到了其它视频帧
			uc_ip_frame = 3;
		}
	}
	//pt_store_data = &gt_store_mem[uc_channel];
	pt_store_data = &(gt_store_av_manage[uc_channel].data_av);
	pt_store_a_data = &(gt_store_av_manage[uc_channel].data_a);
	if( uc_ip_frame )
	{
		if(uc_ip_frame == 3)		///短帧信息，需要暂存在v_buf中
		{
			memcpy(pt_store_data->v_buf + pt_store_data->v_buf_wr,p_data,len);
			pt_store_data->v_buf_wr += len;
		}
		else						///数据帧，直接存储处理，并将暂存的帧信息写入
		{
			pt_store_data->iswrite = 1;		///正在写入buf
			///计算视频总长度
			ul_v_len = pt_store_data->v_buf_wr + len;
			///计算音频总长度
			//ul_a_len = pt_store_a_data->a_buf_wr;
			//ul_a_len = pt_store_a_data->a_len;
			if(pt_store_a_data->a_len > DF_STORE_AUDIO_DOUBLE)
			{
				ul_a_len = DF_STORE_AUDIO_FRAME_SIZE * 2;
			}
			else
			{
				ul_a_len = DF_STORE_AUDIO_FRAME_SIZE;
			}
			///计算音视频总长度
			ul_av_len = ul_v_len + ul_a_len;
			///对齐包头信息部分，该部分暂定为16字节对齐
			ul_len = ((pt_store_data->av_buf_wr + 15) & 0xFFFFFFF0) - pt_store_data->av_buf_wr;
			memset(uc_buf, 0, sizeof(uc_buf));
			i_ret += store_save_av_data( pt_store_data, uc_buf, ul_len );
			///填充包头信息
			sprintf(uc_buf,"GPSAV00%d",uc_ip_frame);		///包头固定内容"GPSAV00"
			ul_len = 8;
			uc_buf[ul_len++] 	= (u8)ul_av_len;			///总长度
			uc_buf[ul_len++] 	= (u8)(ul_av_len>>8);
			uc_buf[ul_len++] 	= (u8)(ul_av_len>>16);
			uc_buf[ul_len++] 	= (u8)ul_v_len;				///视频总长度
			uc_buf[ul_len++] 	= (u8)(ul_v_len>>8);
			uc_buf[ul_len++] 	= (u8)(ul_v_len>>16);

			utc_to_bcd(uc_buf, sgt_utc_now);
			ul_len += 6;
			
			uc_buf[ul_len++] = gt_store_av_manage[uc_channel].param_video.channel;
			uc_buf[ul_len++] = gt_store_av_manage[uc_channel].param_video.e_pic_size;
			uc_buf[ul_len++] = gt_store_av_manage[uc_channel].param_video.e_video_norm;
			ul_val = (u32)gt_store_av_manage[uc_channel].param_video.e_payload_type;
			uc_buf[ul_len++] 	= (u8)ul_val;				///要加载的文件格式
			uc_buf[ul_len++] 	= (u8)(ul_val>>8);
			uc_buf[ul_len++] = gt_store_av_manage[uc_channel].param_video.e_trige_type;			
			ul_val = (u32)gt_store_av_manage[uc_channel].param_audio.e_payload_type;
			uc_buf[ul_len++] 	= (u8)ul_val;				///要加载的文件格式
			uc_buf[ul_len++] 	= (u8)(ul_val>>8);
			ul_len += 2;
			uc_buf[ul_len++] 	= 'E';						///结尾字符'E'
			uc_buf[ul_len++] 	= 'D';						///结尾字符'D'	
			///写入包头信息,长度为16字节整数倍
			i_ret += store_save_av_data( pt_store_data, uc_buf, ((ul_len + 15) & 0xFFF0) );
			///写入视频数据
			i_ret += store_save_av_data( pt_store_data, pt_store_data->v_buf, pt_store_data->v_len);
			i_ret += store_save_av_data( pt_store_data, p_data, len );
			///写入音频数据
			if(pt_store_a_data->a_buf_rd == pt_store_a_data->a_buf_wr)	///无音频数据
			{
				pt_store_a_data->a_len = 0;
				i = (pt_store_a_data->a_buf_wr + 2)%DF_STORE_AUDIO_FRAME_MAX;
				memset(pt_store_a_data->a_buf[i], 0, DF_STORE_AUDIO_FRAME_SIZE);
				i_ret += store_save_av_data( pt_store_data, pt_store_a_data->a_buf[i], DF_STORE_AUDIO_FRAME_SIZE);
			}
			else			///有音频数据
			{
				///当前总的音频包数量
				pt_store_a_data->a_len = ( pt_store_a_data->a_buf_wr + DF_STORE_AUDIO_FRAME_MAX - pt_store_a_data->a_buf_rd ) % DF_STORE_AUDIO_FRAME_MAX;
				///如果当检测到当前音频包太多，需要丢掉几个包，保证有3个空闲的包供写入段访问，防止读写冲突
				if( pt_store_a_data->a_len > ( DF_STORE_AUDIO_FRAME_MAX - 3 ) )
				{
					i = ( pt_store_a_data->a_buf_wr + 3 ) % DF_STORE_AUDIO_FRAME_MAX;
				}
				else
				{
					i = pt_store_a_data->a_buf_rd % DF_STORE_AUDIO_FRAME_MAX;
				}
				while(ul_a_len)
				{
					ul_a_len -= DF_STORE_AUDIO_FRAME_SIZE;
					i_ret += store_save_av_data( pt_store_data, pt_store_a_data->a_buf[i++], DF_STORE_AUDIO_FRAME_SIZE);
					i %=  DF_STORE_AUDIO_FRAME_MAX;
				}
				pt_store_a_data->a_buf_rd = i;
			}

			///写入时间
			if(pt_store_data->av_buf_wr > DF_STORE_AV_PACK_SIZE)
			{
				uc_proc_index = 1;
			}
			else
			{
				uc_proc_index = 0;
			}
			///当前正在写入的区域有I帧，则标记该数据段中有I帧
			if(uc_ip_frame == 2)
			{
				pt_store_data->frame_style[uc_proc_index] |= BIT(1);
				pt_store_data->time[uc_proc_index] = sgt_utc_now;
			}
			else if( 0 == pt_store_data->time[uc_proc_index])
			{
				pt_store_data->time[uc_proc_index] = sgt_utc_now;
			}
			///清空写入状态信息
			pt_store_data->v_len	= 0;
			pt_store_data->iswrite = 0;		///写入结束
		}
	}
	if(i_ret >= 100)
		return 2;
	else if(i_ret)
	{
		
		return 1;
	}
	else
		return 0;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

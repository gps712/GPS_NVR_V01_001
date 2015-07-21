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


#define FIFO_NAME						"/tmp/cmd_pipe"



#define DF_STORE_AUDIO_DOUBLE		3			///当buf中的音频包数大于该值时，需要两包发送


st_store_av_manage gt_store_av_manage[DF_STORE_CHANNEL_MAX];			///定义的进行音视频数据存取管理的结构体
st_store_av_data gt_store_mem[DF_STORE_CHANNEL_MAX];			///定义的进行音视频数据存取的buf
static pthread_t gt_store_thread_pid;					///录像数据存储线程PID
int		gt_store_fifo_fd = 0;							///录像数据存储有名管道的句柄
static st_gps_thread_param	gt_store_thread_para;		///调试接口线程参数
st_store_hdisk		gt_store_hdisk;			///硬盘状态信息和相关硬盘参数





/*********************************************************************************
  *函数名称:static int store_remove_oldest_files(void)
  *功能描述:录像存储满了，需要删除最老的数据和相关信息时调用该函数
  *输	入: none
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
static int store_remove_oldest_files(void)
{
	return 0;
}




/*********************************************************************************
  *函数名称:static int store_remove_oldest_files(void)
  *功能描述:将指定通道的录像文件进行保存
  *输	入: uc_channel	，需要保存测通道编号，bit0表示通道0，bit1表示通道1，........
  *输	出: none
  *返 回 值:0，表示正常返回，-1，表示出现了错误，1表示存储了有效的数据
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
static int store_close_channel_nvr(u8 close_channel)
{
	u8 i;
	MYTIME	my_time1,my_time2;				///
	char folder_name[32];
	char file_name[64];
	u8	 uc_buf[128];
	s8	 c_buf[128];
	u32 	ul_len,ul_temp1;
	
	for(i=0; i<DF_STORE_CHANNEL_MAX; i++)
	{
		if( close_channel & (BIT( i )) )
		{
			if( gt_store_hdisk.channel_param[i].write_index )
			{
				if(( gt_store_hdisk.channel_param[i].write_file_fd ) && (gt_store_hdisk.write_file_list_fd))
				{
					///关闭文件，并清空句柄
					close( gt_store_hdisk.channel_param[i].write_file_fd );
					gt_store_hdisk.channel_param[i].write_file_fd = 0;

					///新文件名称生成
					my_time1 = utc_to_mytime(gt_store_hdisk.channel_param[i].time_start );
					my_time2 = utc_to_mytime(gt_store_hdisk.channel_param[i].time_end );
					sprintf(c_buf,"%s\0000-%02d%02d%02d-%02d%02d%02d-%02d%02d%02d-%d-%d",
						gt_store_hdisk.write_folder,
						YEAR(my_time1),MONTH(my_time1),DAY(my_time1),
						HOUR(my_time1),MINUTE(my_time1),SEC(my_time1),
						HOUR(my_time2),MINUTE(my_time2),SEC(my_time2),
						gt_store_hdisk.channel_param[i].param_video.e_trige_type,
						gt_store_hdisk.channel_param[i].param_video.channel									
						);
					///重命名文件名称
					rename( gt_store_hdisk.channel_param[i].write_file, c_buf);

					///填充文件列表内容
					memset(uc_buf,0,sizeof(uc_buf));
					strcpy(uc_buf,"LIST");
					ul_len = 8;
					strcpy(uc_buf+8, c_buf);
					ul_len += 96;
					uc_buf[ul_len++] = (u8)(gt_store_hdisk.channel_param[i].write_index);
					uc_buf[ul_len++] = (u8)(gt_store_hdisk.channel_param[i].write_index >> 8);
					ul_temp1 = gt_store_hdisk.channel_param[i].time_start%86400;
					memcpy(&uc_buf[ul_len],(u8 *)&ul_temp1,4);
					ul_len += 4;
					ul_temp1 = gt_store_hdisk.channel_param[i].time_end%86400;
					memcpy(&uc_buf[ul_len],(u8 *)&ul_temp1,4);
					ul_len += 4;
					uc_buf[ul_len++] = (u8)(gt_store_hdisk.channel_param[i].param_video.channel);
					ul_len = 128;

					///将文件信息写入列表文件中
					ul_temp1 = gt_store_hdisk.file_list_head.file_num;
					ul_temp1 = DF_STORE_FILE_LIST_LEN * ul_temp1 + DF_STORE_FILE_LIST_HEAD_LEN;
					disk_write(gt_store_hdisk.write_file_list_fd,ul_temp1,uc_buf,DF_STORE_FILE_LIST_LEN);

					///改变文件列表头信息的内容
					gt_store_hdisk.file_list_head.file_num++;
					gt_store_hdisk.file_list_head.filelist_size += gt_store_hdisk.channel_param[i].write_index;
					if(gt_store_hdisk.file_list_head.time_end < gt_store_hdisk.channel_param[i].time_end)
						gt_store_hdisk.file_list_head.time_end = gt_store_hdisk.channel_param[i].time_end;
					if(gt_store_hdisk.file_list_head.time_start == 0)
						gt_store_hdisk.file_list_head.time_start = gt_store_hdisk.channel_param[i].time_start;
					
				}
			}
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
	int	close_file;
	MYTIME	my_time1,my_time2;				///
	char folder_name[32];
	char file_name[64];
	u8	 uc_buf[128];
	s8	 c_buf[128];
	u32 	ul_len,ul_temp1;
	
	if(gt_store_hdisk.disk_state != STORE_HDISK_MOUNTED )
	{
		return -1;
	}

	for(i=0;i<DF_STORE_CHANNEL_MAX;i++)
	{
		p_av_manage = &gt_store_av_manage[i];
		new_file  = 0;
		close_file = 0;
		///查询是否有需要写入的数据，有的话直接先写入
		for(j=0;j<2;j++)
		{
			if(p_av_manage->data_av.frame_style[j])
			{
				p_av_manage->data_av.frame_style[j] = 0;
				p_av_manage->data_av.av_buf_rd = 0x10000 * ( 1 - j );
				p_av_manage->data_av.av_len %= 0x10000;
				if(gt_store_hdisk.write_hdisk_fd)
				{
					///写入硬盘数据，并更改写入位置信息
					s64_write_addr = gt_store_hdisk.write_index;
					s64_write_addr *= 0x10000;
					disk_write(gt_store_hdisk.write_hdisk_fd, s64_write_addr, p_av_manage->data_av.av_buf + j * 0x10000, 0x10000);
					
					gt_store_hdisk.write_index++;
					if(	gt_store_hdisk.write_index >= gt_store_hdisk.disk_size_av )
					{
						gt_store_hdisk.write_index = DF_STORE_WRITE_INDEX_START;
						/////////////////////////////////在这里增加删除最早的录像信息的代码start
						store_remove_oldest_files();
						/////////////////////////////////在这里增加删除最早的录像信息的代码end
					}
					///生成当前时间对应的日期名称文件夹
					my_time1 = utc_to_mytime(p_av_manage->data_av.time[j]);
					
					sprintf(folder_name,"%s\%04d-%02d-%02d",DF_STORE_HDISK_AV,
						YEAR(my_time1)+2000,MONTH(my_time1),DAY(my_time1));
					if(strcmp(gt_store_hdisk.write_folder,folder_name))
					{
						disk_create_dir(folder_name);
						//strcpy(gt_store_hdisk.channel_param[i].write_folder,folder_name);
						new_file |= 2;
					}
					
					///检查并写入信息，发现当前存入的和之前的不相同，则需要重新生成新的录像信息文件
					if( memcmp((u8 *)&gt_store_hdisk.channel_param[i].style, (u8 *)&p_av_manage->style,sizeof(en_store_style)+sizeof(st_store_video_param)+sizeof(st_store_audio_param)) )
					{
						new_file |= 1;
					}

					if( new_file )
					{
						if( gt_store_hdisk.channel_param[i].write_index )
						{
							if( gt_store_hdisk.channel_param[i].write_file_fd )
							{
								///关闭文件，并清空句柄
								close( gt_store_hdisk.channel_param[i].write_file_fd );
								gt_store_hdisk.channel_param[i].write_file_fd = 0;

								///新文件名称生成
								my_time1 = utc_to_mytime(gt_store_hdisk.channel_param[i].time_start );
								my_time2 = utc_to_mytime(gt_store_hdisk.channel_param[i].time_end );
								sprintf(c_buf,"%s\0000-%02d%02d%02d-%02d%02d%02d-%02d%02d%02d-%d-%d",
									gt_store_hdisk.write_folder,
									YEAR(my_time1),MONTH(my_time1),DAY(my_time1),
									HOUR(my_time1),MINUTE(my_time1),SEC(my_time1),
									HOUR(my_time2),MINUTE(my_time2),SEC(my_time2),
									gt_store_hdisk.channel_param[i].param_video.e_trige_type,
									gt_store_hdisk.channel_param[i].param_video.channel									
									);
								///重命名文件名称
								rename( gt_store_hdisk.channel_param[i].write_file, c_buf);

								///填充文件列表内容
								memset(uc_buf,0,sizeof(uc_buf));
								strcpy(uc_buf,"LIST");
								ul_len = 8;
								strcpy(uc_buf+8, c_buf);
								ul_len += 96;
								uc_buf[ul_len++] = (u8)(gt_store_hdisk.channel_param[i].write_index);
								uc_buf[ul_len++] = (u8)(gt_store_hdisk.channel_param[i].write_index >> 8);
								ul_temp1 = gt_store_hdisk.channel_param[i].time_start%86400;
								memcpy(&uc_buf[ul_len],(u8 *)&ul_temp1,4);
								ul_len += 4;
								ul_temp1 = gt_store_hdisk.channel_param[i].time_end%86400;
								memcpy(&uc_buf[ul_len],(u8 *)&ul_temp1,4);
								ul_len += 4;
								uc_buf[ul_len++] = (u8)(gt_store_hdisk.channel_param[i].param_video.channel);
								ul_len = 128;
							}
						}
					}

					///要结束
					if(!new_file)
					{
						gt_store_hdisk.channel_param[i].write_index++;
						gt_store_hdisk.channel_param[i].time_end = p_av_manage->data_av.time[j];

						///
						disk_write(gt_store_hdisk.channel_param[i].write_file_fd, s64_write_addr, p_av_manage->data_av.av_buf + j * 0x10000, 64);
						if(gt_store_hdisk.write_limit_mode)
						{
							if( gt_store_hdisk.channel_param[i].write_index >= gt_store_hdisk.write_size_max )
							{
								close_file = 1;
							}
						}
						else
						{
							if( gt_store_hdisk.channel_param[i].time_end - gt_store_hdisk.channel_param[i].time_start >= gt_store_hdisk.write_time_max )
							{
								close_file = 1;
							}
						}
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
    remove(FIFO_NAME);
    ret = mkfifo( FIFO_NAME, 0666 );
    if (ret == 0)
	{
		printf("成功创建管道!\n");
		pipe_fd = open(FIFO_NAME, O_NONBLOCK | O_RDWR);
		
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
	remove(FIFO_NAME);
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
		if(pt_store_buf->frame_style[1] == 0)
		{
			pt_store_buf->frame_style[1] = 1;
		}
		return 1;
	}
	///检查是否写入新的64K区域，后半段写入
	else if((start_wr < DF_STORE_AV_PACK_SIZE)&&(pt_store_buf->av_buf_wr >= DF_STORE_AV_PACK_SIZE))
	{
		pt_store_buf->time[1] = 0;
		pt_store_buf->frame_style[1] = 0;
		///如果没有检测到I帧(写入I帧后，该值frame_style为2)，则认为都是p b帧
		if(pt_store_buf->frame_style[0] == 0)
		{
			pt_store_buf->frame_style[0] = 1;
		}
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
	utc_time			uct_time1;				///要存储的数据生成的时间
	MYTIME				my_time1;				///
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
	
	uct_time1 = rtc_get_time(0);
	my_time1 = utc_to_mytime(uct_time1);
	if(my_time1)
	{
		printf("\n time:  %d-%d-%d %d:%d:%d",YEAR(my_time1)+2000,
				MONTH(my_time1),
				DAY(my_time1),
				HOUR(my_time1),
				MINUTE(my_time1),
				SEC(my_time1)
				);
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
	utc_time			uct_time1;				///要存储的数据生成的时间
	MYTIME				my_time1;				///
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
	
	uct_time1 = rtc_get_time(0);
	my_time1 = utc_to_mytime(uct_time1);
	if(my_time1)
	{
		printf("\n time:  %d-%d-%d %d:%d:%d",YEAR(my_time1)+2000,
				MONTH(my_time1),
				DAY(my_time1),
				HOUR(my_time1),
				MINUTE(my_time1),
				SEC(my_time1)
				);
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

			if(my_time1)
			{
				uc_buf[ul_len++] = YEAR(my_time1);
				uc_buf[ul_len++] = MONTH(my_time1);
				uc_buf[ul_len++] = DAY(my_time1);
				uc_buf[ul_len++] = HOUR(my_time1);
				uc_buf[ul_len++] = MINUTE(my_time1);
				uc_buf[ul_len++] = SEC(my_time1);
			}
			else
			{
				ul_len += 6;
			}
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
			if(uc_ip_frame == 2)
			{
				if(0 == pt_store_data->frame_style[uc_proc_index])
				{
					pt_store_data->frame_style[uc_proc_index] = 2;
					pt_store_data->time[uc_proc_index] = uct_time1;
				}
			}
			else if( 0 == pt_store_data->time[uc_proc_index])
			{
				pt_store_data->time[uc_proc_index] = uct_time1;
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

/*********************************************************************************
  * @文件名称 :gps_disk_io.c
  * @功能描述 :所有硬盘和存储盘操作的驱动函数及应用函数
  * @作	   者 :白养民
  * @创建日期 :2015-6-17
  * @文件版本 :0.01
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-17	创建
*********************************************************************************/
#ifdef __cplusplus
#if __cplusplus
	extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include "gps_typedef.h"
#include <fcntl.h>





/*********************************************************************************
  *函数名称:int disk_printf_dir(const char* pc_path)  
  *功能描述:函数功能，打印pc_path目录下的所有文件,该目录最长为255字节
  *输	入: pc_path :要打印的目录路径，长度限制为255字节
  *输	出: none
  *返 回 值:1	:该目录已经存在，0:该目录已经创建
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-17	创建
*********************************************************************************/
int disk_printf_dir(const char* pc_path)
{
	DIR * pt_dir;
	struct dirent * pt_dirent;
	char c_tmp_buf[256];
	
	pt_dir = opendir(pc_path);
	
	if(pt_dir != NULL)
	{
		while((pt_dirent = readdir(pt_dir)) != NULL)
		{
			if(strcmp(pt_dirent->d_name,"..") && strcmp(pt_dirent->d_name,"."))
			{
				if(4 == pt_dirent->d_type)
				{
					memset(c_tmp_buf,0,sizeof(c_tmp_buf));
					strcpy(c_tmp_buf,pc_path);
					strcat(c_tmp_buf,"/");
					strcat(c_tmp_buf,pt_dirent->d_name);
					disk_printf_dir(c_tmp_buf);
				}
				else
				{
					printf("FIL_:%s/%s   d_type:%d\n",pc_path, pt_dirent->d_name,pt_dirent->d_type);
				}
			}
		}
		closedir(pt_dir);
	}
	printf("DIR&:%s\n", pc_path);
	return 0;
}  

/* 
struct dirent
{
	ino_t d_ino; 	//d_ino 此目录进入点的inode
	ff_t d_off; 	//d_off 目录文件开头至此目录进入点的位移
	signed short int d_reclen; 	//d_reclen _name 的长度, 不包含NULL 字符
	unsigned char d_type; 		//d_type 所指的文件类型
	har d_name[256];			// d_name 文件名
};
*/
/*********************************************************************************
  *函数名称:int disk_remove_dir(const char* pc_path)  
  *功能描述:删除一个目录(文件夹)和文件夹内部的所有文件，该目录最长为255字节
  *输	入: pc_path :要删除的目录路径，长度限制为255字节
  *输	出: none
  *返 回 值:非0值:该目录不存在或删除失败，0:删除成功
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-17	创建
*********************************************************************************/
int disk_remove_dir(const char* pc_path)  
{
	DIR * pt_dir;
	struct dirent * pt_dirent;
	char c_tmp_buf[256];
	int  i_ret = 0;
	
	pt_dir = opendir(pc_path);
	
	if(pt_dir != NULL)
	{
		while((pt_dirent = readdir(pt_dir)) != NULL)
		{
			if(strcmp(pt_dirent->d_name,"..") && strcmp(pt_dirent->d_name,"."))
			{
				if(4 == pt_dirent->d_type)
				{
					memset(c_tmp_buf,0,sizeof(c_tmp_buf));
					strcpy(c_tmp_buf,pc_path);
					strcat(c_tmp_buf,"/");
					strcat(c_tmp_buf,pt_dirent->d_name);
					i_ret += disk_remove_dir(c_tmp_buf);
				}
				else
				{
					remove(pt_dirent->d_name);
					printf("FIL_REMOVE:%s/%s   d_type:%d\n",pc_path, pt_dirent->d_name,pt_dirent->d_type);
				}
			}
		}
		closedir(pt_dir);
	}
	else
	{
		i_ret = 1;
	}
	rmdir(pc_path);
	printf("DIR_REMOVE:%s\n", pc_path);
	return i_ret;
}  


/*********************************************************************************
  *函数名称:int disk_create_dir(const char* pc_path)
  *功能描述:创建一个目录(文件夹)或多级目录，该目录的路径长度限制为255字节
  *输	入: pc_path	:要创建的目录路径，长度限制为255字节
  *输	出: none
  *返 回 值:1	:该目录已经存在，0:该目录已经创建
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-17	创建
*********************************************************************************/
int disk_create_dir(const char* pc_path)
{
	if(-1 != access(pc_path,0))
		return 1;

	char c_tmp_buf[256];
	const char* pc_cur = pc_path;

	memset(c_tmp_buf,0,sizeof(c_tmp_buf));
	int pos=0;
	//printf("\n disk_create_dir=%s\n",pc_path);
	while(*pc_cur++!='\0')
	{
		c_tmp_buf[pos++] = *(pc_cur-1);

		if(*pc_cur=='/' || *pc_cur=='\0')
		{
			if(0!=access(c_tmp_buf,0)&&strlen(c_tmp_buf)>0)
			{
				mkdir(c_tmp_buf,S_IRWXU|S_IRWXG|S_IRWXO );
			}
		}
	}
	return 0;
}

/*********************************************************************************
  *函数名称:int disk_write(int fd, u64 addr, u8 *pdata , int len)
  *功能描述:向已open方式打开的文件中写入数据
  *输	入: fd		:要写入的设备的句柄
  			addr	:要写入的位置，相对于文件的开始
  			pdata	:要写入的数据
  			len		:要写入的长度
  *输	出: none
  *返 回 值:非0:写入失败，0:写入成功
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-17	创建
*********************************************************************************/
int disk_write(int fd, s64 addr, u8 *pdata , int len)
{
    int i_ret;
    int i_size;
	
	if((fd == NULL)||( len == 0)||( pdata == NULL))
	{
        return -1;
    }
	
	lseek64(fd,addr,SEEK_SET);
	i_size=write(fd,pdata,len);
	if(i_size < 0)
	{
		printf("disk_hd_write: ERROR!");
        return -1;
	}
	return 0;
}

/*********************************************************************************
  *函数名称:int disk_read(int fd, u64 addr, u8 *pdata , int len)
  *功能描述:从已经用open方式打开的文件中读取数据
  *输	入: fd		:要读取的设备的句柄
  			addr	:要读取的位置，相对于文件的开始
  			pdata	:要读取的数据存放的位置
  			len		:要读取的长度
  *输	出: none
  *返 回 值:非0:读取失败，0:读取成功
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-17	创建
*********************************************************************************/
int disk_read(int fd, s64 addr, u8 *pdata , int len)
{
    int i_ret;
    int i_size;
	
	if((fd == NULL)||( len == 0)||( pdata == NULL))
	{
        return -1;
    }
	printf("disk_read_1\n");
	lseek64(fd,addr,SEEK_SET);
	printf("disk_read_2\n");
	i_size=read(fd,pdata,len);
	if(i_size < 0)
	{
		printf("disk_hd_read: ERROR!");
        return -1;
	}
	printf("disk_read_ok\n");
	return 0;
}

/***************************************************************************************************************/
//这是第一个正式程序，开始独立调试过程_START
/***************************************************************************************************************/


/******************************************************************************
* function    : main()
* Description : video venc sample
******************************************************************************/
int disk_pro_test01(int argc, char *argv[])
{
    s32 s32Ret;
   	int i, strt, ch_out;
	int cmd = 4;  
	long long sector = 0;  
    char buf[512];  
    int fd,size,len;
	char *buf_wr="Hello! I'm writing to this file!";
	
    cmd = atoi(argv[1]);
    sector = atoi(argv[2]);
	printf("sector=%d",sector);
	sector *= 512;
	//getchar();
    /*0：成功，-1：失败*/  
    ////////////////////////////////////////
 
	if(cmd&& (argc<4))
	{
    	printf("Please input param3( write string data. )!\n");
        return FALSE;
    }
	
	fd = open("/dev/sda2", O_RDWR );
	if(fd < 0)
	{
		printf("open: ERROR!");
		 return FALSE;
	}
	else
		printf("open file OK: %d\n",fd);
	/*
	while(sector)
	{
		if(sector>0xFFFFFFFF)
		{
			lseek64(fd,0xFFFFFFFF,SEEK_CUR);
			sector -= 0xFFFFFFFF;
		}
		else
		{
			lseek64(fd,sector,SEEK_CUR);
			sector = 0;
		}
	}
	*/
	lseek64(fd,sector,SEEK_SET);
	if(cmd)
	{
		size=write(fd,argv[3],strlen(argv[3]));
		if(size < 0)
		{
			printf("wtrite: ERROR!");
			close(fd);
			return FALSE;	
		}
	}
	else
	{
		memset(buf,0,sizeof(buf));
		size=read(fd,buf,128);
		if(size < 0)
		{
			printf("read: ERROR!");
			close(fd);
			return FALSE;
		}
		printf("Read OK\n");  
		printf_hex_data(buf,128);
	    printf("STRING=%s",buf);
	}
	//len = strlen(buf_wr);
	//buf_wr[10] = '\0';
	//if((size = write( fd, buf_wr, len)) < 0)
	//perror("write:");
	//exit(1);
	close(fd);
    ////////////////////////////////////////
    return FALSE;
}

/*********************************************************************************
  *函数名称:int console_disk_test( char *p, uint16_t len )
  *功能描述:测试程序
  *输	入: p		:传递的字符串
  			len		:字符串长度
  *输	出: none
  *返 回 值:	0:OK  非0:错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
int console_disk_test( char *p, int len )
{
	char c_buf[64];
	char * pc_str;
	static int fd = 0;
	static int i_address1,i_address2;
	int size;
 	int i;
	static char *pc_argv1[]={"0","0","1000","1234567890aaaaaabbbbbbbccccccc"};
	static char *pc_argv2[]={"0","1","1000","1234567890aaaaaabbbbbbbccccccc"};
	
	if(*p == 't')
	{
		if( *(p+1) == '0')
		{
			disk_pro_test01(4,pc_argv1);
		}
		else
		{
			disk_pro_test01(4,pc_argv2);
		}
		printf("test_disk_t\n");
		return;
	}
	
	if(*p == 'o')
	{
		i_address1 = 0xFFFFFF;
		i_address2 = 0xFFFFFF;
		fd = open("/dev/sda2", O_RDWR );
		if(fd < 0)
		{
			printf("open: ERROR!");
			fd = 0;
			return 1;
		}
		else
		{
			printf("open disk OK: %d\n",fd);
		}
	}
	
	if(fd == NULL)
	{
		printf("ERROR:file not open!\n");
		return 1;
	}
	
	if(strlen(p) < 2)
	{
		printf("ERROR:input is to short!\n");
		return 1;
	}
	
	if(*p == 'c')
	{
		i = close(fd);
		if( i )
		{
			printf("ERROR:close = %d",i);
			return 1;
		}
		printf("disk close: %d\n",fd);
		i_address1 = 0xFFFFFF;
		i_address2 = 0xFFFFFF;
		fd = 0;
	}
	
	else if(*p == 'w')
	{
		if( *(p+1) == '0')
		{
			disk_write(fd,i_address1,"TEST1234567890",strlen("TEST1234567890"));
			i_address1 += strlen("TEST1234567890");
		}
		else
		{
			disk_write(fd,i_address1,p,strlen(p));
			i_address1 += strlen(p);
		}
	}
	
	else if(*p == 'r')
	{
		memset(c_buf,0,sizeof(c_buf));
		if( *(p+1) == '0')
		{
			disk_read(fd,i_address2,c_buf,14);
			i_address2 += 14;
		}
		else
		{
			disk_read(fd,0xFFFFFF,c_buf,32);
		}
		printf("str:%s\n",c_buf);
	}
	
	return 0;
	
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

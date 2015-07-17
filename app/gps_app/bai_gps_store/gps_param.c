/*********************************************************************************
  * @文件名称 :gps_disk_io.c
  * @功能描述 :所有参数相关的函数都在这里，参数的读取，修改，等
  * @作	   者 :白养民
  * @创建日期 :2015-6-08
  * @文件版本 :0.01
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-08	创建
*********************************************************************************/




#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include "gps_typedef.h"
#include "common_func.h"
#include "inifile.h"
#include "gps_param.h"

#define DF_PARAM_INI_FILE				"myconfig.ini"
#define DF_PARAM_INI_SECT_GPS_PARAM		"GPS_PARAM"
#define DF_PARAM_INI_SECT_VDR_PARAM		"VDR_PARAM"


#define MAIN_PARA_CRC_ERROR				0x01			///设备主参数CRC错误
#define MAIN_PARA_VER_UP				0x02			///设备主参数版本升级
#define MAIN_PARA_FORMAT_ERR			0x04			///设备主参数格式错误
 	

#define JT808_FLASH_FORMA_VER		0x464D5431			///"FMT0",采用可见字符是为了显示和后期远程修改方便
#define JT808_FLASH_DATA_VER		0x30303034			///"0004",采用可见字符是为了显示和后期远程修改方便

#define BKSRAM_VERSION				0x00000002
#define BKSRAM_FORMAT				0x57584448


GPS_PARAM 		gps_param;
st_vdr_param	gt_vdr_param;


typedef struct _st_tbl_id_lookup
{
	char 	*id;
	uint8_t		type;
	uint8_t		* val;
	uint8_t		* default_val;
} st_tbl_id_lookup;


#define ID_LOOKUP( id, type,val ) { # id, type, (uint8_t*)&( gps_param.id_ ## id ),# val}
st_tbl_id_lookup	tbl_lookup_gps_param[] = {
	ID_LOOKUP( 0x0000, TYPE_STR ,FMT0),    	//uint32_t  id_0x0000;   /*0x0000 版本*/
	ID_LOOKUP( 0x0001, TYPE_DWORD ,20),    //uint32_t  id_0x0001;   /*0x0001 心跳发送间隔*/
	ID_LOOKUP( 0x0002, TYPE_DWORD ,60),    //uint32_t  id_0x0002;   /*0x0002 TCP应答超时时间*/
	ID_LOOKUP( 0x0003, TYPE_DWORD ,30),    //uint32_t  id_0x0003;   /*0x0003 TCP超时重传次数*/
	ID_LOOKUP( 0x0004, TYPE_DWORD ,30),    //uint32_t  id_0x0004;   /*0x0004 UDP应答超时时间*/
	ID_LOOKUP( 0x0005, TYPE_DWORD ,30),    //uint32_t  id_0x0005;   /*0x0005 UDP超时重传次数*/
	ID_LOOKUP( 0x0006, TYPE_DWORD ,30),    //uint32_t  id_0x0006;   /*0x0006 SMS消息应答超时时间*/
	ID_LOOKUP( 0x0007, TYPE_DWORD ,3),    //uint32_t  id_0x0007;   /*0x0007 SMS消息重传次数*/
	ID_LOOKUP( 0x0010, TYPE_STR ,CMNET),      //char   id_0x0010[32];  /*0x0010 主服务器APN*/
	ID_LOOKUP( 0x0011, TYPE_STR ,card),      //char   id_0x0011[32];  /*0x0011 用户名*/
	ID_LOOKUP( 0x0012, TYPE_STR ,card),      //char   id_0x0012[32];  /*0x0012 密码*/
	ID_LOOKUP( 0x0013, TYPE_STR ,192.168.2.20),      //char   id_0x0013[32];  /*0x0013 主服务器地址*/
	ID_LOOKUP( 0x0014, TYPE_STR ,unicom),      //char   id_0x0014[32];  /*0x0014 备份APN*/
	ID_LOOKUP( 0x0015, TYPE_STR ,card),      //char   id_0x0015[32];  /*0x0015 备份用户名*/
	ID_LOOKUP( 0x0016, TYPE_STR ,card),      //char   id_0x0016[32];  /*0x0016 备份密码*/
	ID_LOOKUP( 0x0017, TYPE_STR ,192.168.2.21),      //char   id_0x0017[32];  /*0x0017 备份服务器地址，ip或域名*/
	ID_LOOKUP( 0x0018, TYPE_DWORD ,9131),    //uint32_t  id_0x0018;   /*0x0018 TCP端口*/
	ID_LOOKUP( 0x0019, TYPE_DWORD ,9131),    //uint32_t  id_0x0019;   /*0x0019 UDP端口*/
	ID_LOOKUP( 0x001A, TYPE_STR ,60.28.50.210),      //char   id_0x001A[32];  /*0x001A ic卡主服务器地址，ip或域名*/
	ID_LOOKUP( 0x001B, TYPE_DWORD ,6001),    //uint32_t  id_0x001B;   /*0x001B ic卡服务器TCP端口*/
	ID_LOOKUP( 0x001C, TYPE_DWORD ,8001),    //uint32_t  id_0x001C;   /*0x001C ic卡服务器UDP端口*/
	ID_LOOKUP( 0x001D, TYPE_STR ,60.28.50.211),      //char   id_0x001D[32];  /*0x001D ic卡备份服务器地址，ip或域名*/
	ID_LOOKUP( 0x0020, TYPE_DWORD ,123 ),    //uint32_t  id_0x0020;   /*0x0020 位置汇报策略*/
	ID_LOOKUP( 0x0021, TYPE_DWORD ,80 ),    //uint32_t  id_0x0021;   /*0x0021 位置汇报方案*/
};

///VDR 参数
#define ID_LOOKUP_vdr( id, type,val ) { # id, type, (uint8_t*)&( gt_vdr_param.id_ ## id ),# val}
st_tbl_id_lookup	tbl_lookup_vdr_param[] = {
	ID_LOOKUP_vdr( 0x0000, TYPE_L(16) ,FMT1),    	//uint32_t  id_0x0000;   /*0x0000 版本*/
	ID_LOOKUP_vdr( 0x0001, TYPE_DWORD ,20),    //uint32_t  id_0x0001;   /*0x0001 心跳发送间隔*/
	ID_LOOKUP_vdr( 0x0002, TYPE_DWORD ,60),    //uint32_t  id_0x0002;   /*0x0002 TCP应答超时时间*/
	ID_LOOKUP_vdr( 0x0003, TYPE_DWORD ,30),    //uint32_t  id_0x0003;   /*0x0003 TCP超时重传次数*/
	ID_LOOKUP_vdr( 0x0004, TYPE_DWORD ,30),    //uint32_t  id_0x0004;   /*0x0004 UDP应答超时时间*/
	ID_LOOKUP_vdr( 0x0005, TYPE_DWORD ,30),    //uint32_t  id_0x0005;   /*0x0005 UDP超时重传次数*/
	ID_LOOKUP_vdr( 0x0006, TYPE_L(32) ,test-vdr-param),    //uint32_t  id_0x0006;   /*0x0006 SMS消息应答超时时间*/
};


#define ADD_INI_PARAM( sect, tbl ) { # sect, tbl, sizeof(tbl)}
typedef struct _st_tbl_ini_lookup
{
	char 				*section;
	st_tbl_id_lookup	*param;
	int 				param_size;
}st_tbl_ini_lookup;

st_tbl_ini_lookup tbl_ini_lookup[]={
	ADD_INI_PARAM(gps_param,		tbl_lookup_gps_param	),
	ADD_INI_PARAM(gt_vdr_param,		tbl_lookup_vdr_param	),
};


#include "inifile.h"


#define BUF_SIZE 256



#if 0
int test_proc(void)
{
	const char *file ="myconfig.ini";
	const char *section = "student";
	const char *name_key = "name";
	const char *age_key = "age";
	char name[BUF_SIZE]={0};
	char buf[MAX_FILE_SIZE]={0};
	int age;
	int file_size;
	int i;
	
	//write name key value pair
	if(write_profile_string(section,name_key,"Tony",file))
	{
		printf("write name pair to ini file fail\n");
		return -1;
	}
	
	//write age key value pair
	if(write_profile_string(section,age_key,"20",file))
	{
		printf("write age pair to ini file fail\n");
		return -1;
	}
	
	printf("[%s]\n",section);
	//read string pair, test read string value
	if(read_profile_string(section, name_key, name, BUF_SIZE,"",file))
	{
		printf("read ini file fail\n");
		return -1;
	}
	else
	{
		printf("%s=%s\n",name_key,name);
	}

	//read age pair, test read int value.
	//if read fail, return default value
	age = read_profile_int(section,age_key,0,file);
	printf("%s=%d\n",age_key,age);
	
	return 0;
}
#endif


/*********************************************************************************
  *函数名称:void param_fact_set(void)
  *功能描述:将设备主要参数设置为出厂参数
  *输	入: none
  *输	出: none
  *返 回 值:none
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-08	创建
*********************************************************************************/
void param_read(void)
{
	return;
}


/*********************************************************************************
  *函数名称:void param_out_ex(char * sect, char * id )
  *功能描述:将ini文件参数输出到标准输出设备，如果参数id为空则输出所有参数
  *输	入: section	:ini文件参数对应的的section
  			id		:要输出的参数名称，不包含前面的"id_"
  *输	出: 从标准输出输出参数
  *返 回 值:none
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-08	创建
*********************************************************************************/
void param_out_ex(char * section, char * id )
{
	int		i,i_sect;
	uint32_t u32data;
	uint8_t *p;
	char buffer[64];
	
	//param_load_addr(&jt808_param,ADDR_DF_PARAM_MAIN);
	for( i_sect = 0; i_sect < sizeof( tbl_ini_lookup ) / sizeof( struct _st_tbl_ini_lookup ); i_sect++ )
	{
		if((strlen(section) == 0)||(strncmp(section,tbl_ini_lookup[i_sect].section,strlen(tbl_ini_lookup[i_sect].section)) == 0))
		{
			for( i = 0; i < tbl_ini_lookup[i_sect].param_size / sizeof( st_tbl_id_lookup ); i++ )
			{
				//if(( id == tbl_id_lookup[i].id ) ||( 0 == id ))
				if((strncmp( id ,tbl_ini_lookup[i_sect].param[i].id, strlen(tbl_ini_lookup[i_sect].param[i].id) ) == 0) ||( 0 == strlen(id) ))
				{
				printf("\r\n %s.id_%s = ", tbl_ini_lookup[i_sect].section, tbl_ini_lookup[i_sect].param[i].id );
				switch(tbl_ini_lookup[i_sect].param[i].type)
					{
					case TYPE_BYTE:
					case TYPE_WORD:
					case TYPE_DWORD:
						if(tbl_ini_lookup[i_sect].param[i].type == TYPE_BYTE)
							{
							u32data=*tbl_ini_lookup[i_sect].param[i].val;
							}
						else if(tbl_ini_lookup[i_sect].param[i].type == TYPE_WORD)
							{
							u32data=*(uint16_t *)tbl_ini_lookup[i_sect].param[i].val;
							}
						else
							{
							u32data=*(uint32_t *)tbl_ini_lookup[i_sect].param[i].val;
							}
						printf("%u;",u32data);
						break;
					case TYPE_CAN:
						p = tbl_ini_lookup[i_sect].param[i].val;
						printf_hex_data(p, 8);
						break;
					/*
					case TYPE_STR:
						memset(buffer,0,sizeof(buffer));
						memcpy(buffer,tbl_id_lookup[i].val,32);
						rt_kprintf("\"%s\";",buffer);
						break;
					*/
					default :
						memset(buffer,0,sizeof(buffer));
						memcpy(buffer,tbl_ini_lookup[i_sect].param[i].val,tbl_ini_lookup[i_sect].param[i].type);
						printf("\"%s\";",buffer);
						break;
					}
				}
			}
		}
	}
	printf("\r\n" );
}



/*********************************************************************************
  *函数名称:void param_out(char * id)
  *功能描述:将ini文件参数输出到标准输出设备，如果参数id为空则输出所有参数
  *输	入: id		:要输出的参数名称，不包含前面的"id_"
  *输	出: 从标准输出输出参数
  *返 回 值:none
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-08	创建
*********************************************************************************/
void param_out(char * id)
{
	param_out_ex("",id);
}


/*********************************************************************************
  *函数名称:int param_set(char * id,char * value)
  *功能描述:修改ini文件中的一个参数，并保存INI文件
  *输	入: section	:ini文件参数对应的的section
 			id		:要修改的参数名称，不包含前面的"id_"
  			value	:参数值，字符串格式
  *输	出: none
  *返 回 值: 0: ok save ,1:not find id
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-08	创建
*********************************************************************************/
int param_set_ex(char * section, char * id,char * value)
{
	int		i,i_sect;
	uint32_t u32data;
	uint8_t *p;
	for( i_sect = 0; i_sect < sizeof( tbl_ini_lookup ) / sizeof( struct _st_tbl_ini_lookup ); i_sect++ )
	{
		if(strncmp(section,tbl_ini_lookup[i_sect].section,strlen(tbl_ini_lookup[i_sect].section)) == 0)
		{
			for( i = 0; i < tbl_ini_lookup[i_sect].param_size / sizeof( st_tbl_id_lookup ); i++ )
			{
				if(strncmp( id ,tbl_ini_lookup[i_sect].param[i].id, strlen(tbl_ini_lookup[i_sect].param[i].id) ) == 0)
				{
					switch(tbl_ini_lookup[i_sect].param[i].type)
					{
					case TYPE_BYTE:
						printf("TYPE_BYTE val\n");
					case TYPE_WORD:
						printf("TYPE_WORD val\n");
					case TYPE_DWORD:
						printf("TYPE_DWORD val\n");
						if( sscanf(value,"%d",&u32data) == 1)
							{
							if(tbl_ini_lookup[i_sect].param[i].type == TYPE_BYTE)
								{
								*tbl_ini_lookup[i_sect].param[i].val = u32data;
								}
							else if(tbl_ini_lookup[i_sect].param[i].type == TYPE_WORD)
								{
								*(uint16_t *)tbl_ini_lookup[i_sect].param[i].val = u32data;
								}
							else
								{
								*(uint32_t *)tbl_ini_lookup[i_sect].param[i].val = u32data;
								}
							}
						break;
					case TYPE_CAN:
						if(strlen(value) < 16)
							{
							p = tbl_ini_lookup[i_sect].param[i].val;
							memset(tbl_ini_lookup[i_sect].param[i].val,0,8);
							Ascii_To_Hex(p,value,8);
							//strncpy( (char*)p, (char*)value, strlen(value) );
							}
						break;
					/*
					case TYPE_STR:
						if(strlen(value) < 32)
							{
							p = tbl_id_lookup[i].val;
							memset(tbl_id_lookup[i].val,0,32);
							strncpy( (char*)p, (char*)value, strlen(value) );
							}
						break;
					*/
					default :
						printf("string val,i_sect=%d\n",i_sect);
						if(strlen(value) < tbl_ini_lookup[i_sect].param[i].type)
							{
							printf("string val_2\n");
							p = tbl_ini_lookup[i_sect].param[i].val;
							memset(tbl_ini_lookup[i_sect].param[i].val,0,tbl_ini_lookup[i_sect].param[i].type);
							strncpy( (char*)p, (char*)value, strlen(value) );
							
							printf("string_id=%s,string=%s\n",tbl_ini_lookup[i_sect].param[i].id,tbl_ini_lookup[i_sect].param[i].val);
							}
						break;
					}
					write_profile_string(tbl_ini_lookup[i_sect].section, id, value, DF_PARAM_INI_FILE);
					return 0;
				}
			}
		}
	}
	return  1;
}


/*********************************************************************************
  *函数名称:int param_set(char * id,char * value)
  *功能描述:修改ini文件中的一个参数，并保存INI文件
  *输	入: id		:要修改的参数名称，不包含前面的"id_"
  			value	:参数值，字符串格式
  *输	出: none
  *返 回 值: 0: ok save ,1:not find id
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-08	创建
*********************************************************************************/
int param_set(char * id,char * value)
{
	return param_set_ex(DF_PARAM_INI_SECT_GPS_PARAM,id,value);
}


/*********************************************************************************
  *函数名称:int param_save_ex(char * section)
  *功能描述:将所有当前参数保存
  *输	入: section	:ini文件参数对应的的section
  *输	出: none
  *返 回 值:  0:成功，非0:表示失败， -1 表示打开文件失败
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-08	创建
*********************************************************************************/
int param_save_ex(char * section)
{
	int		i,i_sect;
	const char *file =DF_PARAM_INI_FILE;
	char write_val[BUF_SIZE]={0};
	char buf[MAX_FILE_SIZE]={0};
	int age;
	int file_size;
	int ret_val = 0;
	long temp_val;
	u32		ul_data;
	
	if(load_ini_file(file,buf,&file_size))
		return -1;

	for( i_sect = 0; i_sect < sizeof( tbl_ini_lookup ) / sizeof( struct _st_tbl_ini_lookup ); i_sect++ )
	{
		if((strlen(section) == 0)||(strncmp(section,tbl_ini_lookup[i_sect].section,strlen(tbl_ini_lookup[i_sect].section)) == 0))
		{
			for( i = 0; i < tbl_ini_lookup[i_sect].param_size / sizeof( st_tbl_id_lookup ); i++ )
			{
				memset(write_val,0,sizeof(write_val));
				switch(tbl_ini_lookup[i_sect].param[i].type)
				{
					case TYPE_BYTE:
					{
						ul_data=*(uint8_t *)tbl_ini_lookup[i_sect].param[i].val;
					}
					case TYPE_WORD:
					{
						ul_data=*(uint16_t *)tbl_ini_lookup[i_sect].param[i].val;
					}
					case TYPE_DWORD:
					{
						ul_data=*(uint32_t *)tbl_ini_lookup[i_sect].param[i].val;
						sprintf(write_val,"%d",ul_data);
						break;
					}
					case TYPE_CAN:
					{
						Hex_To_Ascii(write_val, tbl_ini_lookup[i_sect].param[i].val, 8);
						break;
					}
					/*
					case TYPE_STR:
					{
						memset(buffer,0,sizeof(buffer));
						memcpy(buffer,tbl_id_lookup[i].val,32);
						rt_kprintf("\"%s\";",buffer);
						break;
					}
					*/
					default :
					{
						memcpy(write_val,tbl_ini_lookup[i_sect].param[i].val,tbl_ini_lookup[i_sect].param[i].type);
						break;
					}
				}
				write_profile_string_buf(tbl_ini_lookup[i_sect].section,tbl_ini_lookup[i_sect].param[i].id,write_val,buf);
			}
		}
	}
	return save_ini_file(file,buf);
}


/*********************************************************************************
  *函数名称:int param_save(void)
  *功能描述:将所有当前参数保存
  *输	入: none
  *输	出: none
  *返 回 值:  0:成功，-1:表示失败
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-08	创建
*********************************************************************************/
int param_save(void)
{
	return param_save_ex( DF_PARAM_INI_SECT_GPS_PARAM );
}



/*********************************************************************************
  *函数名称:int param_load(void)
  *功能描述:将ini文件参数加载进来，如果为空则创建文件，如果没有某一个参数，则添加该参数
  *输	入: none
  *输	出: none
  *返 回 值:  0:成功读取，大于0:读取时发现需要更新，-1:表示失败
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-08	创建
*********************************************************************************/
int param_load(void)
{
	int		i,i_sect;
	const char *file =DF_PARAM_INI_FILE;
	char read_val[BUF_SIZE]={0};
	char buf[MAX_FILE_SIZE]={0};
	int age;
	int file_size;
	int ret_val = 0;
	long temp_val;
	
	memset(buf,0,sizeof(buf));
	load_ini_file(file,buf,&file_size);
	
	for( i_sect = 0; i_sect < sizeof( tbl_ini_lookup ) / sizeof( struct _st_tbl_ini_lookup ); i_sect++ )
	{
		for( i = 0; i < tbl_ini_lookup[i_sect].param_size / sizeof( st_tbl_id_lookup ); i++ )
		{
			//printf("%s\n",tbl_id_lookup[i].id);
			memset(read_val,0,sizeof(read_val));
			ret_val += read_profile_write_buf(tbl_ini_lookup[i_sect].section, tbl_ini_lookup[i_sect].param[i].id,read_val, 
				BUF_SIZE,tbl_ini_lookup[i_sect].param[i].default_val,buf);
			if(tbl_ini_lookup[i_sect].param[i].type <= TYPE_DWORD)
			{
				temp_val = atoi(read_val);
				memcpy(tbl_ini_lookup[i_sect].param[i].val,(uint8_t *)&(temp_val),tbl_ini_lookup[i_sect].param[i].type);
			}
			else if(tbl_ini_lookup[i_sect].param[i].type != TYPE_CAN)
			{
				memcpy(tbl_ini_lookup[i_sect].param[i].val,(uint8_t *)(read_val),tbl_ini_lookup[i_sect].param[i].type);
			}
			else
			{
				Ascii_To_Hex(tbl_ini_lookup[i_sect].param[i].val,read_val,8);
			}
		}
	}
	if(ret_val)
	{
		save_ini_file(file,buf);
		printf("****************存储数据更新!***********************\n");
	}
	return ret_val;
}


int param_proc(void)
{
	char c_buf[64];
	param_load();
	param_out("");
	//sleep(1);
	sprintf(c_buf,"cat %s",DF_PARAM_INI_FILE);
	system(c_buf);
	return 0;
}

////////////////////////////////////////////////////////////////数据类型定义结束

#include<stdio.h>
#include<stdlib.h>
#include <stdint.h>

#include "jt808_head.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if 0
typedef struct _gps_baseinfo
{
	uint32_t	alarm;
	uint32_t	status;
	uint32_t	latitude;                       /*纬度*/
	uint32_t	longitude;                      /*精度*/
	uint16_t	altitude;
	uint16_t	speed_10x;                      /*对地速度 0.1KMH*/
	uint16_t	cog;                            /*对地角度*/
	uint8_t		datetime[6];                    /*BCD格式*/
	uint8_t		mileage_id;                     /*附加消息_里程ID*/
	uint8_t		mileage_len;                    /*附加消息_里程长度*/
	uint32_t	mileage;                   		/*附加消息_里程*/
	uint8_t     oil_id;
	uint8_t		oil_len;
	uint16_t	oil;
	uint8_t		pulse_id;
	uint8_t		pulse_len;
	uint16_t	pulse;
	uint8_t		manalarm_id;
	uint8_t		manalarm_len;
	uint16_t	manalarm;
	uint8_t		exspeed_id;
	uint8_t		exspeed_len;
	uint8_t		exspeed[5];
	uint8_t		region_id;
	uint8_t		region_len;
	uint8_t		region[6];
	uint8_t		road_id;
	uint8_t		road_len;
	uint8_t		road[7];
	uint8_t		carsignal_id;                    
	uint8_t		carsignal_len;                    
	uint32_t	carsignal;
	uint8_t     io_id;
	uint8_t		io_len;
	uint16_t	io;
	uint8_t		analog_id;                    
	uint8_t		analog_len;                    
	uint32_t	analog;
	uint8_t		csq_id;                     	/*附加消息_无线通信网ID*/
	uint8_t		csq_len;                    	/*附加消息_无线通信网长度*/
	uint8_t		csq;                   			/*附加消息_无线通信网信号强度*/
	uint8_t		NoSV_id;                     	/*附加消息_定位卫星数量ID*/
	uint8_t		NoSV_len;                    	/*附加消息_定位卫星数量长度*/
	uint8_t		NoSV;                   		/*附加消息_定位卫星数量*/
	uint8_t     data[];
}gps_baseinfo;

typedef struct _gps_info_save
{
	uint32_t	magic;
	uint32_t	gpsid;
	uint8_t		flag;
	gps_baseinfo data;
}gps_info_save;
#endif

gps_contrl data_contrl={0};


//存储方法是将现有信息追加到文件的尾部
int gps_save(char *path ,const void* buffer, size_t size, size_t count)
{
	int result;
	static uint8_t once =0;
	int maxid =0;
	FILE *fp=NULL;
	if ( (fp=fopen(path,"a+"))==NULL )  
	{  
    	JT808_PRT("fail to open file\n");
   	 	return 1 ;  
	}
	#if 0
	if(0==once)
	{
		maxid=gps_maxid(path);
		if(maxid!=-1)
		{
			memcpy(buffer,&maxid,sizeof(maxid));
		}
		else
		{
			//相当于buffer前四个字节为0
			memset(buffer,0,4);
		}
		once =1;
	}
	#endif
	result =fwrite(buffer,size, count, fp);
	if(result != count)
	{
		return 1;
	}
	fclose(fp);
	return 0;
}
int gps_read(char *path,void *buffer,size_t size, size_t count,int*num)
{
	int res;
	FILE *fp=NULL;
	if ( (fp=fopen(path,"rb+"))==NULL )  
	{  
    	JT808_PRT("fail to open file\n");
   	 	return 1;  
	}
	if(*num==-1)
	{
		return 1;		
	}
	//*num= (*num)*(sizeof(gps_info_save));
	fseek(fp ,*num, SEEK_SET);
	res = fread( buffer, size, count, fp) ;
	if (res != count)
	{
		return 1;
	}
	fclose(fp);
	return 0;
}


int gps_update(char *path ,int* offset)
{
	FILE *fp=NULL;
	uint8_t reported =0x7f;
	if ( (fp=fopen(path,"rb+"))==NULL )  
	{  
    	JT808_PRT("fail to open file\n");        
   	 	return 1 ;  
	}
	//改变文件的指针到记录上报标志的位置
	if(fseek(fp,(*offset)+4, SEEK_SET)==0)
	{	
		fwrite(&reported,sizeof(reported), 1, fp);
		*offset = *offset +sizeof(gps_info_save);
		JT808_PRT("offset = %d\n",*offset);
	
	}
	fclose(fp);
	return 0;
}

int gps_datacount(char *path,int *numread,int *numwrite)
{
	int res;
	int read,write;
	gps_info_save *tmpdata;
	FILE *fp=NULL;
	read=write=0;
	if ( (fp=fopen(path,"rb"))==NULL )  
	{  
    	JT808_PRT("fail to open file\n");
   	 	return -1 ;  
	}
	tmpdata = malloc(sizeof(gps_info_save));
	
	while(1)
	{
		memset(tmpdata,0,sizeof(gps_info_save));
		res = fread(tmpdata,sizeof(gps_info_save),1,fp);
		JT808_PRT("RLEN =%d\n",sizeof(gps_info_save));
		outprint_hex("rgps",tmpdata,sizeof(gps_info_save));
		JT808_PRT("tmpdata->flag = %x\n",tmpdata->flag);
		if(1==res)
		{
			//had reported
			if(tmpdata->flag==0x7f)
			{
				read = read+sizeof(gps_info_save);
				
			}
			//find unreported data
			if(tmpdata->flag==0xff)
			{
				*numread = read;
				fclose(fp);
				free(tmpdata);
				JT808_PRT("readoffset=%d \n",read);
				return 0;
			}
		}
		//no data or no unreported data;
		else
		{		
				if(read ==0)
				{
					read = read-1;
					*numread = read;
				}
				/*
				if(write == 0)
				{
					write = write-1;
					*numread = write;
				}
				*/
				fclose(fp);
				free(tmpdata);
				JT808_PRT("readoffset=%d \n",read);
				return 0;
		}
		//fseek(fp ,count, SEEK_SET);
	}
	fclose(fp);
	
}

int gps_maxid(char *path)
{
	FILE *fp=NULL;
	int maxid=0;
	int result;
	if ( (fp=fopen(path,"rb"))==NULL )  
	{  
    	JT808_PRT("fail to open file\n");
		
   	 	return -1 ;  
	}
	result = fseek(fp,-(sizeof(gps_info_save)),SEEK_END);
	if(result!=0)
	{
		JT808_PRT("fseek error \n");
		return -1;
	}
	result = fread(&maxid,sizeof(maxid),1,fp);
	if(result!=1)
	{
		return -1;
	}
	else
	{
		maxid= maxid+1;
	}
	return maxid;
}
//bpath是指的备份的路径，path指的现在的路径
int gps_data_move(char *bpath,char*path,int offset)
{
	int r_pstr,w_pstr,number;
	gps_info_save *data;
	r_pstr =0;
	w_pstr=0;
	number = 0;
	data =	NULL;
	//diao yong jiao ben 
	system("/mnt/sharefile/ban.sh");
	//gps_datacount(bpath,&r_pstr,&w_pstr);
	r_pstr = offset;
	JT808_PRT("r_pstr =%d\n",r_pstr);
	data = malloc(sizeof(gps_info_save));
	
	while(1)
	{
		if(gps_read(bpath,data,sizeof(gps_info_save), 1,&r_pstr)==0)
		{
			r_pstr= r_pstr+sizeof(gps_info_save);
			memcpy(data,&number,sizeof(number));
			outprint_hex("banyi",data,sizeof(gps_info_save));
			gps_save(path ,data, sizeof(gps_info_save), 1);
			number++;
			JT808_PRT("ban yi ci shu %d\n",number);
			//memset(data+4,0,sizeof(gps_info_save)-4);
		}
		else
		{
			break;
		}
	}
	w_pstr = number;
	free(data);
	return w_pstr;
}

#if 0
int main(void)
{
	int write,read;
	int res;  
	int i = 0;  
	write =read=0;
	uint32_t data = 255;
	gps_baseinfo *gps_info;
	
	if ( (fp=fopen("savefile.txt","a+"))==NULL )  
    {  
        printf("fail to open file\n");        
        return 0 ;  
    } 

	for(i=0; i<10; i++)
	{
		printf("len = %d\n",sizeof(gps_baseinfo));
		gps_info = malloc(sizeof(gps_baseinfo));
		gps_info->alarm = i+10;
		res = save_gps(gps_info,sizeof(gps_baseinfo),1);
		if(res != 0)
		{
			printf("存储数据失败");
		}
		free(gps_info);
		//write = write +sizeof(gps_baseinfo);
		//fseek(fp, write, SEEK_SET);
		
	}
	fclose(fp);
	if ( (fp=fopen("savefile.txt","rb+"))==NULL )  
    	{  
        	printf("fail to open file\n");        
       	 	return 0 ;  
    	} 
	
	if(fseek(fp, 400, SEEK_SET)==0)
	{	
		fwrite(&data,sizeof(data), 1, fp);
		printf("zhi xing cheng gong \n");
	}
	#if 1 
	fseek(fp ,200, SEEK_SET);
	for(i=0;i<7;i++)
	{
		gps_info = malloc(sizeof(gps_baseinfo));
		read_gps(gps_info,sizeof(gps_baseinfo),1);
		if(gps_info->alarm == data)
		{
			printf("du dao xiu gai shu ju \n");
		}
		outprint_hex("read", gps_info, sizeof(gps_baseinfo) );
		free(gps_info);
	}
	#endif
	return 0;
}
#endif


#ifdef __cplusplus
}; //end of extern "C" {
#endif


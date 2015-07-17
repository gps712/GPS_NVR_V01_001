#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

#include "jt808_head.h"

#define TRUE 1
#define BUFSIZE 512
static uint8_t last_ch = 0;
static uint16_t buffwr = 0;

#ifdef __cplusplus
extern "C"
{
#endif	

#define MAXNUMNER	300


static pthread_t gps_pid;
static int fd;
gps_baseinfo gpsdata;
uint8_t		gps_hextime[6];



void setTermios(struct termios *pNewtio, int uBaudRate)

{
/* clear struct for new port settings */
	bzero(pNewtio, sizeof(struct termios));
	//8N1
	pNewtio->c_cflag = uBaudRate | CS8 | CREAD | CLOCAL;
	pNewtio->c_iflag = IGNPAR;
	pNewtio->c_oflag = 0;
	pNewtio->c_lflag = 0; //non ICANON

/*
initialize all control characters
*/
	pNewtio->c_cc[VINTR] = 0; /* Ctrl-c */
	pNewtio->c_cc[VQUIT] = 0; /* Ctrl-\ */
	pNewtio->c_cc[VERASE] = 0; /* del */
	pNewtio->c_cc[VKILL] = 0; /* @ */
	pNewtio->c_cc[VEOF] = 4; /* Ctrl-d */
	pNewtio->c_cc[VTIME] = 5; /* inter-character timer, timeout VTIME*0.1 */
	pNewtio->c_cc[VMIN] = 0; /* blocking read until VMIN character arrives */
	pNewtio->c_cc[VSWTC] = 0; /* '\0' */
	pNewtio->c_cc[VSTART] = 0; /* Ctrl-q */
	pNewtio->c_cc[VSTOP] = 0; /* Ctrl-s */
	pNewtio->c_cc[VSUSP] = 0; /* Ctrl-z */
	pNewtio->c_cc[VEOL] = 0; /* '\0' */
	pNewtio->c_cc[VREPRINT] = 0; /* Ctrl-r */
	pNewtio->c_cc[VDISCARD] = 0; /* Ctrl-u */
	pNewtio->c_cc[VWERASE] = 0; /* Ctrl-w */
	pNewtio->c_cc[VLNEXT] = 0; /* Ctrl-v */
	pNewtio->c_cc[VEOL2] = 0; /* '\0' */

}

static void * gps_uart(void *arg)

{
	
	int nread;
	char buff[BUFSIZE];
	struct termios oldtio, newtio;
	struct timeval tv;
	uint8_t ch;
	char *dev = "/dev/ttyAMA1";
	fd_set rfds;
	if ((fd = open(dev, O_RDWR | O_NOCTTY)) < 0)
	{
		printf("err: can't open serial port!\n");
		return  - 1;
	} tcgetattr(fd, &oldtio); /* save current serial port settings */
	setTermios(&newtio, B115200);
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);
	//Non-blocking mode,Timeout wait 5 seconds
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	while (TRUE)
	{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		if (select(1+fd, &rfds, NULL, NULL, &tv) > 0)
		{
			if (FD_ISSET(fd, &rfds))
			{
				//ioctl(fd, FIONREAD, &nread);
				 nread = read(fd, &ch, 1);
				//first get 0x0d then 0x0a;
				if ((ch == 0x0a) && (last_ch == 0x0d))
				{
					buff[buffwr++] = ch;
					if (buffwr < 124)
					{
						//JT808_PRT("readlength=%d\n", buffwr);
						buff[buffwr] = '\0';
						//JT808_PRT("%s\n", buff);
						gps_rx( buff, buffwr);
					}
					buffwr = 0;
				}
				else
				{
					// 1. get  head char
					if (ch == '$')
					{
						buffwr = 0;
					}
					// 2.  judge  head char	
					if (buff[0] != '$')
					// add later
					{
						buffwr = 0;
					}
					// 3.  rx data  	
					buff[buffwr++] = ch;
					if (buffwr == BUFSIZE)
					{
						buffwr = 0;
					}
					buff[buffwr] = 0;
				}
				last_ch = ch;
			}

		}

	}
	tcsetattr(fd, TCSANOW, &oldtio);
	close(fd);
}

int gps_thread(void)
{
	int res;
	res = pthread_create(&gps_pid, 0, gps_uart, NULL);	
	if( res != DF_SUCCESS )
	{
		JT808_PRT("gps_thread creat failed\n");
		return 1;
	}
	
}
int gps_thread_join(void)
{
   
	pthread_join(gps_pid, 0);
    return DF_SUCCESS;
}

int gps_write(uint8_t *pstr)
{
	int result;

	if(strlen(pstr)!=0)
	{
		result =write(fd, pstr, strlen(pstr));
		if(result == strlen(pstr))
		{
			JT808_PRT("gps write data sucess \n");
			return 0;
		}
		else
		{
			JT808_PRT("gps write data failed \n");
		}
	}
}
/*********************************proce Original data**************************/

/*
   $GNRMC,074001.00,A,3905.291037,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E
   $GNTXT,01,01,01,ANTENNA OK*2B7,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E
   $GNGGA,074002.00,3905.291085,N,11733.138264,E,1,11,0.9,8.2,M,-1.6,M,,,1.4*68E
   $GNGLL,3905.291085,N,11733.138264,E,074002.00,A,0*02.9,8.2,M,-1.6,M,,,1.4*68E
   $GPGSA,A,3,18,05,08,02,26,29,15,,,,,,,,,,,,,,,,,,,,,,,,,,1.6,0.9,1.4,0.9*3F8E
   $BDGSA,A,3,04,03,01,07,,,,,,,,,,,,,,,,,,,,,,,,,,,,,1.6,0.9,1.4,0.9*220.9*3F8E
   $GPGSV,2,1,7,18,10,278,29,05,51,063,08,21,052,24,02,24,140,45*4C220.9*3F8E
   $GPGSV,2,2,7,26,72,055,24,29,35,244,37,15,66,224,37*76,24,140,45*4C220.9*3F8E
   $BDGSV,1,1,4,04,27,124,38,03,42,190,34,01,38,146,37,07,34,173,35*55220.9*3F8E

   返回处理的字段数，如果正确的话
 */
//这个函数肯定要拆分 圈度复杂度49
static uint8_t process_rmc( uint8_t * pinfo )
{
	//检查数据完整性,执行数据转换
	uint8_t		year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0, fDateModify = 0;
	uint32_t	degrees, minutes;
	uint8_t		commacount = 0, count = 0;

	uint32_t	lati		= 0, longi = 0;
	uint16_t	speed_10x	= 0;
	uint16_t	cog			= 0;    /*course over ground*/
	static long int starttime =0;

	uint8_t		i;
	uint8_t		buf[22];
	uint8_t		*psrc = pinfo + 6;  /*指向开始位置 $GNRMC,074001.00,A,3905.291037,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E*/

/*因为自增了一次，所以从pinfo+6开始*/
	while( *psrc++ )
	{
		if( *psrc != ',' )
		{
			buf[count++]	= *psrc;
			buf[count]		= '0';
			buf[count+1]	= 0;
			if(count+2 >= sizeof(buf))
				return 1;
			continue;
		}

		commacount++;
		switch( commacount )
		{
			case 1: /*时间*/
				if( count < 6 )
				{
					return 1;
				}

				i = ( buf[0] - 0x30 ) * 10 + ( buf[1] - 0x30 ) + 8;
				if( i > 23 )
				{
					fDateModify = 1;
					i			-= 24;
				}
				/*转成HEX格式*/
				hour	= i;
				min		= ( buf[2] - 0x30 ) * 10 + ( buf[3] - 0x30 );
				sec		= ( buf[4] - 0x30 ) * 10 + ( buf[5] - 0x30 );
				break;
			case 2:                         /*A_V*/
				if( buf[0] != 'A' )         /*未定位*/
				{
					gpsdata.status &= ~BIT_STATUS_FIXED;
					return 2;
				}
				break;
			case 3: /*纬度处理ddmm.mmmmmm*/
				if( count < 9 )
				{
					return 3;
				}

				degrees = ( ( buf [0] - 0x30 ) * 10 + ( buf [1] - 0x30 ) ) * 1000000;
				minutes = ( buf [2] - 0x30 ) * 1000000 +
				          ( buf [3] - 0x30 ) * 100000 +
				          ( buf [5] - 0x30 ) * 10000 +
				          ( buf [6] - 0x30 ) * 1000 +
				          ( buf [7] - 0x30 ) * 100 +
				          ( buf [8] - 0x30 ) * 10 +
				          ( buf [9] - 0x30 );   /*多加了一个位，想要保证精度*/
				lati = degrees + minutes / 6;
				gpsdata.latitude	= BYTESWAP4( lati );
				break;
			case 4:                             /*N_S处理*/
				if( buf[0] == 'N' )
				{
					gpsdata.status &= ~BIT_STATUS_NS;
				} else if( buf[0] == 'S' )
				{
					gpsdata.status |= BIT_STATUS_NS;
				}else
				{
					return 4;
				}
				break;
			case 5: /*经度处理*/
				if( count < 10 )
				{
					return 5;
				}
				degrees = ( ( buf [0] - 0x30 ) * 100 + ( buf [1] - 0x30 ) * 10 + ( buf [2] - 0x30 ) ) * 1000000;
				minutes = ( buf [3] - 0x30 ) * 1000000 +
				          ( buf [4] - 0x30 ) * 100000 +
				          ( buf [6] - 0x30 ) * 10000 +
				          ( buf [7] - 0x30 ) * 1000 +
				          ( buf [8] - 0x30 ) * 100 +
				          ( buf [9] - 0x30 ) * 10 +
				          ( buf [10] - 0x30 );
				longi = degrees + minutes / 6;
				
				gpsdata.longitude	= BYTESWAP4( longi );
				break;
			case 6: /*E_W处理*/
				if( buf[0] == 'E' )
				{
					gpsdata.status &= ~BIT_STATUS_EW;
					
				} else if( buf[0] == 'W' )
				{
					gpsdata.status |= BIT_STATUS_EW;
					
				}else
				{
					return 6;
				}
				break;
			case 7: /*速度处理 */
				speed_10x = 0;
				for( i = 0; i < count; i++ )
				{
					if( buf[i] == '.' )
					{
						if(i+1 < count)
							speed_10x += ( buf[i + 1] - 0x30 );
						break;
					}else
					{
						speed_10x	+= ( buf[i] - 0x30 );
						speed_10x	= speed_10x * 10;
					}
				}
				/*当前是0.1knot => 0.1Kmh  1海里=1.852Km  1852=1024+512+256+32+16+8+4*/
				//计算的是千米每小时
				speed_10x *= 1.852;
				//i=speed_10x;
				//speed_10x=(i<<10)|(i<<9)|(i<<8)|(i<<5)|(i<<4)|(i<<3)|(i<<2);
				//speed_10x/=1000;
				gpsdata.speed_10x	= BYTESWAP2( speed_10x );
				break;

			case 8: /*方向处理*/
				cog = 0;
				for( i = 0; i < count; i++ )
				{
					if( buf[i] == '.' )
					{
						break;
					}
					else
					{
						cog = cog * 10;
						cog += ( buf[i] - 0x30 );
						gpsdata.cog		= BYTESWAP2( cog );
					}
				}
				break;

			case 9: /*日期处理*/
				if( count < 6 )
				{
					return 9;
				}

				day		= ( ( buf [0] - 0x30 ) * 10 ) + ( buf [1] - 0x30 );
				mon		= ( ( buf [2] - 0x30 ) * 10 ) + ( buf [3] - 0x30 );
				year	= ( ( buf [4] - 0x30 ) * 10 ) + ( buf [5] - 0x30 );

				if( fDateModify )
				{
					day++;
					if( mon == 2 )
					{
						if( ( year % 4 ) == 0 ) /*没有考虑整百时，要被400整除，NM都2100年*/
						{
							if( day == 30 )
							{
								day = 1; mon++;
							}
						} else
						if( day == 29 )
						{
							day = 1; mon++;
						}
					} else
					if( ( mon == 4 ) || ( mon == 6 ) || ( mon == 9 ) || ( mon == 11 ) )
					{
						if( day == 31 )
						{
							mon++; day = 1;
						}
					} else
					{
						if( day == 32 )
						{
							mon++; day = 1;
						}
						if( mon == 13 )
						{
							mon = 1; year++;
						}
					}
				}

				/*都处理完了更新 gps_baseinfo,没有高程信息*/
				gps_hextime[0] = year;
				gps_hextime[1] = mon;
				gps_hextime[2] = day;
				gps_hextime[3] = hour;
				gps_hextime[4] = min;
				gps_hextime[5] = sec;
				gpsdata.datetime[0]	= HEX2BCD( year );
				gpsdata.datetime[1]	= HEX2BCD( mon );
				gpsdata.datetime[2]	= HEX2BCD( day );
				gpsdata.datetime[3]	= HEX2BCD( hour );
				gpsdata.datetime[4]	= HEX2BCD( min );
				gpsdata.datetime[5]	= HEX2BCD( sec );
				gpsdata.status |= BIT_STATUS_FIXED;
				/*
				if((interval_time(10,starttime)>0)&&())
				{
					gps_data_filled(&gpsdata);
					starttime = now_time();
					jt808_tx_gpsdata();
					JT808_PRT("cun chu shu ju \n");
				}
				*/
				return 0;
		}	
				count	= 0;
				bzero(buf,sizeof(buf));
	}
	return 10;
}
	
/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t process_gga( uint8_t * pinfo )
{
	//检查数据完整性,执行数据转换
	uint8_t 	NoSV = 0;
	uint8_t 	i;
	uint8_t 	buf[20];
	uint8_t 	commacount	= 0, count = 0;
	uint8_t 	*psrc		= pinfo + 7; //指向开始位置
	uint16_t	altitute;

	while( *psrc++ )
	{
		if( *psrc != ',' )
		{
			buf[count++]	= *psrc;
			buf[count]		= 0;
			if(count+1 >= sizeof(buf))
				return 1;
			continue;
		}
		commacount++;
		switch( commacount )
		{
			case 1: /*时间处理 */
				if( count < 6 )
				{
					gpsdata.NoSV = NoSV;
					return 1;
				}
				break;

			case 2: /*纬度处理ddmm.mmmmmm*/
				break;

			case 3: /*N_S处理*/
				break;

			case 4: /*经度处理*/

				break;
			case 5: /*E_W处理*/
				break;
			case 6: /*定位类型*/
				break;
			case 7: /*NoSV,卫星数*/
				/*
				if( count < 1 )
				{
					break;
				}
				*/
				NoSV = 0;
				for( i = 0; i < count; i++ )
				{
					NoSV	= NoSV * 10;
					NoSV	+= ( buf[i] - 0x30 );
				}
				gpsdata.NoSV = NoSV;
				break;
			case 8: /*HDOP*/
				break;

			case 9: /*MSL Altitute*/
				if( count < 1 )
				{
					break;
				}
				/*
				$GNRMC,102641.000,A,3955.2964,N,11600.0003,E,29.0,180.179,050313,,E,A*0A
				$GNGGA,100058.000,4000.0001,N,11600.0001,E,1,09,0.898,54.3,M,0.0,M,,0000,1.613*49
				*/
				altitute = 0;
				for( i = 0; i < count; i++ )
				{
					if( buf[i] == '.' )
					{
						break;
					}
					altitute	= altitute * 10;
					altitute	+= ( buf[i] - '0' );
				}
				gpsdata.altitude	= BYTESWAP2( altitute );
				return 0;
		}
		count	= 0;
		buf[0]	= 0;
	}
	return 9;
}



void gps_rx( uint8_t * pinfo, uint16_t length )
{
	uint8_t ret;
	uint16_t i;
	char	* psrc;
	psrc = (char*)pinfo;
	if(length < 6)
		return;
	/*保存RAW数据*/
	//jt808_gps_pack( (char*)pinfo, length );
	gpsdata.status &= ~0X003C0000;
	if( strncmp( psrc + 3, "GGA,", 4 ) == 0 )
	{
		if( strncmp( psrc + 1, "GN", 2 ) == 0 )
		{
			gpsdata.status |= BIT_STATUS_BD;
		}else if( strncmp( psrc + 1, "GP", 2 ) == 0 )
		{
			gpsdata.status |= BIT_STATUS_GPS;;
		}else if( strncmp( psrc + 1, "BD", 2 ) == 0 )
		{
			gpsdata.status |= BIT_STATUS_GPS|BIT_STATUS_GPS;
		}
		process_gga( (uint8_t*)psrc );
	}
	if( strncmp( psrc + 3, "RMC,", 4 ) == 0 )
	{	
		//去掉后面的不可见字符
		for(i=length-1;i>0;i--)
			{
			if(pinfo[i] > 0x1F)
				break;
			else
				{
				pinfo[i] = 0;
				}
			}
		ret = process_rmc( (uint8_t*)psrc );
		//JT808_PRT("ret %d\n",ret);
	}	
}

int gps_data_filled(gps_baseinfo *pstr)
{
	gps_info_save *tmpdata = NULL;
	
	if(pstr !=NULL)
	{
		tmpdata = malloc(sizeof(gps_info_save));
		memset(tmpdata,0,sizeof(gps_info_save));
		JT808_PRT("wlen = %d\n",sizeof(gps_info_save));	
		
		tmpdata->flag =0xff;
		memcpy(&(tmpdata->data),pstr,sizeof(gps_baseinfo));
		tmpdata->gpsnum = data_contrl.write_offset;
		gps_save("gps.log" ,tmpdata, sizeof(gps_info_save),1);
		outprint_hex("wgps",tmpdata,sizeof(gps_info_save));
		data_contrl.write_offset= data_contrl.write_offset+1;
		JT808_PRT("gpsnum = %d ,data_contrl.write_offset =%d\n",tmpdata->gpsnum,data_contrl.write_offset);
		if(tmpdata->gpsnum > MAXNUMNER)
		{
			JT808_PRT("shu ju ban yi \n");
			data_contrl.write_offset = gps_data_move("/dback/gps.log","gps.log",data_contrl.read_offset);
			data_contrl.read_offset=0;
			JT808_PRT("data_contrl.write_offset = %d\n",data_contrl.write_offset);
		}
		
		if(data_contrl.read_offset == -1)
		{
			data_contrl.read_offset =0;
			JT808_PRT("kaishi xieru diyi tiaoxinxi\n");
		}
		free(tmpdata);
	}
	return 0;
}

#ifdef __cplusplus
}; //end of extern "C" {
#endif



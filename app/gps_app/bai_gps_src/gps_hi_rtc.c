/*********************************************************************************
  * @文件名称 :gps_hi_rtc.c
  * @功能描述 :所有和RTC相关的函数都在这里
  * @作	   者 :白养民
  * @创建日期 :2015-7-15
  * @文件版本 :0.01
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-15	创建文件
*********************************************************************************/
#ifdef __cplusplus
#if __cplusplus
	extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>

#include "gps_typedef.h"
#include "common_func.h"
#include "gps_hi_rtc.h"


rtc_time_t rtc;
static int s_rtc_fd =  0;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
__inline MYTIME buf_to_mytime( uint8_t *p )
{
	uint32_t ret;
	ret = (uint32_t)( ( *p++ ) << 26 );
	ret |= (uint32_t)( ( *p++ ) << 22 );
	ret |= (uint32_t)( ( *p++ ) << 17 );
	ret |= (uint32_t)( ( *p++ ) << 12 );
	ret |= (uint32_t)( ( *p++ ) << 6 );
	ret |= ( *p );
	return ret;
}

/*
   从buf中获取时间信息
   如果是0xFF 组成的!!!!!!!特殊对待

 */
MYTIME mytime_from_hex( uint8_t* buf )
{
	MYTIME	ret = 0;
	uint8_t *p	= buf;
	if( *p == 0xFF ) /*不是有效的数据*/
	{
		return 0xFFFFFFFF;
	}
	ret = (uint32_t)( ( *p++ ) << 26 );
	ret |= (uint32_t)( ( *p++ ) << 22 );
	ret |= (uint32_t)( ( *p++ ) << 17 );
	ret |= (uint32_t)( ( *p++ ) << 12 );
	ret |= (uint32_t)( ( *p++ ) << 6 );
	ret |= ( *p );
	return ret;
}

/*转换bcd时间*/
MYTIME mytime_from_bcd( uint8_t* buf )
{
	uint32_t year, month, day, hour, minute, sec;
	uint8_t *psrc = buf;
	year	= BCD2HEX( *psrc++ );
	month	= BCD2HEX( *psrc++ );
	day		= BCD2HEX( *psrc++ );
	hour	= BCD2HEX( *psrc++ );
	minute	= BCD2HEX( *psrc++ );
	sec		= BCD2HEX( *psrc );
	return MYDATETIME( year, month, day, hour, minute, sec );
}

/*转换bcd时间*/
unsigned long utc_from_bcd( uint8_t* buf )
{
	uint32_t year, month, day, hour, minute, sec;
	uint8_t *psrc = buf;
	
	year	= BCD2HEX( *psrc++ );
	month	= BCD2HEX( *psrc++ );
	day		= BCD2HEX( *psrc++ );
	hour	= BCD2HEX( *psrc++ );
	minute	= BCD2HEX( *psrc++ );
	sec		= BCD2HEX( *psrc );
	return mytime_to_utc(MYDATETIME( year, month, day, hour, minute, sec ));
}

/*转换为十六进制的时间 例如 2013/07/18 => 0x0d 0x07 0x12*/
void mytime_to_hex( uint8_t* buf, MYTIME time )
{
	uint8_t *psrc = buf;
	*psrc++ = YEAR( time );
	*psrc++ = MONTH( time );
	*psrc++ = DAY( time );
	*psrc++ = HOUR( time );
	*psrc++ = MINUTE( time );
	*psrc	= SEC( time );
}

/*转换为bcd字符串为自定义时间 例如 0x13 0x07 0x12=>代表 13年7月12日*/
void mytime_to_bcd( uint8_t* buf, MYTIME time )
{
	uint8_t *psrc = buf;
	*psrc++ = HEX2BCD( YEAR( time ) );
	*psrc++ = HEX2BCD( MONTH( time ) );
	*psrc++ = HEX2BCD( DAY( time ) );
	*psrc++ = HEX2BCD( HOUR( time ) );
	*psrc++ = HEX2BCD( MINUTE( time ) );
	*psrc	= HEX2BCD( SEC( time ) );
}


/*转换为bcd字符串为自定义时间 例如 0x13 0x07 0x12=>代表 13年7月12日*/
void utc_to_bcd( uint8_t* buf, unsigned long  utc_time )
{
	uint8_t *psrc = buf; 
	MYTIME time;
	time=utc_to_mytime(utc_time);
	*psrc++ = HEX2BCD( YEAR( time ) );
	*psrc++ = HEX2BCD( MONTH( time ) );
	*psrc++ = HEX2BCD( DAY( time ) );
	*psrc++ = HEX2BCD( HOUR( time ) );
	*psrc++ = HEX2BCD( MINUTE( time ) );
	*psrc	= HEX2BCD( SEC( time ) );
}


/*********************************************************************************
  *函数名称:unsigned long mytime_to_utc(MYTIME	time)
  *功能描述:将格式为MYTIME的时间转换为UTC时间
  *输	入:	time	:MYTIME时间
  *输	出:	none
  *返 回 值:unsigned long，表示输出的UTC时间
  *作	者:白养民
  *创建日期:2013-12-18
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
unsigned long mytime_to_utc(MYTIME	time)
{
	unsigned long	utc;
	unsigned int 	year;
	unsigned int 	month;
	unsigned int 	day;
	unsigned int 	hour;
	unsigned int 	minute;
	unsigned int 	sec;

	year	= YEAR(time)+2000;
	month	= MONTH(time);
	day		= DAY(time);
	hour	= HOUR(time);
	minute	= MINUTE(time);
	sec		= SEC(time);
	
	if( 0 >= (int)( month -= 2 ) )    /**//* 1..12 -> 11,12,1..10 */
	{
		month	+= 12;              /**//* Puts Feb last since it has leap day */
		year	-= 1;
	}
	utc = ( ( ( (unsigned long)( year / 4 - year / 100 + year / 400 + 367 * month / 12 + day ) +
					 year * 365 - 719499
					 ) * 24 + hour		/**//* now have hours */
				   ) * 60 + minute		   /**//* now have minutes */
				 ) * 60 + sec;			/**//* finally seconds */
	return utc;
}


/*********************************************************************************
  *函数名称:MYTIME utc_to_mytime(unsigned long utc)
  *功能描述:将utc时间转换为MYTIME时间格式
  *输	入:	utc	:UTC时间
  *输	出:	none
  *返 回 值:MYTIME时间格式的时间
  *作	者:白养民
  *创建日期:2013-12-18
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
MYTIME utc_to_mytime(unsigned long utc)
{
	int i,leapyear,day2;
	uint32_t day1;
	unsigned int 	year;
	unsigned int 	month;
	unsigned int 	day;
	unsigned int 	hour;
	unsigned int 	minute;
	unsigned int 	sec;
	MYTIME 			time;
	
	///10957   10988

	sec		= utc%60;
	minute	= utc%3600/60;
	hour	= utc%86400/3600;
	utc		/= 86400;
	if(utc<10957)
		utc	= 10967;
	year	= 2000 + (utc-10957)/1461*4;		///10957表示为2000年1月1日的UTC天数，1461表示4年为单位的天数，因为从2000年开始计算，所以第一年为366天，后面3年为365天
	day1=(utc-10957)%1461;
	if(day1 >= 366)
		{
		year++;
		day1 -= 366;
		year	+= day1/365;

		day1	%= 365;
		leapyear	= 0;
		}
	else
		{
		leapyear	= 1;
		}
	day2=0;
	for(i=1;i<=12;i++)
 	{
	 	day=Get_Month_Day(i,leapyear);
	 	day2+=day;
		//如果当前月的总天数小于计算天数则得到了月份
		if(day2>day1)
		{
			day2-=day;
			break;
		}
 	}
	month=i;
	day=day1-day2+1;
	time	= MYDATETIME(year-2000, month, day, hour, minute, sec);
	return time;
}


/*********************************************************************************
  *函数名称:int console_param_load( char *p, uint16_t len )
  *功能描述:重新加载所有参数
  *输	入: p		:传递的字符串
  			len		:字符串长度
  *输	出: none
  *返 回 值:	0:OK  非0:错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
/*used for convert time frome string to struct rtc_time_t*/
static int parse_string(char *string, rtc_time_t *p_tm)
{
	char *comma, *head;
	int value[10];
	int i;

	if (!string || !p_tm)
		return -1;

	if (!strchr(string, '/'))
		return -1;

	head = string;
	i = 0;
	comma = NULL;

	for(;;) {	
		comma = strchr(head, '/');

		if (!comma){
			value[i++] = atoi(head);
			break;
		}

		*comma = '\0';
		value[i++] = atoi(head);
		head = comma+1;	
	}
	
	if (i < 5)
		return -1;

	p_tm->year   = value[0];
	p_tm->month  = value[1];
	p_tm->date   = value[2];
	p_tm->hour   = value[3];
	p_tm->minute = value[4];
	p_tm->second = value[5];
	p_tm->weekday = 0;

	return 0;
}


/*********************************************************************************
  *函数名称:int rtc_manage_hi(const char *pstr, int commd)
  *功能描述:读取RTC时间或者设置RTC时间
  *输	入: pstr	:
  			commd	:
  *输	出: none
  *返 回 值:	0:OK  非0:错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-15	创建
*********************************************************************************/
int rtc_manage_hi(const char *pstr, int commd)
{
	//rtc_time_t tm;
	int ret =  - 1;
	int fd =  - 1;
	const char *dev_name = "/dev/hi_rtc";
	char string[50];
	fd = open(dev_name, O_RDWR);
	if (!fd)
	{
		printf("open %s failed\n", dev_name);
		return  - 1;
	}
	if(pstr!=NULL)
	{
		memset(string,0,sizeof(string));
		memcpy(string, pstr,strlen(pstr));
	}
	switch (commd)
	{
		case 1:
			ret = parse_string(string, &rtc);
			if (ret < 0)
			{
				printf("parse time param failed\n");
				goto err1;
			}
			printf("set time\n");
			#if 1	
			printf("year:%d\n", rtc.year);
			printf("month:%d\n", rtc.month);
			printf("date:%d\n", rtc.date);
			printf("hour:%d\n", rtc.hour);
			printf("minute:%d\n", rtc.minute);
			printf("second:%d\n", rtc.second);
			#endif
			ret = ioctl(fd, HI_RTC_SET_TIME, &rtc);
			if (ret < 0)
			{
				printf("ioctl: HI_RTC_SET_TIME failed\n");
				goto err1;
			}
			break;
		case 2:
			ret = ioctl(fd, HI_RTC_RD_TIME, &rtc);
			if (ret < 0)
			{
				printf("ioctl: HI_RTC_RD_TIME failed\n");
				goto err1;
			}
			printf("Current time value: \n");
			printf("year:%d\n", rtc.year);
			printf("month:%d\n", rtc.month);
			printf("date:%d\n", rtc.date);
			printf("hour:%d\n", rtc.hour);
			printf("minute:%d\n", rtc.minute);
			printf("second:%d\n", rtc.second);
			break;
	}
	return 3;
	err1:
    close(fd);

	return 0;
}


/*********************************************************************************
  *函数名称:int rtc_init_time(void)
  *功能描述:初始化RTC，在该函数中可以打开RTC设备
  *输	入: none
  *输	出: none
  *返 回 值:	0:OK，非0表示ERROR
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-15	创建
*********************************************************************************/
int rtc_init_time(void)
{
	//rtc_time_t tm;
	int ret =  - 1;
	const char *dev_name = "/dev/hi_rtc";
	
	s_rtc_fd = open(dev_name, O_RDWR);
	if (!s_rtc_fd)
	{
		printf("open %s failed\n", dev_name);
		return  -1;
	}
	return 0;
}



/*********************************************************************************
  *函数名称:unsigned long rtc_get_time(rtc_time_t *p_time)
  *功能描述:读取RTC时间
  *输	入: p_time	:要返回的时间，如果为NULL则只有函数返回值是当前时间
  *输	出: none
  *返 回 值:UTC时间，如果为0表示读取错误
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-15	创建
*********************************************************************************/
unsigned long rtc_get_time(rtc_time_t *p_time)
{
	//rtc_time_t tm;
	int ret =  - 1;
	int fd =  - 1;
	const char *dev_name = "/dev/hi_rtc";
	rtc_time_t r_time;
	unsigned long	utc = 0;
	unsigned int 	year;
	unsigned int 	month;
	unsigned int 	day;
	unsigned int 	hour;
	unsigned int 	minute;
	unsigned int 	sec;

	if (!s_rtc_fd)
	{
		if(rtc_init_time())
		{
			return  0;
		}
	}
	if( (void *)p_time == 0 )
	{
		p_time = &r_time;
	}
	
	ret = ioctl(s_rtc_fd, HI_RTC_RD_TIME, p_time);
	if (ret < 0)
	{
		printf("ioctl: HI_RTC_RD_TIME failed\n");
		return  0;
	}
	year	= p_time->year;
	month	= p_time->month;
	day		= p_time->date;
	hour	= p_time->hour;
	minute	= p_time->minute;
	sec		= p_time->second;
	
	if( 0 >= (int)( month -= 2 ) )    /**//* 1..12 -> 11,12,1..10 */
	{
		month	+= 12;              /**//* Puts Feb last since it has leap day */
		year	-= 1;
	}
	utc = ( ( ( (unsigned long)( year / 4 - year / 100 + year / 400 + 367 * month / 12 + day ) +
				 year * 365 - 719499
				 ) * 24 + hour		/**//* now have hours */
			   ) * 60 + minute		   /**//* now have minutes */
			 ) * 60 + sec;			/**//* finally seconds */

	return utc;
}



/*********************************************************************************
  *函数名称:unsigned long rtc_set_time(rtc_time_t *p_time)
  *功能描述:设置RTC时间
  *输	入: p_time	:保存要设置的时间
  *输	出: none
  *返 回 值:	0:OK，非0表示ERROR
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-15	创建
*********************************************************************************/
int rtc_set_time(rtc_time_t *p_time)
{
	//rtc_time_t tm;
	int ret =  - 1;
	int fd =  - 1;
	const char *dev_name = "/dev/hi_rtc";
	
	if (!s_rtc_fd)
	{
		if(rtc_init_time())
		{
			return  -1;
		}
	}
	
	ret = ioctl(s_rtc_fd, HI_RTC_SET_TIME, p_time);
	if (ret < 0)
	{
		printf("ioctl: HI_RTC_SET_TIME failed\n");
		return  -1;
	}
	return 0;
}



/*********************************************************************************
  *函数名称:int rtc_set_time_utc(unsigned long uct_time)
  *功能描述:设置RTC时间，设置时间格式为UTC时间
  *输	入: uct_time	:要设置的时间，格式为utc格式
  *输	出: none
  *返 回 值:	0:OK，非0表示ERROR
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-15	创建
*********************************************************************************/
int rtc_set_time_mytime(MYTIME my_time)
{
	rtc_time_t 	r_time;
	
	r_time.year = YEAR(my_time) + 2000;
	r_time.month= MONTH(my_time);
	r_time.date = DAY(my_time);
	r_time.hour = HOUR(my_time);
	r_time.minute= MINUTE(my_time);
	r_time.second= SEC(my_time);
	r_time.weekday = 0;

	/*
	printf( " rtc_set_time_mytime:%d-%d-%d-%d-%d-%d\n",
		r_time.year,
		r_time.month,
		r_time.date,
		r_time.hour,
		r_time.minute,
		r_time.second
		);
		*/
	return rtc_set_time(&r_time);
}



/*********************************************************************************
  *函数名称:int rtc_set_time_utc(unsigned long uct_time)
  *功能描述:设置RTC时间，设置时间格式为UTC时间
  *输	入: uct_time	:要设置的时间，格式为utc格式
  *输	出: none
  *返 回 值:	0:OK，非0表示ERROR
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-07-15	创建
*********************************************************************************/
int rtc_set_time_utc(unsigned long uct_time)
{
	rtc_time_t 	r_time;
	MYTIME 		my_time;
	
	my_time = utc_to_mytime(uct_time);
	return rtc_set_time_mytime(my_time);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


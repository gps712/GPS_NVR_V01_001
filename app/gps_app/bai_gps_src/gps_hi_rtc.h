#ifndef _H_GPS_HI_RTC_
#define _H_GPS_HI_RTC_
/*********************************************************************************
  * @文件名称 :gps_console.c
  * @功能描述 :用户进行人机交互的接口函数，所有需要通过控制台进行人机交互的代码都在这里
  * @作	   者 :白养民
  * @创建日期 :2015-6-18
  * @文件版本 :0.01
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-18	创建
*********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gps_typedef.h"
#include "hi_rtc.h"


#ifdef __cplusplus
#if __cplusplus
	extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


///MYTIME格式的时间最大表示的时间为2063-12-31 23:59:59
typedef uint32_t MYTIME;			

#define MYDATETIME( year, month, day, hour, minute, sec ) \
    ( (uint32_t)( ( year ) << 26 ) | \
      (uint32_t)( ( month ) << 22 ) | \
      (uint32_t)( ( day ) << 17 ) | \
      (uint32_t)( ( hour ) << 12 ) | \
      (uint32_t)( ( minute ) << 6 ) | ( sec ) )

#define YEAR( datetime )	( ( datetime >> 26 ) & 0x3F )
#define MONTH( datetime )	( ( datetime >> 22 ) & 0xF )
#define DAY( datetime )		( ( datetime >> 17 ) & 0x1F )
#define HOUR( datetime )	( ( datetime >> 12 ) & 0x1F )
#define MINUTE( datetime )	( ( datetime >> 6 ) & 0x3F )
#define SEC( datetime )		( datetime & 0x3F )


extern MYTIME mytime_from_hex( uint8_t* buf );
/*转换bcd时间*/
extern MYTIME mytime_from_bcd( uint8_t* buf );
/*转换为十六进制的时间 例如 2013/07/18 => 0x0d 0x07 0x12*/
extern void mytime_to_hex( uint8_t* buf, MYTIME time );
/*转换为bcd字符串为自定义时间 例如 0x13 0x07 0x12=>代表 13年7月12日*/
extern void mytime_to_bcd( uint8_t* buf, MYTIME time );
extern unsigned long mytime_to_utc(MYTIME	time);
extern MYTIME utc_to_mytime(unsigned long utc);
extern int rtc_init_time(void);
extern unsigned long rtc_get_time(rtc_time_t *p_time);
extern int rtc_set_time(rtc_time_t *p_time);
extern int rtc_set_time_mytime(MYTIME my_time);
extern int rtc_set_time_utc(unsigned long uct_time);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef _H_GPS_HI_RTC_ */


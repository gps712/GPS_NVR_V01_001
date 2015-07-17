#ifndef _H_GPS_TYPEDEF_
#define _H_GPS_TYPEDEF_
/*********************************************************************************
  * @文件名称 :gps_disk_io.h
  * @功能描述 :gps项目组需要定义的所有全局结构体，define，和typedef 都在这里
  * @作	   者 :白养民
  * @创建日期 :2015-6-18
  * @文件版本 :0.01
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-18  创建函数
*********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#ifdef __cplusplus
#if __cplusplus
	extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

//typedef		unsigned char			uint8_t;
//typedef		short unsigned int		uint16_t;
//typedef		unsigned long 			uint32_t;


typedef		unsigned long 			utc_time;


#ifndef FALSE
	#define FALSE	0
#endif
#ifndef TRUE
	#define TRUE	1
#endif




/*********************************************************************************
  *结 构 体:gps_thread_param
  *功能描述:定义了一个结构体类型，它表示传递给一个线程的参数
  *---------------------------------------------------------------------------------
  *修改人    修改时间   修改内容
  *白养民   2015-06-08  创建函数
*********************************************************************************/
typedef struct _st_gps_thread_param
{
     u8   thread_start;
     u32  thread_param;
}st_gps_thread_param;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef _H_GPS_TYPEDEF_ */


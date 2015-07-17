#ifndef _H_GPS_DISK_IO_
#define _H_GPS_DISK_IO_
/*********************************************************************************
  * @文件名称 :gps_disk_io.h
  * @功能描述 :所有硬盘和存储盘操作的驱动函数及应用函数的声明文件
  * @作	   者 :白养民
  * @创建日期 :2015-6-17
  * @文件版本 :0.01
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-17	创建
*********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
#if __cplusplus
	extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


int console_disk_test( char *p, unsigned int len );

int disk_printf_dir(const char* pc_path);
int disk_remove_dir(const char* pc_path);
int disk_create_dir(const char* pc_path);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef _H_GPS_DISK_IO_ */

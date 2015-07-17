#ifndef _HI_TIME_H_
#define _HI_TIME_H_

#ifdef __cplusplus
extern "C"
{
#endif

long int now_time(void);
double interval_time(unsigned char interval, long int timeout);
int jt808_timer(void);
void  sys_time(void);
#ifdef __cplusplus
}; //end of extern "C" {
#endif

#endif
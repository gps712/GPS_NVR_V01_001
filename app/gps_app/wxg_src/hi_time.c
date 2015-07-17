#include <sys/times.h>
#include <sys/types.h>
#include <sys/times.h>
#include <signal.h> 
#include <unistd.h> 
#include <stdio.h>
#include <math.h>

#include   <time.h>  
#include   <string.h>

#include "jt808_head.h"



#ifdef __cplusplus
extern "C"
{
#endif

#if 0
#define TIME_LOOP_NUM    1000000*20
void TimingFunc(void)
{
	unsigned int i = 0;
	double y = 0.0;
	for(; i < TIME_LOOP_NUM; i++)
	y = sin((double)i);
}

void TimesTiming(char input)
{
    clock_t tBeginTime = times(NULL);
    TimingFunc();
	if(input == 1)
	{
		sleep(2);
	}
    clock_t tEndTime = times(NULL);
    double fCostTime = (double)(tEndTime - tBeginTime)/sysconf(_SC_CLK_TCK);
    printf("[times]Cost Time = %fSec\n", fCostTime);
}

void main(void)
{
	TimesTiming(0);
	TimesTiming(1);
}
#endif
extern gps_baseinfo gpsdata;

/****************************************get tick******************************/
long int now_time(void)
{
	clock_t begintime = times(NULL);
	return begintime;
}
//timeout is the start time,interval is : how long the program need wait;
double interval_time(unsigned char interval, long int timeout)
{
	double costtime;
	clock_t endtime= times(NULL);
	costtime = (double)(endtime-timeout)/sysconf(_SC_CLK_TCK);
	costtime = costtime - interval;
	return costtime;
}
/***************************************timer******************************/

void sys_time(void)
{
	time_t t;
	char p[32];
	time(&t);
	strftime(p, sizeof(p), "%T", localtime(&t));
	printf("time is %s\n", p);
	bzero(p,32);
}



void  timeout_function(void)
{
	static int subtime =1;
	
	if((subtime%60)==0)
	{
		//jt808_tx_heart();
		;
	}
	if((subtime%30)==0)
	{
		gps_data_filled(&gpsdata);
		jt808_tx_gpsdata();
	}
	if(subtime>=60)
	{
		subtime =0;
	}
	subtime++;
	//JT808_PRT("subtime =%d\n",subtime);
}


int jt808_timer(void)
{
	struct sigevent evp;
	struct itimerspec ts;
	timer_t timer;
	int ret;

	evp.sigev_value.sival_ptr = &timer;
	evp.sigev_notify = SIGEV_SIGNAL;
	evp.sigev_signo = SIGUSR1;
	signal(SIGUSR1, timeout_function);

	ret = timer_create(CLOCK_REALTIME, &evp, &timer);
	if( ret )
	{
		JT808_PRT("timer_create failed\n");
		return 1;
	}
	ts.it_interval.tv_sec = 1;
	ts.it_interval.tv_nsec = 0;
	//If this field is nonzero,then each time that an armed timer expires,  
	//the timer is reloaded from the it_interval.tv_sec
	ts.it_value.tv_sec = 1;
	ts.it_value.tv_nsec = 0;

	ret = timer_settime(timer, 0, &ts, NULL);
	if( ret )
	{
		JT808_PRT("timer_settime failed\n");
		return 1;
	}
	return 0;
}

#ifdef __cplusplus
}; //end of extern "C" {
#endif

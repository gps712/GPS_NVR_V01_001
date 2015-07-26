/******************************************************************************

  Copyright (C), 2012-2015, Satellite navigation department

 ******************************************************************************
  File Name     : timer.c
  Version       : Initial Draft
  Author        : wxg
  Created       : 2015/7/23
  Last Modified :
  Description   : this is a timer
  Function List :
              hitimer
              starttimer
  History       :
  1.Date        : 2015/7/23
    Author      : wxg
    Modification: Created file

******************************************************************************/
/*gcc -o example example.c   -lrt */
#include <unistd.h>
#include <signal.h>
#include <stdio.h>  
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>

static timer_t timer;


/*****************************************************************************
 Prototype    : hitimer
 Description  : timeout callback
 Input        : None
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
	History        :
  1.Date         : 2015/7/23
    Author       : wxg
    Modification : Created function

*****************************************************************************/
void  hitimer()
{
	
}



/*****************************************************************************
 Prototype    : starttimer
 Description  : start timer 100ms
 Input        : None
 Output       : None
 Return Value : 0 sucess, 1 failed
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2015/7/23
    Author       : wxg
    Modification : Created function

*****************************************************************************/
int starttimer()
{
	struct sigevent evp;
	struct itimerspec ts;
	int ret;

	evp.sigev_value.sival_ptr = &timer;
	evp.sigev_notify = SIGEV_SIGNAL;
	evp.sigev_signo = SIGUSR1;
	signal(SIGUSR1, hitimer);

	ret = timer_create(CLOCK_REALTIME, &evp, &timer);
	if( ret )
	{
		perror("timer_create");
	return 1;
	}
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 10000000;
	ts.it_value.tv_sec = 1;
	ts.it_value.tv_nsec = 0;
	ret = timer_settime(timer, 0, &ts, NULL);
	if( ret )
	{
		perror("timer_settime");
	return 1;
	}
	return 0;
}


/*
*********************************************************************************************************
*                                                uC/GUI
*                        Universal graphic software for embedded applications
*
*                       (c) Copyright 2002, Micrium Inc., Weston, FL
*                       (c) Copyright 2002, SEGGER Microcontroller Systeme GmbH
*
*              µC/GUI is protected by international copyright laws. Knowledge of the
*              source code may not be used to write a similar product. This file may
*              only be used in accordance with a license and should not be redistributed
*              in any way. We appreciate your understanding and fairness.
*
----------------------------------------------------------------------
File        : GUI_X.C
Purpose     : Config / System dependent externals for GUI
---------------------------END-OF-HEADER------------------------------
*/

#include "../Core/GUI.h"
#include "../Core/GUI_X.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*********************************************************************
*
*       Global data
*/
volatile int OS_TimeMS;

/*********************************************************************
*
*      Timing:
*                 GUI_X_GetTime()
*                 GUI_X_Delay(int)

  Some timing dependent routines require a GetTime
  and delay function. Default time unit (tick), normally is
  1 ms.
*/

int GUI_X_GetTime(void) 
{
       struct timeval tv;
        int tm;
        gettimeofday(&tv, NULL);
        tm = tv.tv_sec*1024 + tv.tv_usec/1024;
    
       return tm;
}


void GUI_X_Delay(int Period) 
{
       while(Period--)
      {
               usleep(1000);
       }
       return;
}


/*********************************************************************
*
*       GUI_X_Init()
*
* Note:
*     GUI_X_Init() is called from GUI_Init is a possibility to init
*     some hardware which needs to be up and running before the GUI.
*     If not required, leave this routine blank.
*/

void GUI_X_Init(void) 
{}


/*********************************************************************
*
*       GUI_X_ExecIdle
*
* Note:
*  Called if WM is in idle state
*/

void GUI_X_ExecIdle(void) 
{
       usleep(1000);
       return;
}


void GUI_X_Unlock(void)
{
        pthread_mutex_unlock(&mutex);
        return;
}
void GUI_X_Lock(void)
{
       pthread_mutex_lock(&mutex); 
       return;
}
U32 GUI_X_GetTaskId(void)
{
       pthread_t id;       id = pthread_self();
       //printf("GUI_X_GetTaskId %d \n", (U32)id);
       return ((U32)id);
}
void GUI_X_InitOS(void)
{
       printf("GUI_X_InitOS\n");
       GUI_MOUSE_DRIVER_PS2_Init();                              /* create read mouse thread for linux */
       return;
}

/*********************************************************************
*
*      Logging: OS dependent

Note:
  Logging is used in higher debug levels only. The typical target
  build does not use logging and does therefor not require any of
  the logging routines below. For a release build without logging
  the routines below may be eliminated to save some space.
  (If the linker is not function aware and eliminates unreferenced
  functions automatically)

*/

void GUI_X_Log     (const char *s) { GUI_USE_PARA(s); }
void GUI_X_Warn    (const char *s) { GUI_USE_PARA(s); }
void GUI_X_ErrorOut(const char *s) { GUI_USE_PARA(s); }

/*----------------------------------------------------------------------
File        : LCDConf_7529.h
Purpose     : configuration file
Author      : ³ÂÎ°ÎÄ
Data        : 2007.2.8
----------------------------------------------------------------------
*/

#include <string.h>            
#include <stddef.h>          
#include "LCD_Private.h"      
#include "GUI_Private.h"
#include "GUIDebug.h"


#ifndef LCDCONF_H
#define LCDCONF_H
#endif


//Òº¾§¿ØÖÆÆ÷µÄÃüÁî
/***************************************************************************************/
#define      ExtIn       0x0030
#define      ExtOut      0x0031

//Ext=0                                     
#define      DISON       0x00AF
#define      DISOFF      0x00AE
#define      DISNOR      0x00A6
#define      DISINV      0x00A7
#define      COMSCN      0x00BB
#define      DISCTRL     0x00CA
#define      SLPIN       0x0095
#define      SLPOUT      0x0094
#define      LASET       0x0075
#define      CASET       0x0015
#define      DATSDR      0x00BC
#define      RAMWR       0x005C
#define      RAMRD       0x005D
#define      PTLIN       0x00A8
#define      PTLOUT      0x00A9
#define      RMWIN       0x00E0
#define      RMWOUT      0x00EE
#define      ASCSET      0x00AA
#define      SCSTART     0x00AB
#define      OSCON       0x00D1
#define      OSCOFF      0x00D2
#define      PWRCTRL     0x0020
#define      VOLCTRL     0x0081
#define      VOLUP       0x00D6
#define      VOLDOWN     0x00D7
#define      RESERVED    0x0082
#define      EPSRRD1     0x007C
#define      EPSRRD2     0x007D
#define      NOP         0x0025
#define      EPINT       0x0007

//Ext=1
#define      Gray1Set    0x0020
#define      Gray2Set    0x0021
#define      ANASET      0x0032
#define      SWINT       0x0034
#define      EPCTIN      0x00CD
#define      EPCOUT      0x00CC
#define      EPMWR       0x00FC
#define      EPMRD       0x00FD 



/*----------------------------------------------------------------------
File        : LCDConf_7529.c
Purpose     : configuration file
Author      : 陈伟文
Data        : 2007.2.8
----------------------------------------------------------------------
*/


/*
注意:
1  我是用16灰度的例子修改而成的,而在实际应用中只用到了5个灰度,故在用前景色画图时,
   我是把0~15的索引值转换为我要用到的5个灰度.
2  我是用一个16位数画3个点的模式的,如果用其他模式的需要修改部分代码.
3  在画位图的4个函数中,我只优化了画8位那个,1,2,4位的还未优化

*/
#include <string.h>                             /* for memset */
#include <stddef.h>                             /* needed for definition of NULL */
#include "LCD_Private.h"                        /* private modul definitions & config */
#include "GUI_Private.h"
#include "GUIDebug.h"
#include "GUI.h"
#include "LCDConf.h"
//#include "LCDConf_ST7735.h"

/* for linux framebuffer */
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static char *pFrameBuffer = NULL;





#ifndef LCDCONF_H
#define LCDCONF_H
#endif

#define TRUE  1
#define FALSE 0

#define WIN32                1
#define LCD_OPTIMIZE         0

//#define  PixelIndex  unsigned char

#ifndef LCD_DISPLAY_INDEX
  #define LCD_DISPLAY_INDEX 0
#endif

#define SETPIXEL(x, y, c)   _SetPixel(x, y, c)
#define GETPIXEL(x, y)      _GetPixel(x,y)
#define XORPIXEL(x, y)      XorPixel(x,y)
#define LCD_ON()            
#define LCD_OFF()            



void _SetPixel(int  x, int y, int PixelIndex)           
{
     //SETPIXEL(x, y, PixelIndex);
     /* Convert logical into physical coordinates (Dep. on LCDConf.h) */
	#if LCD_SWAP_XY | LCD_MIRROR_X| LCD_MIRROR_Y
	int xPhys = LOG2PHYS_X(x, y);
	int yPhys = LOG2PHYS_Y(x, y);
	#else
	#define xPhys x
	#define yPhys y
	#endif     
    int location = 0; 
	location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel >> 3) + (y+vinfo.yoffset) * finfo.line_length;
	*(short*)(pFrameBuffer + location) = (short)PixelIndex; /* 16bpp */      
	return; 
}


unsigned int _GetPixel(int x, int y)                       
{
      LCD_PIXELINDEX PixelIndex;
      /* Convert logical into physical coordinates (Dep. on LCDConf.h) */
      #if LCD_SWAP_XY | LCD_MIRROR_X| LCD_MIRROR_Y
      int xPhys = LOG2PHYS_X(x, y);
      int yPhys = LOG2PHYS_Y(x, y);
      #else
      #define xPhys x
      #define yPhys y
      #endif
      /* Read from hardware ... Adapt to your system */
     {
          int location = 0; 
          location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel >> 3) + (y+vinfo.yoffset) * finfo.line_length;
          PixelIndex = *(short*)(pFrameBuffer + location); /* 16bpp */
      }
      return PixelIndex;
}

/***************************************************************************************/
void XorPixel(int x, int y)                              //异或函数
{   
    _SetPixel(x, y, LCD_COLORINDEX);
}

/***************************************************************************************/

void LCD_L0_SetPixelIndex(int x, int y, int PixelIndex)   //上层调用的画点函数
{
    SETPIXEL(x, y, PixelIndex);
}


/***************************************************************************************/
unsigned int LCD_L0_GetPixelIndex(int x, int y)           //上层调用的取点函数
{
     unsigned int temp;
    temp = GETPIXEL(x, y);
    return (temp);
}


/***************************************************************************************/
void LCD_L0_XorPixel(int x, int y)                       //上层调用的异或函数
{
     XORPIXEL(x, y);
}
/*********************************************************************
*
*       LCD_L0_DrawHLine
*/
void LCD_L0_DrawHLine(int x0, int y,  int x1) 
{
    for (;x0 <= x1; x0++) 
    {
        SETPIXEL(x0, y, LCD_COLORINDEX);
    }
}

/*********************************************************************
*
*       LCD_L0_DrawVLine
*/
void LCD_L0_DrawVLine(int x, int y0,  int y1) 
{
    while (y0 <= y1) 
    {
        SETPIXEL(x, y0, LCD_COLORINDEX);
        y0++;
    }
}

/*********************************************************************
*
*       LCD_L0_FillRect
*/
void LCD_L0_FillRect(int x0, int y0, int x1, int y1) {
  for (; y0 <= y1; y0++) 
  {
    LCD_L0_DrawHLine(x0,y0, x1);
  }
}

void  LCDSIM_SetPixelIndex(int x, int y, int Index, int LayerIndex) 
{
    _SetPixel( x,  y,  Index);
}
int LCDSIM_GetPixelIndex(int x, int y, int LayerIndex)
{
    return (_GetPixel( x,  y));
}
/*********************************************************************
*
*       _DrawBitLine1BPP
*/
static void _DrawBitLine1BPP(int x, int y, U8 const*p, int Diff, int xsize, const LCD_PIXELINDEX*pTrans) {
  LCD_PIXELINDEX Index0 = *(pTrans+0);
  LCD_PIXELINDEX Index1 = *(pTrans+1);
  x+=Diff;
  switch (GUI_Context.DrawMode & (LCD_DRAWMODE_TRANS|LCD_DRAWMODE_XOR)) {
  case 0:    /* Write mode */
    do {
      LCDSIM_SetPixelIndex(x++,y, (*p & (0x80>>Diff)) ? Index1 : Index0, LCD_DISPLAY_INDEX);
			if (++Diff==8) {
        Diff=0;
				p++;
			}
		} while (--xsize);
    break;
  case LCD_DRAWMODE_TRANS:
    do {
  		if (*p & (0x80>>Diff))
        LCDSIM_SetPixelIndex(x,y, Index1, LCD_DISPLAY_INDEX);
      x++;
			if (++Diff==8) {
        Diff=0;
				p++;
			}
		} while (--xsize);
    break;
  case LCD_DRAWMODE_XOR:;
    do {
  		if (*p & (0x80>>Diff)) {
        int Pixel = LCDSIM_GetPixelIndex(x,y, LCD_DISPLAY_INDEX);
        LCDSIM_SetPixelIndex(x,y, LCD_NUM_COLORS-1-Pixel, LCD_DISPLAY_INDEX);
      }
      x++;
			if (++Diff==8) {
        Diff=0;
				p++;
			}
		} while (--xsize);
    break;
	}
}

/*********************************************************************
*
*       _DrawBitLine2BPP
*/
#if (LCD_MAX_LOG_COLORS > 2)
static void _DrawBitLine2BPP(int x, int y, U8 const * p, int Diff, int xsize, const LCD_PIXELINDEX * pTrans) {
  LCD_PIXELINDEX Pixels = *p;
  int CurrentPixel = Diff;
  x += Diff;
  switch (GUI_Context.DrawMode & (LCD_DRAWMODE_TRANS | LCD_DRAWMODE_XOR)) {
    case 0:
      if (pTrans) {
        do {
          int Shift = (3 - CurrentPixel) << 1;
          int Index = (Pixels & (0xC0 >> (6 - Shift))) >> Shift;
          LCD_PIXELINDEX PixelIndex = *(pTrans + Index);
          SETPIXEL(x++, y, PixelIndex);
          if (++CurrentPixel == 4) {
            CurrentPixel = 0;
            Pixels = *(++p);
          }
		    } while (--xsize);
      } else {
        do {
          int Shift = (3 - CurrentPixel) << 1;
          int Index = (Pixels & (0xC0 >> (6 - Shift))) >> Shift;
          SETPIXEL(x++, y, Index);
          if (++CurrentPixel == 4) {
            CurrentPixel = 0;
            Pixels = *(++p);
          }
		    } while (--xsize);
      }
      break;
    case LCD_DRAWMODE_TRANS:
      if (pTrans) {
        do {
          int Shift = (3 - CurrentPixel) << 1;
          int Index = (Pixels & (0xC0 >> (6 - Shift))) >> Shift;
          if (Index) {
            LCD_PIXELINDEX PixelIndex = *(pTrans + Index);
            SETPIXEL(x, y, PixelIndex);
          }
          x++;
          if (++CurrentPixel == 4) {
            CurrentPixel = 0;
            Pixels = *(++p);
          }
		    } while (--xsize);
      } else {
        do {
          int Shift = (3 - CurrentPixel) << 1;
          int Index = (Pixels & (0xC0 >> (6 - Shift))) >> Shift;
          if (Index) {
            SETPIXEL(x, y, Index);
          }
          x++;
          if (++CurrentPixel == 4) {
            CurrentPixel = 0;
            Pixels = *(++p);
          }
		    } while (--xsize);
      }
      break;
  }
}
#endif

/*********************************************************************
*
*       _DrawBitLine4BPP
*/
#if (LCD_MAX_LOG_COLORS > 4)
static void _DrawBitLine4BPP(int x, int y, U8 const * p, int Diff, int xsize, const LCD_PIXELINDEX * pTrans) {
  LCD_PIXELINDEX Pixels = *p;
  int CurrentPixel = Diff;
  x += Diff;
  switch (GUI_Context.DrawMode & (LCD_DRAWMODE_TRANS | LCD_DRAWMODE_XOR)) {
    case 0:
      if (pTrans) {
        do {
          int Shift = (1 - CurrentPixel) << 2;
          int Index = (Pixels & (0xF0 >> (4 - Shift))) >> Shift;
          LCD_PIXELINDEX PixelIndex = *(pTrans + Index);
          SETPIXEL(x++, y, PixelIndex);
          if (++CurrentPixel == 2) {
            CurrentPixel = 0;
            Pixels = *(++p);
          }
		    } while (--xsize);
      } else {
        do {
          int Shift = (1 - CurrentPixel) << 2;
          int Index = (Pixels & (0xF0 >> (4 - Shift))) >> Shift;
          SETPIXEL(x++, y, Index);
          if (++CurrentPixel == 2) {
            CurrentPixel = 0;
            Pixels = *(++p);
          }
		    } while (--xsize);
      }
      break;
    case LCD_DRAWMODE_TRANS:
      if (pTrans) {
        do {
          int Shift = (1 - CurrentPixel) << 2;
          int Index = (Pixels & (0xF0 >> (4 - Shift))) >> Shift;
          if (Index) {
            LCD_PIXELINDEX PixelIndex = *(pTrans + Index);
            SETPIXEL(x, y, PixelIndex);
          }
          x++;
          if (++CurrentPixel == 2) {
            CurrentPixel = 0;
            Pixels = *(++p);
          }
		    } while (--xsize);
      } else {
        do {
          int Shift = (1 - CurrentPixel) << 2;
          int Index = (Pixels & (0xF0 >> (4 - Shift))) >> Shift;
          if (Index) {
            SETPIXEL(x, y, Index);
          }
          x++;
          if (++CurrentPixel == 2) {
            CurrentPixel = 0;
            Pixels = *(++p);
          }
		    } while (--xsize);
      }
      break;
  }
}
#endif

/*********************************************************************
*
*       _DrawBitLine8BPP
*/
#if (LCD_MAX_LOG_COLORS > 16)
static void _DrawBitLine8BPP(int x, int y, U8 const*p, int xsize, const LCD_PIXELINDEX*pTrans) {
  LCD_PIXELINDEX pixel;
  if ((GUI_Context.DrawMode & LCD_DRAWMODE_TRANS)==0) {
    if (pTrans) {
      for (;xsize > 0; xsize--,x++,p++) {
        pixel = *p;
        SETPIXEL(x, y, *(pTrans+pixel));
      }
    } else {
      for (;xsize > 0; xsize--,x++,p++) {
        SETPIXEL(x, y, *p);
      }
    }
  } else {   /* Handle transparent bitmap */
    if (pTrans) {
      for (; xsize > 0; xsize--, x++, p++) {
        pixel = *p;
        if (pixel) {
          SETPIXEL(x+0, y, *(pTrans+pixel));
        }
      }
    } else {
      for (; xsize > 0; xsize--, x++, p++) {
        pixel = *p;
        if (pixel) {
          SETPIXEL(x+0, y, pixel);
        }
      }
    }
  }
}
#endif

/*********************************************************************
*
*       _DrawBitLine16BPP
*/
#if (LCD_BITSPERPIXEL > 8)
static void _DrawBitLine16BPP(int x, int y, U16 const*p, int xsize, const LCD_PIXELINDEX*pTrans) {
  LCD_PIXELINDEX pixel;
  if ((GUI_Context.DrawMode & LCD_DRAWMODE_TRANS)==0) {
    if (pTrans) {
      for (;xsize > 0; xsize--,x++,p++) {
        pixel = *p;
        SETPIXEL(x, y, *(pTrans+pixel));
      }
    } else {
      for (;xsize > 0; xsize--,x++,p++) {
        SETPIXEL(x, y, *p);
      }
    }
  } else {   /* Handle transparent bitmap */
    if (pTrans) {
      for (; xsize > 0; xsize--, x++, p++) {
        pixel = *p;
        if (pixel) {
          SETPIXEL(x+0, y, *(pTrans+pixel));
        }
      }
    } else {
      for (; xsize > 0; xsize--, x++, p++) {
        pixel = *p;
        if (pixel) {
          SETPIXEL(x+0, y, pixel);
        }
      }
    }
  }
}
#endif

/*********************************************************************
*
*       LCD_L0_DrawBitmap
*/
void LCD_L0_DrawBitmap(int x0, int y0,
                       int xsize, int ysize,
                       int BitsPerPixel, 
                       int BytesPerLine,
                       const U8* pData, int Diff,
                       const LCD_PIXELINDEX* pTrans)
{
  int i;
  /*
     Use DrawBitLineXBPP
  */
  for (i=0; i<ysize; i++) {
    switch (BitsPerPixel) {
    case 1:
      _DrawBitLine1BPP(x0, i+y0, pData, Diff, xsize, pTrans);
      break;
    #if (LCD_MAX_LOG_COLORS > 2)
      case 2:
        _DrawBitLine2BPP(x0, i+y0, pData, Diff, xsize, pTrans);
        break;
    #endif
    #if (LCD_MAX_LOG_COLORS > 4)
      case 4:
        _DrawBitLine4BPP(x0, i+y0, pData, Diff, xsize, pTrans);
        break;
    #endif
    #if (LCD_MAX_LOG_COLORS > 16)
      case 8:
        _DrawBitLine8BPP(x0, i+y0, pData, xsize, pTrans);
        break;
    #endif
    #if (LCD_BITSPERPIXEL > 8)
      case 16:
        _DrawBitLine16BPP(x0, i+y0, (const U16 *)pData, xsize, pTrans);
        break;
    #endif
    }
    pData += BytesPerLine;
  }
}

/***************************************************************************************/
int OrgX, OrgY;
void LCD_L0_SetOrg(int x, int y) 
{
    OrgX = x;
    OrgY = y;
}


/***************************************************************************************/
int  LCD_L0_Init(void)
{
     int f_fbDev;
     int ScreenSize;
     static struct fb_bitfield g_r16 = {10, 5, 0};
     static struct fb_bitfield g_g16 = {5, 5, 0};
     static struct fb_bitfield g_b16 = {0, 5, 0};
     static struct fb_bitfield g_a16 = {15, 1, 0};     
     f_fbDev = open("/dev/fb0", O_RDWR);
     if (f_fbDev <= 0) 
     {
           printf("Error: cannot open framebuffer device.\n");
           return (-1);
     }     
     if (ioctl(f_fbDev, FBIOGET_VSCREENINFO, &vinfo) < 0) 
     {
          printf("Error reading variable information.\n");
          close(f_fbDev);
          return (-1);
     }
     vinfo.xres = vinfo.xres_virtual = 720;
     vinfo.yres = 576;
     vinfo.yres_virtual = 576*2;     
     vinfo.transp= g_a16;
     vinfo.red = g_r16;
     vinfo.green = g_g16;
     vinfo.blue = g_b16;
     vinfo.bits_per_pixel = 16;
     if (ioctl(f_fbDev, FBIOPUT_VSCREENINFO, &vinfo) < 0)
     {
            printf("Put variable screen info failed!\n");
            close(f_fbDev);
            return -1;
     }     
     if (ioctl(f_fbDev, FBIOGET_FSCREENINFO, &finfo)) 
     {
           printf("Error reading fixed information.\n");
           return -1;
     }     
     printf("xres is %d\n, yres is %d\n", vinfo.xres, vinfo.yres);
     ScreenSize = vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel >> 3);     
     pFrameBuffer =(char *)mmap(0, ScreenSize, PROT_READ | PROT_WRITE, MAP_SHARED,f_fbDev, 0);
     if ((long)pFrameBuffer == -1) 
     {
           printf("Error: failed to map framebuffer device to memory.\n");
           close(f_fbDev);
           return (-1);
     }    return 0; 
}

//{
  //  LCD_INIT_CONTROLLER();
    //return 0;
//} 


/*
        *********************************************************
        *                                                       *
        *    Support for dynamic inversion of entire LCD        *
        *                                                       *
        *********************************************************

*/
#define LCD_REVERSEMODE_SUPPORT 1

#if LCD_REVERSEMODE_SUPPORT

void LCD_SetNormalDispMode    (void) 
{
}
void LCD_SetReverseDispMode   (void) 
{
}

#endif


/*
        *********************************************************
        *                                                       *
        *                   LCD_SetPaletteEntry                 *
        *                                                       *
        *********************************************************

*/

void LCD_L0_SetLUTEntry(U8 Pos, LCD_COLOR color) 
{
  Pos=Pos;
  color=color;
}


/*
        *********************************************************
        *                                                       *
        *       LCD_On                                          *
        *       LCD_Off                                         *
        *                                                       *
        *********************************************************
*/

void LCD_Off          (void) 
{
#ifdef LCD_OFF
  LCD_OFF();
#endif
}

void LCD_On           (void) 
{
#ifdef LCD_ON
  LCD_ON();
#endif
}


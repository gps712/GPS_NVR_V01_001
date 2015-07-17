/******************************************************************************
  A simple program of Hisilicon HI3520D video input and output implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2012-12 Created
******************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "sample_comm.h"
#include "loadbmp.h"
//#include <ft2build.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "hi_rtc.h"

/* 此处开始添加叠加OSD的代码 */
/******************************************************************************
  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
  File Name     : sample_hifb.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2011/10/15
  Description   :
  History       :
  1.Date        : 2011/10/15
    Author      : s00187460
    Modification: Created file
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/mman.h>   //mmap
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <signal.h>
#include "hi_common.h"
#include "hi_type.h"
#include "hi_comm_vb.h"
#include "hi_comm_sys.h"
#include "hi_comm_venc.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
//#include "hi_comm_group.h"
#include "hi_comm_region.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_venc.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_region.h"
#include "sample_comm.h"
#include <linux/fb.h>
#include "hifb.h"
#include "loadbmp.h"
#include "hi_tde_api.h"
#include "hi_tde_type.h"
#include "hi_tde_errcode.h"
#include <time.h>
#include "hi_rtc.h"
#include <sys/stat.h>
#include <locale.h>
#include "unicodetogbk.h"
#include "GUI.h"
#include "GUI_Protected.h"


HI_BOOL bCase0 = HI_FALSE;
HI_BOOL bCase1 = HI_FALSE;
VI_SCAN_MODE_E  gs_enViScanMode = VI_SCAN_INTERLACED;
static HI_U32 gs_s32ChnCnt = 8;     /* vi, venc chn count */
static HI_S32 gs_s32RgnCntCur = 0;
static HI_S32 gs_s32RgnCnt = 8;
#define SET_TIME    1
#define GET_TIME    2
#define SAMPLE_RGN_SLEEP_TIME (200*1000)
#define SAMPLE_RGN_LOOP_COUNT 6
#define SAMPLE_RGN_NOT_PASS(err)\
do {\
printf("\033[0;31mtest case <%s>not pass at line:%d err:%x\033[0;39m\n",\
    __FUNCTION__,__LINE__,err);\
exit(-1);\
}while(0)

#define SAMPLE_YUV_D1_FILEPATH         "SAMPLE_420_D1.yuv"
#define SAMPLE_MAX_VDEC_CHN_CNT 8

typedef struct sample_vdec_sendparam
{
    pthread_t Pid;
    HI_BOOL bRun;
    VDEC_CHN VdChn;
    PAYLOAD_TYPE_E enPayload;
    HI_S32 s32MinBufSize;
    VIDEO_MODE_E enVideoMode;
} SAMPLE_VDEC_SENDPARAM_S;

static SAMPLE_VENC_GETSTREAM_PARA_S gs_stPara;
static VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_PAL;
SAMPLE_VIDEO_LOSS_S gs_stVideoLoss;
static HI_U32 gs_u32ViFrmRate = 0;
SAMPLE_VDEC_SENDPARAM_S gs_SendParam[SAMPLE_MAX_VDEC_CHN_CNT];
HI_S32 gs_s32Cnt;
static pthread_t gs_RegionVencPid;
static pthread_t phifb0;
extern OSD_COMP_INFO s_OSDCompInfo[OSD_COLOR_FMT_BUTT];

//the time struct
rtc_time_t rtc;
#define SET_TIME    1
#define GET_TIME    2
#define SAMPLE_RGN_SLEEP_TIME (200*1000)
static VO_DEV VoDev = SAMPLE_VO_DEV_DHD0;

HI_S32 SAMPLE_HIFB_VO_Start(void)
{
#define HIFB_HD_WIDTH  1280
#define HIFB_HD_HEIGHT 720
    HI_S32 s32Ret = HI_SUCCESS;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_CHN_ATTR_S stChnAttr;
    HI_MPI_VO_Disable(VoDev);
    stPubAttr.enIntfType = VO_INTF_VGA;
    stPubAttr.enIntfSync = VO_OUTPUT_720P50;
    stPubAttr.u32BgColor = 0xff0000ff;
    /* Attr of video layer */
    stLayerAttr.stDispRect.s32X       = 0;
    stLayerAttr.stDispRect.s32Y       = 0;
    stLayerAttr.stDispRect.u32Width   = HIFB_HD_WIDTH;
    stLayerAttr.stDispRect.u32Height  = HIFB_HD_HEIGHT;
    stLayerAttr.stImageSize.u32Width  = HIFB_HD_WIDTH;
    stLayerAttr.stImageSize.u32Height = HIFB_HD_HEIGHT;
    stLayerAttr.u32DispFrmRt          = 50;
    stLayerAttr.enPixFormat           = PIXEL_FORMAT_YUV_SEMIPLANAR_422;
    /* Attr of vo chn */
    stChnAttr.stRect.s32X               = 0;
    stChnAttr.stRect.s32Y               = 0;
    stChnAttr.stRect.u32Width           = HIFB_HD_WIDTH;
    stChnAttr.stRect.u32Height          = HIFB_HD_HEIGHT;
    stChnAttr.bDeflicker                = HI_FALSE;
    stChnAttr.u32Priority               = 1;
    /* set public attr of VO*/
    if (HI_SUCCESS != HI_MPI_VO_SetPubAttr(VoDev, &stPubAttr))
    {
        printf("set VO pub attr failed !\n");
        return -1;
    }
    if (HI_SUCCESS != HI_MPI_VO_Enable(VoDev))
    {
        printf("enable vo device failed!\n");
        return -1;
    }
    s32Ret = HI_MPI_VO_SetVideoLayerAttr(VoDev, &stLayerAttr);
    if (s32Ret != HI_SUCCESS)
    {
        printf("set video layer attr failed with %#x!\n", s32Ret);
        return -1;
    }
    if (HI_SUCCESS != HI_MPI_VO_EnableVideoLayer(VoDev))
    {
        printf("enable video layer failed!\n");
        return -1;
    }
    return 0;
}
HI_S32 SAMPLE_HIFB_VO_Stop(void)
{
    if (HI_SUCCESS != HI_MPI_VO_DisableVideoLayer(VoDev))
    {
        printf("Disable video layer failed!\n");
        return -1;
    }
    if (HI_SUCCESS != HI_MPI_VO_Disable(VoDev))
    {
        printf("Disable vo device failed!\n");
        return -1;
    }
    return 0;
}
#define SAMPLE_IMAGE_WIDTH     300
#define SAMPLE_IMAGE_HEIGHT    150
#define SAMPLE_IMAGE_SIZE      (300*150*2)
#define SAMPLE_IMAGE_NUM       20

#define SAMPLE_IMAGE_PATH       "./res/%d.bmp"
#define SAMPLE_CURSOR_PATH      "./res/cursor.bmp"

#define DIF_LAYER_NAME_LEN 20
#define HIL_MMZ_NAME_LEN 32
#define HIFB_RED_1555   0xfc00
#define SAMPLE_VIR_SCREEN_WIDTH     SAMPLE_IMAGE_WIDTH          /*virtual screen width*/
#define SAMPLE_VIR_SCREEN_HEIGHT    SAMPLE_IMAGE_HEIGHT*2       /*virtual screen height*/
#define s32fd 0
#define HIL_MMB_NAME_LEN 16
#define g_s32fd  0
/*if you want to use standard mode ,please delete this define*/
//#define ExtendMode
static struct fb_bitfield g_r16 = {10, 5, 0};
static struct fb_bitfield g_g16 = {5, 5, 0};
static struct fb_bitfield g_b16 = {0, 5, 0};
static struct fb_bitfield g_a16 = {15, 1, 0};
typedef enum
{
    HIFB_LAYER_0 = 0x0,
    HIFB_LAYER_1,
    HIFB_LAYER_2,
    HIFB_LAYER_CURSOR_0,
    HIFB_LAYER_ID_BUTT
} HIFB_LAYER_ID_E;
typedef struct _LayerID_NAME_S
{
    HIFB_LAYER_ID_E     sLayerID;
    HI_CHAR             sLayerName[DIF_LAYER_NAME_LEN];
} LayerID_NAME_S;
typedef struct hiPTHREAD_HIFB_SAMPLE
{
    int fd;
    int layer;
    int ctrlkey;
} PTHREAD_HIFB_SAMPLE_INFO;

HI_S32 SAMPLE_HIFB_LoadBmp(const char *filename, HI_U8 *pAddr)
{
    OSD_SURFACE_S Surface;
    OSD_BITMAPFILEHEADER bmpFileHeader;
    OSD_BITMAPINFO bmpInfo;
    if(GetBmpInfo(filename,&bmpFileHeader,&bmpInfo) < 0)
    {
        printf("GetBmpInfo err!\n");
        return HI_FAILURE;
    }
    Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;
    CreateSurfaceByBitMap(filename,&Surface,pAddr);
    return HI_SUCCESS;
}

#if 0
HI_VOID *SAMPLE_HIFB_REFRESH(void *pData)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HIFB_LAYER_INFO_S stLayerInfo = {0};
    HIFB_BUFFER_S stCanvasBuf;
    HI_U16 *pBuf;
    HI_U8 *pDst = NULL;
    HI_U32 x, y,i;
    char image_name[128];
    HI_BOOL Show;
    HI_BOOL bCompress = HI_TRUE;
    HIFB_POINT_S stPoint = {0};
    struct fb_var_screeninfo stVarInfo;
    char file[12] = "/dev/fb0";
    HI_U32 maxW,maxH;
    PTHREAD_HIFB_SAMPLE_INFO *pstInfo;
    pstInfo = (PTHREAD_HIFB_SAMPLE_INFO *)pData;
    HIFB_COLORKEY_S stColorKey;
    TDE2_RECT_S stSrcRect,stDstRect;
    TDE2_SURFACE_S stSrc,stDst;
    HI_U32 Phyaddr;
    HI_VOID *Viraddr;
    TDE_HANDLE s32Handle;
    switch (pstInfo->layer)
    {
    case 0 :
        strcpy(file, "/dev/fb0");
        break;
    case 1 :
        strcpy(file, "/dev/fb1");
        break;
    case 2 :
        strcpy(file, "/dev/fb2");
        break;
    case 3 :
        strcpy(file, "/dev/fb3");
        break;
    case 4 :
        strcpy(file, "/dev/fb4");
        break;
    default:
        strcpy(file, "/dev/fb0");
        break;
    }
    /* 1. open framebuffer device overlay 0 */
    pstInfo->fd = open(file, O_RDWR, 0);
    if(pstInfo->fd < 0)
    {
        printf("open %s failed!\n",file);
        return HI_NULL;
    }
    if(pstInfo->layer == HIFB_LAYER_0 )
    {
        if (ioctl(pstInfo->fd, FBIOPUT_COMPRESSION_HIFB, &bCompress) < 0)
        {
            printf("FBIOPUT_COMPRESSION_HIFB failed!\n");
            close(pstInfo->fd);
            return HI_NULL;
        }
    }
    /*all layer surport colorkey*/
    stColorKey.bKeyEnable = HI_TRUE;
    stColorKey.u32Key = 0x0;
    if (ioctl(pstInfo->fd, FBIOPUT_COLORKEY_HIFB, &stColorKey) < 0)
    {
        printf("FBIOPUT_COLORKEY_HIFB!\n");
        close(pstInfo->fd);
        return HI_NULL;
    }
    s32Ret = ioctl(pstInfo->fd, FBIOGET_VSCREENINFO, &stVarInfo);
    if(s32Ret < 0)
    {
        printf("GET_VSCREENINFO failed!\n");
        return HI_NULL;
    }
    if (ioctl(pstInfo->fd, FBIOPUT_SCREEN_ORIGIN_HIFB, &stPoint) < 0)
    {
        printf("set screen original show position failed!\n");
        return HI_NULL;
    }
    maxW = 1280;
    maxH = 720;
    stVarInfo.xres = stVarInfo.xres_virtual = maxW;
    stVarInfo.yres = stVarInfo.yres_virtual = maxH;
    s32Ret = ioctl(pstInfo->fd, FBIOPUT_VSCREENINFO, &stVarInfo);
    if(s32Ret < 0)
    {
        printf("PUT_VSCREENINFO failed!\n");
        return HI_NULL;
    }
    switch (pstInfo->ctrlkey)
    {
    case 0 :
    {
        stLayerInfo.BufMode = HIFB_LAYER_BUF_ONE;
        stLayerInfo.u32Mask = HIFB_LAYERMASK_BUFMODE;
        break;
    }
    case 1 :
    {
        stLayerInfo.BufMode = HIFB_LAYER_BUF_DOUBLE;
        stLayerInfo.u32Mask = HIFB_LAYERMASK_BUFMODE;
        break;
    }
    default:
    {
        stLayerInfo.BufMode = HIFB_LAYER_BUF_NONE;
        stLayerInfo.u32Mask = HIFB_LAYERMASK_BUFMODE;
    }
    }
    s32Ret = ioctl(pstInfo->fd, FBIOPUT_LAYER_INFO, &stLayerInfo);
    if(s32Ret < 0)
    {
        printf("PUT_LAYER_INFO failed!\n");
        return HI_NULL;
    }
    Show = HI_TRUE;
    if (ioctl(pstInfo->fd, FBIOPUT_SHOW_HIFB, &Show) < 0)
    {
        printf("FBIOPUT_SHOW_HIFB failed!\n");
        return HI_NULL;
    }
    if (HI_FAILURE == HI_MPI_SYS_MmzAlloc(&(stCanvasBuf.stCanvas.u32PhyAddr), ((void**)&pBuf),
                                          NULL, NULL, maxW*maxH*2))
    {
        printf("allocate memory (maxW*maxH*2 bytes) failed\n");
        return HI_NULL;
    }
    stCanvasBuf.stCanvas.u32Height = maxH;
    stCanvasBuf.stCanvas.u32Width = maxW;
    stCanvasBuf.stCanvas.u32Pitch = maxW*2;
    stCanvasBuf.stCanvas.enFmt = HIFB_FMT_ARGB1555;
    memset(pBuf, 0x00, stCanvasBuf.stCanvas.u32Pitch*stCanvasBuf.stCanvas.u32Height);
    /*change bmp*/
    if (HI_FAILURE == HI_MPI_SYS_MmzAlloc(&Phyaddr, ((void**)&Viraddr),
                                          NULL, NULL, SAMPLE_IMAGE_WIDTH*SAMPLE_IMAGE_HEIGHT*2))
    {
        printf("allocate memory  failed\n");
        return HI_NULL;
    }
    s32Ret = HI_TDE2_Open();
    if(s32Ret < 0)
    {
        printf("HI_TDE2_Open failed :%d!\n",s32Ret);
        HI_MPI_SYS_MmzFree(Phyaddr, Viraddr);
        return HI_FALSE;
    }
    printf("expected:two red  line!\n");
    /*time to play*/
    for(i = 0; i < SAMPLE_IMAGE_NUM; i++)
    {
        for (y = 358; y < 362; y++)
        {
            for (x = 0; x < maxW; x++)
            {
                *(pBuf + y * maxW + x) = HIFB_RED_1555;
            }
        }
        for (y = 0; y < maxH; y++)
        {
            for (x = 638; x < 642; x++)
            {
                *(pBuf + y * maxW + x) = HIFB_RED_1555;
            }
        }
        sprintf(image_name, SAMPLE_IMAGE_PATH, i%2);
        pDst = (HI_U8 *)Viraddr;
        SAMPLE_HIFB_LoadBmp(image_name,pDst);
        /* 0. open tde */
        stSrcRect.s32Xpos = 0;
        stSrcRect.s32Ypos = 0;
        stSrcRect.u32Height = SAMPLE_IMAGE_HEIGHT;
        stSrcRect.u32Width = SAMPLE_IMAGE_WIDTH;
        stDstRect.s32Xpos = 0;
        stDstRect.s32Ypos = 0;
        stDstRect.u32Height = stSrcRect.u32Width;
        stDstRect.u32Width = stSrcRect.u32Width;
        stDst.enColorFmt = TDE2_COLOR_FMT_ARGB1555;
        stDst.u32Width = 1280;
        stDst.u32Height = 720;
        stDst.u32Stride = maxW*2;
        stDst.u32PhyAddr = stCanvasBuf.stCanvas.u32PhyAddr;
        stSrc.enColorFmt = TDE2_COLOR_FMT_ARGB1555;
        stSrc.u32Width = SAMPLE_IMAGE_WIDTH;
        stSrc.u32Height = SAMPLE_IMAGE_HEIGHT;
        stSrc.u32Stride = 2*SAMPLE_IMAGE_WIDTH;
        stSrc.u32PhyAddr = Phyaddr;
        stSrc.bAlphaExt1555 = HI_TRUE;
        stSrc.bAlphaMax255 = HI_TRUE;
        stSrc.u8Alpha0 = 0XFF;
        stSrc.u8Alpha1 = 0XFF;
        /* 1. start job */
        s32Handle = HI_TDE2_BeginJob();
        if(HI_ERR_TDE_INVALID_HANDLE == s32Handle)
        {
            printf("start job failed!\n");
            HI_MPI_SYS_MmzFree(Phyaddr, Viraddr);
            HI_MPI_SYS_MmzFree(stCanvasBuf.stCanvas.u32PhyAddr, pBuf);
            return HI_FALSE;
        }
        s32Ret = HI_TDE2_QuickCopy(s32Handle, &stSrc, &stSrcRect,&stDst, &stDstRect);
        if(s32Ret < 0)
        {
            printf("HI_TDE2_QuickCopy:%d failed,ret=0x%x!\n", __LINE__, s32Ret);
            HI_TDE2_CancelJob(s32Handle);
            HI_MPI_SYS_MmzFree(Phyaddr, Viraddr);
            HI_MPI_SYS_MmzFree(stCanvasBuf.stCanvas.u32PhyAddr, pBuf);
            return HI_FALSE;
        }
        /* 3. submit job */
        s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 10);
        if(s32Ret < 0)
        {
            printf("Line:%d,HI_TDE2_EndJob failed,ret=0x%x!\n", __LINE__, s32Ret);
            HI_TDE2_CancelJob(s32Handle);
            HI_MPI_SYS_MmzFree(Phyaddr, Viraddr);
            HI_MPI_SYS_MmzFree(stCanvasBuf.stCanvas.u32PhyAddr, pBuf);
            return HI_FALSE;
        }
        stCanvasBuf.UpdateRect.x = 0;
        stCanvasBuf.UpdateRect.y = 0;
        stCanvasBuf.UpdateRect.w = maxW;
        stCanvasBuf.UpdateRect.h = maxH;
        s32Ret = ioctl(pstInfo->fd, FBIO_REFRESH, &stCanvasBuf);
        if(s32Ret < 0)
        {
            printf("REFRESH failed!\n");
            HI_MPI_SYS_MmzFree(Phyaddr, Viraddr);
            HI_MPI_SYS_MmzFree(stCanvasBuf.stCanvas.u32PhyAddr, pBuf);
            return HI_NULL;
        }
        sleep(1);
    }
    HI_MPI_SYS_MmzFree(Phyaddr, Viraddr);
    HI_MPI_SYS_MmzFree(stCanvasBuf.stCanvas.u32PhyAddr, pBuf);
    close(pstInfo->fd);
    return HI_NULL;
}
#endif


/***************************************************************
*名称: TransByteToData
*描述: 从指定字串中组合出指定长度的数据类型，可指定大小端模式,
*             目前最多可组合uint型数据 类型
*输入:  INT8 *String,  指定字串
*              UINT16 SrcStrLen 源字串或数组长度
*              UINT16 StartSlot 源字串的起始位置
*              UINT32 Len,      要组合数据类型的长度
*              UINT8 Flag, 大小端模式标志,TRUE =  按照大端模式组合，False = 按照小端模式组合
*
*输出:   无
*返回值:组合好的数据
*修改记录:
*----------------------------------------
*修改人      修改时间     修改内容
*侯艳琪      2011.4.12             创建函数
****************************************************************/
HI_U32 TransByteToData (const HI_U8 *String,
                        HI_U16 SrcStrLen,
                        HI_U16 StartSlot,
                        HI_U32 Len,
                        HI_U8 Flag)
{
    HI_U16 usTempDataLen = 0;
    HI_U16 usTempLocSlot = 0;
    HI_U16 usTempStrLen = 0;
    HI_U16 usTemp = 0;
    HI_U32 uiTemp = 0;
    usTempStrLen = SrcStrLen;
    /*为防止越界，起始位置模处理 */
    usTempLocSlot = StartSlot%usTempStrLen;
    /*需组合的数据类型长度大于字串长度*/
    if (Len > usTempStrLen)
    {
        usTempDataLen = usTempStrLen;
    }
    else
    {
        usTempDataLen = (HI_U16)Len;
    }
    /*大端模式*/
    if (HI_TRUE == Flag)
    {
        for (usTemp = usTempDataLen; usTemp > 0; usTemp--)
        {
            uiTemp += String[usTempLocSlot] << (8*(usTemp - 1));
            usTempLocSlot = (usTempLocSlot + 1)%usTempStrLen;
        }
    }
    else
    {
        /*低位在前*/
        for (usTemp = 0; usTemp < usTempDataLen; usTemp++)
        {
            uiTemp += String[usTempLocSlot] << (8 * usTemp);
            usTempLocSlot = (usTempLocSlot + 1)%usTempStrLen;
        }
    }
    return uiTemp;
}
/***************************************************************
*名称: void DecToStringReduce(UINT8 Type,UINT8 *OutStrBuffer, UINT16 Hex, UINT8 *DesCount)
*描述: 把一个10进制的数转换成字符型的数据
                主要用在给GPRS模块发送命令上，如025---->"25"
*输入:   DispType  ----类型，1为长显示，0为短显示
                  OutStrBuffer   -------转换后的数据
                  Hex ------  待转换的数据
                  DesCount------转换后的数据的个数，即长度
*输出:  无
*修改记录:
*----------------------------------------
*修改人      修改时间     修改内容
  张博爱      2014-8-21            创建函数
****************************************************************/
void DecToString(HI_U8 DispType, HI_U8 *OutStrBuffer, HI_U16 Hex, HI_U8 *DesCount)
{
    HI_U8 ucCount = 0, ucTemp, ucFlag = HI_FALSE;
    HI_U16 usTemp;
    if (DispType == HI_TRUE)/* 十进制的短显示 */
    {
        if (Hex > 255)/* 大于255时 */
        {
            //10000
            ucTemp = (HI_U8)(Hex / 10000);
            if (Hex >= 10000)
            {
                ucFlag = HI_TRUE;
                OutStrBuffer[ucCount++] = ucTemp + '0';
            }
            usTemp = Hex % 10000;
            //1000
            ucTemp = (HI_U8)(usTemp / 1000);
            if ((usTemp >= 1000) || (HI_TRUE == ucFlag))
            {
                ucFlag = HI_TRUE;
                OutStrBuffer[ucCount++] = ucTemp + '0';
            }
            usTemp = Hex % 1000;
            //100
            ucTemp = (HI_U8)(usTemp / 100);
            if ((usTemp >= 100) || (HI_TRUE == ucFlag))
            {
                ucFlag = HI_TRUE;
                OutStrBuffer[ucCount++] = ucTemp + '0';
            }
            usTemp = Hex % 100;
            //10
            ucTemp = (HI_U8)(usTemp / 10);
            if ((usTemp >= 10) || (HI_TRUE == ucFlag))
            {
                ucFlag = HI_TRUE;
                OutStrBuffer[ucCount++] = ucTemp + '0';
            }
            ucTemp = Hex % 10;
            OutStrBuffer[ucCount++] = ucTemp + '0';
        }
        else
        {
            /* 把012当成12,短数据显示，用于给以太网模块发命令*/
            usTemp = Hex;
            //100
            ucTemp = (HI_U8)(usTemp / 100);
            if (usTemp >= 100)
            {
                ucFlag = HI_TRUE;
                OutStrBuffer[ucCount++] = ucTemp + '0';
            }
            usTemp = Hex % 100;
            //10
            ucTemp = (HI_U8)(usTemp / 10);
            if ((usTemp >= 10) || (HI_TRUE == ucFlag))
            {
                ucFlag = HI_TRUE;
                OutStrBuffer[ucCount++] = ucTemp + '0';
            }
            ucTemp = Hex % 10;
            OutStrBuffer[ucCount++] = ucTemp + '0';
        }
    }
    else
    {
        if (Hex > 255)/* 大于255时 */
        {
            //10000
            // ucTemp = (HI_U8)(Hex / 10000);
            //  OutStrBuffer[ucCount++] = ucTemp + '0';
            usTemp = Hex % 10000;
            //1000
            ucTemp = (HI_U8)(usTemp / 1000);
            OutStrBuffer[ucCount++] = ucTemp + '0';
            usTemp = Hex % 1000;
            //100
            ucTemp = (HI_U8)(usTemp / 100);
            OutStrBuffer[ucCount++] = ucTemp + '0';
            usTemp = Hex % 100;
            //10
            ucTemp = (HI_U8)(usTemp / 10);
            OutStrBuffer[ucCount++] = ucTemp + '0';
            ucTemp = Hex % 10;
            OutStrBuffer[ucCount++] = ucTemp + '0';
        }
        else
        {
            if (Hex > 100)
            {
                /* 把012当成12,短数据显示，用于给以太网模块发命令*/
                usTemp = Hex;
                //100
                ucTemp = (HI_U8)(usTemp / 100);
                OutStrBuffer[ucCount++] = ucTemp + '0';
            }
            usTemp = Hex % 100;
            //10
            ucTemp = (HI_U8)(usTemp / 10);
            OutStrBuffer[ucCount++] = ucTemp + '0';
            ucTemp = Hex % 10;
            OutStrBuffer[ucCount++] = ucTemp + '0';
        }
    }
    *DesCount = ucCount;
}
/***************************************************************
*名称: TimeConvertToString
*描述:
*输入:  无
*输出:  无
返回值: 无
*修改记录:
*----------------------------------------
*修改人      修改时间     修改内容
*曹海波      2010.01.24           创建函数
****************************************************************/
void TimeConvertToString(HI_U8 *Buf, HI_U8 *Time, HI_U8 Flag, HI_U8 *Count)
{
    HI_U8 ucCount = 0, ucCount1 = 0;
    HI_U32 uiTemp;
    uiTemp = TransByteToData(Time, sizeof(rtc_time_t), 0,sizeof(HI_U32), HI_FALSE);
    DecToString(HI_FALSE, &Buf[ucCount1], (HI_U16)uiTemp, &ucCount);
    ucCount1 += ucCount;
    if (HI_TRUE == Flag)
    {
        Buf[ucCount1++] = '-';
    }
    uiTemp = TransByteToData(Time, sizeof(rtc_time_t), 4,sizeof(HI_U32), HI_FALSE);
    DecToString(HI_FALSE, &Buf[ucCount1], (HI_U16)uiTemp, &ucCount);
    ucCount1 += ucCount;
    if (HI_TRUE == Flag)
    {
        Buf[ucCount1++] = '-';
    }
    uiTemp = TransByteToData(Time, sizeof(rtc_time_t), 4 * 2,sizeof(HI_U32), HI_FALSE);
    DecToString(HI_FALSE, &Buf[ucCount1], (HI_U16)uiTemp, &ucCount);
    ucCount1 += ucCount;
    Buf[ucCount1++] = ' ';
    uiTemp = TransByteToData(Time, sizeof(rtc_time_t), 4 * 3,sizeof(HI_U32), HI_FALSE);
    DecToString(HI_FALSE, &Buf[ucCount1], (HI_U16)uiTemp, &ucCount);
    ucCount1 += ucCount;
    Buf[ucCount1++] = ':';
    uiTemp = TransByteToData(Time, sizeof(rtc_time_t), 4 * 4,sizeof(HI_U32), HI_FALSE);
    DecToString(HI_FALSE, &Buf[ucCount1], (HI_U16)uiTemp, &ucCount);
    ucCount1 += ucCount;
    Buf[ucCount1++] = ':';
    uiTemp = TransByteToData(Time, sizeof(rtc_time_t), 4 * 5,sizeof(HI_U32), HI_FALSE);
    DecToString(HI_FALSE, &Buf[ucCount1], (HI_U16)uiTemp, &ucCount);
    ucCount1 += ucCount;
    *Count = ucCount1;
}
HI_U16 UncodeToGbk(HI_U16 src)
{
    const HI_U16 *p;
    HI_U16 c;
    int i, n, li, hi;
    /* Unicode to OEMCP */
    p = uni2oem;
    hi = sizeof(uni2oem) / 4 - 1;
    li = 0;
    for (n = 16; n; n--)
    {
        i = li + (hi - li) / 2;
        if (src == p[i * 2])
        {
            break;
        }
        if (src > p[i * 2])
        {
            li = i;
        }
        else
        {
            hi = i;
        }
    }
    c = n ? p[i * 2 + 1] : 0;
    return c;
}

/**
 * DESCRIPTION: 实现由utf8编码到gbk编码的转换
                         转换后的英文为1byte , 中文汉字为2 bytes
 *
* Input: gbkStr,转换后的字符串;  srcStr,待转换的字符串; maxGbkStrlen, gbkStr的最
 大长度
* Output: gbkStr
.* Returns: -1,fail;>0,success
 *
*/
int utf82gbk(char *gbkStr, const char *srcStr, int maxGbkStrlen)
{
    HI_U16 usCount, gbkLen, usTemp;
    if (NULL == srcStr)
    {
        printf("Bad Parameter1\n");
        return -1;
    }
    //首先先将utf8编码转换为unicode编码
    if (NULL == setlocale(LC_ALL, "zh_CN.UTF-8")) //设置转换为unicode前的码,当前为utf8编码
    {
        printf("Bad Parameter2\n");
        return -1;
    }
    int unicodeLen = mbstowcs(NULL, srcStr, 0); //计算转换后的长度
    if (unicodeLen <= 0)
    {
        printf("Can not Transfer!!!\n");
        return -1;
    }
    wchar_t *unicodeStr = (wchar_t *) calloc(sizeof(wchar_t), unicodeLen + 1);
    mbstowcs(unicodeStr, srcStr, strlen(srcStr)); //将utf8转换为unicode
    for (usCount = 0, gbkLen = 0; usCount < unicodeLen * 2; usCount += 2)
    {
        if (unicodeStr[usCount / 2] < 0x7f)/* 英文字符 */
        {
            gbkStr[gbkLen] = (HI_U8)unicodeStr[usCount / 2];
            gbkLen++;
        }
        else/* 中文汉字 */
        {
            usTemp = UncodeToGbk(unicodeStr[usCount / 2]);
            gbkStr[gbkLen] = (HI_U8)(usTemp >> 8);
            gbkStr[gbkLen + 1] = (HI_U8)usTemp;
            gbkLen += 2;
        }
    }
    gbkStr[gbkLen] = 0; //添加结束符
    free(unicodeStr);
    return gbkLen;
}

HI_VOID *SAMPLE_HIFB_PANDISPLAY(void *pData)
{
    HI_S32 i,x,y,s32Ret,j,num,uiCount;
    TDE_HANDLE s32Handle;
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
    HI_U32 u32FixScreenStride = 0;
    unsigned char *pShowScreen;
    unsigned char *pHideScreen;
    HI_U32 u32HideScreenPhy = 0;
    HI_U16 *pShowLine;
    HIFB_ALPHA_S stAlpha;
    HIFB_POINT_S stPoint = {40, 112};
    char file[12] = "/dev/fb0";
    HI_BOOL g_bCompress = HI_FALSE;
    char image_name[128];
    HI_U8 *pDst = NULL;
    HI_BOOL bShow;
    PTHREAD_HIFB_SAMPLE_INFO *pstInfo;
    HIFB_COLORKEY_S stColorKey;
    TDE2_RECT_S stSrcRect,stDstRect;
    TDE2_SURFACE_S stSrc,stDst;
    HI_U32 Phyaddr;
    HI_VOID *Viraddr;
    int last_sec = 0;
    struct tm *tmnow;
    struct timeval tv;
    HI_U8 Dispbuf[100], ucBuf[100], ucCount;
    (void)GUI_Init();
    if(HI_NULL == pData)
    {
        return HI_NULL;
    }
    pstInfo = (PTHREAD_HIFB_SAMPLE_INFO *)pData;
    switch (pstInfo->layer)
    {
    case 0 :
        strcpy(file, "/dev/fb0");
        break;
    case 1 :
        strcpy(file, "/dev/fb1");
        break;
    case 2 :
        strcpy(file, "/dev/fb2");
        break;
    case 3 :
        strcpy(file, "/dev/fb3");
        break;
    default:
        strcpy(file, "/dev/fb0");
        break;
    }
    /* 1. open framebuffer device overlay 0 */
    pstInfo->fd = open(file, O_RDWR, 0);
    if(pstInfo->fd < 0)
    {
        printf("open %s failed!\n",file);
        return HI_NULL;
    }
    if(pstInfo->layer == HIFB_LAYER_0  )
    {
        if (ioctl(pstInfo->fd, FBIOPUT_COMPRESSION_HIFB, &g_bCompress) < 0)
        {
            printf("Func:%s line:%d FBIOPUT_COMPRESSION_HIFB failed!\n",
                   __FUNCTION__, __LINE__);
            close(pstInfo->fd);
            return HI_NULL;
        }
    }
    bShow = HI_FALSE;
    if (ioctl(pstInfo->fd, FBIOPUT_SHOW_HIFB, &bShow) < 0)
    {
        printf("FBIOPUT_SHOW_HIFB failed!\n");
        return HI_NULL;
    }
    /* 2. set the screen original position */
    switch(pstInfo->ctrlkey)
    {
    case 3:
    {
        stPoint.s32XPos = 150;
        stPoint.s32YPos = 150;
    }
    break;
    default:
    {
        stPoint.s32XPos = 0;
        stPoint.s32YPos = 0;
    }
    }
    if (ioctl(pstInfo->fd, FBIOPUT_SCREEN_ORIGIN_HIFB, &stPoint) < 0)
    {
        printf("set screen original show position failed!\n");
        close(pstInfo->fd);
        return HI_NULL;
    }
    /* 3.set alpha */
    stAlpha.bAlphaEnable = HI_FALSE;
    stAlpha.bAlphaChannel = HI_FALSE;
    stAlpha.u8Alpha0 = 0x0;
    stAlpha.u8Alpha1 = 0xff;
    stAlpha.u8GlobalAlpha = 0x80;
    if (ioctl(pstInfo->fd, FBIOPUT_ALPHA_HIFB,  &stAlpha) < 0)
    {
        printf("Set alpha failed!\n");
        close(pstInfo->fd);
        return HI_NULL;
    }
    /*all layer surport colorkey*/
    stColorKey.bKeyEnable = HI_TRUE;
    stColorKey.u32Key = 0x0;
    if (ioctl(pstInfo->fd, FBIOPUT_COLORKEY_HIFB, &stColorKey) < 0)
    {
        printf("FBIOPUT_COLORKEY_HIFB!\n");
        close(pstInfo->fd);
        return HI_NULL;
    }
    /* 4. get the variable screen info */
    if (ioctl(pstInfo->fd, FBIOGET_VSCREENINFO, &var) < 0)
    {
        printf("Get variable screen info failed!\n");
        close(pstInfo->fd);
        return HI_NULL;
    }
    /* 5. modify the variable screen info
    the screen size: IMAGE_WIDTH*IMAGE_HEIGHT
    the virtual screen size: VIR_SCREEN_WIDTH*VIR_SCREEN_HEIGHT
    (which equals to VIR_SCREEN_WIDTH*(IMAGE_HEIGHT*2))
    the pixel format: ARGB1555
    */
    usleep(4*1000*1000);
    switch(pstInfo->ctrlkey)
    {
    case 3:
    {
        var.xres_virtual = 48;
        var.yres_virtual = 48;
        var.xres = 48;
        var.yres = 48;
    }
    break;
    default:
    {
        var.xres_virtual = 1280;
        var.yres_virtual = 720*2;
        var.xres = 1280;
        var.yres = 720;
    }
    }
    var.transp= g_a16;
    var.red = g_r16;
    var.green = g_g16;
    var.blue = g_b16;
    var.bits_per_pixel = 16;
    var.activate = FB_ACTIVATE_NOW;
    /* 6. set the variable screeninfo */
    if (ioctl(pstInfo->fd, FBIOPUT_VSCREENINFO, &var) < 0)
    {
        printf("Put variable screen info failed!\n");
        close(pstInfo->fd);
        return HI_NULL;
    }
    /* 7. get the fix screen info */
    if (ioctl(pstInfo->fd, FBIOGET_FSCREENINFO, &fix) < 0)
    {
        printf("Get fix screen info failed!\n");
        close(pstInfo->fd);
        return HI_NULL;
    }
    u32FixScreenStride = fix.line_length;   /*fix screen stride*/
    /* 8. map the physical video memory for user use */
    pShowScreen = mmap(HI_NULL, fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, pstInfo->fd, 0);
    if(MAP_FAILED == pShowScreen)
    {
        printf("mmap framebuffer failed!\n");
        close(pstInfo->fd);
        return HI_NULL;
    }
    memset(pShowScreen, 0x00, fix.smem_len);
    /* time to paly*/
    bShow = HI_TRUE;
    if (ioctl(pstInfo->fd, FBIOPUT_SHOW_HIFB, &bShow) < 0)
    {
        printf("FBIOPUT_SHOW_HIFB failed!\n");
        munmap(pShowScreen, fix.smem_len);
        return HI_NULL;
    }
    /**************************************获得时间数据***************************/
    gettimeofday(&tv,NULL);
    tmnow = localtime(&tv.tv_sec);
    (void)GUI_Init();
    /* show bitmap or cosor*/
    switch(pstInfo->ctrlkey)
    {
    case 2:
    {
        while(1)
        {
            while(1)
            {
                gettimeofday(&tv,NULL);
                if(tv.tv_sec != last_sec)
                {
                    last_sec = tv.tv_sec;
                    break;
                }
                usleep(20000);
            }
            rtc_manage(NULL,GET_TIME );
            TimeConvertToString(ucBuf, (HI_U8 *)&rtc, HI_TRUE, &ucCount);
            utf82gbk(Dispbuf, ucBuf, sizeof(Dispbuf));
            GUI_DispStringAt(Dispbuf, 100, 300);
            GUI_DispStringAt(Dispbuf, 100, 400);
            GUI_DispStringAt(Dispbuf, 100, 500);
            GUI_DispStringAt(Dispbuf, 100, 600);
            //utf82gbk(Dispbuf, "天津123abcABC23七一二通信广播有限公司", sizeof(Dispbuf));
            //GUI_DispStringAt(Dispbuf, 100, 500);
        }
        break;
    }
    default:
    {
        break;
    }
    }
    /* unmap the physical memory */
    munmap(pShowScreen, fix.smem_len);
    bShow = HI_FALSE;
    if (ioctl(pstInfo->fd, FBIOPUT_SHOW_HIFB, &bShow) < 0)
    {
        printf("FBIOPUT_SHOW_HIFB failed!\n");
        return HI_NULL;
    }
    close(pstInfo->fd);
    return HI_NULL;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
HI_U16 gusShowBitmap[50][2000]= {0};

/* 函数描述:
得到某个字符的点阵，放在DestBuf里，
其中的字符的宽度为Width，高度为Hight，
该字符实际所需的点阵宽度为RealWidth ,
CharRealWidth 主要用来在多字符显示时的宽度
因为有时点阵和字节宽度不匹配*/
void GUIPROP_FillAreaChar(HI_U16 c, HI_U16 *DestBuf, HI_U8 *Width, HI_U8 *Hight, HI_U8 *CharRealWidth)
{
    int BytesPerLine, i, j, usCount;
    HI_U8 *Ptr, ucCount, ucTemp;
    const GUI_FONT_PROP GUI_UNI_PTR * pProp = GUIPROP_FindChar(GUI_Context.pAFont->p.pProp, c);
    if (pProp)
    {
        const GUI_CHARINFO GUI_UNI_PTR * pCharInfo = pProp->paCharInfo+(c-pProp->First);
        BytesPerLine = pCharInfo->BytesPerLine;
        Ptr =  pCharInfo->pData;
        for (i = 0; i < GUI_Context.pAFont->YSize; i++)
        {
            for (j = 0; j < BytesPerLine; j++)
            {
                ucTemp = *Ptr;
                for (ucCount = 0; ucCount < 8; ucCount++)
                {
                    if (0x80 == (ucTemp & 0x80))
                    {
                        *DestBuf = 0xffff;//(HI_U16)(1 << 15);
                    }
                    else
                    {
                        *DestBuf = 0x0000;//(HI_U16)(0 << 15);
                    }
                    DestBuf++;
                    ucTemp <<= 1;
                }
                Ptr++;
            }
        }
        *Width =  BytesPerLine * 8;
        *Hight = GUI_Context.pAFont->YSize;
        *CharRealWidth = pCharInfo->XSize;
    }
}

void GUIPROP_FillAreaString(HI_U8 *p ,HI_U8 Length, HI_U16 *TotalDotWidth, HI_U16 *TotalDotHight)
{
    HI_U16 ConvertBit[2000]= {0};
    HI_U16 usTotalWidth;
    HI_U16 usWidthCount, usHightCount;
    HI_U8 ucNum, ucFlag;
    HI_U16  usTemp, usCharDotHight, usCharDotWidth, usCharRealWidth;
    usTotalWidth = 0;
    for(ucNum=0; ucNum<Length; ucNum++, p++)
    {
        if (*p > 0x7f)/* 英文字符 */
        {
            usTemp = ((HI_U16)*p << 8) + (HI_U16)*(p+ 1);
            ucFlag = HI_TRUE;
        }
        else/* 中文字符 */
        {
            usTemp = (HI_U16) *p;
            ucFlag = HI_FALSE;
        }
        GUIPROP_FillAreaChar(usTemp, &ConvertBit, &usCharDotWidth, &usCharDotHight, &usCharRealWidth);
        for (usHightCount = 0; usHightCount < usCharDotHight; usHightCount++)
        {
            for (usWidthCount = 0; usWidthCount < usCharDotWidth; usWidthCount++)
            {
                if(HI_TRUE == ucFlag)/* 英文字符 */
                {
                    gusShowBitmap[usHightCount][usWidthCount + ucNum * usCharRealWidth / 2] = ConvertBit[usWidthCount +  usHightCount * usCharDotWidth];
                }
                else/* 中文字符 */
                {
                    gusShowBitmap[usHightCount][usWidthCount + ucNum * usCharRealWidth] = ConvertBit[usWidthCount +  usHightCount * usCharDotWidth];
                }
            }
        }
        usTotalWidth += usCharRealWidth;
        if(HI_TRUE == ucFlag)
        {
            ucNum++;
            p++;
        }
    }
    *TotalDotWidth = usTotalWidth;
    *TotalDotHight = usCharDotHight;
}


/******************************************************************************
* function : send stream to vdec
******************************************************************************/
void* SAMPLE_Hifb_Region_VENC_Proc(void* p)
{
    HI_S32 i, j,num;
    HI_S32 s32Ret = HI_FAILURE;
    RGN_HANDLE RgnHandle;
    RGN_ATTR_S stRgnAttr;
    MPP_CHN_S stChn;
    VENC_GRP VencGrp;
    RGN_CHN_ATTR_S stChnAttr;
    HI_U32 u32Layer;
    HI_U32 u32Color;
    HI_U32 u32Alpha;
    POINT_S stPoint;
    BITMAP_S stBitmap;
    SAMPLE_RGN_CHANGE_TYPE_EN enChangeType;
    HI_BOOL bShow = HI_FALSE;
    //这样最大的字不能超过50行
    HI_U16 finaldata[40][400] = {0};
    HI_U16 ShowWidth = 0,ShowRows = 0;
    HI_U32 length = 0, maxrow = 0, width = 0;
    char osdcontent[256] = {0};
    HI_U16 usWidthCount, usHightCount;
    HI_U8 ucCharRealWidth, ucFlag;
    HI_U16 totalWidth, totalHight;


    RGN_HANDLE OverlayExHandle = 0;
    RGN_ATTR_S stOverlayExAttr;
    MPP_CHN_S stOverlayExChn;
    RGN_CHN_ATTR_S stOverlayExChnAttr;
    HI_S32 ViDev = 0;
    
    
    /**************************************
    添加字库的文件
    *******************************************/
    int last_sec = 0;
    struct tm *tmnow;
    struct timeval tv;
    /**************************************获得时间数据***************************/
    gettimeofday(&tv,NULL);
    tmnow = localtime(&tv.tv_sec);

    /* 创建了八个不同的方块区域*/
    //创建了八个不同的方块区域
    for (i = 0; i< gs_s32RgnCnt; i++)
    {
        OverlayExHandle  = i;
        #if 0
        stOverlayExAttr.enType = OVERLAY_RGN;//OVERLAYEX_RGN;
        #endif
        #if 1
        stOverlayExAttr.enType = OVERLAYEX_RGN;
        #endif
        stOverlayExAttr.unAttr.stOverlayEx.enPixelFmt = PIXEL_FORMAT_RGB_1555;
        stOverlayExAttr.unAttr.stOverlayEx.u32BgColor =  0x00ffffff;
        stOverlayExAttr.unAttr.stOverlayEx.stSize.u32Width= 400;
        stOverlayExAttr.unAttr.stOverlayEx.stSize.u32Height = 38;     

        s32Ret = HI_MPI_RGN_Create(OverlayExHandle, &stOverlayExAttr);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_RGN_Create failed with %#x!\n", s32Ret);
            return HI_FAILURE;
        }
    }
    
    /*********************************************
    step 2: display overlay regions to venc groups
    *********************************************/
    OverlayExHandle = 0;
    for (i = 0; i< gs_s32RgnCnt; i++)
    {
#if 1
        stOverlayExChn.enModId = HI_ID_VIU;
        stOverlayExChn.s32DevId = ViDev;
        stOverlayExChn.s32ChnId = i;
        
        stOverlayExChnAttr.enType = OVERLAYEX_RGN;
        stOverlayExChnAttr.bShow = HI_TRUE;
        stOverlayExChnAttr.unChnAttr.stOverlayExChn.stPoint.s32X = 128;
        stOverlayExChnAttr.unChnAttr.stOverlayExChn.stPoint.s32Y = 128;
        stOverlayExChnAttr.unChnAttr.stOverlayExChn.u32BgAlpha = 0;
        stOverlayExChnAttr.unChnAttr.stOverlayExChn.u32FgAlpha = 128;
        stOverlayExChnAttr.unChnAttr.stOverlayExChn.u32Layer = 1;
        
#endif

#if 0
        VencGrp = i;
        printf("VencGrp = %d\n",VencGrp);
        stOverlayExChn.enModId = HI_ID_GROUP;
        stOverlayExChn.s32DevId = VencGrp;
        stOverlayExChn.s32ChnId = 0;

        memset(&stOverlayExChnAttr, 0x00, sizeof(stOverlayExChnAttr));
        stOverlayExChnAttr.bShow = HI_TRUE;
        stOverlayExChnAttr.enType = OVERLAY_RGN;
        stOverlayExChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 128;
        stOverlayExChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = 128;
        stOverlayExChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
        stOverlayExChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
        stOverlayExChnAttr.unChnAttr.stOverlayChn.u32Layer = 1;
        stOverlayExChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
        stOverlayExChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;
#endif
        s32Ret = HI_MPI_RGN_AttachToChn(OverlayExHandle,&stOverlayExChn,&stOverlayExChnAttr);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_RGN_AttachToChn failed with %#x!\n", s32Ret);
            return HI_FAILURE;
        }        
    }

    
    printf("display region to chn success!\n");
    /*********************************************
    step 3: change overlay regions' position
    *********************************************/
    (void)GUI_Init();
    usleep(100000);
    //create font face from font file
    while(1)
    {
        rtc_manage(NULL,GET_TIME );
        tmnow->tm_year = rtc.year;
        tmnow->tm_mon = rtc.month;
        tmnow->tm_mday = rtc.date;
        tmnow->tm_hour = rtc.hour;
        tmnow->tm_min = rtc.minute;
        tmnow->tm_sec = rtc.second;
        printf("run here now... \n");
        sprintf(osdcontent,"%04d-%02d-%02d/%02d:%02d:%02d",tmnow->tm_year, tmnow->tm_mon, tmnow->tm_mday,tmnow->tm_hour,tmnow->tm_min, tmnow->tm_sec);
        printf("enter the loop... \n");
        
        GUIPROP_FillAreaString(osdcontent, strlen(osdcontent), &totalWidth, &totalHight);
        //将图像数据做到行对齐进行数据的二次拷贝
        for (num = 0; num < totalHight; num++)
        {
            memcpy(finaldata[num], gusShowBitmap[num], totalWidth * 2);
        }
        /*********************************************
        step 6: show bitmap
        *********************************************/
        OverlayExHandle = 0;
        stBitmap.u32Width = 400;
        stBitmap.u32Height = totalHight;
        printf("stBitmap.u32Width=%d stBitmap.u32Height =%d \n",stBitmap.u32Width,stBitmap.u32Height);
        stBitmap.pData = (HI_VOID *)&finaldata[0][0];
        stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
        /*********************************************
        step 6: show bitmap
        *********************************************/
        s32Ret = HI_MPI_RGN_SetBitMap(OverlayExHandle,&stBitmap);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
            return HI_FAILURE;
        }
        if (NULL != stBitmap.pData)
        {
            free(stBitmap.pData);
            stBitmap.pData = NULL;
        }
        usleep(SAMPLE_RGN_SLEEP_TIME*5);
        printf("handle:%d,load bmp success!\n",OverlayExHandle);
        memset(finaldata,0,sizeof(finaldata));
        i = 0;
        j = num = 0;
        length = 0;
        maxrow = 0;
        usleep(SAMPLE_RGN_SLEEP_TIME * 1);
    }
    /*********************************************
    step 12: destory region
    *********************************************/
    for (i = 0; i < gs_s32RgnCnt; i++)
    {
        OverlayExHandle = i;
        s32Ret = HI_MPI_RGN_Destroy(OverlayExHandle);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_RGN_Destroy [%d] failed with %#x\n",\
                       OverlayExHandle, s32Ret);
        }
        break;
    }
    SAMPLE_PRT("destory all region success!\n");
    return HI_SUCCESS;
}


/******************************************************************************
* funciton : start get venc stream process thread
******************************************************************************/
HI_S32 SAMPLE_Region_Venc(HI_S32 s32Cnt)
{
    gs_stPara.bThreadStart = HI_TRUE;
    gs_stPara.s32Cnt = s32Cnt;
    //return pthread_create(&gs_RegionVencPid, 0, SAMPLE_Region_VENC_Proc, (HI_VOID*)&gs_stPara);
    return pthread_create(&gs_RegionVencPid, 0, SAMPLE_Hifb_Region_VENC_Proc, (HI_VOID*)&gs_stPara);
}
/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VIO_Usage(char *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);
    printf("index:\n");
    printf("\t 0) VI:8*D1; VO:HD0(HDMI,VGA)+SD0(CVBS)+SD1 video preview.\n");
    return;
}
/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_VIO_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}
/******************************************************************************
* function : video loss detect process
* NOTE: If your ADC stop output signal when NoVideo, you can open VDET_USE_VI macro.
******************************************************************************/
void *SAMPLE_VI_AD26828_VLossDetProc(void *parg)
{
    int fd;
    HI_S32 s32Ret, i, s32ChnPerDev;
    VI_DEV ViDev;
    VI_CHN ViChn;
    tw2865_video_loss video_loss;
    SAMPLE_VI_PARAM_S stViParam;
    SAMPLE_VIDEO_LOSS_S *ctl = (SAMPLE_VIDEO_LOSS_S*)parg;
    fd = open(CX26828_FILE, O_RDWR);//TW2865_FILE
    if (fd < 0)
    {
        printf("open %s fail\n", CX26828_FILE);
        ctl->bStart = HI_FALSE;
        return NULL;
    }
    s32Ret = SAMPLE_COMM_VI_Mode2Param(ctl->enViMode, &stViParam);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("vi get param failed!\n");
        return NULL;
    }
    s32ChnPerDev = stViParam.s32ViChnCnt / stViParam.s32ViDevCnt;
    while (ctl->bStart)
    {
        for (i = 0; i < stViParam.s32ViChnCnt; i++)
        {
            ViChn = i * stViParam.s32ViChnInterval;
            ViDev = SAMPLE_COMM_VI_GetDev(ctl->enViMode, ViChn);
            if (ViDev < 0)
            {
                SAMPLE_PRT("get vi dev failed !\n");
                return NULL;
            }
            //video_loss.chip = stViParam.s32ViDevCnt;
            video_loss.chip = 0;
            video_loss.ch   = ViChn % s32ChnPerDev;
            //printf("video_loss.chip %d, video_loss.ch %d\n",video_loss.chip,video_loss.ch );
            ioctl(fd, CX26828_GET_VIDEO_LOSS, &video_loss);
            if (video_loss.is_lost)
            {
                printf("pic loss\n");
                HI_MPI_VI_EnableUserPic(ViChn);
            }
            else
            {
                HI_MPI_VI_DisableUserPic(ViChn);
            }
        }
        usleep(500000);
    }
    close(fd);
    ctl->bStart = HI_FALSE;
    return NULL;
}
void *SAMPLE_VI_AD2865_VLossDetProc(void *parg)
{
    int fd;
    HI_S32 s32Ret, i, s32ChnPerDev;
    VI_DEV ViDev;
    VI_CHN ViChn;
    tw2865_video_loss video_loss;
    SAMPLE_VI_PARAM_S stViParam;
    SAMPLE_VIDEO_LOSS_S *ctl = (SAMPLE_VIDEO_LOSS_S*)parg;
    fd = open(TW2865_FILE, O_RDWR);//TW2865_FILE
    if (fd < 0)
    {
        printf("open %s fail\n", TW2865_FILE);
        ctl->bStart = HI_FALSE;
        return NULL;
    }
    s32Ret = SAMPLE_COMM_VI_Mode2Param(ctl->enViMode, &stViParam);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("vi get param failed!\n");
        return NULL;
    }
    s32ChnPerDev = stViParam.s32ViChnCnt / stViParam.s32ViDevCnt;
    while (ctl->bStart)
    {
        for (i = 0; i < stViParam.s32ViChnCnt; i++)
        {
            ViChn = i * stViParam.s32ViChnInterval;
            ViDev = SAMPLE_COMM_VI_GetDev(ctl->enViMode, ViChn);
            if (ViDev < 0)
            {
                SAMPLE_PRT("get vi dev failed !\n");
                return NULL;
            }
            //video_loss.chip = stViParam.s32ViDevCnt;
            video_loss.chip = 0;
            video_loss.ch   = ViChn % s32ChnPerDev;
            //printf("video_loss.chip %d, video_loss.ch %d\n",video_loss.chip,video_loss.ch );
            ioctl(fd, TW2865_GET_VIDEO_LOSS, &video_loss);
            if (video_loss.is_lost)
            {
                printf("pic loss\n");
                HI_MPI_VI_EnableUserPic(ViChn);
            }
            else
            {
                HI_MPI_VI_DisableUserPic(ViChn);
            }
        }
        usleep(500000);
    }
    close(fd);
    ctl->bStart = HI_FALSE;
    return NULL;
}

//#define VDET_USE_VI
#ifdef VDET_USE_VI
static HI_S32 s_astViLastIntCnt[VIU_MAX_CHN_NUM] = {0};
void *SAMPLE_VI_VLossDetProc(void *parg)
{
    VI_CHN ViChn;
    SAMPLE_VI_PARAM_S stViParam;
    HI_S32 s32Ret, i, s32ChnPerDev;
    VI_CHN_STAT_S stStat;
    SAMPLE_VIDEO_LOSS_S *ctl = (SAMPLE_VIDEO_LOSS_S*)parg;
    s32Ret = SAMPLE_COMM_VI_Mode2Param(ctl->enViMode, &stViParam);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("vi get param failed!\n");
        return NULL;
    }
    s32ChnPerDev = stViParam.s32ViChnCnt / stViParam.s32ViDevCnt;
    while (ctl->bStart)
    {
        for (i = 0; i < stViParam.s32ViChnCnt; i++)
        {
            ViChn = i * stViParam.s32ViChnInterval;
            s32Ret = HI_MPI_VI_Query(ViChn, &stStat);
            if (HI_SUCCESS !=s32Ret)
            {
                SAMPLE_PRT("HI_MPI_VI_Query failed with %#x!\n", s32Ret);
                return NULL;
            }
            if (stStat.u32IntCnt == s_astViLastIntCnt[i])
            {
                printf("VI Chn (%d) int lost , int_cnt:%d \n", ViChn, stStat.u32IntCnt);
                HI_MPI_VI_EnableUserPic(ViChn);
            }
            else
            {
                HI_MPI_VI_DisableUserPic(ViChn);
            }
            s_astViLastIntCnt[i] = stStat.u32IntCnt;
        }
        usleep(500000);
    }
    ctl->bStart = HI_FALSE;
    return NULL;
}
#endif
HI_S32 SAMPLE_VI_StartVLossDet(SAMPLE_VI_MODE_E enViMode)
{
    HI_S32 s32Ret;
    gs_stVideoLoss.bStart= HI_TRUE;
    gs_stVideoLoss.enViMode = enViMode;
#ifdef VDET_USE_VI
    s32Ret = pthread_create(&gs_stVideoLoss.Pid, 0, SAMPLE_VI_VLossDetProc, &gs_stVideoLoss);
#else
#ifdef DEMO
    s32Ret = pthread_create(&gs_stVideoLoss.Pid, 0, SAMPLE_VI_AD26828_VLossDetProc, &gs_stVideoLoss);
#else
    s32Ret = pthread_create(&gs_stVideoLoss.Pid, 0, SAMPLE_VI_AD2865_VLossDetProc, &gs_stVideoLoss);
#endif
#endif
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("pthread_create failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}
HI_VOID SAMPLE_VI_StopVLossDet()
{
    if (gs_stVideoLoss.bStart)
    {
        gs_stVideoLoss.bStart = HI_FALSE;
        pthread_join(gs_stVideoLoss.Pid, 0);
    }
    return;
}
HI_S32 SAMPLE_VI_SetUserPic(HI_CHAR *pszYuvFile, HI_U32 u32Width, HI_U32 u32Height,
                            HI_U32 u32Stride, VIDEO_FRAME_INFO_S *pstFrame)
{
    FILE *pfd;
    VI_USERPIC_ATTR_S stUserPicAttr;
    /* open YUV file */
    pfd = fopen(pszYuvFile, "rb");
    if (!pfd)
    {
        printf("open file -> %s fail \n", pszYuvFile);
        return -1;
    }
    /* read YUV file. WARNING: we only support planar 420) */
    if (SAMPLE_COMM_VI_GetVFrameFromYUV(pfd, u32Width, u32Height, u32Stride, pstFrame))
    {
        return -1;
    }
    fclose(pfd);
    stUserPicAttr.bPub= HI_TRUE;
    stUserPicAttr.enUsrPicMode = VI_USERPIC_MODE_PIC;
    memcpy(&stUserPicAttr.unUsrPic.stUsrPicFrm, pstFrame, sizeof(VIDEO_FRAME_INFO_S));
    if (HI_MPI_VI_SetUserPic(0, &stUserPicAttr))
    {
        return -1;
    }
    printf("set vi user pic ok, yuvfile:%s\n", pszYuvFile);
    return HI_SUCCESS;
}
/******************************************************************************
* function :  VI:8*D1; VO:HD0(HDMI,VGA)+SD0(CVBS)+SD1 video preview
******************************************************************************/
HI_S32 SAMPLE_VIO_8_D1(PIC_SIZE_E enCodeSize, HI_U8 EncMode, VO_DEV VoDevMode)
{
    SAMPLE_VI_MODE_E enViMode =  SAMPLE_VI_MODE_8_D1;
    HI_U32 u32ViChnCnt = 8;
    HI_S32 s32VpssGrpCnt = 16;/* must 16 */
    HI_S32 s32RgnCnt = 8;
    VB_CONF_S stVbConf;
    VI_CHN ViChn;
    VPSS_GRP VpssGrp;
    VPSS_GRP_ATTR_S stGrpAttr;
    VPSS_CHN VpssChn_VoHD0 = VPSS_PRE0_CHN;//VPSS_BYPASS_CHN;//VPSS_PRE0_CHN;
    VO_DEV VoDev = VoDevMode;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr,stVoPubAttrSD;
    SAMPLE_VO_MODE_E enVoMode, enPreVoMode;
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    HI_CHAR ch;
    SIZE_S stSize;
    HI_U32 u32WndNum = 8;
    VO_WBC_ATTR_S stWbcAttr;
    PAYLOAD_TYPE_E enPayLoad = PT_H264;
    PIC_SIZE_E enSize =  enCodeSize;
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode;
    /******************************************
     step  1: init variable
    ******************************************/
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL== gs_enNorm)?25:30;
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                 enCodeSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    /* video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 8;
    memset(stVbConf.astCommPool[0].acMmzName, 0, sizeof(stVbConf.astCommPool[0].acMmzName));
    /* hist buf*/
    stVbConf.astCommPool[1].u32BlkSize = (196 * 4);
    stVbConf.astCommPool[1].u32BlkCnt = u32ViChnCnt * 8;
    memset(stVbConf.astCommPool[1].acMmzName, 0, sizeof(stVbConf.astCommPool[1].acMmzName));
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_8D1_0;
    }
    /******************************************
     step 3: start vi dev & chn
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_8D1_0;
    }
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_8D1_0;
    }
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_FALSE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM,NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_8D1_1;
    }
    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_8D1_2;
    }
    /******************************************
    step 5: select encode mode CBR VCR FIXQP
    ******************************************/
    enRcMode = EncMode;
    /******************************************
     step 5: start stream venc (big + little)
    ******************************************/
    for (i=0; i<u32ViChnCnt; i++)
    {
        /*** main stream **/
        VencGrp = i;
        VencChn = i;
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad,\
                                        gs_enNorm, enSize, enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_8D1_2;
        }
        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VPSS_BSTR_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_8D1_3;
        }
    }
    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32ViChnCnt);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_8D1_3;
    }
    /*Overlay Region Process*/
    s32Ret = SAMPLE_Region_Venc(VencChn);//(VencChn, s32RgnCnt);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_8D1_3;
    }
    printf("please press any key to exit\n");
    /* 输出显示 */
    if (SAMPLE_VO_DEV_DHD0 == VoDevMode)
    {
        VpssChn_VoHD0 = VPSS_PRE0_CHN;
        /*  HDMI + VGA */
        /******************************************
         step 6: start vo HD0 (HDMI+VGA), multi-screen, you can switch mode
        ******************************************/
        printf("start vo HD0.\n");
        enVoMode = VO_MODE_9MUX;
        if(VIDEO_ENCODING_MODE_PAL == gs_enNorm)
        {
            stVoPubAttr.enIntfSync =  VO_OUTPUT_720P50;
        }
        else
        {
            stVoPubAttr.enIntfSync =  VO_OUTPUT_720P60;
        }
#ifdef HI_FPGA
        stVoPubAttr.enIntfType = VO_INTF_HDMI|VO_INTF_VGA|VO_INTF_BT1120;
#else
        stVoPubAttr.enIntfType =  VO_INTF_VGA;//VO_INTF_HDMI|VO_INTF_VGA;
#endif
        stVoPubAttr.u32BgColor = 0x000000ff;
        stVoPubAttr.bDoubleFrame = HI_TRUE;
    }
    else
    {
        VpssChn_VoHD0 = VPSS_BYPASS_CHN;
        /******************************************
        step 5: start vo SD1(CVBS)
        ******************************************/
        printf("start vo SD1.\n");
        VoDev = SAMPLE_VO_DEV_DSD1;
        //u32WndNum = 8;
        enVoMode = VO_MODE_9MUX;
        stVoPubAttr.enIntfSync = VO_OUTPUT_PAL;
        stVoPubAttr.enIntfType = VO_INTF_CVBS;
        stVoPubAttr.u32BgColor = 0x000000ff;
        stVoPubAttr.bDoubleFrame = HI_FALSE;
    }
    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, gs_u32ViFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartDevLayer failed!\n");
        goto END_8D1_4;
    }
    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartChn failed!\n");
        goto END_8D1_5;
    }
    /* if it's displayed on HDMI, we should start HDMI */
    if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
    {
        if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
        {
            SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
            goto END_8D1_5;
        }
    }
    for (i = 0; i < u32WndNum; i++)
    {
        VoChn = i ;/* out channel 0-----vpss 7 */
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VpssChn_VoHD0);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
            goto END_8D1_5;
        }
    }
    /******************************************
    step 8: HD0 switch mode
    ******************************************/
    enVoMode = VO_MODE_9MUX;
    while(1)
    {
        enPreVoMode = enVoMode;
        printf("please choose preview mode, press 'q' to exit this sample.\n");
        printf("\t0) 1 preview\n");
        printf("\t1) 4 preview\n");
        printf("\t2) 8 preview\n");
        printf("\t3) dec code\n");
        printf("\tq) quit\n");
        ch = getchar();
        getchar();
        if ('0' == ch)
        {
            enVoMode = VO_MODE_1MUX;
        }
        else if ('1' == ch)
        {
            enVoMode = VO_MODE_4MUX;
        }
        /*Indeed only 8 chns show*/
        else if ('2' == ch)
        {
            enVoMode = VO_MODE_9MUX;
        }
        else if  ('3' == ch)/* 解码开始 */
        {
            enVoMode = VO_MODE_9MUX;
            if (SAMPLE_VO_DEV_DHD0 == VoDevMode)
            {
                VpssChn_VoHD0 = VPSS_PRE0_CHN;
            }
            else
            {
                VpssChn_VoHD0 = VPSS_BYPASS_CHN;
            }
            for(i=0; i<u32WndNum; i++)
            {
                VoChn = i;
                VpssGrp = i;
                s32Ret = SAMPLE_COMM_VO_UnBindVpss(VoDevMode,VoChn,VpssGrp,VpssChn_VoHD0);
                if (HI_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("SAMPLE_COMM_VO_UnBindVpss failed!\n");
                    goto END_8D1_4;
                }
            }
            SAMPLE_VDEC_Process1(PIC_D1, PT_H264, u32WndNum, VoDevMode);
            for(i=0; i<u32WndNum; i++)
            {
                VoChn = i ;/* out channel 0-----vpss 7 */
                VpssGrp = i;
                s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VpssChn_VoHD0);
                if (HI_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("Start VO failed!\n");
                    goto END_8D1_5;
                }
            }
        }
        else if ('q' == ch)
        {
            break;
        }
        else
        {
            SAMPLE_PRT("preview mode invaild! please try again.\n");
            continue;
        }
        SAMPLE_PRT("vo(%d) switch to %d mode\n", VoDev, u32WndNum);
        s32Ret= HI_MPI_VO_SetAttrBegin(VoDev);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
            goto END_8D1_7;
        }
        s32Ret = SAMPLE_COMM_VO_StopChn(VoDev, enPreVoMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
            goto END_8D1_7;
        }
        s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
            goto END_8D1_7;
        }
        s32Ret= HI_MPI_VO_SetAttrEnd(VoDev);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
            goto END_8D1_7;
        }
    }
    /******************************************
     step 8: exit process
    ******************************************/
END_VENC_8D1_3:
    SAMPLE_COMM_VENC_StopGetStream();
    for (i=0; i<u32ViChnCnt; i++)
    {
        VencGrp = i;
        VencChn = i;
        VpssGrp = i;
        VpssChn_VoHD0 = VPSS_PRE0_CHN;
        SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn_VoHD0);
        SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
    }
END_8D1_7:
    SAMPLE_COMM_VO_UnBindVoWbc(SAMPLE_VO_DEV_DSD0, 0);
    HI_MPI_VO_DisableWbc(SAMPLE_VO_DEV_DHD0);
END_8D1_6:
    VoDev = SAMPLE_VO_DEV_DSD0;
    VoChn = 0;
    enVoMode = VO_MODE_1MUX;
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_8D1_5:
    if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
    {
        SAMPLE_COMM_VO_HdmiStop();
    }
    VoDev = SAMPLE_VO_DEV_DHD0;
    u32WndNum = 16;
    enVoMode = VO_MODE_16MUX;
    /*??disableChn ,????V??????*/
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
    for(i=0; i<u32WndNum; i++)
    {
        VoChn = i;
        VpssGrp = i;
        SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,VpssChn_VoHD0);
    }
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_8D1_4:
END_8D1_3:  //vi unbind vpss
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_8D1_2:  //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_8D1_1:  //vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_8D1_0:  //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}
/*used for convert time frome string to struct rtc_time_t*/
static int parse_string(char *string, rtc_time_t *p_tm)
{
    char *comma, *head;
    int value[10];
    int i;
    if (!string || !p_tm)
    {
        return -1;
    }
    if (!strchr(string, '/'))
    {
        return -1;
    }
    head = string;
    i = 0;
    comma = NULL;
    for(;;)
    {
        comma = strchr(head, '/');
        if (!comma)
        {
            value[i++] = atoi(head);
            break;
        }
        *comma = '\0';
        value[i++] = atoi(head);
        head = comma+1;
    }
    if (i < 5)
    {
        return -1;
    }
    p_tm->year   = value[0];
    p_tm->month  = value[1];
    p_tm->date   = value[2];
    p_tm->hour   = value[3];
    p_tm->minute = value[4];
    p_tm->second = value[5];
    p_tm->weekday = 0;
    return 0;
}
int rtc_manage(const char *pstr, int commd)
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
#if 0
        printf("Current time value: \n");
        printf("year:%d\n", rtc.year);
        printf("month:%d\n", rtc.month);
        printf("date:%d\n", rtc.date);
        printf("hour:%d\n", rtc.hour);
        printf("minute:%d\n", rtc.minute);
        printf("second:%d\n", rtc.second);
#endif
        break;
    }
    return 3;
err1:
    close(fd);
    return 0;
}
/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VENC_Usage(char *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);
    printf("index:\n");
    printf("\t 0) 8D1 H264 encode.\n");
    printf("\t 1) 1*720p H264 encode.\n");
    printf("\t 2) 8D1 MJPEG encode.\n");
    printf("\t 3) 4D1 JPEG snap with StartRecvPic.\n");
    printf("\t 4) 4D1 JPEG snap with StartRecvPicEx.\n");
    printf("\t 5) 8*Cif JPEG snap.\n");
    printf("\t 6) 1D1 User send pictures for H264 encode.\n");
    printf("\t 7) 4D1 H264 encode with color2grey.\n");
    return;
}
/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_VENC_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}
/******************************************************************************
* function : to process abnormal case - the case of stream venc
******************************************************************************/
void SAMPLE_VENC_StreamHandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }
    exit(0);
}
/******************************************************************************
* function :  1HD H264 encode
******************************************************************************/
HI_S32 SAMPLE_VENC_1HD_H264(HI_VOID)
{
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_1_720P;
    HI_U32 u32ViChnCnt = 1;
    HI_S32 s32VpssGrpCnt = 1;
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H264, PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD720, PIC_D1};
    VB_CONF_S stVbConf;
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stGrpAttr;
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode;
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    HI_CHAR ch;
    SIZE_S stSize;
    /******************************************
     step  1: init variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                 PIC_HD720, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    /* video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 6;
    memset(stVbConf.astCommPool[0].acMmzName,0,
           sizeof(stVbConf.astCommPool[0].acMmzName));
    /* hist buf*/
    stVbConf.astCommPool[1].u32BlkSize = (196*4);
    stVbConf.astCommPool[1].u32BlkCnt = u32ViChnCnt * 4;
    memset(stVbConf.astCommPool[1].acMmzName,0,
           sizeof(stVbConf.astCommPool[1].acMmzName));
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1HD_0;
    }
    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1HD_0;
    }
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, PIC_HD720, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1HD_0;
    }
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM, &stGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_1HD_1;
    }
    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_1HD_2;
    }
    /******************************************
     step 5: select rc mode
    ******************************************/
    while(1)
    {
        printf("please choose rc mode:\n");
        printf("\t0) CBR\n");
        printf("\t1) VBR\n");
        printf("\t2) FIXQP\n");
        ch = getchar();
        getchar();
        if ('0' == ch)
        {
            enRcMode = SAMPLE_RC_CBR;
            break;
        }
        else if ('1' == ch)
        {
            enRcMode = SAMPLE_RC_VBR;
            break;
        }
        else if ('2' == ch)
        {
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        }
        else
        {
            printf("rc mode invaild! please try again.\n");
            continue;
        }
    }
    /******************************************
     step 6: start stream venc (big + little)
    ******************************************/
    for (i=0; i<u32ViChnCnt; i++)
    {
        /*** main frame **/
        VencGrp = i*2;
        VencChn = i*2;
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad[0],\
                                        gs_enNorm, enSize[0], enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_1HD_3;
        }
        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VPSS_BSTR_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_1HD_3;
        }
        /*** Sub frame **/
        VencGrp ++;
        VencChn ++;
        s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad[1], \
                                        gs_enNorm, enSize[1], enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_1HD_3;
        }
        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VPSS_PRE0_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_1HD_3;
        }
    }
    
    /******************************************
     step 7: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32ViChnCnt*2);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1HD_3;
    }
    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();
    /******************************************
     step 8: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();
END_VENC_1HD_3:
    for (i=0; i<u32ViChnCnt*2; i++)
    {
        VencGrp = i;
        VencChn = i;
        VpssGrp = i/2;
        VpssChn = (VpssGrp%2)?VPSS_PRE0_CHN:VPSS_BSTR_CHN;
        SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
        SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
    }
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_1HD_2: //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_1HD_1: //vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_1HD_0: //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}
/******************************************************************************
* function :  8D1 H264 encode
******************************************************************************/
HI_S32 SAMPLE_VENC_8D1_H264(HI_VOID)
{
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_D1;
    HI_U32 u32ViChnCnt = 8;
    HI_S32 s32VpssGrpCnt = 8;
    PAYLOAD_TYPE_E enPayLoad= PT_H264;
    PIC_SIZE_E enSize = PIC_D1;
    VB_CONF_S stVbConf;
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stGrpAttr;
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode;
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    HI_CHAR ch;
    SIZE_S stSize;
    /******************************************
     step  1: init variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                 PIC_D1, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 6;
    memset(stVbConf.astCommPool[0].acMmzName,0,
           sizeof(stVbConf.astCommPool[0].acMmzName));
    /* hist buf*/
    stVbConf.astCommPool[1].u32BlkSize = (196*4);
    stVbConf.astCommPool[1].u32BlkCnt = u32ViChnCnt * 6;
    memset(stVbConf.astCommPool[1].acMmzName,0,
           sizeof(stVbConf.astCommPool[1].acMmzName));
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_8D1_0;
    }
    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_8D1_0;
    }
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, PIC_D1, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_8D1_0;
    }
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM,NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_8D1_1;
    }
    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_8D1_2;
    }
    /******************************************
     step 5: select rc mode
    ******************************************/
    while(1)
    {
        printf("please choose rc mode:\n");
        printf("\t0) CBR\n");
        printf("\t1) VBR\n");
        printf("\t2) FIXQP\n");
        ch = getchar();
        getchar();
        if ('0' == ch)
        {
            enRcMode = SAMPLE_RC_CBR;
            break;
        }
        else if ('1' == ch)
        {
            enRcMode = SAMPLE_RC_VBR;
            break;
        }
        else if ('2' == ch)
        {
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        }
        else
        {
            printf("rc mode invaild! please try again.\n");
            continue;
        }
    }
    /******************************************
     step 5: start stream venc (big + little)
    ******************************************/
    for (i=0; i<u32ViChnCnt; i++)
    {
        /*** main stream **/
        VencGrp = i;
        VencChn = i;
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad,\
                                        gs_enNorm, enSize, enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_8D1_2;
        }
        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VPSS_BSTR_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_8D1_3;
        }
    }
    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32ViChnCnt);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_8D1_3;
    }
    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();
    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();
END_VENC_8D1_3:
    for (i=0; i<u32ViChnCnt; i++)
    {
        VencGrp = i;
        VencChn = i;
        VpssGrp = i;
        VpssChn = VPSS_PRE0_CHN;
        SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
        SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
    }
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_8D1_2: //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_8D1_1: //vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_8D1_0: //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}
/******************************************************************************
* function :  8D1 MJPEG encode
******************************************************************************/
HI_S32 SAMPLE_VENC_8D1_MJPEG(HI_VOID)
{
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_D1;
    HI_U32 u32ViChnCnt = 8;
    HI_S32 s32VpssGrpCnt = 8;
    PAYLOAD_TYPE_E enPayLoad = PT_MJPEG;
    PIC_SIZE_E enSize = PIC_D1;
    VB_CONF_S stVbConf;
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stGrpAttr;
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode;
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    HI_CHAR ch;
    SIZE_S stSize;
    /******************************************
     step  1: init variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                 PIC_D1, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    /*video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 8;
    memset(stVbConf.astCommPool[0].acMmzName,0,
           sizeof(stVbConf.astCommPool[0].acMmzName));
    /* hist buf*/
    stVbConf.astCommPool[1].u32BlkSize = (196*4);
    stVbConf.astCommPool[1].u32BlkCnt = u32ViChnCnt * 8;
    memset(stVbConf.astCommPool[1].acMmzName,0,
           sizeof(stVbConf.astCommPool[1].acMmzName));
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_MJPEG_0;
    }
    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_MJPEG_0;
    }
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, PIC_D1, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_MJPEG_0;
    }
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM,NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_MJPEG_1;
    }
    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_MJPEG_2;
    }
    /******************************************
     step 5: select rc mode
    ******************************************/
    while(1)
    {
        printf("please choose rc mode:\n");
        printf("\t0) CBR\n");
        printf("\t1) VBR\n");
        printf("\t2) FIXQP\n");
        ch = getchar();
        getchar();
        if ('0' == ch)
        {
            enRcMode = SAMPLE_RC_CBR;
            break;
        }
        else if ('1' == ch)
        {
            enRcMode = SAMPLE_RC_VBR;
            break;
        }
        else if ('2' == ch)
        {
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        }
        else
        {
            printf("rc mode invaild! please try again.\n");
            continue;
        }
    }
    /******************************************
     step 5: start stream venc
    ******************************************/
    for (i=0; i<u32ViChnCnt; i++)
    {
        /*** main frame **/
        VencGrp = i;
        VencChn = i;
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad,\
                                        gs_enNorm, enSize, enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_MJPEG_3;
        }
        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VPSS_PRE0_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_MJPEG_3;
        }
    }
    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32ViChnCnt);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_3;
    }
    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();
    /******************************************
     step 8: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();
END_VENC_MJPEG_3:
    for (i=0; i<u32ViChnCnt; i++)
    {
        VencGrp = i;
        VencChn = i;
        VpssGrp = i;
        VpssChn =VPSS_BSTR_CHN;
        SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
        SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
    }
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_MJPEG_2:   //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_MJPEG_1:   //vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_MJPEG_0:   //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}
/******************************************************************************
* function :  4D1 SNAP
******************************************************************************/
HI_S32 SAMPLE_VENC_4D1_Snap(HI_VOID)
{
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_4_D1;
    HI_U32 u32ViChnCnt = 4;
    HI_S32 s32VpssGrpCnt = 4;
    PIC_SIZE_E enSize = PIC_D1;
    VB_CONF_S stVbConf;
    VPSS_GRP VpssGrp;
    VPSS_GRP_ATTR_S stGrpAttr;
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    /******************************************
     step  1: init variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                 enSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    /* video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 15;
    memset(stVbConf.astCommPool[0].acMmzName,0,
           sizeof(stVbConf.astCommPool[0].acMmzName));
    /* hist buf*/
    stVbConf.astCommPool[1].u32BlkSize = (196*4);
    stVbConf.astCommPool[1].u32BlkCnt = u32ViChnCnt * 15;
    memset(stVbConf.astCommPool[1].acMmzName,0,
           sizeof(stVbConf.astCommPool[1].acMmzName));
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_SNAP_0;
    }
    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_SNAP_0;
    }
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_SNAP_0;
    }
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM,NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_SNAP_1;
    }
    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_SNAP_2;
    }
    /******************************************
     step 5: snap process
    ******************************************/
    VencGrp = 0;
    VencChn = 0;
    s32Ret = SAMPLE_COMM_VENC_SnapStart(VencGrp, VencChn, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start snap failed!\n");
        goto END_VENC_SNAP_3;
    }
    for (i=0; i<u32ViChnCnt; i++)
    {
        /*** main frame **/
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VENC_SnapProcess(VencGrp, VencChn, VpssGrp, VPSS_PRE0_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("snap process failed!\n");
            goto END_VENC_SNAP_4;
        }
        printf("snap chn %d ok!\n", i);
        sleep(1);
    }
    /******************************************
     step 8: exit process
    ******************************************/
    printf("snap over!\n");
END_VENC_SNAP_4:
    s32Ret = SAMPLE_COMM_VENC_SnapStop(VencGrp, VencChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Stop snap failed!\n");
        goto END_VENC_SNAP_3;
    }
END_VENC_SNAP_3:
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_SNAP_2:    //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_SNAP_1:    //vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_SNAP_0:    //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}
/******************************************************************************
* function :  16*Cif SNAP
******************************************************************************/
HI_S32 SAMPLE_VENC_8_Cif_Snap(HI_VOID)
{
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_2Cif;
    HI_U32 u32ViChnCnt = 8;
    HI_S32 s32VpssGrpCnt = 8;
    PIC_SIZE_E enSize = PIC_2CIF;
    VB_CONF_S stVbConf;
    VPSS_GRP VpssGrp;
    VPSS_GRP_ATTR_S stGrpAttr;
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    /******************************************
     step  1: init variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                 enSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    /* video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 6;
    memset(stVbConf.astCommPool[0].acMmzName,0,
           sizeof(stVbConf.astCommPool[0].acMmzName));
    /* hist buf*/
    stVbConf.astCommPool[1].u32BlkSize = (196*4);
    stVbConf.astCommPool[1].u32BlkCnt = u32ViChnCnt * 6;
    memset(stVbConf.astCommPool[1].acMmzName,0,
           sizeof(stVbConf.astCommPool[1].acMmzName));
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_SNAP_0;
    }
    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_SNAP_0;
    }
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_SNAP_0;
    }
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM,NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_SNAP_1;
    }
    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_SNAP_2;
    }
    /******************************************
     step 5: snap process
    ******************************************/
    VencGrp = 0;
    VencChn = 0;
    /*snap Cif pic*/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, PIC_CIF, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_SNAP_0;
    }
    s32Ret = SAMPLE_COMM_VENC_SnapStart(VencGrp, VencChn, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start snap failed!\n");
        goto END_VENC_SNAP_3;
    }
    for (i=0; i<u32ViChnCnt; i++)
    {
        /*** main frame **/
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VENC_SnapProcess(VencGrp, VencChn, VpssGrp, VPSS_PRE0_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("snap process failed!\n");
            goto END_VENC_SNAP_4;
        }
        printf("snap chn %d ok!\n", i);
    }
    /******************************************
     step 8: exit process
    ******************************************/
    printf("snap over!\n");
END_VENC_SNAP_4:
    s32Ret = SAMPLE_COMM_VENC_SnapStop(VencGrp, VencChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Stop snap failed!\n");
        goto END_VENC_SNAP_3;
    }
END_VENC_SNAP_3:
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_SNAP_2:    //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_SNAP_1:    //vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_SNAP_0:    //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}
/******************************************************************************
* function :  1D1 User send pictures for H264 encode
******************************************************************************/
HI_S32 SAMPLE_VENC_1D1_USER_SEND_PICTURES(HI_VOID)
{
    VB_CONF_S stVbConf;
    VENC_GRP VencGrp = 0;
    VENC_CHN VencChn = 0;
    PIC_SIZE_E enSize = PIC_D1;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    VB_POOL hPool  = VB_INVALID_POOLID;
    FILE *pfp_img = HI_NULL;
    HI_U32 u32PicLStride            = 0;
    HI_U32 u32PicCStride            = 0;
    HI_U32 u32LumaSize              = 0;
    HI_U32 u32ChrmSize              = 0;
    HI_U32 u32Cnt                   = 0;
    HI_U32 u32ChnCnt                = 1;
    /******************************************
     step  1: init variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                 enSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    /*ddr0 video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = 10;
    memset(stVbConf.astCommPool[0].acMmzName,0,
           sizeof(stVbConf.astCommPool[0].acMmzName));
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_USER_0;
    }
    /******************************************
     step 3: open yuv file
    ******************************************/
    if (pfp_img != HI_NULL)
    {
        fclose(pfp_img);
        pfp_img = HI_NULL;
    }
    pfp_img = fopen(SAMPLE_YUV_D1_FILEPATH, "rb" );
    if (pfp_img == HI_NULL)
    {
        SAMPLE_PRT("Open yuv file failed!Check if the file %s exit\n",SAMPLE_YUV_D1_FILEPATH);
        goto END_VENC_USER_0;
    }
    /******************************************
     step 4: create private pool on ddr0
    ******************************************/
    hPool   = HI_MPI_VB_CreatePool( u32BlkSize, 10,NULL );
    if (hPool == VB_INVALID_POOLID)
    {
        SAMPLE_PRT("HI_MPI_VB_CreatePool failed! \n");
        goto END_VENC_USER_1;
    }
    /******************************************
     step 5: encode process
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_USER_2;
    }
    s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, PT_H264, gs_enNorm, enSize, SAMPLE_RC_CBR);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start snap failed!\n");
        goto END_VENC_USER_2;
    }
    /******************************************
      step 6: stream venc process -- get stream, then save it to file.
     ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32ChnCnt);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("StartGetStream failed!\n");
        goto END_VENC_USER_3;
    }
    u32PicLStride = CEILING_2_POWER(stSize.u32Width, SAMPLE_SYS_ALIGN_WIDTH);
    u32PicCStride = CEILING_2_POWER(stSize.u32Width, SAMPLE_SYS_ALIGN_WIDTH);
    u32LumaSize = (u32PicLStride * stSize.u32Height);
    u32ChrmSize = (u32PicCStride * stSize.u32Height) >> 2;
    while(0 == feof(pfp_img))
    {
        SAMPLE_MEMBUF_S stMem = {0};
        VIDEO_FRAME_INFO_S stFrmInfo;
        stMem.hPool = hPool;
        u32Cnt ++;
        while((stMem.hBlock = HI_MPI_VB_GetBlock(stMem.hPool, u32BlkSize,NULL)) == VB_INVALID_HANDLE)
        {
            ;
        }
        stMem.u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(stMem.hBlock);
        stMem.pVirAddr = (HI_U8 *) HI_MPI_SYS_Mmap( stMem.u32PhyAddr, u32BlkSize );
        if(stMem.pVirAddr == NULL)
        {
            SAMPLE_PRT("Mem dev may not open\n");
            goto END_VENC_USER_4;
        }
        memset(&stFrmInfo.stVFrame, 0, sizeof(VIDEO_FRAME_S));
        stFrmInfo.stVFrame.u32PhyAddr[0] = stMem.u32PhyAddr;
        stFrmInfo.stVFrame.u32PhyAddr[1] = stFrmInfo.stVFrame.u32PhyAddr[0] + u32LumaSize;
        stFrmInfo.stVFrame.u32PhyAddr[2] = stFrmInfo.stVFrame.u32PhyAddr[1] + u32ChrmSize;
        stFrmInfo.stVFrame.pVirAddr[0] = stMem.pVirAddr;
        stFrmInfo.stVFrame.pVirAddr[1] = (HI_U8 *) stFrmInfo.stVFrame.pVirAddr[0] + u32LumaSize;
        stFrmInfo.stVFrame.pVirAddr[2] = (HI_U8 *) stFrmInfo.stVFrame.pVirAddr[1] + u32ChrmSize;
        stFrmInfo.stVFrame.u32Width     = stSize.u32Width;
        stFrmInfo.stVFrame.u32Height    = stSize.u32Height;
        stFrmInfo.stVFrame.u32Stride[0] = u32PicLStride;
        stFrmInfo.stVFrame.u32Stride[1] = u32PicLStride;
        stFrmInfo.stVFrame.u32Stride[2] = u32PicLStride;
        stFrmInfo.stVFrame.u64pts     = (u32Cnt * 40);
        stFrmInfo.stVFrame.u32TimeRef = (u32Cnt * 2);
        /*  Different channsel with different picture sequence  */
        SAMPLE_COMM_VENC_ReadOneFrame( pfp_img, stFrmInfo.stVFrame.pVirAddr[0],
                                       stFrmInfo.stVFrame.pVirAddr[1], stFrmInfo.stVFrame.pVirAddr[2],
                                       stFrmInfo.stVFrame.u32Width, stFrmInfo.stVFrame.u32Height,
                                       stFrmInfo.stVFrame.u32Stride[0], stFrmInfo.stVFrame.u32Stride[1] >> 1 );
        if(0 != feof(pfp_img))
        {
            break;
        }
        SAMPLE_COMM_VENC_PlanToSemi( stFrmInfo.stVFrame.pVirAddr[0], stFrmInfo.stVFrame.u32Stride[0],
                                     stFrmInfo.stVFrame.pVirAddr[1], stFrmInfo.stVFrame.u32Stride[1],
                                     stFrmInfo.stVFrame.pVirAddr[2], stFrmInfo.stVFrame.u32Stride[1],
                                     stFrmInfo.stVFrame.u32Width,    stFrmInfo.stVFrame.u32Height );
        stFrmInfo.stVFrame.enPixelFormat = SAMPLE_PIXEL_FORMAT;
        stFrmInfo.stVFrame.u32Field = VIDEO_FIELD_FRAME;
        stMem.u32PoolId = HI_MPI_VB_Handle2PoolId( stMem.hBlock );
        stFrmInfo.u32PoolId = stMem.u32PoolId;
        s32Ret = HI_MPI_VENC_SendFrame(VencGrp, &stFrmInfo);
        HI_MPI_SYS_Munmap( stMem.pVirAddr, u32BlkSize );
        HI_MPI_VB_ReleaseBlock(stMem.hBlock);
    }
    /******************************************
     step 7: exit process
    ******************************************/
END_VENC_USER_4:
    SAMPLE_COMM_VENC_StopGetStream();
END_VENC_USER_3:
    s32Ret = SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);;
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Stop encode failed!\n");
        goto END_VENC_USER_2;
    }
END_VENC_USER_2:
    //before destroy private pool,must stop venc
    HI_MPI_VB_DestroyPool( hPool );
END_VENC_USER_1:
    //close the yuv file
    fclose( pfp_img );
    pfp_img = HI_NULL;
END_VENC_USER_0:
    //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}
/******************************************************************************
* function : 4D1 H264 encode with color2grey
******************************************************************************/
HI_S32 SAMPLE_VENC_4D1_H264_COLOR2GREY(HI_VOID)
{
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_4_D1;
    HI_U32 u32ViChnCnt = 4;
    HI_S32 s32VpssGrpCnt = 4;
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H264, PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_D1, PIC_CIF};
    VB_CONF_S stVbConf;
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stGrpAttr;
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode;
    GROUP_COLOR2GREY_CONF_S stGrpColor2GreyConf;
    GROUP_COLOR2GREY_S stGrpColor2Grey;
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    HI_CHAR ch;
    SIZE_S stSize;
    /******************************************
     step  1: init variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                 PIC_D1, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    /*ddr0 video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 6;
    memset(stVbConf.astCommPool[0].acMmzName,0,
           sizeof(stVbConf.astCommPool[0].acMmzName));
    /*ddr0 hist buf*/
    stVbConf.astCommPool[1].u32BlkSize = (196*4);
    stVbConf.astCommPool[1].u32BlkCnt = u32ViChnCnt * 3;
    memset(stVbConf.astCommPool[1].acMmzName,0,
           sizeof(stVbConf.astCommPool[1].acMmzName));
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_16D1_0;
    }
    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_16D1_0;
    }
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, PIC_D1, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_16D1_0;
    }
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM,NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_16D1_1;
    }
    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_16D1_2;
    }
    /******************************************
     step 5: Set color2grey conf
     ******************************************/
    stGrpColor2GreyConf.bEnable = HI_TRUE;
    stGrpColor2GreyConf.u32MaxWidth = 720;
    stGrpColor2GreyConf.u32MaxHeight = 576;
    s32Ret = HI_MPI_VENC_SetColor2GreyConf(&stGrpColor2GreyConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SetColor2GreyConf failed!\n");
        goto END_VENC_16D1_2;
    }
    /******************************************
     step 6: select rc mode
    ******************************************/
    while(1)
    {
        printf("please choose rc mode:\n");
        printf("\t0) CBR\n");
        printf("\t1) VBR\n");
        printf("\t2) FIXQP\n");
        ch = getchar();
        getchar();
        if ('0' == ch)
        {
            enRcMode = SAMPLE_RC_CBR;
            break;
        }
        else if ('1' == ch)
        {
            enRcMode = SAMPLE_RC_VBR;
            break;
        }
        else if ('2' == ch)
        {
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        }
        else
        {
            printf("rc mode invaild! please try again.\n");
            continue;
        }
    }
    /******************************************
     step 7: start stream venc (big + little)
    ******************************************/
    for (i=0; i<u32ViChnCnt; i++)
    {
        /*** main frame **/
        VencGrp = i*2;
        VencChn = i*2;
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad[0],\
                                        gs_enNorm, enSize[0], enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_16D1_3;
        }
        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VPSS_BSTR_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_16D1_3;
        }
        /*** Enable a grp with color2grey **/
        stGrpColor2Grey.bColor2Grey = HI_TRUE;
        s32Ret = HI_MPI_VENC_SetGrpColor2Grey(VencGrp, &stGrpColor2Grey);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SetGrpColor2Grey failed!\n");
            goto END_VENC_16D1_3;
        }
        /*** Sub frame **/
        VencGrp ++;
        VencChn ++;
        s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad[1], \
                                        gs_enNorm, enSize[1], enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_16D1_3;
        }
        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VPSS_PRE0_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_16D1_3;
        }
        /*** Enable a grp with color2grey **/
        stGrpColor2Grey.bColor2Grey = HI_TRUE;
        s32Ret = HI_MPI_VENC_SetGrpColor2Grey(VencGrp, &stGrpColor2Grey);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SetGrpColor2Grey failed!\n");
            goto END_VENC_16D1_3;
        }
    }
    /******************************************
     step 8: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32ViChnCnt*2);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_16D1_3;
    }
    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();
    /******************************************
     step 9: exit process
    ******************************************/
    for (i=0; i<u32ViChnCnt; i++)
    {
        VencGrp = i*2;
        /*** Enable a grp with color2grey **/
        stGrpColor2Grey.bColor2Grey = HI_FALSE;
        s32Ret = HI_MPI_VENC_SetGrpColor2Grey(VencGrp, &stGrpColor2Grey);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SetGrpColor2Grey failed!\n");
            goto END_VENC_16D1_3;
        }
        VencGrp = i*2 + 1;
        /*** Enable a grp with color2grey **/
        stGrpColor2Grey.bColor2Grey = HI_FALSE;
        s32Ret = HI_MPI_VENC_SetGrpColor2Grey(VencGrp, &stGrpColor2Grey);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SetGrpColor2Grey failed!\n");
            goto END_VENC_16D1_3;
        }
    }
    SAMPLE_COMM_VENC_StopGetStream();
END_VENC_16D1_3:
    for (i=0; i<u32ViChnCnt*2; i++)
    {
        VencGrp = i;
        VencChn = i;
        VpssGrp = i/2;
        VpssChn = (VpssGrp%2)?VPSS_PRE0_CHN:VPSS_BSTR_CHN;
        SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
        stGrpColor2Grey.bColor2Grey = HI_FALSE;
        HI_MPI_VENC_SetGrpColor2Grey(VencGrp, &stGrpColor2Grey);
        SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
    }
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_16D1_2:    //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_16D1_1:    //vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_16D1_0:    //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}
/******************************************************************************
* function :  4D1 SNAP with StartRecvPicEx
******************************************************************************/
HI_S32 SAMPLE_VENC_4D1_SnapEx(HI_VOID)
{
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_4_D1;
    HI_U32 u32ViChnCnt = 4;
    HI_S32 s32VpssGrpCnt = 4;
    PIC_SIZE_E enSize = PIC_D1;
    VB_CONF_S stVbConf;
    VPSS_GRP VpssGrp;
    VPSS_GRP_ATTR_S stGrpAttr;
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    /******************************************
     step  1: init variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                 enSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    /* video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 15;
    memset(stVbConf.astCommPool[0].acMmzName,0,
           sizeof(stVbConf.astCommPool[0].acMmzName));
    /* hist buf*/
    stVbConf.astCommPool[1].u32BlkSize = (196*4);
    stVbConf.astCommPool[1].u32BlkCnt = u32ViChnCnt * 15;
    memset(stVbConf.astCommPool[1].acMmzName,0,
           sizeof(stVbConf.astCommPool[1].acMmzName));
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_SNAP_0;
    }
    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_SNAP_0;
    }
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_SNAP_0;
    }
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM,NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_SNAP_1;
    }
    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_SNAP_2;
    }
    /******************************************
     step 5: snap process
    ******************************************/
    VencGrp = 0;
    VencChn = 0;
    s32Ret = SAMPLE_COMM_VENC_SnapStart(VencGrp, VencChn, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start snap failed!\n");
        goto END_VENC_SNAP_3;
    }
    for (i=0; i<u32ViChnCnt; i++)
    {
        /*** main frame **/
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VENC_SnapProcessEx(VencGrp, VencChn, VpssGrp, VPSS_PRE0_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("snap process failed!\n");
            goto END_VENC_SNAP_4;
        }
        printf("snap chn %d ok!\n", i);
        sleep(1);
    }
    /******************************************
     step 8: exit process
    ******************************************/
    printf("snap over!\n");
END_VENC_SNAP_4:
    s32Ret = SAMPLE_COMM_VENC_SnapStop(VencGrp, VencChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Stop snap failed!\n");
        goto END_VENC_SNAP_3;
    }
END_VENC_SNAP_3:
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_SNAP_2:    //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_SNAP_1:    //vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_SNAP_0:    //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

/******************************************************************************
* function : send stream to vdec
******************************************************************************/
void* SAMPLE_VDEC_SendStream1(void* p)
{
    VDEC_STREAM_S stStream;
    SAMPLE_VDEC_SENDPARAM_S *pstSendParam;
    char sFileName[50], sFilePostfix[20];
    FILE* fp = NULL;
    HI_S32 s32BlockMode = HI_IO_BLOCK;
    HI_U8 *pu8Buf;
    HI_U64 u64pts;
    HI_S32 s32IntervalTime = 40000;
    HI_S32 i, s32Ret, len, start;
    HI_S32 s32UsedBytes, s32ReadLen;
    HI_BOOL bFindStart, bFindEnd;
    HI_U8 ucCount;
    start = 0;
    u64pts= 0;
    s32UsedBytes = 0;
    pstSendParam = (SAMPLE_VDEC_SENDPARAM_S *)p;
    /******************* open the stream file *****************/
    SAMPLE_COMM_SYS_Payload2FilePostfix(pstSendParam->enPayload, sFilePostfix);
    memcpy(sFileName, "stream_chn", strlen("stream_chn"));
    ucCount = strlen("stream_chn");
    sFileName[ucCount] = pstSendParam ->VdChn + '0';
    ucCount++;
    sprintf(&sFileName[ucCount], "%s", sFilePostfix);
    fp = fopen(sFileName, "r");
    if (HI_NULL == fp)
    {
        SAMPLE_PRT("can't open file %s in send stream thread:%d\n", sFileName,pstSendParam->VdChn);
        return (HI_VOID *)(HI_FAILURE);
    }
    printf("open file [%s] ok in send stream thread:%d!\n", sFileName,pstSendParam->VdChn);
    /******************* malloc the  stream buffer in user space *****************/
    if(pstSendParam->s32MinBufSize!=0)
    {
        pu8Buf=malloc(pstSendParam->s32MinBufSize);
        if(pu8Buf==NULL)
        {
            SAMPLE_PRT("can't alloc %d in send stream thread:%d\n",pstSendParam->s32MinBufSize,pstSendParam->VdChn);
            fclose(fp);
            return (HI_VOID *)(HI_FAILURE);
        }
    }
    else
    {
        SAMPLE_PRT("none buffer to operate in send stream thread:%d\n",pstSendParam->VdChn);
        return (HI_VOID *)(HI_FAILURE);
    }
    while (pstSendParam->bRun)
    {
        fseek(fp, s32UsedBytes, SEEK_SET);
        s32ReadLen = fread(pu8Buf, 1, pstSendParam->s32MinBufSize, fp);
        if (s32ReadLen<=0)
        {
            printf("file end.\n");
            break;
        }
        /******************* cutting the stream for frame *****************/
        if( (pstSendParam->enVideoMode==VIDEO_MODE_FRAME) && (pstSendParam->enPayload== PT_H264) )
        {
            bFindStart = HI_FALSE;
            bFindEnd   = HI_FALSE;
            for (i=0; i<s32ReadLen-5; i++)
            {
                if (  pu8Buf[i  ] == 0 && pu8Buf[i+1] == 0 && pu8Buf[i+2] == 1 &&
                        ((pu8Buf[i+3]&0x1F) == 0x5 || (pu8Buf[i+3]&0x1F) == 0x1) &&
                        ((pu8Buf[i+4]&0x80) == 0x80)
                   )
                {
                    bFindStart = HI_TRUE;
                    i += 4;
                    break;
                }
            }
            for (; i<s32ReadLen-5; i++)
            {
                if (  pu8Buf[i  ] == 0 && pu8Buf[i+1] == 0 && pu8Buf[i+2] == 1 &&
                        ((pu8Buf[i+3]&0x1F) == 0x5 || (pu8Buf[i+3]&0x1F) == 0x1) &&
                        ((pu8Buf[i+4]&0x80) == 0x80)
                   )
                {
                    bFindEnd = HI_TRUE;
                    break;
                }
            }
            s32ReadLen = i;
            if (bFindStart == HI_FALSE)
            {
                SAMPLE_PRT("can not find start code in send stream thread:%d\n",pstSendParam->VdChn);
            }
            else if (bFindEnd == HI_FALSE)
            {
                s32ReadLen = i+5;
            }
        }
        else if( (pstSendParam->enPayload== PT_JPEG) || (pstSendParam->enPayload == PT_MJPEG) )
        {
            bFindStart = HI_FALSE;
            bFindEnd   = HI_FALSE;
            for (i=0; i<s32ReadLen-2; i++)
            {
                if (pu8Buf[i] == 0xFF && pu8Buf[i+1] == 0xD8)
                {
                    start = i;
                    bFindStart = HI_TRUE;
                    i = i + 2;
                    break;
                }
            }
            for (; i<s32ReadLen-4; i++)
            {
                if ((pu8Buf[i] == 0xFF) && (pu8Buf[i+1]& 0xF0) == 0xE0)
                {
                    len = (pu8Buf[i+2]<<8) + pu8Buf[i+3];
                    i += 1 + len;
                }
                else
                {
                    break;
                }
            }
            for (; i<s32ReadLen-2; i++)
            {
                if (pu8Buf[i] == 0xFF && pu8Buf[i+1] == 0xD8)
                {
                    bFindEnd = HI_TRUE;
                    break;
                }
            }
            s32ReadLen = i;
            if (bFindStart == HI_FALSE)
            {
                printf("\033[0;31mALERT!!!,can not find start code in send stream thread:%d!!!\033[0;39m\n",
                       pstSendParam->VdChn);
            }
            else if (bFindEnd == HI_FALSE)
            {
                s32ReadLen = i+2;
            }
        }
        stStream.u64PTS  = u64pts;
        stStream.pu8Addr = pu8Buf + start;
        stStream.u32Len  = s32ReadLen;
        if(pstSendParam->enVideoMode==VIDEO_MODE_FRAME)
        {
            u64pts+=40000;
        }
        /******************* send stream *****************/
        if (s32BlockMode == HI_IO_BLOCK)
        {
            s32Ret=HI_MPI_VDEC_SendStream(pstSendParam->VdChn, &stStream, HI_IO_BLOCK);
        }
        else if (s32BlockMode == HI_IO_NOBLOCK)
        {
            s32Ret=HI_MPI_VDEC_SendStream(pstSendParam->VdChn, &stStream, HI_IO_NOBLOCK);
        }
        else
        {
            s32Ret=HI_MPI_VDEC_SendStream_TimeOut(pstSendParam->VdChn, &stStream, 8000);
        }
        if (HI_SUCCESS == s32Ret)
        {
            s32UsedBytes = s32UsedBytes +s32ReadLen + start;
        }
        else
        {
            if (s32BlockMode != HI_IO_BLOCK)
            {
                SAMPLE_PRT("failret:%x\n",s32Ret);
            }
            usleep(s32IntervalTime);
        }
        usleep(20000);
    }
    printf("send steam thread %d return ...\n", pstSendParam->VdChn);
    fflush(stdout);
    if (pu8Buf != HI_NULL)
    {
        free(pu8Buf);
    }
    fclose(fp);
    return (HI_VOID *)HI_SUCCESS;
}
/******************************************************************************
* function : create vdec chn
******************************************************************************/
static HI_S32 SAMPLE_VDEC_CreateVdecChn(HI_S32 s32ChnID, SIZE_S *pstSize, PAYLOAD_TYPE_E enType, VIDEO_MODE_E enVdecMode)
{
    VDEC_CHN_ATTR_S stAttr;
    VDEC_PRTCL_PARAM_S stPrtclParam;
    HI_S32 s32Ret;
    memset(&stAttr, 0, sizeof(VDEC_CHN_ATTR_S));
    stAttr.enType = enType;
    stAttr.u32BufSize = pstSize->u32Height * pstSize->u32Width;//This item should larger than u32Width*u32Height*3/4
    stAttr.u32Priority = 1;//????????????0
    stAttr.u32PicWidth = pstSize->u32Width;
    stAttr.u32PicHeight = pstSize->u32Height;
    switch (enType)
    {
    case PT_H264:
        stAttr.stVdecVideoAttr.u32RefFrameNum = 2;
        stAttr.stVdecVideoAttr.enMode = enVdecMode;
        stAttr.stVdecVideoAttr.s32SupportBFrame = 0;
        break;
    case PT_JPEG:
        stAttr.stVdecJpegAttr.enMode = enVdecMode;
        break;
    case PT_MJPEG:
        stAttr.stVdecJpegAttr.enMode = enVdecMode;
        break;
    default:
        SAMPLE_PRT("err type \n");
        return HI_FAILURE;
    }
    s32Ret = HI_MPI_VDEC_CreateChn(s32ChnID, &stAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_CreateChn failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }
    s32Ret = HI_MPI_VDEC_GetPrtclParam(s32ChnID, &stPrtclParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_GetPrtclParam failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }
    stPrtclParam.s32MaxSpsNum = 21;
    stPrtclParam.s32MaxPpsNum = 22;
    stPrtclParam.s32MaxSliceNum = 100;
    s32Ret = HI_MPI_VDEC_SetPrtclParam(s32ChnID, &stPrtclParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_SetPrtclParam failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }
    s32Ret = HI_MPI_VDEC_StartRecvStream(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_StartRecvStream failed errno 0x%x \n", s32Ret);
        return s32Ret;
    }
    return HI_SUCCESS;
}
/******************************************************************************
* function : force to stop decoder and destroy channel.
*            stream left in decoder will not be decoded.
******************************************************************************/
void SAMPLE_VDEC_ForceDestroyVdecChn(HI_S32 s32ChnID)
{
    HI_S32 s32Ret;
    s32Ret = HI_MPI_VDEC_StopRecvStream(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_StopRecvStream failed errno 0x%x \n", s32Ret);
    }
    s32Ret = HI_MPI_VDEC_DestroyChn(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32Ret);
    }
}
/******************************************************************************
* function : wait for decoder finished and destroy channel.
*            Stream left in decoder will be decoded.
******************************************************************************/
void SAMPLE_VDEC_WaitDestroyVdecChn(HI_S32 s32ChnID, VIDEO_MODE_E enVdecMode)
{
    HI_S32 s32Ret;
    VDEC_CHN_STAT_S stStat;
    memset(&stStat, 0, sizeof(VDEC_CHN_STAT_S));
    s32Ret = HI_MPI_VDEC_StopRecvStream(s32ChnID);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VDEC_StopRecvStream failed errno 0x%x \n", s32Ret);
        return;
    }
    /*** wait destory ONLY used at frame mode! ***/
    if (VIDEO_MODE_FRAME == enVdecMode)
    {
        while (1)
        {
            //printf("LeftPics:%d, LeftStreamFrames:%d\n", stStat.u32LeftPics,stStat.u32LeftStreamFrames);
            usleep(40000);
            s32Ret = HI_MPI_VDEC_Query(s32ChnID, &stStat);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("HI_MPI_VDEC_Query failed errno 0x%x \n", s32Ret);
                return;
            }
            if ((stStat.u32LeftPics == 0) && (stStat.u32LeftStreamFrames == 0))
            {
                printf("had no stream and pic left\n");
                break;
            }
        }
    }
    s32Ret = HI_MPI_VDEC_DestroyChn(s32ChnID);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32Ret);
        return;
    }
}
/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VDEC_Usage(char *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);
    printf("index:\n");
    printf("\t0) H264 -> VPSS -> VO(HD).\n");
    printf("\t1) JPEG ->VPSS -> VO(HD).\n");
    printf("\t2) MJPEG -> VO(SD).\n");
    printf("\t3) H264 -> VPSS -> VO(HD PIP PAUSE STEP).\n");
    return;
}
/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_VDEC_HandleSig(HI_S32 signo)
{
    HI_S32 i;
    if (SIGINT == signo || SIGTSTP == signo)
    {
        printf("SAMPLE_VDEC_HandleSig\n");
        for (i=0; i<gs_s32Cnt; i++)
        {
            if (HI_FALSE != gs_SendParam[i].bRun)
            {
                gs_SendParam[i].bRun = HI_FALSE;
                pthread_join(gs_SendParam[i].Pid, 0);
            }
            printf("join thread %d.\n", i);
        }
        SAMPLE_COMM_SYS_Exit();
        printf("program exit abnormally!\n");
    }
    exit(-1);
}
/******************************************************************************
* function : vdec process
*            vo is sd : vdec -> vo
*            vo is hd : vdec -> vpss -> vo
******************************************************************************/
HI_S32 SAMPLE_VDEC_Process1(PIC_SIZE_E enPicSize, PAYLOAD_TYPE_E enType, HI_S32 s32Cnt, VO_DEV VoDev)
{
    VDEC_CHN VdChn;
    HI_S32 s32Ret;
    SIZE_S stSize;
    VB_CONF_S stVbConf;
    HI_S32 i;
    VPSS_GRP VpssGrp;
    VIDEO_MODE_E enVdecMode;
    HI_CHAR ch;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr;
    SAMPLE_VO_MODE_E enVoMode;
    HI_U32 u32WndNum = s32Cnt, u32BlkSize;
    HI_BOOL bVoHd; // through Vpss or not. if vo is SD, needn't through vpss
    /******************************************
     step 1: init varaible.
    ******************************************/
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm)?25:30;
    if (s32Cnt > SAMPLE_MAX_VDEC_CHN_CNT || s32Cnt <= 0)
    {
        SAMPLE_PRT("Vdec count %d err, should be in [%d,%d]. \n", s32Cnt, 1, SAMPLE_MAX_VDEC_CHN_CNT);
        return HI_FAILURE;
    }
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enPicSize, &stSize);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return HI_FAILURE;
    }
    if (704 == stSize.u32Width)
    {
        stSize.u32Width = 720;
    }
    else if (352 == stSize.u32Width)
    {
        stSize.u32Width = 360;
    }
    else if (176 == stSize.u32Width)
    {
        stSize.u32Width = 180;
    }
    // through Vpss or not. if vo is SD, needn't through vpss
    if (SAMPLE_VO_DEV_DHD0 != VoDev )
    {
        bVoHd = HI_FALSE;
    }
    else
    {
        bVoHd = HI_TRUE;
    }
    for(i=0; i<u32WndNum; i++)
    {
        VoChn = i;
        if (HI_TRUE == bVoHd)
        {
            VpssGrp = i + 8;
            s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE0_CHN);
            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
                goto END_3;
            }
        }
    }
    /******************************************
     step 5: start vdec & bind it to vpss or vo
    ******************************************/
    enVdecMode = VIDEO_MODE_FRAME;
    for (i=0; i<s32Cnt; i++)
    {
        /***** create vdec chn *****/
        VdChn = i;
        s32Ret = SAMPLE_VDEC_CreateVdecChn(VdChn, &stSize, enType, enVdecMode);
        if (HI_SUCCESS !=s32Ret)
        {
            SAMPLE_PRT("create vdec chn failed!\n");
            goto END_3;
        }
        /***** bind vdec to vpss *****/
        if (HI_TRUE == bVoHd)
        {
            VpssGrp = i + 8;
            s32Ret = SAMLE_COMM_VDEC_BindVpss(VdChn, VpssGrp);
            if (HI_SUCCESS !=s32Ret)
            {
                SAMPLE_PRT("vdec(vdch=%d) bind vpss(vpssg=%d) failed!\n", VdChn, VpssGrp);
                goto END_3;
            }
        }
        else
        {
            VoChn =  i;
            s32Ret = SAMLE_COMM_VDEC_BindVo(VdChn, VoDev, VoChn);
            if (HI_SUCCESS !=s32Ret)
            {
                SAMPLE_PRT("vdec(vdch=%d) bind vo(vo=%d) failed!\n", VdChn, VoChn);
                goto END_3;
            }
        }
    }
    /******************************************
     step 6: open file & video decoder
    ******************************************/
    for (i=0; i<s32Cnt; i++)
    {
        gs_SendParam[i].bRun = HI_TRUE;
        gs_SendParam[i].VdChn = i;
        gs_SendParam[i].enPayload = enType;
        gs_SendParam[i].enVideoMode = enVdecMode;
        gs_SendParam[i].s32MinBufSize = stSize.u32Height * stSize.u32Width * 3/ 4;
        pthread_create(&gs_SendParam[i].Pid, NULL, SAMPLE_VDEC_SendStream1, &gs_SendParam[i]);
    }
    if (PT_JPEG != enType)
    {
        printf("you can press ctrl+c to terminate program before normal exit.\n");
    }
    /******************************************
     step 7: join thread
    ******************************************/
    for (i=0; i<s32Cnt; i++)
    {
        pthread_join(gs_SendParam[i].Pid, 0);
        printf("join thread %d.\n", i);
    }
    /******************************************
     step 8: Unbind vdec to vpss & destroy vdec-chn
    ******************************************/
END_3:
    for (i=0; i<s32Cnt; i++)
    {
        VdChn = i;
        SAMPLE_VDEC_WaitDestroyVdecChn(VdChn, enVdecMode);
        if (HI_TRUE == bVoHd)
        {
            VpssGrp = i + 8;
            SAMLE_COMM_VDEC_UnBindVpss(VdChn, VpssGrp);
        }
        else
        {
            VoChn = i;
            SAMLE_COMM_VDEC_UnBindVo(VdChn, VoDev, VoChn);
        }
    }
    /******************************************
     step 8: Unbind Vo to vpss(dec channel)
    ******************************************/
    for (i=0; i<s32Cnt; i++)
    {
        VoChn = i;
        if (HI_TRUE == bVoHd)
        {
            VpssGrp = i + 8;
            SAMPLE_COMM_VO_UnBindVpss(VoDev, VoChn, VpssGrp, VPSS_PRE0_CHN);
            SAMLE_COMM_VDEC_UnBindVpss(VdChn, VpssGrp);
        }
    }
    return HI_SUCCESS;
}
HI_S32 SAMPLE_VDEC_ProcessForPip(PIC_SIZE_E enPicSize, PAYLOAD_TYPE_E enType, HI_S32 s32Cnt, VO_DEV VoDev)
{
    VDEC_CHN VdChn;
    HI_S32 s32Ret;
    SIZE_S stSize;
    VB_CONF_S stVbConf;
    HI_S32 i, j;
    VPSS_GRP VpssGrp, VpssGrpForPip = 1;
    VIDEO_MODE_E enVdecMode;
    HI_CHAR ch;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr;
    SAMPLE_VO_MODE_E enVoMode;
    HI_U32 u32WndNum, u32BlkSize;
    HI_BOOL bVoHd; // through Vpss or not. if vo is SD, needn't through vpss
    VO_CHN_ATTR_S stVoChnAttr;
    VO_VIDEO_LAYER_ATTR_S stPipLayerAttr;
    VPSS_CROP_INFO_S stVpssCropInfo;
    VIDEO_FRAME_INFO_S stVideoFrame;
    /******************************************
     step 1: init varaible.
    ******************************************/
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL== gs_enNorm)?25:30;
    if (SAMPLE_MAX_VDEC_CHN_CNT < s32Cnt)
    {
        SAMPLE_PRT("Vdec count is bigger than sample define!\n");
        return HI_FAILURE;
    }
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enPicSize, &stSize);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return HI_FAILURE;
    }
    if (704 == stSize.u32Width)
    {
        stSize.u32Width = 720;
    }
    else if (352 == stSize.u32Width)
    {
        stSize.u32Width = 360;
    }
    else if (176 == stSize.u32Width)
    {
        stSize.u32Width = 180;
    }
    /* through Vpss or not. if vo is SD, needn't through vpss */
    if (SAMPLE_VO_DEV_DHD0 != VoDev)
    {
        bVoHd = HI_FALSE;
    }
    else
    {
        bVoHd = HI_TRUE;
    }
    /******************************************
     step 2: mpp system init.
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                 PIC_D1, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    /*vdec no need common video buffer!*/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("mpp init failed!\n");
        return HI_FAILURE;
    }
    /******************************************
     step 3: start vpss, if ov is hd.
    ******************************************/
    if (HI_TRUE == bVoHd)
    {
        s32Ret = SAMPLE_COMM_VPSS_Start(s32Cnt+1, &stSize, VPSS_MAX_CHN_NUM,NULL);
        if (HI_SUCCESS !=s32Ret)
        {
            SAMPLE_PRT("vpss start failed!\n");
            goto END_0;
        }
    }
    /******************************************
     step 4: start vo
    ******************************************/
    u32WndNum = 1;
    enVoMode = VO_MODE_1MUX;
    if (HI_TRUE == bVoHd)
    {
        if(VIDEO_ENCODING_MODE_PAL== gs_enNorm)
        {
            stVoPubAttr.enIntfSync = VO_OUTPUT_720P50;//VO_OUTPUT_1080P50;
        }
        else
        {
            stVoPubAttr.enIntfSync =VO_OUTPUT_720P50;//VO_OUTPUT_1080P60;
        }
#ifdef HI_FPGA
        stVoPubAttr.enIntfType = VO_INTF_HDMI|VO_INTF_VGA|VO_INTF_BT1120;
#else
        stVoPubAttr.enIntfType = VO_INTF_HDMI|VO_INTF_VGA;
#endif
        stVoPubAttr.u32BgColor = 0x000000ff;
        stVoPubAttr.bDoubleFrame = HI_FALSE;
    }
    else
    {
        if(VIDEO_ENCODING_MODE_PAL== gs_enNorm)
        {
            stVoPubAttr.enIntfSync = VO_OUTPUT_PAL;
        }
        else
        {
            stVoPubAttr.enIntfSync = VO_OUTPUT_NTSC;
        }
        stVoPubAttr.enIntfType = VO_INTF_CVBS;
        stVoPubAttr.u32BgColor = 0x000000ff;
        stVoPubAttr.bDoubleFrame = HI_FALSE;
    }
    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, gs_u32ViFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartDevLayer failed!\n");
        goto END_1;
    }
    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartChn failed!\n");
        goto END_2;
    }
    if (HI_TRUE == bVoHd)
    {
        /* if it's displayed on HDMI, we should start HDMI */
        if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
        {
            if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
            {
                SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
                goto END_1;
            }
        }
    }
    for(i=0; i<u32WndNum; i++)
    {
        VoChn = i;
        if (HI_TRUE == bVoHd)
        {
            VpssGrp = i;
            s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE0_CHN);
            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
                goto END_2;
            }
        }
    }
    /******************************************
     step 5: start vdec & bind it to vpss or vo
    ******************************************/
    enVdecMode = VIDEO_MODE_FRAME;
    for (i=0; i<s32Cnt; i++)
    {
        /*** create vdec chn ***/
        VdChn = i;
        s32Ret = SAMPLE_VDEC_CreateVdecChn(VdChn, &stSize, enType, enVdecMode);
        if (HI_SUCCESS !=s32Ret)
        {
            SAMPLE_PRT("create vdec chn failed!\n");
            goto END_3;
        }
        /*** bind vdec to vpss ***/
        if (HI_TRUE == bVoHd)
        {
            VpssGrp = i;
            s32Ret = SAMLE_COMM_VDEC_BindVpss(VdChn, VpssGrp);
            if (HI_SUCCESS !=s32Ret)
            {
                SAMPLE_PRT("vdec(vdch=%d) bind vpss(vpssg=%d) failed!\n", VdChn, VpssGrp);
                goto END_3;
            }
        }
        else
        {
            VoChn =  i;
            s32Ret = SAMLE_COMM_VDEC_BindVo(VdChn, VoDev, VoChn);
            if (HI_SUCCESS !=s32Ret)
            {
                SAMPLE_PRT("vdec(vdch=%d) bind vpss(vpssg=%d) failed!\n", VdChn, VpssGrp);
                goto END_3;
            }
        }
    }
    /******************************************
     step 6: open file & video decoder
    ******************************************/
    for (i=0; i<s32Cnt; i++)
    {
        gs_SendParam[i].bRun = HI_TRUE;
        gs_SendParam[i].VdChn = i;
        gs_SendParam[i].enPayload = enType;
        gs_SendParam[i].enVideoMode = enVdecMode;
        gs_SendParam[i].s32MinBufSize = stSize.u32Height * stSize.u32Width *3 / 4;
        pthread_create(&gs_SendParam[i].Pid, NULL, SAMPLE_VDEC_SendStream1, &gs_SendParam[i]);
    }
    if (PT_JPEG != enType)
    {
        printf("you can press ctrl+c to terminate program before normal exit.\n");
    }
    printf("please press any key to show pip:\n");
    getchar();
    s32Ret = HI_MPI_VO_ChnPause(VoDev, 0);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VO_ChnPause err 0x%x\n",s32Ret);
        return s32Ret;
    }
    s32Ret = HI_MPI_VO_PipLayerBindDev(VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VO_PipLayerBindDev failed with %#x!\n", s32Ret);
        return s32Ret;
    }
    stPipLayerAttr.stDispRect.s32X = 0;
    stPipLayerAttr.stDispRect.s32Y = 0;
    stPipLayerAttr.stDispRect.u32Height  = 1080;
    stPipLayerAttr.stDispRect.u32Width   = 1920;
    stPipLayerAttr.stImageSize.u32Height = 1080;
    stPipLayerAttr.stImageSize.u32Width = 1920;
    stPipLayerAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;
    stPipLayerAttr.u32DispFrmRt = 25;
    s32Ret = HI_MPI_VO_SetPipLayerAttr(&stPipLayerAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VO_SetPipLayerAttr failed with %#x!\n", s32Ret);
        goto END_3;
    }
    s32Ret = HI_MPI_VO_EnablePipLayer();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VO_EnablePipLayer failed with %#x!\n", s32Ret);
        goto END_3;
    }
    stVoChnAttr.stRect.s32X       = 0;
    stVoChnAttr.stRect.s32Y       = 0;
    stVoChnAttr.stRect.u32Width   = 1920;
    stVoChnAttr.stRect.u32Height  = 1080;
    stVoChnAttr.u32Priority       = 0;
    stVoChnAttr.bDeflicker        = HI_FALSE;
    s32Ret = HI_MPI_VO_SetChnAttr(VoDev, 0, &stVoChnAttr);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VO_SetChnAttr err 0x%x\n",s32Ret);
        goto END_3;
    }
    stVoChnAttr.stRect.s32X       = 1200;
    stVoChnAttr.stRect.s32Y       = 504;
    stVoChnAttr.stRect.u32Width   = 720;
    stVoChnAttr.stRect.u32Height  = 576;
    stVoChnAttr.u32Priority       = 1;
    stVoChnAttr.bDeflicker        = HI_FALSE;
    s32Ret = HI_MPI_VO_SetChnAttr(VoDev, 1, &stVoChnAttr);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VO_SetChnAttr err 0x%x\n",s32Ret);
        goto END_3;
    }
    s32Ret = HI_MPI_VO_EnableChn(VoDev, 1);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VO_EnableChn err 0x%x\n",s32Ret);
        goto END_3;
    }
    s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev, 1, VpssGrpForPip, VPSS_PRE0_CHN);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
        goto END_3;
    }
#if 0
    s32Ret = HI_MPI_VPSS_UserGetGrpFrame(VpssGrp, &stVideoFrame, 0);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VPSS_UserGetGrpFrame err 0x%x\n",s32Ret);
        goto END_3;
    }
    s32Ret = HI_MPI_VPSS_UserSendFrame(VpssGrpForPip, &stVideoFrame);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VPSS_UserSendFrame err 0x%x\n",s32Ret);
        goto END_3;
    }
    s32Ret = HI_MPI_VPSS_UserReleaseGrpFrame(VpssGrp, &stVideoFrame);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VPSS_UserReleaseGrpFrame err 0x%x\n",s32Ret);
        goto END_3;
    }
#endif
    stVpssCropInfo.bEnable = HI_TRUE;
    stVpssCropInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
    stVpssCropInfo.stCropRect.s32X = 0;
    stVpssCropInfo.stCropRect.s32Y = 0;
    stVpssCropInfo.stCropRect.u32Height = 288;
    stVpssCropInfo.stCropRect.u32Width = 352;
    stVpssCropInfo.enCapSel = VPSS_CAPSEL_BOTH;
    s32Ret = HI_MPI_VPSS_SetCropCfg(VpssGrp, &stVpssCropInfo);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed with %#x!\n", s32Ret);
        goto END_3;
    }
    s32Ret = HI_MPI_VO_ChnRefresh(VoDev, 0);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VO_ChnRefresh err 0x%x\n",s32Ret);
        goto END_3;
    }
    for (i = 0; i < 10; i++)
    {
        printf("press any key to step %d!\n", i);
        getchar();
        s32Ret = HI_MPI_VO_ChnStep(VoDev, 0);
        if(s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VO_ChnStep err 0x%x\n",s32Ret);
            goto END_3;
        }
        usleep(20000); // Here we should usleep some ms to make sure the step has taken effect, then we can get the same pic from VPSS.
#if 0
        s32Ret = HI_MPI_VPSS_UserGetGrpFrame(VpssGrp, &stVideoFrame, 0);
        if(s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VPSS_UserGetGrpFrame err 0x%x\n",s32Ret);
            goto END_3;
        }
        s32Ret = HI_MPI_VPSS_UserSendFrame(VpssGrpForPip, &stVideoFrame);
        if(s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VPSS_UserSendFrame err 0x%x\n",s32Ret);
            goto END_3;
        }
        s32Ret = HI_MPI_VPSS_UserReleaseGrpFrame(VpssGrp, &stVideoFrame);
        if(s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VPSS_UserReleaseGrpFrame err 0x%x\n",s32Ret);
            goto END_3;
        }
#endif
    }
    printf("press any key to resume!\n");
    getchar();
    s32Ret = HI_MPI_VO_ChnResume(VoDev, 0);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VO_ChnResume err 0x%x\n",s32Ret);
        goto END_3;
    }
    s32Ret = SAMLE_COMM_VDEC_BindVpss(VdChn, VpssGrpForPip);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
        goto END_3;
    }
    s32Ret = HI_MPI_VPSS_ResetGrp(VpssGrp);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VPSS_ResetGrp failed!\n");
        goto END_3;
    }
    s32Ret = HI_MPI_VPSS_ResetGrp(VpssGrpForPip);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VPSS_ResetGrp failed!\n");
        goto END_3;
    }
    s32Ret = HI_MPI_VO_ClearChnBuffer(VoDev, 0, HI_FALSE);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VO_ClearChnBuffer failed!\n");
        goto END_3;
    }
    s32Ret = HI_MPI_VO_ClearChnBuffer(VoDev, 1, HI_FALSE);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VO_ClearChnBuffer failed!\n");
        goto END_3;
    }
    /******************************************
     step 7: join thread
    ******************************************/
    for (i=0; i<s32Cnt; i++)
    {
        pthread_join(gs_SendParam[i].Pid, 0);
        printf("join thread %d.\n", i);
    }
    printf("press two enter to quit!\n");
    getchar();
    getchar();
    /******************************************
     step 8: Unbind vdec to vpss & destroy vdec-chn
    ******************************************/
END_3:
    for (i=0; i<s32Cnt; i++)
    {
        VdChn = i;
        SAMPLE_VDEC_WaitDestroyVdecChn(VdChn, enVdecMode);
        if (HI_TRUE == bVoHd)
        {
            VpssGrp = i;
            SAMLE_COMM_VDEC_UnBindVpss(VdChn, VpssGrp);
        }
        else
        {
            VoChn = i;
            SAMLE_COMM_VDEC_UnBindVo(VdChn, VoDev, VoChn);
        }
    }
    s32Ret = HI_MPI_VO_DisableChn(VoDev, 1);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VO_DisableChn failed with %#x!\n", s32Ret);
    }
    s32Ret = HI_MPI_VO_DisablePipLayer();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VO_DisablePipLayer failed with %#x!\n", s32Ret);
    }
    /******************************************
     step 9: stop vo
    ******************************************/
END_2:
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
    for(i=0; i<u32WndNum; i++)
    {
        VoChn = i;
        if (HI_TRUE == bVoHd)
        {
            VpssGrp = i;
            SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE0_CHN);
        }
    }
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_1:
    if (HI_TRUE == bVoHd)
    {
        if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
        {
            SAMPLE_COMM_VO_HdmiStop();
        }
        SAMPLE_COMM_VPSS_Stop(s32Cnt+1, VPSS_MAX_CHN_NUM);
    }
    /******************************************
     step 10: exit mpp system
    ******************************************/
END_0:
    SAMPLE_COMM_SYS_Exit();
    return HI_SUCCESS;
}
/******************************************************************************
* function :  VI:8*D1; VO:HD0(HDMI,VGA)+SD0(CVBS)+SD1 video preview
******************************************************************************/
HI_S32 SAMPLE_VIO_8_D1_Hifb(PIC_SIZE_E enCodeSize, HI_U8 EncMode, VO_DEV VoDevMode)
{
    SAMPLE_VI_MODE_E enViMode =  SAMPLE_VI_MODE_8_D1;
    HI_U32 u32ViChnCnt = 8;
    HI_S32 s32VpssGrpCnt = 16;/* must 16 */
    HI_S32 s32RgnCnt = 8;
    VB_CONF_S stVbConf;
    VI_CHN ViChn;
    VPSS_GRP VpssGrp;
    VPSS_GRP_ATTR_S stGrpAttr;
    VPSS_CHN VpssChn_VoHD0 = VPSS_PRE0_CHN;//VPSS_BYPASS_CHN;//VPSS_PRE0_CHN;
    VO_DEV VoDev = VoDevMode;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr,stVoPubAttrSD;
    SAMPLE_VO_MODE_E enVoMode, enPreVoMode;
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    HI_CHAR ch;
    SIZE_S stSize;
    HI_U32 u32WndNum = 8;
    VO_WBC_ATTR_S stWbcAttr;
    PAYLOAD_TYPE_E enPayLoad = PT_H264;
    PIC_SIZE_E enSize =  enCodeSize;
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode;
    /******************************************
     step  1: init variable
    ******************************************/
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL== gs_enNorm)?25:30;
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                 enCodeSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    /* video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 8;
    memset(stVbConf.astCommPool[0].acMmzName, 0, sizeof(stVbConf.astCommPool[0].acMmzName));
    /* hist buf*/
    stVbConf.astCommPool[1].u32BlkSize = (196 * 4);
    stVbConf.astCommPool[1].u32BlkCnt = u32ViChnCnt * 8;
    memset(stVbConf.astCommPool[1].acMmzName, 0, sizeof(stVbConf.astCommPool[1].acMmzName));
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_8D1_0;
    }
    /******************************************
     step 3: start vi dev & chn
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_8D1_0;
    }
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_8D1_0;
    }
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_FALSE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM,NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_8D1_1;
    }
    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_8D1_2;
    }
    /******************************************
    step 5: select encode mode CBR VCR FIXQP
    ******************************************/
    enRcMode = EncMode;
    /******************************************
     step 5: start stream venc (big + little)
    ******************************************/
    for (i=0; i<u32ViChnCnt; i++)
    {
        /*** main stream **/
        VencGrp = i;
        VencChn = i;
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad,\
                                        gs_enNorm, enSize, enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_8D1_2;
        }
        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VPSS_BSTR_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_8D1_3;
        }
    }
    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32ViChnCnt);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_8D1_3;
    }
    /*Overlay Region Process*/
    s32Ret = SAMPLE_Region_Venc(VencChn);//(VencChn, s32RgnCnt);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_8D1_3;
    }
    printf("please press any key to exit\n");
    /* 输出显示 */
    if (SAMPLE_VO_DEV_DHD0 == VoDevMode)
    {
        VpssChn_VoHD0 = VPSS_PRE0_CHN;
        /*  HDMI + VGA */
        /******************************************
         step 6: start vo HD0 (HDMI+VGA), multi-screen, you can switch mode
        ******************************************/
        printf("start vo HD0.\n");
        enVoMode = VO_MODE_9MUX;
        if(VIDEO_ENCODING_MODE_PAL == gs_enNorm)
        {
            stVoPubAttr.enIntfSync =  VO_OUTPUT_720P50;
        }
        else
        {
            stVoPubAttr.enIntfSync =  VO_OUTPUT_720P60;
        }
#ifdef HI_FPGA
        stVoPubAttr.enIntfType = VO_INTF_HDMI|VO_INTF_VGA|VO_INTF_BT1120;
#else
        stVoPubAttr.enIntfType =  VO_INTF_VGA;//VO_INTF_HDMI|VO_INTF_VGA;
#endif
        stVoPubAttr.u32BgColor = 0x000000ff;
        stVoPubAttr.bDoubleFrame = HI_TRUE;
    }
    else
    {
        VpssChn_VoHD0 = VPSS_BYPASS_CHN;
        /******************************************
        step 5: start vo SD1(CVBS)
        ******************************************/
        printf("start vo SD1.\n");
        VoDev = SAMPLE_VO_DEV_DSD1;
        //u32WndNum = 8;
        enVoMode = VO_MODE_9MUX;
        stVoPubAttr.enIntfSync = VO_OUTPUT_PAL;
        stVoPubAttr.enIntfType = VO_INTF_CVBS;
        stVoPubAttr.u32BgColor = 0x000000ff;
        stVoPubAttr.bDoubleFrame = HI_FALSE;
    }
    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, gs_u32ViFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartDevLayer failed!\n");
        goto END_8D1_4;
    }
    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartChn failed!\n");
        goto END_8D1_5;
    }
    /* if it's displayed on HDMI, we should start HDMI */
    if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
    {
        if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
        {
            SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
            goto END_8D1_5;
        }
    }
    for (i = 0; i < u32WndNum; i++)
    {
        VoChn = i ;/* out channel 0-----vpss 7 */
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VpssChn_VoHD0);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
            goto END_8D1_5;
        }
    }
    /******************************************
    step 8: HD0 switch mode
    ******************************************/
    enVoMode = VO_MODE_9MUX;
    while(1)
    {
        enPreVoMode = enVoMode;
        printf("please choose preview mode, press 'q' to exit this sample.\n");
        printf("\t0) 1 preview\n");
        printf("\t1) 4 preview\n");
        printf("\t2) 8 preview\n");
        printf("\t3) dec code\n");
        printf("\tq) quit\n");
        ch = getchar();
        getchar();
        if ('0' == ch)
        {
            enVoMode = VO_MODE_1MUX;
        }
        else if ('1' == ch)
        {
            enVoMode = VO_MODE_4MUX;
        }
        /*Indeed only 8 chns show*/
        else if ('2' == ch)
        {
            enVoMode = VO_MODE_9MUX;
        }
        else if  ('3' == ch)/* 解码开始 */
        {
            pthread_join(gs_RegionVencPid,0);
            enVoMode = VO_MODE_9MUX;
            if (SAMPLE_VO_DEV_DHD0 == VoDevMode)
            {
                VpssChn_VoHD0 = VPSS_PRE0_CHN;
            }
            else
            {
                VpssChn_VoHD0 = VPSS_BYPASS_CHN;
            }
            for(i=0; i<u32WndNum; i++)
            {
                VoChn = i;
                VpssGrp = i;
                s32Ret = SAMPLE_COMM_VO_UnBindVpss(VoDevMode,VoChn,VpssGrp,VpssChn_VoHD0);
                if (HI_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("SAMPLE_COMM_VO_UnBindVpss failed!\n");
                    goto END_8D1_4;
                }
            }
            SAMPLE_VDEC_Process1(PIC_D1, PT_H264, u32WndNum, VoDevMode);
            for(i=0; i<u32WndNum; i++)
            {
                VoChn = i ;/* out channel 0-----vpss 7 */
                VpssGrp = i;
                s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VpssChn_VoHD0);
                if (HI_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("Start VO failed!\n");
                    goto END_8D1_5;
                }
            }
        }
        else if ('q' == ch)
        {
            break;
        }
        else
        {
            SAMPLE_PRT("preview mode invaild! please try again.\n");
            continue;
        }
        SAMPLE_PRT("vo(%d) switch to %d mode\n", VoDev, u32WndNum);
        s32Ret= HI_MPI_VO_SetAttrBegin(VoDev);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
            goto END_8D1_7;
        }
        s32Ret = SAMPLE_COMM_VO_StopChn(VoDev, enPreVoMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
            goto END_8D1_7;
        }
        s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
            goto END_8D1_7;
        }
        s32Ret= HI_MPI_VO_SetAttrEnd(VoDev);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
            goto END_8D1_7;
        }
    }
    /******************************************
     step 8: exit process
    ******************************************/
END_VENC_8D1_3:
    SAMPLE_COMM_VENC_StopGetStream();
    for (i=0; i<u32ViChnCnt; i++)
    {
        VencGrp = i;
        VencChn = i;
        VpssGrp = i;
        VpssChn_VoHD0 = VPSS_PRE0_CHN;
        SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn_VoHD0);
        SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
    }
END_8D1_7:
    SAMPLE_COMM_VO_UnBindVoWbc(SAMPLE_VO_DEV_DSD0, 0);
    HI_MPI_VO_DisableWbc(SAMPLE_VO_DEV_DHD0);
END_8D1_6:
    VoDev = SAMPLE_VO_DEV_DSD0;
    VoChn = 0;
    enVoMode = VO_MODE_1MUX;
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_8D1_5:
    if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
    {
        SAMPLE_COMM_VO_HdmiStop();
    }
    VoDev = SAMPLE_VO_DEV_DHD0;
    u32WndNum = 16;
    enVoMode = VO_MODE_16MUX;
    /*??disableChn ,????V??????*/
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
    for(i=0; i<u32WndNum; i++)
    {
        VoChn = i;
        VpssGrp = i;
        SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,VpssChn_VoHD0);
    }
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_8D1_4:
END_8D1_3:  //vi unbind vpss
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_8D1_2:  //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_8D1_1:  //vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_8D1_0:  //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}



/* 以下为视频处理的封装后的函数 */
#define ENPAYLOAD_INVALID  0xff  

#define ENCODE_VPASSGRP_START  0 /* 编码所用的VPSS的GRP从0--15 */
#define SNAP_VPASSGRP_START  16  /* 抓拍所用的VPSS的GRP从16--23 */
#define DECODE_VPASSGRP_START  24  /* 解码所用的VPSS的GRP从24--40 */
#define VPASSGRP_MAX   40  

#define SNAP_ENCODE_CNT   8  /* 抓拍所用的编码通道 */
#define SNAP_ENCODE_GRP   8  /* 抓拍所用的编码组 */


/* 编码通道 */
typedef enum
{
    ENC_VPSS_GRP_1 = 1,
    ENC_VPSS_GRP_2,    
    ENC_VPSS_GRP_3,     
    ENC_VPSS_GRP_4,    
    ENC_VPSS_GRP_5 ,   
    ENC_VPSS_GRP_6 ,     
    ENC_VPSS_GRP_7 ,    
    ENC_VPSS_GRP_8,     
    ENC_VPSS_GRP_ALL, 
}ENCODE_VPSS_GRP_E;


/* 解码通道 */
#define DECODE_CHANNEL_1     1
#define DECODE_CHANNEL_2     2
#define DECODE_CHANNEL_3     3
#define DECODE_CHANNEL_4     4
#define DECODE_CHANNEL_5     5
#define DECODE_CHANNEL_6     6
#define DECODE_CHANNEL_7     7
#define DECODE_CHANNEL_8     8
#define DECODE_CHANNEL_ALL  100


VO_PUB_ATTR_S gstVoPubAttr;
VPSS_CHN giVpssChn_VoHD0 = VPSS_PRE1_CHN;
SAMPLE_VO_MODE_E genVoMode = VO_MODE_9MUX;
HI_S32 guiWndNum;
SIZE_S gsDecPicSize;
SAMPLE_VI_MODE_E geEnViMode;
VB_CONF_S gsVbConf;

/***************************************************************
*名称: void Hal_VariableInit(VB_CONF_S *p, HI_U32 u32ViChnCnt, PIC_SIZE_E enCodeSize)

*描述: 硬件抽象层视频的参数初始化
*输出:  无
*修改记录:
*----------------------------------------
*修改人      修改时间     修改内容
  张博爱      2015-06-12         创建函数
****************************************************************/
void Hal_VariableInit(HI_U32 u32ViChnCnt, PIC_SIZE_E enCodeSize)
{
    HI_U32 uiBlkSize, uiViChnCnt;

    uiViChnCnt = u32ViChnCnt;
    
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm) ? 25 : 30;

    /* 根据视频分辨率得出所需的blksize */
    uiBlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                            enCodeSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);

    gsVbConf.u32MaxPoolCnt = 128;
    /* video buffer */
    gsVbConf.astCommPool[0].u32BlkSize = uiBlkSize;
    gsVbConf.astCommPool[0].u32BlkCnt = uiViChnCnt * 8;
    memset(gsVbConf.astCommPool[0].acMmzName, 0, sizeof(gsVbConf.astCommPool[0].acMmzName));

    /* hist buf */
    gsVbConf.astCommPool[1].u32BlkSize = (196 * 4);
    gsVbConf.astCommPool[1].u32BlkCnt = uiViChnCnt * 8;
    memset(gsVbConf.astCommPool[1].acMmzName, 0, sizeof(gsVbConf.astCommPool[1].acMmzName));
}


HI_S32 Hal_Init(VB_CONF_S VbConf)
{
    HI_S32 s32Ret;
    
    s32Ret = SAMPLE_COMM_SYS_Init(&VbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        SAMPLE_COMM_SYS_Exit();
    }
    return s32Ret;
}

HI_S32 Hal_ViInit(SAMPLE_VI_MODE_E EnViMode)
{
    HI_S32 s32Ret;
    s32Ret = SAMPLE_COMM_VI_Start(EnViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        SAMPLE_COMM_SYS_Exit();
    }
    return s32Ret;
}

HI_S32 Hal_VpssInit(SAMPLE_VI_MODE_E EnViMode, HI_S32 s32VpssGrpCnt, PIC_SIZE_E enSize)
{
    HI_S32 s32Ret;
    SIZE_S stSize;
    VPSS_GRP_ATTR_S stGrpAttr;
    
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        SAMPLE_COMM_SYS_Exit();
        return s32Ret;
    }
    
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_FALSE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM, &stGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        SAMPLE_COMM_VI_Stop(EnViMode);
        SAMPLE_COMM_SYS_Exit();
        return s32Ret;
    }
    
    return s32Ret;
}

HI_S32 Hal_ViBindVpss(SAMPLE_VI_MODE_E EnViMode)
{
    HI_S32 s32Ret;

    s32Ret = SAMPLE_COMM_VI_BindVpss(EnViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        SAMPLE_COMM_VPSS_Stop(VPASSGRP_MAX, VPSS_MAX_CHN_NUM);
        SAMPLE_COMM_VI_Stop(EnViMode);
        SAMPLE_COMM_SYS_Exit();
    }
    return s32Ret;
}

HI_S32 Hal_VencRelese(HI_S32 VencChn, 
                                        SAMPLE_RC_E EncMode, 
                                        PAYLOAD_TYPE_E MainEnPayLoad,
                                        PAYLOAD_TYPE_E SubEnPayLoad,
                                        PIC_SIZE_E MainEnSize,
                                        PIC_SIZE_E SubEnSize
                                        )
{
    HI_S32 s32VencGrp, s32VpssGrp, s32Ret,s32VencChn; 
    
    /* 无子码流时 */
    if (ENPAYLOAD_INVALID == SubEnPayLoad)
    {
        s32VencChn = VencChn * 2;
        s32VencGrp = s32VencChn;
        s32VpssGrp = s32VencChn;
        SAMPLE_COMM_VENC_UnBindVpss(s32VencGrp, s32VpssGrp, VPSS_BSTR_CHN);
        SAMPLE_COMM_VENC_Stop(s32VencGrp,VencChn);
    }
    else
    {
        s32VencChn = VencChn * 2;
        s32VencGrp = s32VencChn;
        s32VpssGrp = s32VencChn;
        SAMPLE_COMM_VENC_UnBindVpss(s32VencGrp, s32VpssGrp, VPSS_BSTR_CHN);
        SAMPLE_COMM_VENC_Stop(s32VencGrp,VencChn);

        s32VencChn = VencChn * 2 + 1;
        s32VencGrp = s32VencChn;
        s32VpssGrp = s32VencChn;
        SAMPLE_COMM_VENC_UnBindVpss(s32VencGrp, s32VpssGrp, VPSS_PRE0_CHN);
        SAMPLE_COMM_VENC_Stop(s32VencGrp,VencChn);
    }
    SAMPLE_COMM_VI_UnBindVpss(geEnViMode);
    
    //vpss stop
    SAMPLE_COMM_VPSS_Stop(VPASSGRP_MAX, VPSS_MAX_CHN_NUM);
    //vi stop
    SAMPLE_COMM_VI_Stop(geEnViMode);
    //system exit
    SAMPLE_COMM_SYS_Exit();
}

/***************************************************************
*名称: Hal_VencSet(HI_S32 VencChn, 
                                SAMPLE_RC_E EncMode, 
                                PAYLOAD_TYPE_E MainEnPayLoad,
                                PAYLOAD_TYPE_E SubEnPayLoad,
                                PIC_SIZE_E MainEnSize,
                                PIC_SIZE_E SubEnSize
                                )
*描述: 硬件抽象层编码器的设置
*输入:   HI_S32 VencChn----编码通道
                SAMPLE_RC_E EncMode----编码的模式:CBR, VBR
                PAYLOAD_TYPE_E MainEnPayLoad----主码流方式
                PAYLOAD_TYPE_E SubEnPayLoad-----从码流方式
                PIC_SIZE_E MainEnSize------主码流大小
                PIC_SIZE_E SubEnSize-------从码流大小
*输出:  该通道设置状态
*修改记录:
*----------------------------------------
*修改人      修改时间     修改内容
  张博爱      2015-06-12         创建函数
****************************************************************/
HI_S32 Hal_VencSet(HI_S32 VencChn, 
                                SAMPLE_RC_E EncMode, 
                                PAYLOAD_TYPE_E MainEnPayLoad,
                                PAYLOAD_TYPE_E SubEnPayLoad,
                                PIC_SIZE_E MainEnSize,
                                PIC_SIZE_E SubEnSize
                                )
{
    HI_S32 s32VencGrp, s32VpssGrp, s32Ret,s32VencChn; 

    /* 没有子码流时 */
    if (ENPAYLOAD_INVALID == SubEnPayLoad)
    {
        /*** main stream **/
        s32VencChn = VencChn;
        s32VencGrp = s32VencChn;
        s32VpssGrp = s32VencChn;
        s32Ret = SAMPLE_COMM_VENC_Start(s32VencGrp, s32VencChn, MainEnPayLoad,\
                                                               gs_enNorm, MainEnSize, EncMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            return HI_FAILURE;
        }
        s32Ret = SAMPLE_COMM_VENC_BindVpss(s32VencGrp, s32VpssGrp, VPSS_BSTR_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            return HI_FAILURE;
        }
    }
    else
    {        
        /*** main stream **/
        s32VencChn = VencChn * 2;
        s32VencGrp = s32VencChn;
        s32VpssGrp = s32VencChn;
        s32Ret = SAMPLE_COMM_VENC_Start(s32VencGrp, VencChn, MainEnPayLoad,\
                                                               gs_enNorm, MainEnSize, EncMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            return HI_FAILURE;
        }
        s32Ret = SAMPLE_COMM_VENC_BindVpss(s32VencGrp, s32VpssGrp, VPSS_BSTR_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            return HI_FAILURE;
        }

        /*** sub stream **/
        s32VencGrp++;
        s32VencChn++;
        s32VpssGrp++;
        s32Ret = SAMPLE_COMM_VENC_Start(s32VencGrp, VencChn, SubEnPayLoad,\
                                                               gs_enNorm, SubEnSize, EncMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            return HI_FAILURE;
        }
        s32Ret = SAMPLE_COMM_VENC_BindVpss(s32VencGrp, s32VpssGrp, VPSS_LSTR_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            return HI_FAILURE;
        }
    } 

    return HI_SUCCESS;
}


HI_S32 Hal_VencGetStream(HI_S32 u32ViChnCnt)
{
    HI_S32 s32Ret;
        
    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32ViChnCnt);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
                
        return HI_FAILURE;
    }
    
    return HI_SUCCESS;
}

HI_S32 Hal_VencRegionOsd(HI_S32 VencChn)
{
    HI_S32 s32Ret;
    
    /*Overlay Region Process*/
    s32Ret = SAMPLE_Region_Venc(VencChn);//(VencChn, s32RgnCnt);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
            
        return HI_FAILURE;
    }
    
    return HI_SUCCESS;
}


HI_S32 Hal_VoParaInit(VO_DEV VoDevType, SAMPLE_VO_MODE_E eVoMode) 
{
    /* 把输出的参数清空 */
    memset(&gstVoPubAttr, 0x00, sizeof(VO_PUB_ATTR_S));
    
    /* 设置输出的参数 */
    if (SAMPLE_VO_DEV_DHD0 == VoDevType)
    {
        giVpssChn_VoHD0 = VPSS_PRE1_CHN;
        
        /*  HDMI + VGA */
        /******************************************
         step 6: start vo HD0 (HDMI+VGA), multi-screen, you can switch mode
        ******************************************/
        printf("start vo HD0.\n");
        genVoMode = eVoMode;
        if(VIDEO_ENCODING_MODE_PAL == gs_enNorm)
        {
            gstVoPubAttr.enIntfSync =  VO_OUTPUT_720P50;
        }
        else
        {
            gstVoPubAttr.enIntfSync =  VO_OUTPUT_720P60;
        }
#ifdef HI_FPGA
        gstVoPubAttr.enIntfType = VO_INTF_HDMI|VO_INTF_VGA|VO_INTF_BT1120;
#else
        gstVoPubAttr.enIntfType =  VO_INTF_VGA;//VO_INTF_HDMI|VO_INTF_VGA;
#endif
        gstVoPubAttr.u32BgColor = 0x000000ff;
        gstVoPubAttr.bDoubleFrame = HI_TRUE;
    }
    else
    {
        giVpssChn_VoHD0 = VPSS_BYPASS_CHN;
        /******************************************
        step 5: start vo SD1(CVBS)
        ******************************************/
        printf("start vo SD1.\n");
        genVoMode = eVoMode;
        gstVoPubAttr.enIntfSync = VO_OUTPUT_PAL;
        gstVoPubAttr.enIntfType = VO_INTF_CVBS;
        gstVoPubAttr.u32BgColor = 0x000000ff;
        gstVoPubAttr.bDoubleFrame = HI_FALSE;
    }

	/* 窗口的数量 */
    if (VO_MODE_1MUX == eVoMode)
    {
        guiWndNum = 1;
    }
    if (VO_MODE_4MUX == eVoMode)
    {
        guiWndNum = 4;
    }
    if (VO_MODE_9MUX == eVoMode)
    {
        guiWndNum = 8;
    }
    if (VO_MODE_16MUX == eVoMode)
    {
        guiWndNum = 16;
    } 
}

HI_S32 Hal_VoBindVpss(VO_DEV VoDevMode, SAMPLE_VO_MODE_E eVoMode)
{    
    HI_S32 s32Ret, i;
    VPSS_GRP VpssGrp;
    VO_CHN VoChn;
    VO_DEV VoDev = VoDevMode;

    /* 绑定VPSS和VO */
    for (i = 0; i < guiWndNum; i++)
    {
        VoChn = i ;/* out channel 0-----vpss 7 */
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,giVpssChn_VoHD0);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
        }
    }
}

HI_S32 Hal_VoUnBindVpss(VO_DEV VoDevMode, SAMPLE_VO_MODE_E eVoMode)
{    
    HI_S32 s32Ret, i;
    VPSS_GRP VpssGrp;
    VO_CHN VoChn;
    VO_DEV VoDev = VoDevMode;

    /* 解除绑定VPSS和VO */
    for (i = 0; i < guiWndNum; i++)
    {
        VoChn = i ;/* out channel 0-----vpss 7 */
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,giVpssChn_VoHD0);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
        }
    }
}

HI_S32 Hal_VoBindVi(VO_DEV VoDevMode, SAMPLE_VO_MODE_E eVoMode)
{    
    HI_S32 s32Ret, i;
    VPSS_GRP VpssGrp;
    VO_CHN VoChn;
    VO_DEV VoDev = VoDevMode;
    VI_CHN  ViChn;

    /* 绑定VI和VO */
    for (i = 0; i < guiWndNum; i++)
    {
        VoChn = i ; 
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VO_BindVi(VoDev, VoChn, ViChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
        }
    }
}


HI_S32 Hal_VoUnBindVi(VO_DEV VoDevMode, SAMPLE_VO_MODE_E eVoMode)
{    
    HI_S32 s32Ret, i;
    VO_CHN VoChn;
    VO_DEV VoDev = VoDevMode;

    /* 解除绑定VI和VO */
    for (i = 0; i < guiWndNum; i++)
    {
        VoChn = i ;/* out channel 0-----vpss 7 */
	 s32Ret = SAMPLE_COMM_VO_UnBindVi(VoDev, VoChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Stop VO failed!\n");
        }
    }
}

HI_S32 Hal_VoInit(VO_DEV VoDevMode, SAMPLE_VO_MODE_E eVoMode)
{
    HI_S32 s32Ret, i;
    VPSS_GRP VpssGrp;
    VO_CHN VoChn;
    VO_DEV VoDev = VoDevMode;
    
    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &gstVoPubAttr, gs_u32ViFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartDevLayer failed!\n");
        goto END_8D1_4;
    }
    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &gstVoPubAttr, eVoMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_StartChn failed!\n");
        goto END_8D1_5;
    }
    /* if it's displayed on HDMI, we should start HDMI */
    if (gstVoPubAttr.enIntfType & VO_INTF_HDMI)
    {
        if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(gstVoPubAttr.enIntfSync))
        {
            SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
            goto END_8D1_5;
        }
    }

    return HI_SUCCESS;
    
END_8D1_5:
    if (gstVoPubAttr.enIntfType & VO_INTF_HDMI)
    {
        SAMPLE_COMM_VO_HdmiStop();
    }

    VoDev = VoDevMode;
    SAMPLE_COMM_VO_StopChn(VoDev, eVoMode);
    for(i=0; i<guiWndNum; i++)
    {
        VoChn = i;
        VpssGrp = i;
        SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,giVpssChn_VoHD0);
    }
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
    
END_8D1_4:
    SAMPLE_COMM_SYS_Exit();

    return HI_FAILURE;
}

HI_S32 Hal_VoModifyPara(VO_DEV VoDevMode, SAMPLE_VO_MODE_E enPreVoMode,SAMPLE_VO_MODE_E eVoMode) 
{
    HI_S32 s32Ret, i;
    VPSS_GRP VpssGrp;
    VO_CHN VoChn;
    VO_DEV VoDev = VoDevMode;
        
    s32Ret= HI_MPI_VO_SetAttrBegin(VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start VO failed111!\n");
        goto END_8D1_5;
    }
    s32Ret = SAMPLE_COMM_VO_StopChn(VoDev, enPreVoMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start VO failed222!\n");
        goto END_8D1_5;
    }
    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &gstVoPubAttr, eVoMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start VO failed333!\n");
        goto END_8D1_5;
    }
    s32Ret= HI_MPI_VO_SetAttrEnd(VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start VO failed444!\n");
        goto END_8D1_5;
    }

    return ;
    
END_8D1_5:
    if (gstVoPubAttr.enIntfType & VO_INTF_HDMI)
    {
        SAMPLE_COMM_VO_HdmiStop();
    }
    VoDev = VoDevMode;

    SAMPLE_COMM_VO_StopChn(VoDev, eVoMode);
    for(i=0; i<guiWndNum; i++)
    {
        VoChn = i;
        VpssGrp = i;
        SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,giVpssChn_VoHD0);
    }
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
    
END_8D1_4:
    SAMPLE_COMM_SYS_Exit();
}


HI_S32 Hal_DecParaInit(PIC_SIZE_E enPicSize, HI_S32 s32DecChnCnt)
{
    HI_S32 s32Ret;

    if (s32DecChnCnt > SAMPLE_MAX_VDEC_CHN_CNT || s32DecChnCnt <= 0)
    {
        SAMPLE_PRT("Vdec count %d err, should be in [%d,%d]. \n", s32DecChnCnt, 1, SAMPLE_MAX_VDEC_CHN_CNT);
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enPicSize, &gsDecPicSize);
    if (HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return HI_FAILURE;
    }

    if (704 == gsDecPicSize.u32Width)
    {
        gsDecPicSize.u32Width = 720;
    }
    else if (352 == gsDecPicSize.u32Width)
    {
        gsDecPicSize.u32Width = 360;
    }
    else if (176 == gsDecPicSize.u32Width)
    {
        gsDecPicSize.u32Width = 180;
    }

    return HI_SUCCESS;
}

HI_S32 Hal_DecRelese(HI_S32 s32Cnt, VO_DEV VoDev)
{
    VDEC_CHN VdChn;
    HI_S32 s32Ret;
    HI_S32 i;
    VPSS_GRP VpssGrp;
    VIDEO_MODE_E enVdecMode;
    VO_CHN VoChn;
    HI_U32 u32WndNum = s32Cnt;

    for (i=0; i<s32Cnt; i++)
    {
        VdChn = i;
        SAMPLE_VDEC_WaitDestroyVdecChn(VdChn, enVdecMode);
        if (SAMPLE_VO_DEV_DHD0 == VoDev)
        {
            VpssGrp = i + DECODE_VPASSGRP_START;
            SAMLE_COMM_VDEC_UnBindVpss(VdChn, VpssGrp);
        }
        else
        {
            VoChn = i;
            SAMLE_COMM_VDEC_UnBindVo(VdChn, VoDev, VoChn);
        }
    }
    
    /******************************************
     step 8: Unbind Vo to vpss(dec channel)
    ******************************************/
    for (i = 0; i < s32Cnt; i++)
    {
        VoChn = i;
        if (SAMPLE_VO_DEV_DHD0 == VoDev)
        {
            VpssGrp = i + DECODE_VPASSGRP_START;
            SAMPLE_COMM_VO_UnBindVpss(VoDev, VoChn, VpssGrp, VPSS_PRE0_CHN);
            SAMLE_COMM_VDEC_UnBindVpss(VdChn, VpssGrp);
        }
    }
}

HI_S32 Hal_DecSet(PAYLOAD_TYPE_E enType, HI_S32 s32Cnt, VO_DEV VoDev)
{ 
    VDEC_CHN VdChn;
    HI_S32 s32Ret;
    VB_CONF_S stVbConf;
    HI_S32 i;
    VPSS_GRP VpssGrp;
    VIDEO_MODE_E enVdecMode;
    HI_CHAR ch;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr;
    SAMPLE_VO_MODE_E enVoMode;
    HI_U32 u32WndNum = s32Cnt, u32BlkSize;
    HI_BOOL bVoHd; // through Vpss or not. if vo is SD, needn't through vpss

    /******************************************
     step 5: start vdec & bind it to vpss or vo
    ******************************************/
    enVdecMode = VIDEO_MODE_FRAME;
    
    for (i = 0; i <s32Cnt; i++)
    {
        /***** create vdec chn *****/
        VdChn = i;
        s32Ret = SAMPLE_VDEC_CreateVdecChn(VdChn, &gsDecPicSize, enType, enVdecMode);
        if (HI_SUCCESS !=s32Ret)
        {
            SAMPLE_PRT("create vdec chn failed!\n");
            goto END_3;
        }
		
        /***** bind vdec to vpss *****/
        if (SAMPLE_VO_DEV_DHD0 == VoDev)
        {
            VpssGrp = i + DECODE_VPASSGRP_START;
            s32Ret = SAMLE_COMM_VDEC_BindVpss(VdChn, VpssGrp);
            if (HI_SUCCESS !=s32Ret)
            {
                SAMPLE_PRT("vdec(vdch=%d) bind vpss(vpssg=%d) failed!\n", VdChn, VpssGrp);
                goto END_3;
            }
        }
        else
        {
            VoChn =  i;
            s32Ret = SAMLE_COMM_VDEC_BindVo(VdChn, VoDev, VoChn);
            if (HI_SUCCESS !=s32Ret)
            {
                SAMPLE_PRT("vdec(vdch=%d) bind vo(vo=%d) failed!\n", VdChn, VoChn);
                goto END_3;
            }
        }
    }

    /* VO的操作，绑定VPSS或者VDEC到VO */		
    /* 如果VO为HD，VO ------VPSS 
        如果VO为SD，VO -----VDEC */  
    for (i = 0; i < u32WndNum; i++)
    {
        VoChn = i;
		
        if (SAMPLE_VO_DEV_DHD0 == VoDev)
        {
            VpssGrp = i + DECODE_VPASSGRP_START;
            s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE0_CHN);
            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
                goto END_3;
            }
        }
    }

    return HI_SUCCESS;

    /* 出错处理 */
END_3:
    Hal_DecRelese(s32Cnt, VoDev);
    return HI_FAILURE;
}


void Vedio_VariableInit(HI_U32 u32ViChnCnt, PIC_SIZE_E enCodeSize)
{
    memset(&gsVbConf, 0, sizeof(VB_CONF_S));
    Hal_VariableInit(u32ViChnCnt, enCodeSize);
}

HI_S32 Vedio_Init(VB_CONF_S VbConf)
{    
    return Hal_Init(VbConf);
}

HI_S32 Vedio_ViInit(SAMPLE_VI_MODE_E EnViMode)
{
    return Hal_ViInit(EnViMode);
}

HI_S32 Vedio_VpssInit(SAMPLE_VI_MODE_E EnViMode, HI_S32 s32VpssGrpCnt, PIC_SIZE_E enSize)
{
    return Hal_VpssInit(EnViMode, s32VpssGrpCnt, enSize);
}

HI_S32 Vedio_ViBindVpss(SAMPLE_VI_MODE_E EnViMode)
{
    return  Hal_ViBindVpss(EnViMode);
}

HI_S32 Vedio_VoSet(VO_DEV VoDevType, SAMPLE_VO_MODE_E eVoMode)
{
    /* 设置输出的绑定关系 */
    if (SAMPLE_VO_DEV_DHD0 == VoDevType)
    {
        Hal_VoBindVpss(VoDevType, eVoMode);
    }
    else
    {
        Hal_VoBindVi(VoDevType, eVoMode);
    }
}

HI_S32 Vedio_VoRelease(VO_DEV VoDevType, SAMPLE_VO_MODE_E eVoMode)
{    
    /* 解除输出的绑定关系 */
    if (SAMPLE_VO_DEV_DHD0 == VoDevType)
    {
        Hal_VoUnBindVpss(VoDevType, eVoMode);
    }
    else
    {
        Hal_VoUnBindVi(VoDevType, eVoMode);
    }
}

HI_S32 Vedio_Vo(VO_DEV VoDevMode, SAMPLE_VO_MODE_E eVoMode, SAMPLE_VO_MODE_E ePreVoMode) 
{
    HI_S32 s32Ret, i;
    VPSS_GRP VpssGrp;
    VO_CHN VoChn;
    VO_DEV VoDev = VoDevMode;

    
    if (eVoMode == ePreVoMode)
    {
        Hal_VoParaInit(VoDevMode, eVoMode); 

        s32Ret = Hal_VoInit(VoDevMode, eVoMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start SAMPLE_COMM_VO_StartDevLayer failed!\n");
            goto END_8D1_4;
        }

        /* 设置输出的绑定关系 */
        if (SAMPLE_VO_DEV_DHD0 == VoDevMode)
        {
            s32Ret = Hal_VoBindVpss(VoDevMode, eVoMode);
        }
        else
        {
            s32Ret = Hal_VoBindVi(VoDevMode, eVoMode);
        }
        
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Vo Band Vpss failed!\n");
            goto END_8D1_5;
        }
    }
    else
    {
        Hal_VoModifyPara(VoDevMode, ePreVoMode, eVoMode);
    }
    
    return HI_SUCCESS;
    
END_8D1_5:
    if (gstVoPubAttr.enIntfType & VO_INTF_HDMI)
    {
        SAMPLE_COMM_VO_HdmiStop();
    }

    VoDev = VoDevMode;
    SAMPLE_COMM_VO_StopChn(VoDev, eVoMode);
    for(i=0; i<guiWndNum; i++)
    {
        VoChn = i;
        VpssGrp = i;
        SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,giVpssChn_VoHD0);
    }
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
    
END_8D1_4:
    SAMPLE_COMM_SYS_Exit();
}


HI_S32 Vedio_VencSet(VENC_CHN TotalVencChn, SAMPLE_RC_E EncMode, PAYLOAD_TYPE_E MainEnPayLoad, PAYLOAD_TYPE_E SubEnPayLoad, PIC_SIZE_E MainEnSize, PIC_SIZE_E SubEnSize)
{
    HI_S32 s32Ret;
    VENC_CHN VencChn;

    for (VencChn = 0; VencChn < TotalVencChn; VencChn++)
    {
        s32Ret = Hal_VencSet(VencChn, EncMode, MainEnPayLoad, SubEnPayLoad, MainEnSize, SubEnSize);
        if (HI_SUCCESS != s32Ret)
        {
            Hal_VencRelese(VencChn, EncMode, MainEnPayLoad, SubEnPayLoad, MainEnSize, SubEnSize);
            return s32Ret;
        }
    }
}

HI_S32 Vedio_Venc_Process(SAMPLE_RC_E EncMode, 
                    SAMPLE_VI_MODE_E EnViMode,
                    VENC_CHN TotalVencChn,  
                    PAYLOAD_TYPE_E MainEnPayLoad,
                    PAYLOAD_TYPE_E SubEnPayLoad,
                    PIC_SIZE_E MainEnSize,
                    PIC_SIZE_E SubEnSize
                    )
{
    HI_S32 s32Ret;
     
    s32Ret = Hal_VencGetStream(TotalVencChn);
    if (HI_SUCCESS != s32Ret)
    {
        Hal_VencRelese(TotalVencChn, EncMode, MainEnPayLoad, SubEnPayLoad, MainEnSize, SubEnSize);
        return s32Ret;
    }
    
    s32Ret = Hal_VencRegionOsd(TotalVencChn);
    if (HI_SUCCESS != s32Ret)
    {
        Hal_VencRelese(TotalVencChn, EncMode, MainEnPayLoad, SubEnPayLoad, MainEnSize, SubEnSize);
        return s32Ret;
    }
    
    return s32Ret;
}


/******************************************************************************
* function : vdec process
*            vo is sd : vdec -> vo
*            vo is hd : vdec -> vpss -> vo
******************************************************************************/
HI_S32 Vedio_Vdec_Process(PIC_SIZE_E enPicSize, PAYLOAD_TYPE_E enType, HI_S32 s32Cnt, VO_DEV VoDev)
{
    HI_S32 s32Ret;
    HI_S32 i;
    VIDEO_MODE_E enVdecMode;


    /******************************************
     step 1: 解码所需的参数初始化
    ******************************************/
    s32Ret = Hal_DecParaInit(enPicSize, s32Cnt);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FALSE;
    }

    /******************************************
     step 2: 解码器硬件设置
    ******************************************/
    s32Ret = Hal_DecSet(enType, s32Cnt, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FALSE;
    }
    
    /******************************************
     step 3: 操作解码文件系统//open file & video decoder
    ******************************************/
    for (i=0; i<s32Cnt; i++)
    {
        gs_SendParam[i].bRun = HI_TRUE;
        gs_SendParam[i].VdChn = i;
        gs_SendParam[i].enPayload = enType;
        gs_SendParam[i].enVideoMode = enVdecMode;
        gs_SendParam[i].s32MinBufSize = gsDecPicSize.u32Height * gsDecPicSize.u32Width * 3/ 4;
        pthread_create(&gs_SendParam[i].Pid, NULL, SAMPLE_VDEC_SendStream1, &gs_SendParam[i]);
    }
	
    if (PT_JPEG != enType)
    {
        printf("you can press ctrl+c to terminate program before normal exit.\n");
    }
    
    /******************************************
     step 4: 解码完成,结束解码线程//join thread
    ******************************************/
    for (i = 0; i < s32Cnt; i++)
    {
        pthread_join(gs_SendParam[i].Pid, 0);
        printf("join thread %d.\n", i);
    }

    
    /******************************************
     step 5: 处理完成，释放资源//Unbind vdec to vpss & destroy vdec-chn
    ******************************************/
    Hal_DecRelese(s32Cnt, VoDev);    
    return HI_SUCCESS;
}

/***************************************************************
*名称: HI_S32 Hal_SnapInit(PIC_SIZE_E e_pic_size)
*描述: 硬件抽象层抓拍的初始化
*输入:  抓拍的图片的大小
*输出:  无
*修改记录:
*----------------------------------------
*修改人      修改时间     修改内容
  张博爱      2015-06-12         创建函数
****************************************************************/
HI_S32 Hal_SnapInit(PIC_SIZE_E e_pic_size)
{    
    HI_S32 s32Ret;
    SIZE_S stSize;

    /*snap Cif pic*/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, e_pic_size, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VENC_SnapStart(SNAP_ENCODE_GRP, SNAP_ENCODE_CNT, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start snap failed!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/***************************************************************
*名称: HI_S32 Hal_Snap(HI_S32 ViChannel)
*描述: 硬件抽象层抓拍 
*输入:  通道号
*输出:  无
*修改记录:
*----------------------------------------
*修改人      修改时间     修改内容
  张博爱      2015-06-12         创建函数
****************************************************************/
HI_S32 Hal_Snap(HI_S32 ViChannel)
{
    VPSS_GRP VpssGrp;
    HI_S32 s32Ret;
    
    /*** main frame **/
    VpssGrp = ViChannel ;
    s32Ret = SAMPLE_COMM_VENC_SnapProcess(SNAP_ENCODE_GRP, SNAP_ENCODE_CNT, VpssGrp, VPSS_PRE0_CHN);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("snap process failed!\n");
        
        s32Ret = SAMPLE_COMM_VENC_SnapStop(SNAP_ENCODE_GRP, SNAP_ENCODE_CNT);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Stop snap failed!\n");
        }
        return HI_FAILURE;
    }
    printf("snap chn %d ok!\n", ViChannel);

    return HI_SUCCESS;
}


HI_S32 Vedio_SnapInit(PIC_SIZE_E e_pic_size)
{
    return  Hal_SnapInit(e_pic_size);
}

HI_S32 Vedio_Snap(HI_S32 ViChannel)
{
    return Hal_Snap(ViChannel);
}


/******************************************************************************
* function :  16*Cif SNAP
******************************************************************************/
HI_S32 SAMPLE_VENC_8_D1_Snap(HI_VOID)
{
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_D1;
    HI_U32 u32ViChnCnt = 8;
    HI_S32 s32VpssGrpCnt = 8;
    PIC_SIZE_E enSize = PIC_D1;
    VB_CONF_S stVbConf;
    VPSS_GRP VpssGrp;
    VPSS_GRP_ATTR_S stGrpAttr;
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    /******************************************
     step  1: init variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                 enSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;
    /* video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 6;
    memset(stVbConf.astCommPool[0].acMmzName,0,
           sizeof(stVbConf.astCommPool[0].acMmzName));
    /* hist buf*/
    stVbConf.astCommPool[1].u32BlkSize = (196*4);
    stVbConf.astCommPool[1].u32BlkCnt = u32ViChnCnt * 6;
    memset(stVbConf.astCommPool[1].acMmzName,0,
           sizeof(stVbConf.astCommPool[1].acMmzName));
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_SNAP_0;
    }
    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_Start(enViMode, gs_enNorm);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_SNAP_0;
    }
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_SNAP_0;
    }
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_Start(s32VpssGrpCnt, &stSize, VPSS_MAX_CHN_NUM,NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_SNAP_1;
    }
    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_SNAP_2;
    }
    /******************************************
     step 5: snap process
    ******************************************/
    VencGrp = 8;
    VencChn = 8;
    /*snap Cif pic*/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, PIC_D1, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_SNAP_0;
    }
    s32Ret = SAMPLE_COMM_VENC_SnapStart(VencGrp, VencChn, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start snap failed!\n");
        goto END_VENC_SNAP_3;
    }
    for (i=0; i<u32ViChnCnt; i++)
    {
        /*** main frame **/
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VENC_SnapProcess(VencGrp, VencChn, VpssGrp, VPSS_PRE0_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("snap process failed!\n");
            goto END_VENC_SNAP_4;
        }
        printf("snap chn %d ok!\n", i);
    }
    /******************************************
     step 8: exit process
    ******************************************/
    printf("snap over!\n");
END_VENC_SNAP_4:
    s32Ret = SAMPLE_COMM_VENC_SnapStop(VencGrp, VencChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Stop snap failed!\n");
        goto END_VENC_SNAP_3;
    }
END_VENC_SNAP_3:
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_SNAP_2:    //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_SNAP_1:    //vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_SNAP_0:    //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

 
/***************************************************************
*名称: void SAMPLE_VIO_8_D1_Hifb1(SAMPLE_VI_MODE_E enViMode, PIC_SIZE_E enCodeSize, HI_U8 EncMode, VO_DEV VoDevMode)
*描述: 带区域叠加的编码
*输入:   SAMPLE_VI_MODE_E enViMode ----编码输入方式
                 PIC_SIZE_E enCodeSize -------编码大小
                 HI_U8 EncMode-------编码方式，CBR或者VBR
                 VoDevMode----------输出显示方式
*输出:  无  
*修改记录
*----------------------------------------
*修改人      修改时间     修改内容
  张博爱      2016-06-10          创建函数
****************************************************************/
HI_S32 SAMPLE_VIO_8_D1_Hifb1(SAMPLE_VI_MODE_E enViMode, PIC_SIZE_E enCodeSize, HI_U8 EncMode, VO_DEV VoDevMode)
{
    HI_U32 u32ViChnCnt = 8;
    HI_S32 s32VpssGrpCnt = 24;
    HI_S32 s32RgnCnt = 8;
    VPSS_GRP VpssGrp;
    VO_DEV VoDev = VoDevMode;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr;
    SAMPLE_VO_MODE_E enVoMode, enPreVoMode;
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    HI_CHAR ch;
    PAYLOAD_TYPE_E enPayLoad = PT_H264;
    PIC_SIZE_E enSize =  enCodeSize;
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    
    SIZE_S stSize;
    
    /******************************************
     step  1: 参数初始化
    ******************************************/
    Vedio_VariableInit(u32ViChnCnt, enCodeSize);

    /******************************************
     step 2: 系统初始化
    ******************************************/
    s32Ret = Vedio_Init(gsVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }
    
    /******************************************
     step 3: VI 初始化
    ******************************************/
    s32Ret = Vedio_ViInit(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }
     
    /******************************************
     step 4: VPSS初始化
    ******************************************/
    s32Ret = Vedio_VpssInit(enViMode, s32VpssGrpCnt, enSize);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }
    
    /******************************************
     step 5: VI绑定VPSS
    ******************************************/
    s32Ret = Vedio_ViBindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }
    
    /******************************************
     step 5: snap init
    ******************************************/
    s32Ret = Vedio_SnapInit(PIC_D1);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }   
    
    /******************************************
     step 5: select encode mode CBR VCR FIXQP start stream venc (big + little)
    ******************************************/
    VencChn = 8;
    s32Ret = Vedio_VencSet(VencChn, EncMode, enPayLoad, ENPAYLOAD_INVALID, enSize, 0x00);        
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }
    
    s32Ret = Vedio_Venc_Process(EncMode, enViMode, VencChn, enPayLoad, ENPAYLOAD_INVALID, enSize, 0x00);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }
    
    printf("please press any key to exit\n");

    
    enVoMode = VO_MODE_9MUX;
    enPreVoMode = enVoMode;
    
    /* 输出显示 */
    Vedio_Vo(VoDevMode, enVoMode, enPreVoMode);


     Vedio_Snap(0);
     Vedio_Snap(1);
     Vedio_Snap(2);
     Vedio_Snap(3);
     Vedio_Snap(4);
     Vedio_Snap(5);
     Vedio_Snap(6);
     Vedio_Snap(7);


        
    /******************************************
    step 8: HD0 switch mode
    ******************************************/
    while(1)
    {
        enPreVoMode = enVoMode;
        printf("please choose preview mode, press 'q' to exit this sample.\n");
        printf("\t0) 1 preview\n");
        printf("\t1) 4 preview\n");
        printf("\t2) 8 preview\n");
        printf("\t3) dec code\n");
        printf("\tq) quit\n");
        ch = getchar();
        getchar();
        if ('0' == ch)
        {
            enVoMode = VO_MODE_1MUX;
            Vedio_Vo(VoDevMode, enVoMode, enPreVoMode);
        }
        else if ('1' == ch)
        {
            enVoMode = VO_MODE_4MUX;
            Vedio_Vo(VoDevMode, enVoMode, enPreVoMode);
        }
        /*Indeed only 8 chns show*/
        else if ('2' == ch)
        {
            enVoMode = VO_MODE_9MUX;
            Vedio_Vo(VoDevMode, enVoMode, enPreVoMode);
        }
        else if  ('3' == ch)/* 解码开始 */
        {
            Vedio_VoRelease(VoDevMode, enVoMode);
            
            Vedio_Vdec_Process(PIC_D1, PT_H264, guiWndNum, VoDevMode);

            Vedio_VoSet(VoDevMode, enVoMode);
        }
        else if ('q' == ch)
        {
            break;
        }
        else
        {
            SAMPLE_PRT("preview mode invaild! please try again.\n");
            continue;
        }
        SAMPLE_PRT("vo(%d) switch to %d mode\n", VoDev, guiWndNum);
    }
    
    /******************************************
     step 8: exit process
    ******************************************/
END_VENC_8D1_3:
    SAMPLE_COMM_VENC_StopGetStream();
    for (i=0; i<u32ViChnCnt; i++)
    {
        VencGrp = i;
        VencChn = i;
        VpssGrp = i;
        SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VPSS_PRE0_CHN);
        SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
    }
END_8D1_7:
    SAMPLE_COMM_VO_UnBindVoWbc(SAMPLE_VO_DEV_DSD0, 0);
    HI_MPI_VO_DisableWbc(SAMPLE_VO_DEV_DHD0);
END_8D1_6:
    VoDev = SAMPLE_VO_DEV_DSD0;
    VoChn = 0;
    enVoMode = VO_MODE_1MUX;
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_8D1_5:
    if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
    {
        SAMPLE_COMM_VO_HdmiStop();
    }
    VoDev = SAMPLE_VO_DEV_DHD0;
    enVoMode = VO_MODE_16MUX;
    /*??disableChn ,????V??????*/
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
    for(i=0; i<guiWndNum; i++)
    {
        VoChn = i;
        VpssGrp = i;
        SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE0_CHN);
    }
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_8D1_4:
END_8D1_3:  //vi unbind vpss
    SAMPLE_COMM_VI_UnBindVpss(geEnViMode);
END_8D1_2:  //vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_8D1_1:  //vi stop
    SAMPLE_COMM_VI_Stop(geEnViMode);
END_8D1_0:  //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}






int main_vedio(int argc, char *argv[])
{
    VO_PUB_ATTR_S stPubAttr;
    VB_CONF_S stVbConf;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 i;
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_8_D1;
    SAMPLE_VO_MODE_E stVoMode = VO_MODE_9MUX;
    HI_BOOL bExtendedMode;
    HI_CHAR ch;
    
    if ((argc < 2) || (1 != strlen(argv[1])))
    {
        SAMPLE_VIO_Usage(argv[0]);
        return HI_FAILURE;
    }
    signal(SIGINT, SAMPLE_VIO_HandleSig);
    signal(SIGTERM, SAMPLE_VIO_HandleSig);


//SAMPLE_VENC_8_D1_Snap();

    s32Ret = SAMPLE_VIO_8_D1_Hifb1(SAMPLE_VI_MODE_8_D1, PIC_D1, SAMPLE_RC_CBR, SAMPLE_VO_DEV_DHD0);
    /*should unbind first*/
    if(HI_SUCCESS != HI_MPI_VO_GfxLayerUnBindDev(GRAPHICS_LAYER_HC0, SAMPLE_VO_DEV_DHD0))
    {
        printf("%s: Graphic UnBind to VODev failed!,line:%d\n", __FUNCTION__, __LINE__);
        SAMPLE_COMM_SYS_Exit();
        SAMPLE_HIFB_VO_Stop();
        return -1;
    }
    if (HI_SUCCESS != HI_MPI_VO_GfxLayerBindDev(GRAPHICS_LAYER_HC0, SAMPLE_VO_DEV_DHD0))
    {
        printf("%s: Graphic Bind to VODev failed!,line:%d\n", __FUNCTION__, __LINE__);
        SAMPLE_COMM_SYS_Exit();
        SAMPLE_HIFB_VO_Stop();
        return -1;
    }
    /******************************************
    4  exit process
    ******************************************/
    for(i=0; i<4; i++)
    {
        SAMPLE_COMM_VO_UnBindVi(SAMPLE_VO_DEV_DHD0,i);
    }
    SAMPLE_COMM_VO_StopChn(SAMPLE_VO_DEV_DHD0, stVoMode);
    SAMPLE_COMM_VO_StopDevLayer(SAMPLE_VO_DEV_DHD0);
    if (stPubAttr.enIntfType & VO_INTF_HDMI)
    {
        SAMPLE_COMM_VO_HdmiStop();
    }
    SAMPLE_COMM_VI_Stop(enViMode);
    /*mpi exit */
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
    return 0;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */




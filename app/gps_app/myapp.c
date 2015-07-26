/******************************************************************************
  A simple program of Hisilicon HI3531 video encode implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-2 Created
******************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

//#include <conio.h>
//#include <process.h> 
//#include <dos.h>
#include <fcntl.h>
#include <sys/stat.h> 

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>


#include "gps_typedef.h"
#include "sample_comm.h"
#include "gps_param.h"
#include "nanomsg_proc.h"
#include "gps_console.h"
#include "gps_store.h"


static st_gps_thread_param	gt_console_para;		///调试接口线程参数


static VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_PAL;
static HI_U32 gs_u32ViFrmRate = 0;

#define SAMPLE_YUV_D1_FILEPATH         "SAMPLE_420_D1.yuv"


typedef void (*sighandler_t)(int); 

int my_system1(const char *cmd_line) 
{ 
	int ret = 0; 
	sighandler_t old_handler; 
	old_handler = signal(SIGCHLD, SIG_DFL); 
	ret = system(cmd_line); 
	signal(SIGCHLD, old_handler); 
	return ret; 
} 

static int my_system2(const char * cmd) 
{ 
	FILE * fp; 
	int res; char buf[1024]; 
	if (cmd == NULL) 
	{ 
		printf("my_system cmd is NULL!\n");
		return -1;
	} 
	if ((fp = popen(cmd, "r") ) == NULL) 
	{ 
		perror("popen");
		printf("popen error: %s/n", strerror( 2 )); 
		return -1; 
	} 
	else
	{
		while(fgets(buf, sizeof(buf), fp)) 
		{ 
			printf("%s", buf); 
		} 
		if ( (res = pclose(fp)) == -1) 
		{ 
			printf("close popen file pointer fp error!\n"); 
			return res;
		} 
		else if (res == 0) 
		{
			return res;
		} 
		else 
		{ 
			printf("popen res is :%d\n", res); return res; 
		} 
	}
} 

/******************************************************************************
* function : show usage
******************************************************************************/
void myapp_VENC_Usage(char *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);
    printf("index:\n");
    printf("\t 0) 4D1 H264 encode.\n");
    printf("\t 1) 1*720p H264 encode.\n");
    printf("\t 2) 1D1 MJPEG encode.\n");
    printf("\t 3) 4D1 JPEG snap with StartRecvPic.\n");
    printf("\t 4) 4D1 JPEG snap with StartRecvPicEx.\n");
	printf("\t 5) 8*Cif JPEG snap.\n");
    printf("\t 6) 1D1 User send pictures for H264 encode.\n");
    printf("\t 7) 4D1 H264 encode with color2grey.\n");
    printf("\t 8) TEST system command \"ls -al /etc /bin\"\n");
    printf("\t 9) TEST system command argv[2]!\n");
    printf("\t a) TEST param !\n");
    printf("\t b) TEST nanomsg!\n");
    printf("\t c) TEST mkdir!\n");
    printf("\t d) disk_remove_dir!\n");
    printf("\t e) console_Start!\n");
    printf("\t f) store_init!\n");
    return;
}

/******************************************************************************
* function : to process abnormal case                                         
******************************************************************************/
void myapp_VENC_HandleSig(HI_S32 signo)
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
void myapp_VENC_StreamHandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

/******************************************************************************
* function :  4D1 H264 encode
******************************************************************************/
HI_S32 myapp_VENC_4D1_H264(HI_VOID)
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
	///
	VO_DEV VoDev;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr,stVoPubAttrSD; 
    SAMPLE_VO_MODE_E enVoMode;
    HI_U32 u32WndNum;
	///
    
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
///////////////////////////////////////START
 /******************************************
     step 6: start vo HD0 (VGA), multi-screen, you can switch mode
    ******************************************/
    printf("start vo HD0.\n");
    VoDev = SAMPLE_VO_DEV_DHD0;
    u32WndNum = u32ViChnCnt;
    enVoMode = VO_MODE_4MUX;
    
    if(VIDEO_ENCODING_MODE_PAL == gs_enNorm)
    {
		stVoPubAttr.enIntfSync = VO_OUTPUT_1280x1024_60;
    }
    else
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_720P60;
    }

	stVoPubAttr.enIntfType = VO_INTF_VGA;
    stVoPubAttr.u32BgColor = 0x000000ff;
    stVoPubAttr.bDoubleFrame = HI_TRUE;
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL== gs_enNorm)?25:30;
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
    
    for(i=0;i<u32WndNum;i++)
    {
        VoChn = i;
        VpssGrp = i;
        
        s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE1_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start VO failed!\n");
            goto END_8D1_5;
        }
    }

///////////////////////////////////////END
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
        VencGrp = i*2;
        VencChn = i*2;
        VpssGrp = i;
        s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad[0],\
                                       gs_enNorm, enSize[0], enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_8D1_2;
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VPSS_PRE0_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_8D1_3;
        }

        /*** Sub stream **/
        VencGrp ++;
        VencChn ++;
        s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad[1], \
                                        gs_enNorm, enSize[1], enRcMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_8D1_3;
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VPSS_LSTR_CHN);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_8D1_3;
        }
    }
///////////////////
	/******************************************
	step 5: snap process
	******************************************/
	VencGrp = 2*u32ViChnCnt;
	VencChn = 2*u32ViChnCnt;
	s32Ret = SAMPLE_COMM_VENC_SnapStart(VencGrp, VencChn, &stSize);
	if (HI_SUCCESS != s32Ret)
	{
	  SAMPLE_PRT("Start snap failed!\n");
	  goto END_VENC_SNAP_4;
	}

///////////////////
    /******************************************
     step 6: stream venc process -- get stream, then save it to file. 
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32ViChnCnt*2);
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
END_VENC_SNAP_4:
		///STOP SNAP
		s32Ret = SAMPLE_COMM_VENC_SnapStop(VencGrp, VencChn);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Stop snap failed!\n");
			//goto END_VENC_SNAP_3;
		}
END_8D1_5:
		///STOP VO BIND fjdkaslfdsjkafsjfakljfa
		if (stVoPubAttr.enIntfType & VO_INTF_HDMI)
		{
			SAMPLE_COMM_VO_HdmiStop();
		}
		VoDev = SAMPLE_VO_DEV_DHD0;
		u32WndNum = u32ViChnCnt;
		enVoMode = VO_MODE_4MUX;
		/*先disableChn ,再解除绑定*/
		SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
		for(i=0;i<u32WndNum;i++)
		{
			VoChn = i;
			VpssGrp = i;
			SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,VPSS_PRE1_CHN);
		}
		SAMPLE_COMM_VO_StopDevLayer(VoDev);
	
END_8D1_4:
#if HICHIP == HI3521_V100
		VoDev = SAMPLE_VO_DEV_DSD1;
		VoChn = 0;
		enVoMode = VO_MODE_1MUX;
		SAMPLE_COMM_VO_UnBindVi(VoDev,VoChn); 
		SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
		SAMPLE_COMM_VO_StopDevLayer(VoDev);
#endif

END_VENC_8D1_3:
    for (i=0; i<u32ViChnCnt*2; i++)
    {
        VencGrp = i;
        VencChn = i;
        VpssGrp = i/2;
        VpssChn = (VpssGrp%2)?VPSS_LSTR_CHN:VPSS_PRE0_CHN;
        SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
        SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
    }
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_VENC_8D1_2:	//vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_8D1_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_8D1_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;
}

/******************************************************************************
* function :  1HD H264 encode
******************************************************************************/
HI_S32 myapp_VENC_1HD_H264(HI_VOID)
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
END_VENC_1HD_2:	//vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_1HD_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_1HD_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;
}

/******************************************************************************
* function :  1D1 MJPEG encode
******************************************************************************/
HI_S32 myapp_VENC_1D1_MJPEG(HI_VOID)
{
    SAMPLE_VI_MODE_E enViMode = SAMPLE_VI_MODE_1_D1;

    HI_U32 u32ViChnCnt = 1;
    HI_S32 s32VpssGrpCnt = 1;
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
END_VENC_MJPEG_2:	//vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_MJPEG_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_MJPEG_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;
}

/******************************************************************************
* function :  4D1 SNAP
******************************************************************************/
HI_S32 myapp_VENC_4D1_Snap(HI_VOID)
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
END_VENC_SNAP_2:	//vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_SNAP_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_SNAP_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;
}

/******************************************************************************
* function :  16*Cif SNAP
******************************************************************************/
HI_S32 myapp_VENC_8_Cif_Snap(HI_VOID)
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
END_VENC_SNAP_2:	//vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_SNAP_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_SNAP_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;
}


/******************************************************************************
* function :  1D1 User send pictures for H264 encode
******************************************************************************/
HI_S32 myapp_VENC_1D1_USER_SEND_PICTURES(HI_VOID)
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
HI_S32 myapp_VENC_4D1_H264_COLOR2GREY(HI_VOID)
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
END_VENC_16D1_2:	//vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_16D1_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_16D1_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;
}

/******************************************************************************
* function :  4D1 SNAP with StartRecvPicEx
******************************************************************************/
HI_S32 myapp_VENC_4D1_SnapEx(HI_VOID)
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
END_VENC_SNAP_2:	//vpss stop
    SAMPLE_COMM_VPSS_Stop(s32VpssGrpCnt, VPSS_MAX_CHN_NUM);
END_VENC_SNAP_1:	//vi stop
    SAMPLE_COMM_VI_Stop(enViMode);
END_VENC_SNAP_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;
}
#if 0

/******************************************************************************
* function    : main()
* Description : video venc sample
******************************************************************************/
int main(int argc, char *argv[])
{
    HI_S32 s32Ret;
    if ( (argc < 2) || (1 != strlen(argv[1])))
    {
        SAMPLE_VENC_Usage(argv[0]);
        return HI_FAILURE;
    }

    signal(SIGINT, SAMPLE_VENC_HandleSig);
    signal(SIGTERM, SAMPLE_VENC_HandleSig);
    
    switch (*argv[1])
    {
        case '0':/* 4D1 H264 encode */
            s32Ret = SAMPLE_VENC_4D1_H264();
            break;
        case '1':/* 1*720p H264 encode */
            s32Ret = SAMPLE_VENC_1HD_H264();
            break;
        case '2':/* 1D1 MJPEG encode */
            s32Ret = SAMPLE_VENC_1D1_MJPEG();
            break;
        case '3':/* 4D1 JPEG snap */
            s32Ret = SAMPLE_VENC_4D1_Snap();
            break;
        case '4':/* 4D1 JPEG snap */
            s32Ret = SAMPLE_VENC_4D1_SnapEx();
            break;            
        case '5':/* 8*Cif JPEG snap */
            s32Ret = SAMPLE_VENC_8_Cif_Snap();
            break;
        case '6':/* 1D1 User send pictures for H264 encode */
            s32Ret = SAMPLE_VENC_1D1_USER_SEND_PICTURES();
            break;
        case '7':/* 4D1 H264 encode with color2grey */
            s32Ret = SAMPLE_VENC_4D1_H264_COLOR2GREY();
            break;
        default:
            printf("the index is invaild!\n");
            SAMPLE_VENC_Usage(argv[0]);
            return HI_FAILURE;
    }
    
    if (HI_SUCCESS == s32Ret)
        printf("program exit normally!\n");
    else
        printf("program exit abnormally!\n");
    exit(s32Ret);
}
#endif




/******************************************************************************
* funciton : start get venc stream process thread
******************************************************************************/
HI_S32 console_Start(char c_val)
{
	s32 i_ret;
	static pthread_t gt_console_Pid;
	gt_console_para.thread_start = TRUE;
	gt_console_para.thread_param = c_val;
    i_ret = pthread_create(&gt_console_Pid, 0, console_proc, (void*)&gt_console_para);
	while(gt_console_para.thread_start)
	{
		usleep(50000);
	}
	gt_console_para.thread_start = FALSE;
    pthread_join(gt_console_Pid, 0);
	printf(" 线程结束了!\n");
	
	return i_ret;
}


/******************************************************************************
* function    : main()
* Description : video venc sample
******************************************************************************/
int main_test_fifo(int argc, char *argv[])
{
    HI_S32 s32Ret;
	int i=0;
	param_load();
	//nana_proc(argc,argv);
	//return HI_SUCCESS;
    if ( (argc < 2) || (1 != strlen(argv[1])))
    {
        myapp_VENC_Usage(argv[0]);
        return HI_FAILURE;
    }

    signal(SIGINT, myapp_VENC_HandleSig);
    signal(SIGTERM, myapp_VENC_HandleSig);
	
   
	store_creat_fifo();
	console_Start(*argv[2]);
	s32Ret = store_destroy_fifo();


    if (HI_SUCCESS == s32Ret)
        printf("program exit normally!\n");
    else
        printf("program exit abnormally!\n");
	
    exit(s32Ret);
}


/******************************************************************************
* function    : main()
* Description : video venc sample
******************************************************************************/
int main(int argc, char *argv[])
{
    HI_S32 s32Ret;
	int i=0;
	
	param_load();
	//nana_proc(argc,argv);
	//return HI_SUCCESS;
    if ( (argc < 2) || (1 != strlen(argv[1])))
    {
        myapp_VENC_Usage(argv[0]);
        return HI_FAILURE;
    }

    signal(SIGINT, myapp_VENC_HandleSig);
    signal(SIGTERM, myapp_VENC_HandleSig);
	
    switch (*argv[1])
    {
        case '0':/* 4D1 H264 encode */
            s32Ret = myapp_VENC_4D1_H264();
            break;
        case '1':/* 1*720p H264 encode */
            s32Ret = myapp_VENC_1HD_H264();
            break;
        case '2':/* 1D1 MJPEG encode */
            s32Ret = myapp_VENC_1D1_MJPEG();
            break;
        case '3':/* 4D1 JPEG snap */
            s32Ret = myapp_VENC_4D1_Snap();
            break;
        case '4':/* 4D1 JPEG snap */
            s32Ret = myapp_VENC_4D1_SnapEx();
            break;            
        case '5':/* 8*Cif JPEG snap */
            s32Ret = myapp_VENC_8_Cif_Snap();
            break;
        case '6':/* 1D1 User send pictures for H264 encode */
            s32Ret = myapp_VENC_1D1_USER_SEND_PICTURES();
            break;
        case '7':/* 4D1 H264 encode with color2grey */
            s32Ret = myapp_VENC_4D1_H264_COLOR2GREY();
            break;
        case '8':/* 4D1 H264 encode with color2grey */
			my_system2("ls -al /etc /bin");
            break;
        case '9':/* 4D1 H264 encode with color2grey */
			my_system2(argv[2]);
            break;
        case 'a':/* 4D1 H264 encode with color2grey */
			param_proc();
            break;
        case 'b':/* 4D1 H264 encode with color2grey */
			nano_proc(argc,argv);
            break;
        case 'c':/* 4D1 H264 encode with color2grey */
			i = disk_create_dir(argv[2]);
			if(i)
				printf("\n dir:%s is existed \n",argv[2]);
			else 
				printf("\n dir:%s is not existed \n",argv[2]);
            break;
        case 'd':/* 4D1 H264 encode with color2grey */
			disk_remove_dir(argv[2]);
            break;
        case 'e':/* 4D1 H264 encode with color2grey */
			store_init();
			console_Start(*argv[2]);
			store_end();
            break;
        case 'f':/* 4D1 H264 encode with color2grey */
			store_init();
			//console_Start(*argv[2]);
			sleep(10);
			store_end();
            break;
        default:
            printf("the index is invaild!\n");
            myapp_VENC_Usage(argv[0]);
            return HI_FAILURE;
    }
    
    if (HI_SUCCESS == s32Ret)
        printf("program exit normally!\n");
    else
        printf("program exit abnormally!\n");
    exit(s32Ret);
}

/***************************************************************************************************************/
//这是第一个正式程序，开始独立调试过程_START
/***************************************************************************************************************/


/******************************************************************************
* function    : main()
* Description : video venc sample
******************************************************************************/
int main_del_1(int argc, char *argv[])
{
    HI_S32 s32Ret;
   	int i, strt, ch_out;
	int cmd = 4;  
	long long sector = 0;  
    char buf[512];  
    int fd,size,len;
	char *buf_wr="Hello! I'm writing to this file!";
	
    if ( (argc < 3) || (1 != strlen(argv[1])))
    {
    	printf("Please input param1( 0:read ,1:write ) and param2( 0 ~ 10000000 )!\n");
        return HI_FAILURE;
    }

    signal(SIGINT, myapp_VENC_HandleSig);
    signal(SIGTERM, myapp_VENC_HandleSig);
	
    cmd = atoi(argv[1]); 
    sector = atoi(argv[2]);
	printf("sector=%d",sector);
	sector *= 512;
	//getchar();
    /*0：成功，-1：失败*/  
    ////////////////////////////////////////
 
	if(cmd&& (argc<4))
	{
    	printf("Please input param3( write string data. )!\n");
        return HI_FAILURE;
    }
	
	fd = open("/dev/sda2", O_RDWR );
	if(fd < 0)
	{
		printf("open: ERROR!");
		exit(1);
	}
	else
		printf("open file OK: %d\n",fd);
	/*
	while(sector)
	{
		if(sector>0xFFFFFFFF)
		{
			lseek64(fd,0xFFFFFFFF,SEEK_CUR);
			sector -= 0xFFFFFFFF;
		}
		else
		{
			lseek64(fd,sector,SEEK_CUR);
			sector = 0;
		}
	}
	*/
	lseek64(fd,sector,SEEK_SET);
	if(cmd)
	{
		size=write(fd,argv[3],strlen(argv[3]));
		if(size < 0)
		{
			printf("wtrite: ERROR!");
			close(fd);
			exit(1);		
		}
	}
	else
	{
		memset(buf,0,sizeof(buf));
		size=read(fd,buf,128);
		if(size < 0)
		{
			printf("read: ERROR!");
			close(fd);
			exit(1);		
		}
		printf("Read OK\n");  
		printf_hex_data(buf,128);
	    printf("STRING=%s",buf);
	}
	//len = strlen(buf_wr);
	//buf_wr[10] = '\0';
	//if((size = write( fd, buf_wr, len)) < 0)
	//perror("write:");
	//exit(1);
	close(fd);
    ////////////////////////////////////////
    
    if (HI_SUCCESS == s32Ret)
        printf("\nprogram exit normally!\n");
    else
        printf("\nprogram exit abnormally!\n");
    exit(s32Ret);
}


/***************************************************************************************************************/
//这是第一个正式程序，开始独立调试过程_END
/***************************************************************************************************************/
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

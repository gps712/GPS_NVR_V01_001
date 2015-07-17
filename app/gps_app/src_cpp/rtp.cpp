/***************************************************************************************************
--------------------------------深圳会其通兴有限公司 ------开发部---------------------
**文件:
**编写者:孙胜全
**编写日期:2011-08-14
**简要描述:
**修改者:
**修改日期:
****************************************************************************************************/
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "rtpsession.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtp.h"
#ifdef __cplusplus
extern "C" {
#endif
#define PLOAD_TYPE 98
#define DefaultTimestampIncrement 90000/25

static RTPSession sess;
static pthread_mutex_t rtp_mutex_lock;
/***********************************************************************************************************
**函数:hw_create_random
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
static uint32_t  hw_create_random(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);
	return rand();
}
/***********************************************************************************************************
**函数:Rtp_Lock
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
static void Rtp_Lock(void)
{
    static uint8_t init_lock=0;
    if(!init_lock)
    {
        if(pthread_mutex_init(&rtp_mutex_lock, NULL) != 0)
        {
            printf("rtp_mutex_lock Mutex initialization failed!\n");
            return;
        }
        init_lock = 1;
    }
    pthread_mutex_lock(&rtp_mutex_lock);
}
/***********************************************************************************************************
**函数:Rtp_Lock
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
static void Rtp_UnLock(void)
{
    pthread_mutex_unlock(&rtp_mutex_lock);
}
/***********************************************************************************************************
**函数:Rtp_Lock
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
static int  checkerror(int rtperr)
{
    if(rtperr < 0)
    {
        std::cout << "ERROR: " << RTPGetErrorString(rtperr) << std::endl;
        return -1;
    }
    return 0;
}
/***********************************************************************************************************
**函数:Rtp_Lock
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
static int  RtpSetup( uint16_t portbase)
{
    int status;
	
    RTPUDPv4TransmissionParams transparams;
    RTPSessionParams sessparams;
    sessparams.SetOwnTimestampUnit(1.0/90000.0);
    sessparams.SetMaximumPacketSize(1200);
    transparams.SetPortbase(portbase);
    sess.SetDefaultPayloadType(PLOAD_TYPE);
    sess.SetDefaultMark(false);
    sess.SetDefaultTimestampIncrement(DefaultTimestampIncrement);
    status = sess.Create(sessparams,&transparams);
    checkerror(status);
    return status;
}
/***********************************************************************************************************
**函数:Rtp_Lock
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
void SimpleInitRtp(void)
{
    RtpSetup(20000+hw_create_random()&0xffffe);
}
/***********************************************************************************************************
**函数:Rtp_Lock
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
static int RtpUnSetup(void)
{
    sess.BYEDestroy(RTPTime(10,0),0,0);
    return 0;
}
/***********************************************************************************************************
**函数:Rtp_Lock
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
void SimpleUninitRtp(void)
{
    RtpUnSetup();
}
/***********************************************************************************************************
**函数:Rtp_Lock
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
static int SimpleRtpIsActive(void)
{
    int ret=false;
    Rtp_Lock();
    ret=sess.IsActive();
    Rtp_UnLock();
    return ret;
}
/***********************************************************************************************************
**函数:Rtp_Lock
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
int SimpleAddDestination(uint32_t ipaddr, uint16_t destport)
{
    int status;
    if(SimpleRtpIsActive()!=true)
    {
        printf("rtp Session is error,%s ,%d\n",__FUNCTION__, __LINE__);
        return -1;
    }
    Rtp_Lock();
    RTPIPv4Address addr(ipaddr,destport);
    status = sess.AddDestination(addr);
    checkerror(status);
    Rtp_UnLock();
    return status;
}
/***********************************************************************************************************
**函数:SimpleH264SendPacket
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
int SimpleH264SendPacket(unsigned char *val,uint32_t length)
{
    /*
    *val:为264的原始数据，包含0x00 0x00 0x00 0x01信息，
    */
    int status=0;
    uint32_t  TimestampIncrement;
	uint32_t send_length,valid_len=length-4;
	char NALU=val[4],*sendStartAddr=NULL;
	#define  MAX_STREAM_SLICE 1024
    if(SimpleRtpIsActive()!=true)
    {
        printf("rtp Session is error,%s ,%d\n",__FUNCTION__, __LINE__);
        return -1;
    }
    Rtp_Lock();
	TimestampIncrement=sess.GetDefaultTimestampIncrement();
    if(valid_len<=MAX_STREAM_SLICE)
    {
        status = sess.SendPacket((void *)&val[4],valid_len,PLOAD_TYPE,true,DefaultTimestampIncrement);
        checkerror(status);
    }
    else
    {
        int k=0,l=0,len=valid_len,pos=0;
        k=len/MAX_STREAM_SLICE;
        l=len%MAX_STREAM_SLICE;
        int t=0;
        while(t<=k)
        {
            if(t==0)
            {//发送第1包					
            	sendStartAddr=(char *)(val+4);//发送数据的起始地址
				sendStartAddr[pos-1]=(NALU&0x60)|28;//FU indicator
            	sendStartAddr[pos]=(NALU&0x1f)|0x80;//FU header
				send_length=MAX_STREAM_SLICE+1;//要发送数据的长度
                status = sess.SendPacket(sendStartAddr-1,send_length,PLOAD_TYPE,false,DefaultTimestampIncrement);
				checkerror(status);
                if(status<0)
                {
                    goto end;
                }
				pos+=MAX_STREAM_SLICE;
            }
            else if(k==t&&l!=0)
            {//最后一包
            	sendStartAddr[pos-2]=(NALU&0x60)|28;//FU indicator
            	sendStartAddr[pos-1]=(NALU&0x1f)|0x40;//FU header
				send_length=l+2;//要发送数据的长度
                status = sess.SendPacket(sendStartAddr+pos-2,send_length,PLOAD_TYPE,true,0);
                checkerror(status);
                if(status<0)
                {
                    goto end;
                }
				pos+=l;
            }
            else if(t<k&&0!=t)
            {//发送第2包-----倒数第2包
            	if(l==0&&t==k-1)
            	{
					sendStartAddr[pos-2]=(NALU&0x60)|28;//FU indicator
					sendStartAddr[pos-1]=(NALU&0x1f)|0x40;//FU header
				}
				else
				{
					sendStartAddr[pos-2]=(NALU&0x60)|28;//FU indicator
					sendStartAddr[pos-1]=(NALU&0x1f);//FU header
				}
				send_length=MAX_STREAM_SLICE+2;//要发送数据的长度
                status = sess.SendPacket(sendStartAddr+pos-2,send_length,PLOAD_TYPE,false,0);
                checkerror(status);
                if(status<0)
                {
                    goto end;
                }	
				pos+=MAX_STREAM_SLICE;
            }
			t++;
        }
    }
#ifndef RTP_SUPPORT_THREAD
    checkerror(status);
#endif
end:
    Rtp_UnLock();
    return status;
}
/***********************************************************************************************************
**函数:SimpleSetDefaultPayloadType
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
int SimpleSetDefaultPayloadType(uint8_t pt)
{
    if(SimpleRtpIsActive()!=true)
    {
        printf("rtp Session is error,%s ,%d\n",__FUNCTION__, __LINE__);
        return -1;
    }
    Rtp_Lock();
    sess.SetDefaultPayloadType(pt);
    Rtp_UnLock();
    return 0;
}
/***********************************************************************************************************
**函数:SimpleIncrementTimestamp
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
int SimpleIncrementTimestamp(uint32_t inc)
{
    if(SimpleRtpIsActive()!=true)
    {
        printf("rtp Session is error,%s ,%d\n",__FUNCTION__, __LINE__);
        return -1;
    }

    Rtp_Lock();
    sess.IncrementTimestamp(inc);
    Rtp_UnLock();
    return 0;
}
/***********************************************************************************************************
**函数:SimpleIncrementTimestampDefault
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
int SimpleIncrementTimestampDefault()
{
    if(SimpleRtpIsActive()!=true)
    {
        printf("rtp Session  is error,%s ,%d\n",__FUNCTION__, __LINE__);
        return -1;
    }

    Rtp_Lock();
    sess.IncrementTimestampDefault();
    Rtp_UnLock();
    return 0;
}
/***********************************************************************************************************
**函数:Rtp_Lock
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
int SimpleDeleteDestination(uint32_t ipaddr,uint16_t port)
{
    if(SimpleRtpIsActive()!=true)
    {
        printf("rtp Session is error,%s ,%d\n",__FUNCTION__, __LINE__);
        return -1;
    }
    Rtp_Lock();
    sess.DeleteDestination(ipaddr,port);
    Rtp_UnLock();
    return 0;
}
/***********************************************************************************************************
**函数:SimpleClearDestinations
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
int SimpleClearDestinations(void)
{
    if(SimpleRtpIsActive()!=true)
    {
        printf("rtp Session is error,%s ,%d\n",__FUNCTION__, __LINE__);
        return -1;
    }
    Rtp_Lock();
    sess.ClearDestinations();
    Rtp_UnLock();
    return 0;
}

/***********************************************************************************************************
**函数:SimpleGetRtpPoll
**功能:
**输入参数:
**返回值:
***********************************************************************************************************/
int SimpleGetRtpPoll(void)
{
    int status;
    if(SimpleRtpIsActive()!=true)
    {
        return -1;
    }
    status = sess.Poll();
    checkerror(status);
    return status;
}
#ifdef __cplusplus
}
#endif



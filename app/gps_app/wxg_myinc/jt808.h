#ifndef _H_JT808_H_
#define _H_JT808_H_


#include <stdio.h>
#include "list.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if 0
typedef signed   char                   int8_t;      /**<  8bit integer type */
typedef signed   short                  int16_t;    /**< 16bit integer type */
typedef unsigned char                   uint8_t;     /**<  8bit unsigned integer type */
typedef unsigned short                  uint16_t;    /**< 16bit unsigned integer type */
typedef unsigned long                   uint32_t;    /**< 32bit unsigned integer type */
typedef long long		   int64_t;
typedef unsigned long long uint64_t;
#endif

#define PRT_DEBUG
#ifdef PRT_DEBUG

#define JT808_PRT(fmt...)   \
    do {\
        printf("[%s]>>(%d): ", __FILE__, __LINE__);\
        printf(fmt);\
       }while(0)

#else
#define JT808_PRT(fmt...)   \
    do {\
        ;\
       }while(0)
#endif
typedef enum
{
	IDLE = 1,           /*空闲等待发送*/
	WAIT_ACK,           /*等待ACK中*/
	ACK_OK,             /*已收到ACK应答*/
	ACK_TIMEOUT,        /*ACK超时*/
} JT808_MSG_STATE;
#if 1
typedef  struct _jt808_tx_nodedata
{
/*发送机制相关*/
	uint8_t linkno;                                                                                     /*传输使用的link,包括了协议和远端socket*/
	JT808_MSG_STATE state;                 
	uint32_t		retry;                 
	uint32_t		max_retry;             
	uint32_t		timeout;               
	uint32_t		timeout_tick;          
	uint16_t		head_id;                   
	uint16_t		msg_sn;//除去两个7e和校验字的长度                   
	uint16_t		packet_num;                
	uint16_t		packet_current;            
	uint32_t		packet_size;
	uint16_t        msg_len;	
	uint8_t			msg_prio;
	uint8_t			msg_type;
	void			*user_para;
	struct list_head list;
	uint8_t 		tag_data[]; 
	
}JT808_TX_NODEDATA;
#endif
typedef enum
{
	SINGLE_REGISTER	= 0,
	SINGLE_FIRST	= 1,
	SINGLE_CMD		= 2,
	SINGLE_ACK		= 3,
	MULTI_CMD		= 4,
	MULTI_CMD_NEXT	= 5,
	MULTI_ACK		= 6
}JT808_MSG_TYPE;

typedef struct _jt808_msg_retrans
{
	uint16_t	packet_all_num;
	uint16_t	packet_cur_num;
	uint8_t		timeout;
	uint8_t	    rety;
	uint8_t		msg_prio;
	JT808_MSG_TYPE msgtype;
}JT808_MSG_PROPERTY;

typedef struct _jt808_link_num
{
	int mian_ip;
	int auxiliary_ip;
	int iccard_ip;
	int upgrade_ip;
	int back1;
	int back2;	
	
}jt808_link_num;

extern jt808_link_num linkstatus;

int init_list( void);
JT808_MSG_PROPERTY*  set_retrans_param(uint16_t num,uint16_t subnum,uint8_t times,uint8_t again,uint8_t	type);
JT808_TX_NODEDATA *	data_filter(void);
int data_encapsulation( uint8_t linkno, uint8_t* buff, uint32_t count,char *dest );
int list_data_proc(char *pout,JT808_TX_NODEDATA **pstr);
int proc_selector(uint8_t *instr,uint16_t length ,int linkno);
void jt808_rx_proc( char* pinfo, uint16_t len,int linkfd);
uint8_t jt808_tx_register(void);
uint8_t jt808_tx_auth(void);
uint8_t jt808_tx_heart(void);
JT808_TX_NODEDATA * node_begin( int linkno,uint16_t id,int seq,JT808_MSG_PROPERTY **msg_retrans,uint16_t datasize );
JT808_TX_NODEDATA * node_data( JT808_TX_NODEDATA *pnodedata,uint8_t* pinfo, uint16_t len );
int jt808_tx_gpsdata(void);

uint8_t answer_branch_0200(JT808_TX_NODEDATA **pstr,uint8_t res);
uint8_t answer_branch_0102(JT808_TX_NODEDATA **pstr,uint8_t res);

#ifdef __cplusplus
}; //end of extern "C" {
#endif
#endif
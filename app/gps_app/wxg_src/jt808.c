#include <stdio.h> 
#include <stdlib.h>
#include <string.h>


#include "jt808_head.h"


#define	MSG_HEAD_EX	16
#define	MSG_HEAD	12

struct list_head *pos, *next; 


#ifdef __cplusplus
extern "C"
{
#endif	


#if 0
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



typedef struct
{
	uint16_t id;
	int ( *func )( uint8_t linkno, uint8_t *pmsg );
}HANDLE_JT808_RX_MSG;

static uint8_t  special_node =0;/*the node prio tx*/
static uint16_t tx_seq = 0x76; /*Serial number*/
/*Declare the list structure*/

JT808_TX_NODEDATA * pnodedata;
JT808_TX_NODEDATA node_head;
/*Statement retransmission structure*/
//JT808_MSG_RETRANS  retrans_att;
jt808_link_num linkstatus={0,0,0,0,0,0};



static void convert_deviceid( uint8_t* pout, char* psrc);



/* pc Registration response*/
static int handle_rx_0x8100( uint8_t linkno, uint8_t *pmsg )
{
	uint16_t			body_len; /*Messagebody len*/
	uint16_t			ack_seq;
	uint8_t				res;
	uint8_t				* msg;
	JT808_TX_NODEDATA * partnodedata;
	body_len	= ( ( *( pmsg + 2 ) << 8 ) | ( *( pmsg + 3 ) ) ) & 0x3FF;
	msg			= pmsg + 12;
	ack_seq = ( *msg << 8 ) | *( msg + 1 );
	res		= *( msg + 2 );
	if(res == 0)//response sucess
	{
	 list_for_each(pos, &node_head.list)
	 {
	 	 partnodedata = list_entry(pos, JT808_TX_NODEDATA,list);
		 ////Receive the correct answer Id and the serialnumber++
		 if( ( partnodedata->head_id == 0x0100)&&( partnodedata->msg_sn == ack_seq))
		 {
		 	memset(gps_param.id_0xF003,0,sizeof(gps_param.id_0xF003));
			strncpy( gps_param.id_0xF003, (char*)msg + 3, body_len - 3 );
			JT808_PRT("201506191630 %s \n",gps_param.id_0xF003);
			
			write_profile_string("808_PARAM", "0xF003",
									gps_param.id_0xF003, "config808.ini");
			param_proc();
		 	partnodedata->state = ACK_OK;
			tx_seq++; 
			//need add send authentication
			jt808_tx_auth();
			return 0;
			
		 }
	 }
	}
	else
	{
		JT808_PRT("tne response value is %d\n",res);
	}
	return res;
}



/*
   平台通用应答,收到信息，停止发送
 */
static int handle_rx_0x8001( uint8_t linkno, uint8_t *pmsg )
{

	JT808_TX_NODEDATA * node_8001;
	uint16_t	ack_id;
	uint16_t	ack_seq;
	uint8_t		ack_res;
/*跳过消息头12byte*/
	ack_seq = ( *( pmsg + 12 ) << 8 ) | *( pmsg + 13 );
	ack_id	= ( *( pmsg + 14 ) << 8 ) | *( pmsg + 15 );
	ack_res = ( * (pmsg+16));
	/*单条处理*/
	list_for_each(pos, &node_head.list)
	 {
	 	 node_8001 = list_entry(pos, JT808_TX_NODEDATA,list);
		 switch(ack_id)
		 {
		 	case 0x0002:
				node_8001->state = ACK_OK;
				tx_seq++;
			break;
			case 0x0102:
				answer_branch_0102(&node_8001,ack_res);
			break;	
			case 0x0200:
				 answer_branch_0200(&node_8001,ack_res);
			break;	 
			default:
				JT808_PRT("the id %x is no define\n",ack_id);
		 }
		 return 0;
	 }
	return 1;
}


JT808_MSG_PROPERTY* set_retrans_param(uint16_t num,
						uint16_t subnum,
						uint8_t times,
						uint8_t again,
						uint8_t	type)
{
	JT808_MSG_PROPERTY * msg_8100_att;
	msg_8100_att =  malloc( sizeof( JT808_MSG_PROPERTY ) );
	memset(msg_8100_att,0,sizeof( JT808_MSG_PROPERTY ));
	msg_8100_att->packet_all_num = num;
	msg_8100_att->packet_cur_num = subnum;
	msg_8100_att->timeout = times;
	msg_8100_att->rety    = again; 
	msg_8100_att->msgtype = type;
	msg_8100_att->msg_prio =1;//priority TX value: >1 is valid
	//printf("the JT808_MSG_PROPERTY allocate momery %p\n",msg_8100_att);
	return msg_8100_att;
}						

int init_list( void)
{
	INIT_LIST_HEAD(&node_head.list);
	return 0;
}
int jt808_add_tx( int linkno,
                   uint16_t id,
                   int32_t seq,
                   uint16_t len,
				   JT808_MSG_PROPERTY **msg_retrans,
                   uint8_t *pinfo,
                   void  *userpara )

{
	JT808_TX_NODEDATA* pnodedata;
	
	pnodedata = node_begin( linkno, id, seq, msg_retrans, len );
	if( pnodedata == NULL )
	{
		return 0;
	}
	
	node_data( pnodedata, pinfo, len );
	//node_end(pnodedata, msg_retrans, userpara );
	return 1;
}



/*
   malloc node
 */
JT808_TX_NODEDATA * node_begin( int linkno,
                                uint16_t id,
                                int seq,
								JT808_MSG_PROPERTY ** msg_retrans,
                                uint16_t datasize )
{
	JT808_TX_NODEDATA * pnodedata;

	if((*msg_retrans)->msgtype >= MULTI_CMD )
	{
		JT808_PRT("Starts sending Multi-pack information\n");	
		pnodedata = malloc( sizeof( JT808_TX_NODEDATA ) + MSG_HEAD_EX + datasize );
		//printf("the size of momery %d\n",sizeof( JT808_TX_NODEDATA ) +MSG_HEAD_EX+ datasize);
	} 
	else
	{
		pnodedata = malloc( sizeof( JT808_TX_NODEDATA ) +MSG_HEAD+ datasize );
		//printf("the size of momery %d\n",sizeof( JT808_TX_NODEDATA ) +MSG_HEAD+ datasize);
	}
	if( pnodedata == NULL )
	{
		JT808_PRT("malloc failed!\n");
		return NULL;
	}
	
	memset( pnodedata, 0, sizeof( JT808_TX_NODEDATA ) );    //it must, Otherwise error
	pnodedata->linkno	= linkno;
	pnodedata->state	= IDLE;
	pnodedata->head_id	= id;
	pnodedata->retry	= 0;
	pnodedata->max_retry	= (*msg_retrans)->rety;
	pnodedata->timeout		= (*msg_retrans)->timeout;
	pnodedata->packet_num	= (*msg_retrans)->packet_all_num;
	pnodedata->packet_current	= (*msg_retrans)->packet_cur_num;
	pnodedata->msg_len			= datasize;
	pnodedata->msg_prio = (*msg_retrans)->msg_prio;
	pnodedata->msg_type =  (*msg_retrans)->msgtype;
	if( seq == -1 )
	{
		pnodedata->msg_sn = tx_seq;
		//tx_seq++;
	} else
	{
		pnodedata->msg_sn = seq;
	}
	//JT808_PRT("pnodedata->linkno %d = %d\n",pnodedata->linkno,linkno);
	//printf("JT808_MSG_PROPERTY free momery is  %p\n",*msg_retrans);
	free(*msg_retrans);
	return pnodedata;
}


/*
   Filled with valid data
 */
JT808_TX_NODEDATA * node_data( JT808_TX_NODEDATA *pnodedata,
                               uint8_t* pinfo, uint16_t len )
{
	uint8_t * pdata;

	pdata = pnodedata->tag_data;

	pdata[0]	= pnodedata->head_id >> 8;
	pdata[1]	= pnodedata->head_id & 0xff;
	pdata[2]	= ( len >> 8 );
	pdata[3]	= len & 0xff;
	convert_deviceid( pdata + 4,gps_param.id_0xF006);//phone number
	pdata[10]	= pnodedata->msg_sn >> 8;
	pdata[11]	= pnodedata->msg_sn & 0xff;
	if( pnodedata->msg_type >= MULTI_CMD )      /*Multi-packet data*/
	{
		pdata[2] += 0x20;
		pdata[12] = pnodedata->packet_num >>8;
		pdata[13] = pnodedata->packet_num&0xff;
		pdata[14] = pnodedata->packet_current >>8;
		pdata[15] =	pnodedata->packet_current&0xff;
		memcpy( pdata + 16, pinfo, len );   /*filled messagebody data*/
		pnodedata->msg_len = len + 16;
	} 
	else
	{
		memcpy( pdata + 12, pinfo, len );   /*filled messagebody data*/
		pnodedata->msg_len = len + 12;
	}
	//JT808_PRT(":msg_len %d\n",pnodedata->msg_len);

	/*添加到发送列表*/
	list_add_tail(&(pnodedata->list), &(node_head.list));
	return pnodedata;
}


#define DECL_JT808_RX_HANDLE( a ) { a, handle_rx_ ## a }

HANDLE_JT808_RX_MSG handle_rx_msg[] =
{
	DECL_JT808_RX_HANDLE( 0x8001 ), //	通用应答
	//DECL_JT808_RX_HANDLE( 0x8003 ), //	补传分包请求
	DECL_JT808_RX_HANDLE( 0x8100 ) //  监控中心对终端注册消息的应答
	/*
	DECL_JT808_RX_HANDLE( 0x8103 ), //	设置终端参数
	DECL_JT808_RX_HANDLE( 0x8104 ), //	查询终端参数
	DECL_JT808_RX_HANDLE( 0x8105 ), //  终端控制
	DECL_JT808_RX_HANDLE( 0x8106 ), //  查询指定终端参数
	DECL_JT808_RX_HANDLE( 0x8107 ), //  查询终端属性,应答 0x0107
	DECL_JT808_RX_HANDLE( 0x8108 ), //  下发终端升级包
	DECL_JT808_RX_HANDLE( 0x8201 ), //  位置信息查询    位置信息查询消息体为空
	DECL_JT808_RX_HANDLE( 0x8202 ), //  临时位置跟踪控制
	DECL_JT808_RX_HANDLE( 0x8203 ), //  人工确认报警信息
	DECL_JT808_RX_HANDLE( 0x8300 ), //	文本信息下发
	DECL_JT808_RX_HANDLE( 0x8301 ), //	事件设置
	DECL_JT808_RX_HANDLE( 0x8302 ), //  提问下发
	DECL_JT808_RX_HANDLE( 0x8303 ), //	信息点播菜单设置
	DECL_JT808_RX_HANDLE( 0x8304 ), //	信息服务
	DECL_JT808_RX_HANDLE( 0x8400 ), //	电话回拨
	DECL_JT808_RX_HANDLE( 0x8401 ), //	设置电话本
	DECL_JT808_RX_HANDLE( 0x8500 ), //	车辆控制
	DECL_JT808_RX_HANDLE( 0x8600 ), //	设置圆形区域
	DECL_JT808_RX_HANDLE( 0x8601 ), //	删除圆形区域
	DECL_JT808_RX_HANDLE( 0x8602 ), //	设置矩形区域
	DECL_JT808_RX_HANDLE( 0x8603 ), //	删除矩形区域
	DECL_JT808_RX_HANDLE( 0x8604 ), //	多边形区域
	DECL_JT808_RX_HANDLE( 0x8605 ), //	删除多边区域
	DECL_JT808_RX_HANDLE( 0x8606 ), //	设置路线
	DECL_JT808_RX_HANDLE( 0x8607 ), //	删除路线
	DECL_JT808_RX_HANDLE( 0x8700 ), //	行车记录仪数据采集命令
	DECL_JT808_RX_HANDLE( 0x8701 ), //	行驶记录仪参数下传命令
	DECL_JT808_RX_HANDLE( 0x8800 ), //	多媒体数据上传应答
	DECL_JT808_RX_HANDLE( 0x8801 ), //	摄像头立即拍照
	DECL_JT808_RX_HANDLE( 0x8802 ), //	存储多媒体数据检索
	DECL_JT808_RX_HANDLE( 0x8803 ), //	存储多媒体数据上传命令
	DECL_JT808_RX_HANDLE( 0x8804 ), //	录音开始命令
	DECL_JT808_RX_HANDLE( 0x8805 ), //	单条存储多媒体数据检索上传命令 ---- 补充协议要求
	DECL_JT808_RX_HANDLE( 0x8900 ), //	数据下行透传
	DECL_JT808_RX_HANDLE( 0x8A00 ), //	平台RSA公钥
	*/
};

#if 0
/*添加到发送列表**/
void node_end( JT808_TX_NODEDATA* pnodedata,
			   JT808_MSG_RETRANS  msg_retrans,
               void  *userpara )
{
	pnodedata->user_para = userpara;
	list_add_tail(&(pnodedata->list), &(node_head.list));
}
#endif
/**********************************terminal to pc***********************/

JT808_TX_NODEDATA *	data_filter(void)
{
	JT808_TX_NODEDATA* tmpnode;
	tmpnode = NULL;
	list_for_each(pos, &node_head.list)
	{
		tmpnode = list_entry(pos, JT808_TX_NODEDATA, list);
		// printf("201506191503 \n");
		 if((tmpnode->msg_prio==special_node)&&(tmpnode->state=IDLE)&&(special_node!=0))
		 {
		 	tmpnode->state = WAIT_ACK;
			return tmpnode;
		 }
	}
	tmpnode = NULL;
	list_for_each_safe(pos,next, &node_head.list)
	{
		tmpnode = list_entry(pos, JT808_TX_NODEDATA, list);
		//printf("%s>>(%d): %d %d %d\n", __FUNCTION__,__LINE__,tmpnode->timeout,
		//					tmpnode->state,tmpnode->timeout_tick);
	next_send:	
		if(tmpnode->state == IDLE)
		{/*
			tmpnode->state = WAIT_ACK;
			if(tmpnode->timeout>0)
			{
				tmpnode->timeout_tick = now_time();
			}
			*/
			return tmpnode;
		}
		if(tmpnode->state == WAIT_ACK)
		{
			//printf("201506191511 找到WAIT_ACK\n");
			if(tmpnode->timeout > 0)
			{
				if(interval_time(tmpnode->timeout,tmpnode->timeout_tick)>=0)
				{
					tmpnode->retry++;
					if(tmpnode->retry > tmpnode->max_retry)
					{
						list_del_init(pos);
						free(tmpnode);
						JT808_PRT("\nchong chuan ci shu chao xian ,shan chu jie dian..\n");
						return NULL;
					}
					else
					{
						JT808_PRT("zhe shi di %d ci fa song \n",tmpnode->retry);
						tmpnode->state = IDLE;
						goto next_send;
					}
				}
				else
				{
					tmpnode = NULL;	
				}
			}
			else
			{
				tmpnode =NULL;
			}
		}
	}

	return tmpnode;
}


int data_encapsulation( uint8_t linkno, uint8_t* buff, uint32_t count,char *dest )
{
	uint32_t   len = count;
	uint8_t		*p	= (uint8_t*)buff;
	uint8_t		c;
	uint16_t	num;
	int	ret;

	uint8_t		fcs = 0;
	num = 0;
	bzero(dest,sizeof(dest));
	dest[num++] =0x7e;
	while( len )
	{
		c	= *p++;
		fcs ^= c; /*get fcs*/
		if( c == 0x7E )
		{
			dest[num++] = 0x7d;
			dest[num++] = 0x02;
		}else if( c == 0x7D )
		{
			dest[num++] = 0x7d;
			dest[num++] = 0x01;
		}else
		{
			dest[num++] = c;	
		}
		len--;
	}
/*if fcs ==ox7e encode it base on 808*/
	if( fcs == 0x7E )
	{
		dest[num++] = 0x7d;
		dest[num++] = 0x02;
	}
	else if( fcs == 0x7D )
	{
		dest[num++] = 0x7d;
		dest[num++] = 0x01;
	}
	else
	{
		dest[num++] = fcs;	
	}
	dest[num++] =0x7e;
	return num;
}

int list_data_proc(char *pout,JT808_TX_NODEDATA ** pstr)
{
	uint16_t len=0;
	JT808_TX_NODEDATA * pnodedata;
	//process the "state=ack_ok"'s node -delet it
	list_for_each_safe(pos, next, &node_head.list)
	{
		pnodedata = list_entry(pos, JT808_TX_NODEDATA,list);
		if(pnodedata->state == ACK_OK)
		{
			list_del_init(pos); 
			free(pnodedata);
			
		}
	}
	
	*pstr= data_filter();
	if(*pstr!=NULL)
	{
		//pstr =pnodedata;
		len = data_encapsulation((*pstr)->linkno,(*pstr)->tag_data,
							(*pstr)->msg_len,pout);
//		JT808_PRT(": len = %d\n",len);
		
	}
	return len;
}
/**********************pc to terminal**********************************/
int proc_selector(uint8_t *instr,uint16_t length ,int linkno)
{
	uint16_t id,i;
	id = (instr[1]<<8)|(instr[2]);
	//JT808_PRT("201506191557 %x \n",id);
	for( i = 0; i < sizeof( handle_rx_msg ) / sizeof( HANDLE_JT808_RX_MSG ); i++ )
	{
		if( id == handle_rx_msg [i].id )
		{
			handle_rx_msg [i].func( linkno, instr+1 );
			id = 0;
		}
	}
	return 0;
}

void jt808_rx_proc( char* pinfo, uint16_t len,int linkfd)
{
	uint8_t c,fcs;
	uint8_t buf[1024] = {0};
	uint16_t buf_wr =0,i=0; 
	char *pstr= NULL;
	pstr = pinfo;
again:
	if(*pstr==0x7e)
	{
		while(len)
		{		
			if((*pstr==0x7d)&&(*(pstr+1)==0x02))
			{
				buf[buf_wr++] = 0x7e;
				pstr++;
			    len--;
			}
			else if((*pstr==0x7d)&&(*(pstr+1)==0x01))
			{
				buf[buf_wr++] = 0x7d;
				pstr++;
			    len--;
			}
			else
			{
				buf[buf_wr++] = *pstr;
			}
			
			//process the data between 0x7e,buf contain 0x7e
			if((*pstr==0x7e)&&(buf_wr >2))
			{
				//减去3是到了校验的字节的前一个字节
				 //outprint_hex("test",buf, buf_wr );
				fcs =0;
				for(i=1;i<=buf_wr-3;i++)
				{
					c=buf[i];
					//printf(" %x",c);
					fcs^=c;
				}
				JT808_PRT(" jiao yan %x %x\n",fcs ,buf[buf_wr-2]);
				//verify bit
				if(fcs==buf[buf_wr-2])
				{
					proc_selector(buf,buf_wr,linkfd);
					bzero(buf,1024);
					buf_wr =0;
					pstr++;
					len--;
					goto again;
					
				}
			 }
			pstr++;
			len--;
		}
	}
}

static void convert_deviceid( uint8_t* pout, char* psrc)
{
	uint8_t *pdst = pout;
	*pdst++ = ( ( psrc[0] - 0x30 ) << 4 ) | ( psrc[1] - 0x30 );
	*pdst++ = ( ( psrc[2] - 0x30 ) << 4 ) | ( psrc[3] - 0x30 );
	*pdst++ = ( ( psrc[4] - 0x30 ) << 4 ) | ( psrc[5] - 0x30 );
	*pdst++ = ( ( psrc[6] - 0x30 ) << 4 ) | ( psrc[7] - 0x30 );
	*pdst++ = ( ( psrc[8] - 0x30 ) << 4 ) | ( psrc[9] - 0x30 );
	*pdst	= ( ( psrc[10] - 0x30 ) << 4 )| ( psrc[11] - 0x30 );
}


/*********************************************************************************
  *函数名称:uint8_t jt808_tx_register(void)
  *功能描述:发送注册消息到上位机
  *输	入:none
  *输	出:none
  *返 回 值:uint8_t:0表示失败，1表示可以正常填充发送包，但并不保证能发送成功
  *作	者:白养民
  *创建日期:2013-06-13
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint8_t jt808_tx_register(void)
{
	uint8_t buf[64];
	uint8_t len;
	uint8_t ret;
	JT808_MSG_PROPERTY *msg_att;
	memset(buf,0,sizeof(buf));
	buf[0]	= gps_param.id_0x0081 >> 8;               /*省域*/
	buf[1]	= gps_param.id_0x0081 & 0xff;
	buf[2]	= gps_param.id_0x0082 >> 8;               /*市域*/
	buf[3]	= gps_param.id_0x0082 & 0xff;
	memcpy( buf + 4, gps_param.id_0xF000, 5 );        /*制造商ID*/
	memcpy( buf + 9, gps_param.id_0xF001, 20 );       /*终端型号*/
	memcpy(buf + 29,gps_param.id_0xF006+5,7);		///手机号码后7位
	buf[36] = gps_param.id_0x0084;
	if(buf[36])
		{
		strcpy( (char*)buf + 37, gps_param.id_0x0083 );   /*车辆号牌*/
		len = strlen(gps_param.id_0x0083) + 37;
		}
	else
		{
		strcpy( (char*)buf + 37, gps_param.id_0xF005 );   /*车辆VIN*/
		len = strlen(gps_param.id_0xF005) + 37;
		}
		msg_att =set_retrans_param(0,0,5,3,SINGLE_REGISTER);
	ret=jt808_add_tx( linkstatus.mian_ip,
	              0x0100,
	              -1,
	              len, &msg_att,
	              buf, NULL );
	return ret;
}


uint8_t jt808_tx_auth(void)
{
	uint8_t ret;
	JT808_MSG_PROPERTY *msg_att_0102;
	msg_att_0102 = set_retrans_param(0,0,5,3,SINGLE_REGISTER);
	//JT808_PRT("linkstatus.mian_ip =%d\n",linkstatus.mian_ip);
	ret=jt808_add_tx( linkstatus.mian_ip,
	              0x0102,
	              -1, 
	              strlen( gps_param.id_0xF003 ),
	              &msg_att_0102,
	              (uint8_t*)( gps_param.id_0xF003 ),NULL );
	return ret;
}

uint8_t jt808_tx_heart(void)
{
	JT808_MSG_PROPERTY *msg_att_0002;
	msg_att_0002 = set_retrans_param(0,0,0,0,SINGLE_REGISTER);
	jt808_add_tx( linkstatus.mian_ip, 0x0002, -1,0,&msg_att_0002,NULL,NULL);
}

int jt808_tx_gpsdata(void)
{
	uint8_t buf[128];
	uint8_t len,num;
	uint8_t ret;
	JT808_MSG_PROPERTY *msg_gps;
	gps_info_save *gpstmp;
	gpstmp = malloc(sizeof(gps_info_save));
	memset(gpstmp,0,sizeof(gps_info_save));
	JT808_PRT("data_contrl.read_offset = %d\n",data_contrl.read_offset);
	gps_read("gps.log",gpstmp,sizeof(gps_info_save),1,&data_contrl.read_offset);
	outprint_hex("readdata",gpstmp,sizeof(gps_info_save));
	bzero(buf,sizeof(buf));
	num = 0;
	memcpy( buf+num, &(gpstmp->data.alarm),4 );
	num =num+4;
	memcpy( buf+num, &(gpstmp->data.status),4 );
	num =num+4;
	memcpy( buf+num, &(gpstmp->data.latitude),4 );
	num =num+4;
	memcpy( buf+num, &(gpstmp->data.longitude),4 );
	num =num+4;
	memcpy( buf+num, &(gpstmp->data.altitude),2 );
	num =num+2;
	memcpy( buf+num, &(gpstmp->data.speed_10x),2 );
	num =num+2;
	memcpy( buf+num, &(gpstmp->data.cog), 2);
	num =num+2;
	memcpy( buf+num, gpstmp->data.datetime,6 );
	num =num+6;
	//li cheng
	buf[num++] = 0x01;
	buf[num++] = 0x04;
	buf[num++] = 0;
	buf[num++] = 0;
	buf[num++] = 0;
	buf[num++] = 0;
	//行车记录仪
	buf[num++] = 0x03;
	buf[num++] = 0x02;
	buf[num++] = 0;
	buf[num++] = 0;
	
	buf[num++] = 0x30;
	buf[num++] = 0x01;
	buf[num++] = 16;
	buf[num++] = 0x31;
	buf[num++] = 0x1;
	buf[num++] = gpstmp->data.NoSV;
	free(gpstmp);
	msg_gps =set_retrans_param(0,0,6,3,SINGLE_REGISTER);
	ret=jt808_add_tx( linkstatus.mian_ip,
	              0x0200,
	              -1,
	              num, &msg_gps,
	              buf, NULL );
	return ret;
}

uint8_t answer_branch_0102(JT808_TX_NODEDATA **pstr,uint8_t res)
{
	if((0==res)&&((*pstr)->state==WAIT_ACK))
	{
		(*pstr)->state = ACK_OK;
		tx_seq++;
		JT808_PRT("Authentication is sucess\n");
		//start the timer,send the heart bit;
		if(jt808_timer()==DF_SUCCESS)
		{
			JT808_PRT("creat timer sucess ..\n");
		}
	}
	else
	{
		JT808_PRT("the result is %d\n",res);
	}
	return 0;	
}

uint8_t answer_branch_0200(JT808_TX_NODEDATA **pstr,uint8_t res)
{
	if((0==res)&&((*pstr)->state==WAIT_ACK))
	{
		(*pstr)->state = ACK_OK;
		tx_seq++;
		gps_update("gps.log",&data_contrl.read_offset);
		JT808_PRT("gps data is sucess %d\n",data_contrl.read_offset);
	}
	else
	{
		JT808_PRT("the result is %d %x %x\n",res,(*pstr)->head_id,(*pstr)->msg_sn);
	}
	return 0;	
}




#ifdef __cplusplus
}; //end of extern "C" {
#endif
/*发送信息是做一个优先级的检测，优先级高的可以先发，做一个遍历，在接受解析的时候先删除接受的链表节点依据是id和流水号，这时候流水号才可以自加*/




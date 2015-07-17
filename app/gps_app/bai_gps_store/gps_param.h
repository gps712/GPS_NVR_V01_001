/*********************************************************************************
  * @文件名称 :gps_disk_io.c
  * @功能描述 :所有参数相关的函数都在这里，参数的读取，修改，等
  * @作	   者 :白养民
  * @创建日期 :2015-6-08
  * @文件版本 :0.01
  *---------------------------------------------------------------------------------
  * @修改人		修改时间   	修改内容
  * 白养民		2015-06-08	创建
*********************************************************************************/
	
#include "gps_typedef.h"


#ifndef _H_JT808_PARAM_
#define _H_JT808_PARAM_


#define TYPE_BYTE	1                /*固定为1字节,小端对齐*/
#define TYPE_WORD	2                /*固定为2字节,小端对齐*/
#define TYPE_DWORD	4                /*固定为4字节,小端对齐*/
#define TYPE_CAN	8                /*固定为8字节,当前存储CAN_ID参数*/
#define TYPE_STR	32               /*固定为32字节,网络顺序*/
#define TYPE_L(n)	n


#define CENTER_SERVICE_IP1		"60.28.50.210"
#define CENTER_SERVICE_PORT1	9131
#define CENTER_SERVICE_IP2		"60.28.50.210"
#define CENTER_SERVICE_PORT2	9131
#define PROVINCE_ID				12						///省域ID
#define CITY_ID					100						///市域ID


#define BYTESWAP2( val )    \
    ( ( ( ( val ) & 0xff ) << 8 ) |   \
      ( ( ( val ) & 0xff00 ) >> 8 ) )

#define BYTESWAP4( val )    \
    ( ( ( ( val ) & 0xff ) << 24 ) |   \
      ( ( ( val ) & 0xff00 ) << 8 ) |  \
      ( ( ( val ) & 0xff0000 ) >> 8 ) |  \
      ( ( ( val ) & 0xff000000 ) >> 24 ) )

////////////////////////////////////////////////////////////////数据类型定义开始


typedef struct _jt808_param
{
	char		id_0x0000[16];   /*0x0000 版本*/
	uint32_t	id_0x0001;      /*0x0001 心跳发送间隔*/
	uint32_t	id_0x0002;      /*0x0002 TCP应答超时时间*/
	uint32_t	id_0x0003;      /*0x0003 TCP超时重传次数*/
	uint32_t	id_0x0004;      /*0x0004 UDP应答超时时间*/
	uint32_t	id_0x0005;      /*0x0005 UDP超时重传次数*/
	uint32_t	id_0x0006;      /*0x0006 SMS消息应答超时时间*/
	uint32_t	id_0x0007;      /*0x0007 SMS消息重传次数*/
	char		id_0x0010[32];  /*0x0010 主服务器APN*/
	char		id_0x0011[32];  /*0x0011 用户名*/
	char		id_0x0012[32];  /*0x0012 密码*/
	char		id_0x0013[64];  /*0x0013 主服务器地址*/
	char		id_0x0014[32];  /*0x0014 备份APN*/
	char		id_0x0015[32];  /*0x0015 备份用户名*/
	char		id_0x0016[32];  /*0x0016 备份密码*/
	char		id_0x0017[64];  /*0x0017 备份服务器地址，ip或域名*/
	uint32_t	id_0x0018;      /*0x0018 TCP端口*/
	uint32_t	id_0x0019;      /*0x0019 UDP端口*/
	char		id_0x001A[32];  /*0x001A ic卡主服务器地址，ip或域名*/
	uint32_t	id_0x001B;      /*0x001B ic卡服务器TCP端口*/
	uint32_t	id_0x001C;      /*0x001C ic卡服务器UDP端口*/
	char		id_0x001D[32];  /*0x001D ic卡备份服务器地址，ip或域名*/
	uint32_t	id_0x0020;      /*0x0020 位置汇报策略*/
	uint32_t	id_0x0021;      /*0x0021 位置汇报方案*/
	uint32_t	id_0x0022;      /*0x0022 驾驶员未登录汇报时间间隔*/
	uint32_t	id_0x0027;      /*0x0027 休眠时汇报时间间隔*/
	uint32_t	id_0x0028;      /*0x0028 紧急报警时汇报时间间隔*/
	uint32_t	id_0x0029;      /*0x0029 缺省时间汇报间隔*/
	uint32_t	id_0x002C;      /*0x002c 缺省距离汇报间隔*/
	uint32_t	id_0x002D;      /*0x002d 驾驶员未登录汇报距离间隔*/
	uint32_t	id_0x002E;      /*0x002e 休眠时距离汇报间隔*/
	uint32_t	id_0x002F;      /*0x002f 紧急时距离汇报间隔*/
	uint32_t	id_0x0030;      /*0x0030 拐点补传角度*/
	uint32_t	id_0x0031;      /*0x0031 电子围栏半径（非法位移阈值），单位为米*/
	char		id_0x0040[32];  /*0x0040 监控平台电话号码*/
	char		id_0x0041[32];  /*0x0041 复位电话号码*/
	char		id_0x0042[32];  /*0x0042 恢复出厂设置电话号码*/
	char		id_0x0043[32];  /*0x0043 监控平台SMS号码*/
	char		id_0x0044[32];  /*0x0044 接收终端SMS文本报警号码*/
	uint32_t	id_0x0045;      /*0x0045 终端接听电话策略*/
	uint32_t	id_0x0046;      /*0x0046 每次通话时长*/
	uint32_t	id_0x0047;      /*0x0047 当月通话时长*/
	char		id_0x0048[32];  /*0x0048 监听电话号码*/
	char		id_0x0049[32];  /*0x0049 监管平台特权短信号码*/
	uint32_t	id_0x0050;      /*0x0050 报警屏蔽字*/
	uint32_t	id_0x0051;      /*0x0051 报警发送文本SMS开关*/
	uint32_t	id_0x0052;      /*0x0052 报警拍照开关*/
	uint32_t	id_0x0053;      /*0x0053 报警拍摄存储标志*/
	uint32_t	id_0x0054;      /*0x0054 关键标志*/
	uint32_t	id_0x0055;      /*0x0055 最高速度kmh*/
	uint32_t	id_0x0056;      /*0x0056 超速持续时间*/
	uint32_t	id_0x0057;      /*0x0057 连续驾驶时间门限*/
	uint32_t	id_0x0058;      /*0x0058 当天累计驾驶时间门限*/
	uint32_t	id_0x0059;      /*0x0059 最小休息时间*/
	uint32_t	id_0x005A;      /*0x005A 最长停车时间*/
	uint32_t	id_0x005B;      /*0x005B 超速报警预警差值，单位为 1/10Km/h */
	uint32_t	id_0x005C;      /*0x005C 疲劳驾驶预警差值，单位为秒（s），>0*/
	uint32_t	id_0x005D;      /*0x005D 碰撞报警参数设置:b7..0：碰撞时间(4ms) b15..8：碰撞加速度(0.1g) 0-79 之间，默认为10 */
	uint32_t	id_0x005E;      /*0x005E 侧翻报警参数设置： 侧翻角度，单位 1 度，默认为 30 度*/
	uint32_t	id_0x0064;      /*0x0064 定时拍照控制*/
	uint32_t	id_0x0065;      /*0x0065 定距拍照控制*/
	uint32_t	id_0x0070;      /*0x0070 图像视频质量(1-10)*/
	uint32_t	id_0x0071;      /*0x0071 亮度*/
	uint32_t	id_0x0072;      /*0x0072 对比度*/
	uint32_t	id_0x0073;      /*0x0073 饱和度*/
	uint32_t	id_0x0074;      /*0x0074 色度*/
	uint32_t	id_0x0080;      /*0x0080 车辆里程表读数0.1km*/
	uint32_t	id_0x0081;      /*0x0081 省域ID*/
	uint32_t	id_0x0082;      /*0x0082 市域ID*/
	char		id_0x0083[32];  /*0x0083 机动车号牌*/
	uint32_t	id_0x0084;      /*0x0084 车牌颜色  0 未上牌 1蓝色 2黄色 3黑色 4白色 9其他*/
	uint32_t	id_0x0090;      /*0x0090 GNSS 定位模式*/
	uint32_t	id_0x0091;      /*0x0091 GNSS 波特率*/
	uint32_t	id_0x0092;      /*0x0092 GNSS 模块详细定位数据输出频率*/
	uint32_t	id_0x0093;      /*0x0093 GNSS 模块详细定位数据采集频率*/
	uint32_t	id_0x0094;      /*0x0094 GNSS 模块详细定位数据上传方式*/
	uint32_t	id_0x0095;      /*0x0095 GNSS 模块详细定位数据上传设置*/
	uint32_t	id_0x0100;      /*0x0100 CAN 总线通道 1 采集时间间隔(ms)，0 表示不采集*/
	uint32_t	id_0x0101;      /*0x0101 CAN 总线通道 1 上传时间间隔(s)，0 表示不上传*/
	uint32_t	id_0x0102;      /*0x0102 CAN 总线通道 2 采集时间间隔(ms)，0 表示不采集*/
	uint32_t	id_0x0103;      /*0x0103 CAN 总线通道 2 上传时间间隔(s)，0 表示不上传*/
	uint8_t		id_0x0110[8];   /*0x0110 CAN 总线 ID 单独采集设置*/
	uint8_t		id_0x0111[8];   /*0x0111 其他CAN 总线 ID 单独采集设置*/
	uint8_t		id_0x0112[8];   /*0x0112 其他CAN 总线 ID 单独采集设置*/
	uint8_t		id_0x0113[8];   /*0x0113 其他CAN 总线 ID 单独采集设置*/
	uint8_t		id_0x0114[8];   /*0x0114 其他CAN 总线 ID 单独采集设置*/
	uint8_t		id_0x0115[8];   /*0x0115 其他CAN 总线 ID 单独采集设置*/
	uint8_t		id_0x0116[8];   /*0x0116 其他CAN 总线 ID 单独采集设置*/
	uint8_t		id_0x0117[8];   /*0x0117 其他CAN 总线 ID 单独采集设置*/
	uint8_t		id_0x0118[8];   /*0x0118 其他CAN 总线 ID 单独采集设置*/
	uint8_t		id_0x0119[8];   /*0x0119 其他CAN 总线 ID 单独采集设置*/
	
	/*除了8103默认参数以外的数据*/
	char		id_0xF000[32];	/*0xF000 制造商ID 5byte*/
	char		id_0xF001[32];	/*0xF001 终端型号 20byte*/
	char		id_0xF002[32];	/*0xF002 终端ID 7byte*/
	char		id_0xF003[32];	/*0xF003 鉴权码*/
	uint16_t	id_0xF004;		/*0xF004 终端类型*/
	char		id_0xF005[32];	/*0xF005 VIN*/
	char		id_0xF006[32];	/*0xF006 CARID 上报的终端手机号，系统原来的mobile */
}GPS_PARAM;


typedef struct _st_vdr_param
{
	char		id_0x0000[16];  /*0x0000 版本*/
	uint32_t	id_0x0001;      /*0x0001 VDR 参数1*/
	uint32_t	id_0x0002;      /*0x0001 VDR 参数2*/
	uint32_t	id_0x0003;      /*0x0001 VDR 参数3*/
	uint32_t	id_0x0004;      /*0x0001 VDR 参数4*/
	uint32_t	id_0x0005;      /*0x0001 VDR 参数5*/
	char		id_0x0006[32];   /*0x0001 VDR 参数6*/
}st_vdr_param;


extern GPS_PARAM 		gps_param;
extern st_vdr_param	gt_vdr_param;


void param_read(void);
void param_out_ex(char * section, char * id );
void param_out(char * id);
int param_set_ex(char * section, char * id,char * value);
int param_set(char * id,char * value);
int param_save_ex(char * section);
int param_save(void);
int param_load(void);
int param_proc(void);


#endif

#ifndef _JT808_GPS_H_
#define _JT808_GPS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define BIT_STATUS_ACC		0x00000001	///0：ACC 关；1： ACC 开
#define BIT_STATUS_FIXED	0x00000002	///0：未定位；1：定位
#define BIT_STATUS_NS		0x00000004	///0：北纬；1：南纬
#define BIT_STATUS_EW		0x00000008  ///0：东经；1：西经
#define BIT_STATUS_SERVICE	0x00000010  ///0：运营状态；1：停运状态
#define BIT_STATUS_ENCRYPT	0x00000020	///0：经纬度未经保密插件加密；1：经纬度已经保密插件加密

#define BIT_STATUS_EMPTY	0x00000100	///
#define BIT_STATUS_FULL		0x00000200

#define BIT_STATUS_OIL		0x00000400  ///0：车辆油路正常；1：车辆油路断开
#define BIT_STATUS_ELEC		0x00000800  ///0：车辆电路正常；1：车辆电路断开
#define BIT_STATUS_DOORLOCK 0x00001000  ///0：车门解锁；1：车门加锁
#define BIT_STATUS_DOOR1	0x00002000	///0：门 1 关；1：门 1 开（前门）
#define BIT_STATUS_DOOR2	0x00004000	///0：门 2 关；1：门 2 开（中门）
#define BIT_STATUS_DOOR3	0x00008000	///0：门 3 关；1：门 3 开（后门）
#define BIT_STATUS_DOOR4	0x00010000	///0：门 4 关；1：门 4 开（驾驶席门）
#define BIT_STATUS_DOOR5	0x00020000	///0：门 5 关；1：门 5 开（自定义）
#define BIT_STATUS_GPS		0x00040000	///0：未使用 GPS 卫星进行定位；1：使用 GPS 卫星进行定位
#define BIT_STATUS_BD		0x00080000	///0：未使用北斗卫星进行定位；1：使用北斗卫星进行定位
#define BIT_STATUS_GLONASS	0x00100000	///0：未使用 GLONASS 卫星进行定位；1：使用 GLONASS 卫星进行定位
#define BIT_STATUS_GALILEO	0x00200000	///0：未使用 Galileo 卫星进行定位；1：使用 Galileo 卫星进行定位

#define BIT_ALARM_EMG				0x00000001  
#define BIT_ALARM_OVERSPEED			0x00000002  
#define BIT_ALARM_OVERTIME			0x00000004  
#define BIT_ALARM_DANGER			0x00000008  
#define BIT_ALARM_GPS_ERR			0x00000010  
#define BIT_ALARM_GPS_OPEN			0x00000020  
#define BIT_ALARM_GPS_SHORT			0x00000040  
#define BIT_ALARM_LOW_PWR			0x00000080  
#define BIT_ALARM_LOST_PWR			0x00000100  
#define BIT_ALARM_FAULT_LCD			0x00000200  
#define BIT_ALARM_FAULT_TTS			0x00000400  
#define BIT_ALARM_FAULT_CAM			0x00000800  
#define BIT_ALARM_FAULT_ICCARD		0x00001000  
#define BIT_ALARM_PRE_OVERSPEED		0x00002000  
#define BIT_ALARM_PRE_OVERTIME		0x00004000  
#define BIT_ALARM_TODAY_OVERTIME	0x00040000  
#define BIT_ALARM_STOP_OVERTIME		0x00080000  
#define BIT_ALARM_DEVIATE			0x00800000  
#define BIT_ALARM_VSS				0x01000000  
#define BIT_ALARM_OIL				0x02000000  
#define BIT_ALARM_STOLEN			0x04000000  
#define BIT_ALARM_IGNITION			0x08000000  
#define BIT_ALARM_MOVE				0x10000000  
#define BIT_ALARM_COLLIDE			0x20000000  
#define BIT_ALARM_TILT				0x40000000  
#define BIT_ALARM_DOOR_OPEN			0x80000000  

#if 0
typedef struct _gps_baseinfo
{
	uint32_t	alarm;
	uint32_t	status;
	uint32_t	latitude;                       /*纬度*/
	uint32_t	longitude;                      /*精度*/
	uint16_t	altitude;
	uint16_t	speed_10x;                      /*对地速度 0.1KMH*/
	uint16_t	cog;                            /*对地角度*/
	uint8_t		datetime[6];                    /*BCD格式*/
	uint8_t		mileage_id;                     /*附加消息_里程ID*/
	uint8_t		mileage_len;                    /*附加消息_里程长度*/
	uint32_t	mileage;                   		/*附加消息_里程*/
	uint8_t		csq_id;                     	/*附加消息_无线通信网ID*/
	uint8_t		csq_len;                    	/*附加消息_无线通信网长度*/
	uint8_t		csq;                   			/*附加消息_无线通信网信号强度*/
	uint8_t		NoSV_id;                     	/*附加消息_定位卫星数量ID*/
	uint8_t		NoSV_len;                    	/*附加消息_定位卫星数量长度*/
	uint8_t		NoSV;                   		/*附加消息_定位卫星数量*/
}gps_baseinfo;
#endif

typedef struct _gps_baseinfo
{
	uint32_t	alarm;
	uint32_t	status;
	uint32_t	latitude;                       /*纬度*/
	uint32_t	longitude;                      /*精度*/
	uint16_t	altitude;
	uint16_t	speed_10x;                      /*对地速度 0.1KMH*/
	uint16_t	cog;                            /*对地角度*/
	uint8_t		datetime[6];                    /*BCD格式*/
	uint8_t		mileage_id;                     /*附加消息_里程ID*/
	uint8_t		mileage_len;                    /*附加消息_里程长度*/
	uint32_t	mileage;                   		/*附加消息_里程*/
	uint8_t     oil_id;
	uint8_t		oil_len;
	uint16_t	oil;
	uint8_t		pulse_id;
	uint8_t		pulse_len;
	uint16_t	pulse;
	uint8_t		manalarm_id;
	uint8_t		manalarm_len;
	uint16_t	manalarm;
	uint8_t		exspeed_id;
	uint8_t		exspeed_len;
	uint8_t		exspeed[5];
	uint8_t		region_id;
	uint8_t		region_len;
	uint8_t		region[6];
	uint8_t		road_id;
	uint8_t		road_len;
	uint8_t		road[7];
	uint8_t		carsignal_id;                    
	uint8_t		carsignal_len;                    
	uint32_t	carsignal;
	uint8_t     io_id;
	uint8_t		io_len;
	uint16_t	io;
	uint8_t		analog_id;                    
	uint8_t		analog_len;                    
	uint32_t	analog;
	uint8_t		csq_id;                     	/*附加消息_无线通信网ID*/
	uint8_t		csq_len;                    	/*附加消息_无线通信网长度*/
	uint8_t		csq;                   			/*附加消息_无线通信网信号强度*/
	uint8_t		NoSV_id;                     	/*附加消息_定位卫星数量ID*/
	uint8_t		NoSV_len;                    	/*附加消息_定位卫星数量长度*/
	uint8_t		NoSV;                   		/*附加消息_定位卫星数量*/
	uint8_t     data[];
}gps_baseinfo;

typedef struct _gps_info_save
{
	uint32_t	gpsnum;
	uint8_t		flag;
	uint8_t		status;
	gps_baseinfo data;
}gps_info_save;


typedef struct _gps_contrl_
{
	int read_offset;
	int write_offset;
	int backflag;
}gps_contrl;


int gps_data_filled(gps_baseinfo *pstr);
int gps_thread_join(void);
int gps_thread(void);
int gps_write(uint8_t *pstr);
int jt808_tx_gpsdata(void);

#ifdef __cplusplus
}; //end of extern "C" {
#endif
#endif

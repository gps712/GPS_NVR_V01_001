#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gps_typedef.h"



const unsigned char	tbl_hex_to_assic[] = "0123456789ABCDEF"; 	// 0x0-0xf的字符查找表
const unsigned char tbl_assic_to_hex[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };


/*********************************************************************************
*函数名称:unsigned int Hex_To_Ascii( unsigned char* pDst, const unsigned char* pSrc, unsigned int nSrcLength )
*功能描述:将hex数据转换为在内存中存储的16进制ASSIC码字符串
*输    入:	pDst	:存储转换ASSIC码字符串的结果
			pSrc	:原始数据
			nSrcLength	:pSrc长度
*输    出:unsigned int 型数据，表示转换的ASSIC码pDst的长度
*返 回 值:	
*作    者:白养民
*创建日期:2013-2-19
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
unsigned int Hex_To_Ascii( unsigned char* pDst, const unsigned char* pSrc, unsigned int nSrcLength )
{
	unsigned int			i;

	for( i = 0; i < nSrcLength; i++ )
	{
		// 输出低4位
		*pDst++ = tbl_hex_to_assic[*pSrc >> 4];

		// 输出高4位
		*pDst++ = tbl_hex_to_assic[*pSrc & 0x0f];

		pSrc++;
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标字符串长度
	return ( nSrcLength << 1 );
}


/*********************************************************************************
*函数名称:unsigned int Ascii_To_Hex(unsigned char *dest_buf,char * src_buf,unsigned int max_rx_len)
*功能描述:将16进制ASSIC码字符串转换为在内存中存储的hex数据
*输    入:	dest_buf:存储转换的结果
			src_buf	:ASSIC码字符串
			max_rx_len		:src_dest最大可以接收的长度
*输    出:unsigned int 型数据，表示转换的src_dest的长度
*返 回 值:	
*作    者:白养民
*创建日期:2013-2-19
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
unsigned int Ascii_To_Hex(unsigned char *dest_buf,char * src_buf,unsigned int max_rx_len)
{
 char		c;
 char 		*p;
 unsigned int i,infolen;

 if((unsigned long)dest_buf == 0)
 	return 0;
 infolen = strlen(src_buf)/2;
 p = src_buf;
 for( i = 0; i < infolen; i++ )
 {
 	c		= tbl_assic_to_hex[*p++ - '0'] << 4;
 	c		|= tbl_assic_to_hex[*p++ - '0'];
 	dest_buf[i] = c;
	if(i>=max_rx_len)
		break;
 }
 return i;
}





/*********************************************************************************
*函数名称:unsigned long AssicBufToUL(char * buf,unsigned int num)
*功能描述:将10进制ASSIC码字符串或指定最大长度为num的字符串转为unsigned long型数据
*输    入:	* buf	:ASSIC码字符串
			num		:字符串的最大长度
*输    出:unsigned long 型数据
*返 回 值:	
*作    者:白养民
*创建日期:2013-2-19
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
unsigned long AssicBufToUL(char * buf,unsigned int num)
{
 unsigned char tempChar;
 unsigned int i;
 unsigned long retLong=0;
 
 for(i=0;i<num;i++)
 	{
 	tempChar=(unsigned char)buf[i];
	if((tempChar>='0')&&(tempChar<='9'))
		{
	 	retLong*=10;
		retLong+=tempChar-'0';
		}
	else
		{
		return retLong;
		}
 	}
 return retLong;
}

/*********************************************************************************
*函数名称:void printf_hex_data( const u8* pSrc, u16 nSrcLength )
*功能描述:将hex数据转换为在内存中存储的16进制ASSIC码字符串然后打印出来
*输    入:	pSrc	:原始数据
			nSrcLength	:pSrc长度
*输    出:none
*返 回 值:	
*作    者:白养民
*创建日期:2013-2-19
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void printf_hex_data( const unsigned char* pSrc, unsigned int nSrcLength )
{
 	char 			pDst[3];
	unsigned int	i;

	
	pDst[2]  = 0;
	for( i = 0; i < nSrcLength; i++ )
	{
		// 输出低4位
		pDst[0] = tbl_hex_to_assic[*pSrc >> 4];

		// 输出高4位
		pDst[1] = tbl_hex_to_assic[*pSrc & 0x0f];

		pSrc++;

		printf(pDst);
	}
}



/*********************************************************************************
  *函数名称:unsigned char sample_month_day(unsigned char uc_month,unsigned char uc_leapyear)
  *功能描述:该函数功能为计算当月的天数
  *输	入: uc_month	:要进行计算的月份
  			uc_leapyear	:是否为闰年，润健为1，非闰年为0
  *输	出: none
  *返 回 值:计算出来的天数。
  *---------------------------------------------------------------------------------
  *修改人    修改时间   修改内容
  *白养民   2014-06-08  创建函数
*********************************************************************************/
unsigned char Get_Month_Day(unsigned char month,unsigned char leapyear)
{
	unsigned char day;
	switch(month)
	{
		case 12 :
		{
	 		day=31;
			break;
		}
		case 11 :
		{
			day=30;
			break;
		}
		case 10 :
		{
			day=31;
			break;
		}
		case 9 :
		{
			day=30;
			break;
		}
		case 8 :
		{
			day=31;
			break;
		}
		case 7 :
		{
			day=31;
			break;
		}
		case 6 :
		{
			day=30;
			break;
		}
		case 5 :
		{
			day=31;
			break;
		}
		case 4 :
		{
			day=30;
			break;
		}
		case 3 :
		{
			day=31;
			break;
		}
		case 2 :
		{
			day=28;
			day+=leapyear;	
			break;
		}
		case 1 :
		{
			day=31;
			break;
		}
		default :
		{
			break;
		}
	}
	return day;
}



/*********************************************************************************
  *函数名称:void strtrim( unsigned char* s, unsigned char c )
  *功能描述:该函数功能为去掉字符串中的删除键(0x08)，并将后面的有效字符移动到前面。
  *输	入: s		:要修改的字符串，注意，该字符串必须能被修改
  *输	出: s		:修改完成的字符串
  *返 回 值:none
  *---------------------------------------------------------------------------------
  *修改人    修改时间   修改内容
  *白养民   2015-06-18  创建函数
*********************************************************************************/
void strproc( unsigned char* s )
{
	int 	i, j, len;

	if ( s == 0 )
	{
		return;
	}
	if ( *s == 0 )
	{
		return;
	}
	
	len = strlen( (const char*)s );
	for ( i = 0,j=0; i < len; i++ )
	{
		if ( s[i] == 0x08 )
		{
			if(j)
				j--;
		}
		else
		{
			s[j++] = s[i];
		}
		
	}
	s[j] = 0;
}



/*********************************************************************************
  *函数名称:void strtrim( unsigned char* s, unsigned char c )
  *功能描述:该函数功能为去掉字符串s中前后为c的字符，如果C为0则表示删除前后的不可见字符
  *输	入: s		:要修改的字符串，注意，该字符串必须能被修改
  			c		:单个字符
  *输	出: s		:修改完成的字符串
  *返 回 值:none
  *---------------------------------------------------------------------------------
  *修改人    修改时间   修改内容
  *白养民   2015-06-18  创建函数
*********************************************************************************/
void strtrim( unsigned char* s, unsigned char c )
{
	unsigned char	i, j, * p1, * p2;

	if ( s == 0 )
	{
		return;
	}

	// delete the trailing characters
	if ( *s == 0 )
	{
		return;
	}
	
	j = strlen( (const char*)s );
	p1 = s + j;
	for ( i = 0; i < j; i++ )
	{
		p1--;
		if( c == 0 )
		{
			if ( *p1 > 0x20 )
			{
				break;
			}
		}
		else
		{
			if ( *p1 != c )
			{
				break;
			}
		}
	}
	if ( i < j )
	{
		p1++;
	}
	*p1 = 0;	// null terminate the undesired trailing characters

	// delete the leading characters
	p1 = s;
	if ( *p1 == 0 )
	{
		return;
	}
	if( c == 0 )
	{
		for ( i = 0; *p1++ <= 0x20; i++ )
		{
			;
		}
	}
	else
	{
		for ( i = 0; *p1++ == c; i++ )
		{
			;
		}
	}
	if ( i > 0 )
	{
		p2 = s;
		p1--;
		for ( ; *p1 != 0; )
		{
			*p2++ = *p1++;
		}
		*p2 = 0;
	}
}



/*********************************************************************************
  *函数名称:void strtrim( unsigned char* s, unsigned char c )
  *功能描述:该函数功能为获取系统tick值，单位为10ms，从系统开机开始一直累加
  *输	入: none
  *输	出: none
  *返 回 值:系统tick值
  *---------------------------------------------------------------------------------
  *修改人    修改时间   修改内容
  *白养民   2015-06-18  创建函数
*********************************************************************************/
unsigned long tick_get( void )
{
	return 0;
}




/*********************************************************************************
  *函数名称:uint16_t data_to_buf( uint8_t * pdest, uint32_t data, uint8_t width )
  *功能描述:将不同类型的数据存入buf中，数据在buf中为大端模式
  *输	入:	pdest:  存放数据的buffer
   data:	存放数据的原始数据
   width:	存放的原始数据占用的buf字节数
  *输	出:
  *返 回 值:存入的字节数
  *作	者:白养民
  *创建日期:2013-06-5
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint16_t data_to_buf( uint8_t * pdest, uint32_t data, uint8_t width )
{
	uint8_t *buf;
	buf = pdest;

	switch( width )
	{
		case 1:
			*buf++ = data & 0xff;
			break;
		case 2:
			*buf++	= data >> 8;
			*buf++	= data & 0xff;
			break;
		case 4:
			*buf++	= data >> 24;
			*buf++	= data >> 16;
			*buf++	= data >> 8;
			*buf++	= data & 0xff;
			break;
	}
	return width;
}

/*********************************************************************************
  *函数名称:uint16_t buf_to_data( uint8_t * psrc, uint8_t width )
  *功能描述:将不同类型的数据从buf中取出来，数据在buf中为大端模式
  *输	入:	psrc:   存放数据的buffer
   width:	存放的原始数据占用的buf字节数
  *输	出:	 none
  *返 回 值:uint32_t 返回存储的数据
  *作	者:白养民
  *创建日期:2013-06-5
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint32_t buf_to_data( uint8_t * psrc, uint8_t width )
{
	uint8_t		i;
	uint32_t	outData = 0;

	for( i = 0; i < width; i++ )
	{
		outData <<= 8;
		outData += *psrc++;
	}
	return outData;
}

uint8_t HEX2BCD( uint8_t x )
{
return ( ( ( x ) / 10 ) << 4 | ( ( x ) % 10 ) );
}


uint8_t BCD2HEX( uint8_t x )
{
return ( ( ( ( x ) >> 4 ) * 10 ) + ( ( x ) & 0x0f ) );
}


/************************************************************
 * @file
 * @brief: app use it call the shell command 
 * @cmd  :shell command
 * @author wxg
 * @date 2015-06-12
 * @version 0.1
 * @return if this thread sucess ,retun $?
 */

int my_system(const char * cmd) 
{
#if 0
	FILE * fp;
	char *result_buf;
	extern int errno;
	int res,result;
	result_buf =NULL;
	//char buf[1024]; 
	if (cmd == NULL) 
	{ 
		JT808_PRT("my_system cmd is NULL!\n");
	 	return -1;
	} 
	if ((fp = popen(cmd, "r") ) == NULL) 
	{ 
		perror("popen");
	 	JT808_PRT("popen error: %s/n", strerror(errno)); 
		return -1; 
	} 
	else
	{
		JT808_PRT("命令【%s】\r\n", cmd);
		result_buf = (char*)malloc( sizeof(char)*4096 );
		if(result_buf ==NULL)
		{
			return 0;
		}
	    while(fgets(result_buf, sizeof(result_buf), fp) != NULL)
	    {
	        printf("%s", result_buf);
	    }
	}
	if(result_buf !=NULL)
	{
		free(result_buf);
	}
	if ( (res = pclose(fp)) == -1) 
	{ 
		JT808_PRT("close popen file pointer fp error!\n"); 
		return res;
	}  
	else 
	{ 
		result =WEXITSTATUS(res); 
		JT808_PRT("popen res is :%d %d\n", res,result); 
		return result; 
	} 
#endif
}


void outprint_hex(uint8_t * descrip, char *instr, uint16_t inlen )
{
	uint32_t  i=0;
	uint8_t *THstr = NULL;
	THstr=(uint8_t*)malloc(sizeof(uint8_t)*inlen+1);
	memcpy(THstr,instr,inlen);
	printf("\n%s: (%d)>>%x\n",descrip,inlen,now_time());
	for( i=0;i<inlen;i++)
	{
	printf("%02X ",THstr[i]);
	if((i+1)%16==0)
	{
	printf("\n");
	}
	}

	printf("\n");
	free(THstr);
}


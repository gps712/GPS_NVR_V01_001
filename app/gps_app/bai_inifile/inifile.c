/*********************************************************************************
  * @文件名称 :inifile.c
  * @功能描述 :ini参数读取，写入相关接口函数
  * @作	   者 :白养民
  * @创建日期 :2015-6-8
  * @文件版本 :0.01
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "inifile.h"

#ifdef __cplusplus
extern "C"
{
#endif



/*********************************************************************************
  *函数名称:int load_ini_file(const char *file, char *buf,int *file_size)
  *功能描述:读取配置文件到指定的buf中并返回文件长度
  *输	入: file	:读取的文件名称目录
  			buf		:文件buf
  			file_size:读取的文件长度
  *输	出: buf,file_size
  *返 回 值:int		:1表示读取失败，0表示读取成功
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/
int load_ini_file(const char *file, char *buf,int *file_size)
{
	FILE *in = NULL;
	int i=0;
	*file_size =0;

	assert(file !=NULL);
	assert(buf !=NULL);

	in = fopen(file,"r");
	if( NULL == in) {
		return 1;
	}

	buf[i]=fgetc(in);
	
	//load initialization file
	while( buf[i]!= (char)EOF) 
	{
		i++;
		assert( i < MAX_FILE_SIZE ); //file too big, you can redefine MAX_FILE_SIZE to fit the big file 
		buf[i]=fgetc(in);
	}
	
	buf[i]='\0';
	*file_size = i;

	fclose(in);
	return 0;
}

/*************************************************************************************
  *函数名称:static int newline(char c)
  *功能描述:是否回车或换行，是则返回1
  *输	入: c	:当前字符
  *输	出: none
  *返 回 值:int	:0表示非回车或换行，1表示回车或换行
  *---------------------------------------------------------------------------------
  *修改人    修改时间   修改内容
  *白养民   2015-06-08  创建函数
*************************************************************************************/
static int newline(char c)
{
	return ('\n' == c ||  '\r' == c )? 1 : 0;
}


/*********************************************************************************
  *函数名称:static int end_of_string(char c)
  *功能描述:是否字符串结尾
  *输	入: c	:当前字符
  *输	出: none
  *返 回 值:int	:0表示非结尾，1表示结尾
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/
static int end_of_string(char c)
{
	return '\0'==c? 1 : 0;
}


/*********************************************************************************
  *函数名称:static int left_barce(char c)
  *功能描述:是否INI文件section的左中括号'['
  *输	入: c	:当前字符
  *输	出: none
  *返 回 值:int	:0表示不是，1表示是
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/
static int left_barce(char c)
{
	return LEFT_BRACE == c? 1 : 0;
}

/*********************************************************************************
  *函数名称:static int isright_brace(char c)
  *功能描述:是否INI文件section的右中括号']'
  *输	入: c	:当前字符
  *输	出: none
  *返 回 值:int	:0表示不是，1表示是
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/
static int isright_brace(char c )
{
	return RIGHT_BRACE == c? 1 : 0;
}



/*********************************************************************************
  *函数名称:static int parse_file(const char *section, const char *key, const char *buf,int *sec_s,int *sec_e,
					  int *key_s,int *key_e, int *value_s, int *value_e)
  *功能描述:在buf(读取到的INI文件)中查找指定的section，key，val值对应的偏移位置
  *输	入: section	:INI文件中参数的section
  			key		:INI文件中参数的key(名称)
  			buf		:INI文件信息
  			*sec_s,*sec_e	:sectionde 的开始位置和结束位置，没有找到均为(-1)
  			*key_s,*key_e	:key 的开始位置和结束位置，没有找到均为(-1)
  			*value_s,*value_e	:val 的开始位置和结束位置，没有找到均为(-1)
  *输	出: *sec_s,*sec_e,*key_s,*key_e,*value_s,*value_e
  *返 回 值:int		:1表示没有找到key和val，0表示找到了
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/
static int parse_file(const char *section, const char *key, const char *buf,int *sec_s,int *sec_e,
					  int *key_s,int *key_e, int *value_s, int *value_e)
{
	const char *p = buf;
	int i=0;

	assert(buf!=NULL);
	assert(section != NULL && strlen(section));
	assert(key != NULL && strlen(key));

	*sec_e = *sec_s = *key_e = *key_s = *value_s = *value_e = -1;

	while( !end_of_string(p[i]) ) 
	{
		//find the section
		if( ( 0==i ||  newline(p[i-1]) ) && left_barce(p[i]) )
		{
			int section_start=i+1;

			//find the ']'
			do 
			{
				i++;
			} while( !isright_brace(p[i]) && !end_of_string(p[i]));

			if( 0 == strncmp(p+section_start,section, i-section_start)) 
			{
				int newline_start=0;

				i++;

				//Skip over space char after ']'
				while(isspace(p[i])) 
				{
					i++;
				}

				//find the section
				*sec_s = section_start;
				*sec_e = i;

				while( ! (newline(p[i-1]) && left_barce(p[i])) && !end_of_string(p[i]) ) 
				{
					int j=0;
					//get a new line
					newline_start = i;

					while( !newline(p[i]) &&  !end_of_string(p[i]) ) 
					{
						i++;
					}
					
					//now i  is equal to end of the line
					j = newline_start;

					if(';' != p[j]) //skip over comment
					{
						while(j < i && p[j]!='=') 
						{
							j++;
							if('=' == p[j]) 
							{
								if(strncmp(key,p+newline_start,j-newline_start)==0)
								{
									//find the key ok
									*key_s = newline_start;
									*key_e = j-1;

									*value_s = j+1;
									*value_e = i;

									return 0;
								}
							}
						}
					}
					i++;
				}
			}
		}
		else
		{
			i++;
		}
	}
	return 1;
}

/*********************************************************************************
  *函数名称:int read_profile_string( const char *section, const char *key,char *value, 
		 		int size, const char *default_value, const char *file)
  *功能描述:读取INI文件中指定的section，key，的value值，如过不存在该参数，则返回默认值default_value
  			该函数会读取一次INI文件。
  *输	入: *@param section [in] name of the section containing the key name
			*@param key [in] name of the key pairs to value 
			*@param value [in/out] pointer to the buffer that receives the retrieved string
			*@param size [in] size of result's buffer 
			*@param default_value [in] default value of result
			*@param file [in] path of the initialization file
  *输	出: *value
  *返 回 值:int 	:0 : read success; 1 : read fail
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/
int read_profile_string( const char *section, const char *key,char *value, 
		 int size, const char *default_value, const char *file)
{
	char buf[MAX_FILE_SIZE]={0};
	int file_size;
	int sec_s,sec_e,key_s,key_e, value_s, value_e;

	//check parameters
	assert(section != NULL && strlen(section));
	assert(key != NULL && strlen(key));
	assert(value != NULL);
	assert(size > 0);
	assert(file !=NULL &&strlen(key));

	if(load_ini_file(file,buf,&file_size))
	{
		if(default_value!=NULL)
		{
			strncpy(value,default_value, size);
		}
		return 1;
	}
	value[0] = '\0';
	if(parse_file(section,key,buf,&sec_s,&sec_e,&key_s,&key_e,&value_s,&value_e))
	{
		if(default_value!=NULL)
		{
			strncpy(value,default_value, size);
		}
		return 1; //not find the key
	}
	else
	{
		int cpcount = value_e -value_s;

		if( size-1 < cpcount)
		{
			cpcount =  size-1;
		}
	
		memset(value, 0, size);
		memcpy(value,buf+value_s, cpcount );
		value[cpcount] = '\0';

		return 0;
	}
}

/*********************************************************************************
  *函数名称:int read_profile_int( const char *section, const char *key,int default_value, 
				const char *file)
  *功能描述:读取ini文件中int类型的数据，如果读取失败，则将默认(default_value)数值返回
  			该函数会读取一次INI文件。
  *输	入: *@param section [in] name of the section containing the key name
			*@param key [in] name of the key pairs to value 
			*@param default_value [in] default value of result
			*@param file [in] path of the initialization file
  *输	出: none
  *返 回 值:int 	:读取到的数据，如果读取失败，则该值为default_value
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/
int read_profile_int( const char *section, const char *key,int default_value, 
				const char *file)
{
	char value[32] = {0};
	if(read_profile_string(section,key,value, sizeof(value),NULL,file))
	{
		return default_value;
	}
	else
	{
		return atoi(value);
	}
}

/*********************************************************************************
  *函数名称:int write_profile_string(const char *section, const char *key,
					const char *value, const char *file)
  *功能描述:写入指定参数的value到ini文件中，如果该参数不存在则创建并写入。
  			该函数会读取一次INI文件并写入一次。
  *输	入:  * @param section [in] name of the section,can't be NULL and empty string
			 * @param key [in] name of the key pairs to value, can't be NULL and empty string
			 * @param value [in] profile string value
			 * @param file [in] path of ini file
  *输	出: none
  *返 回 值:int :	0: success	 1: failure
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/
int write_profile_string(const char *section, const char *key,
					const char *value, const char *file)
{
	char buf[MAX_FILE_SIZE]={0};
	char w_buf[MAX_FILE_SIZE]={0};
	int sec_s,sec_e,key_s,key_e, value_s, value_e;
	int value_len = (int)strlen(value);
	int file_size;
	FILE *out;

	//check parameters
	assert(section != NULL && strlen(section));
	assert(key != NULL && strlen(key));
	assert(value != NULL);
	assert(file !=NULL &&strlen(key));

	if(load_ini_file(file,buf,&file_size))
	{
		sec_s = -1;
	}
	else
	{
		parse_file(section,key,buf,&sec_s,&sec_e,&key_s,&key_e,&value_s,&value_e);
	}

	if( -1 == sec_s)
	{
		if(0==file_size)
		{
			sprintf(w_buf+file_size,"[%s]\n%s=%s\n",section,key,value);
		}
		else
		{
			//not find the section, then add the new section at end of the file
			memcpy(w_buf,buf,file_size);
			sprintf(w_buf+file_size,"\n[%s]\n%s=%s\n",section,key,value);
		}
	}
	else if(-1 == key_s)
	{
		//not find the key, then add the new key=value at end of the section
		memcpy(w_buf,buf,sec_e);
		sprintf(w_buf+sec_e,"%s=%s\n",key,value);
		memcpy(w_buf+sec_e+strlen(key)+strlen(value)+2,buf+sec_e, file_size - sec_e);
	}
	else
	{
		//update value with new value
		memcpy(w_buf,buf,value_s);
		memcpy(w_buf+value_s,value, value_len);
		memcpy(w_buf+value_s+value_len, buf+value_e, file_size - value_e);
	}
	
	out = fopen(file,"w");
	if(NULL == out)
	{
		return 1;
	}
	
	if(-1 == fputs(w_buf,out) )
	{
		fclose(out);
		return 1;
	}

	fclose(out);
	return 0;
}


/*********************************************************************************
  *函数名称:int read_profile_string_buf( const char *section, const char *key,char *value, 
		 		int size, const char *default_value, char *buf)
  *功能描述:读取INI文件中指定的section，key，的value值，如过不存在该参数，则返回默认值default_value
  			该函数会读取一次INI文件。
  *输	入: *@param section [in] name of the section containing the key name
			*@param key [in] name of the key pairs to value 
			*@param value [in/out] pointer to the buffer that receives the retrieved string
			*@param size [in] size of result's buffer 
			*@param default_value [in] default value of result
			*@param buf [in] buf of the initialization file
  *输	出: *value
  *返 回 值:int 	:0 : read success; 1 : read fail
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/
int read_profile_string_buf( const char *section, const char *key,char *value, 
		 int size, const char *default_value, char *buf)
{
	//char buf[MAX_FILE_SIZE]={0};
	//int file_size;
	int sec_s,sec_e,key_s,key_e, value_s, value_e;

	//check parameters
	assert(section != NULL && strlen(section));
	assert(key != NULL && strlen(key));
	assert(value != NULL);
	assert(size > 0);
	assert(buf != NULL);

	value[0] = '\0';
	if(parse_file(section,key,buf,&sec_s,&sec_e,&key_s,&key_e,&value_s,&value_e))
	{
		if(default_value!=NULL)
		{
			strncpy(value,default_value, size);
		}
		return 1; //not find the key
	}
	else
	{
		int cpcount = value_e -value_s;

		if( size-1 < cpcount)
		{
			cpcount =  size-1;
		}
	
		memset(value, 0, size);
		memcpy(value,buf+value_s, cpcount );
		value[cpcount] = '\0';

		return 0;
	}
}

/*********************************************************************************
  *函数名称:int write_profile_string_buf(const char *section, const char *key,
					const char *value, char *buf)
  *功能描述:写入指定参数的value到ini文件buf中，如果该参数不存在则创建并写入。
  			该函数不读写ini文件，只修改文件buf，如果想要将更新存储则需要调用函数save_ini_file()。
  *输	入:  * @param section [in] name of the section,can't be NULL and empty string
			 * @param key [in] name of the key pairs to value, can't be NULL and empty string
			 * @param value [in] profile string value
			 * @param buf [in] buf of ini file
  *输	出: none
  *返 回 值:int :	0: success	 1: failure
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/
int write_profile_string_buf(const char *section, const char *key,
					const char *value, char *buf)
{
	//char buf[MAX_FILE_SIZE]={0};
	char w_buf[MAX_FILE_SIZE]={0};
	int sec_s,sec_e,key_s,key_e, value_s, value_e;
	int value_len = (int)strlen(value);
	int file_size = strlen(buf);
	FILE *out;

	//check parameters
	assert(section != NULL && strlen(section));
	assert(key != NULL && strlen(key));
	assert(value != NULL);
	assert(buf !=NULL);

	if(file_size == 0)
	{
		sec_s = -1;
	}
	else
	{
		parse_file(section,key,buf,&sec_s,&sec_e,&key_s,&key_e,&value_s,&value_e);
	}
	memset(w_buf,0,sizeof(w_buf));
	if( -1 == sec_s)
	{
		if(0==file_size)
		{
			sprintf(w_buf+file_size,"[%s]\n%s=%s\n",section,key,value);
		}
		else
		{
			//not find the section, then add the new section at end of the file
			memcpy(w_buf,buf,file_size);
			sprintf(w_buf+file_size,"\n[%s]\n%s=%s\n",section,key,value);
		}
	}
	else if(-1 == key_s)
	{
		//not find the key, then add the new key=value at end of the section
		memcpy(w_buf,buf,sec_e);
		sprintf(w_buf+sec_e,"%s=%s\n",key,value);
		memcpy(w_buf+sec_e+strlen(key)+strlen(value)+2,buf+sec_e, file_size - sec_e);
	}
	else
	{
		//update value with new value
		memcpy(w_buf,buf,value_s);
		memcpy(w_buf+value_s,value, value_len);
		memcpy(w_buf+value_s+value_len, buf+value_e, file_size - value_e);
	}
	sprintf(buf,w_buf);
	/*
	out = fopen(file,"w");
	if(NULL == out)
	{
		return 1;
	}
	
	if(-1 == fputs(w_buf,out) )
	{
		fclose(out);
		return 1;
	}

	fclose(out);
	*/
	return 0;
}



/*********************************************************************************
  *函数名称:int read_profile_string_buf( const char *section, const char *key,char *value, 
		 		int size, const char *default_value, char *buf)
  *功能描述:读取INI文件中指定的section，key，的value值，如过不存在该参数，则返回默认值default_value
  			该函数会读取一次INI文件。
  *输	入: *@param section [in] name of the section containing the key name
			*@param key [in] name of the key pairs to value 
			*@param value [in/out] pointer to the buffer that receives the retrieved string
			*@param size [in] size of result's buffer 
			*@param default_value [in] default value of result
			*@param buf [in] buf of the initialization file
  *输	出: *value
  *返 回 值:int 	:0 : read success; 1 : read fail,then write default value
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/
int read_profile_write_buf( const char *section, const char *key,char *value, 
		 int size, const char *default_value, char *buf)
{
	//char buf[MAX_FILE_SIZE]={0};
	//int file_size;
	int sec_s,sec_e,key_s,key_e, value_s, value_e;

	//check parameters
	assert(section != NULL && strlen(section));
	assert(key != NULL && strlen(key));
	assert(value != NULL);
	assert(size > 0);
	assert(default_value != NULL);
	assert(buf != NULL);

	value[0] = '\0';
	if(parse_file(section,key,buf,&sec_s,&sec_e,&key_s,&key_e,&value_s,&value_e))
	{
		write_profile_string_buf(section,key,default_value,buf);
		strncpy(value,default_value, size);
		return 1; //not find the key,write it.
	}
	else
	{
		int cpcount = value_e -value_s;

		if( size-1 < cpcount)
		{
			cpcount =  size-1;
		}
	
		memset(value, 0, size);
		memcpy(value,buf+value_s, cpcount );
		value[cpcount] = '\0';

		return 0;
	}
}

/*********************************************************************************
  *函数名称:int save_ini_file( const char *file, const char *buf)
  *功能描述:将buf中的数据写入ini文件中
  *输	入:  * @param file [in] path of ini file
  			 * @param buf [in] buf of ini file
  *输	出: none
  *返 回 值:int :	1: success	 0: failure
  *---------------------------------------------------------------------------------
  * @修改人    修改时间   修改内容
  *  白养民   2015-06-08  创建函数
*********************************************************************************/
int save_ini_file( const char *file, const char *buf)
{
	FILE *out;
	out = fopen(file,"w");
	if(NULL == out)
	{
		return 1;
	}
	
	if(-1 == fputs(buf,out) )
	{
		fclose(out);
		return 1;
	}

	fclose(out);
	return 0;
}

#ifdef __cplusplus
}; //end of extern "C" {
#endif

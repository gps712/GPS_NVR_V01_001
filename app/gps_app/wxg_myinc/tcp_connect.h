#ifndef _TCP_CONNECT_H_
#define _TCP_CONNECT_H_


#define  DF_TURE		1
#define  DF_FALSE      (-1)
#define  DF_SUCCESS    0

#ifdef __cplusplus
extern "C"
{

#endif

typedef struct _GET_DATA
{
	int linkfd;
	char *str;
	int length;
}GET_DATA;

typedef struct _THREAD_STATUS
{
	char rx;
	char tx;
}THREAD_STATUS;



extern GET_DATA	 rxdata;

int socket_rx_data(void);
int socket_tx_data(void);
int socket_rx_data_contl(char input);
int socket_tx_data_contl(char input);
int logsave(char* instr);
int tcp_connect(const char *host, const char *serv);
ssize_t	 writen(int fd, const void *vptr, size_t n);
ssize_t readn(int fd, void *vptr, size_t n);



#ifdef __cplusplus
}; //end of extern "C" {
#endif

#endif
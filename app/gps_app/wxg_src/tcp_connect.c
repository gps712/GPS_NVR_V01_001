/* include tcp_connect */
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h> 
#include <netdb.h> 
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
/*use the ioctl function*/
#include <net/if_arp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <sys/ioctl.h>




#include "jt808_head.h"

 

#define	MAX_CACHE_POOL 2048





GET_DATA	 rxdata;
THREAD_STATUS thread_statu;



// the max link number is 10

#ifdef __cplusplus
extern "C"
{
#endif

static pthread_t sock_tx_pid;
static pthread_t sock_rx_pid;





int logsave(char* instr)
{
 openlog("TCB_log", LOG_CONS | LOG_PID, 0);
 syslog(LOG_USER | LOG_INFO, "TCB_712... %s \n", instr);
 closelog();
 return 0;
}


int tcp_connect(const char *host, const char *serv)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;
	//struct sockaddr_in     *server;
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		//err_quit
		JT808_PRT("tcp_connect error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	ressave = res;
	
	//server = (struct sockaddr_in*)rse->ai_addr;
	//server->sin_port = htons(SERVER_PORT);
	//socklen_t server_addr_len = sizeof(*server);
	do {
		JT808_PRT("jian li socket ....\n");
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
			continue;	/* ignore this one */

		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;		/* success */

		close(sockfd);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);
	JT808_PRT("jian li connect end ...\n");
	if (res == NULL)	/* errno set from final connect() */
	{
		JT808_PRT("tcp_connect error for %s, %s", host, serv);
		sockfd =0;
	}
	freeaddrinfo(ressave);
	return(sockfd);
}
/************************************************************
 * @file
 * @brief: close the link which you opened;
 * @closefd :you open the link's fd
 * @author wxg
 * @date 2015-06-12
 * @version 0.1
 * @return if this thread sucess ,retun 0
 */

int tcp_close(int closefd)
{
	close(closefd);
	return 0;
}

//************************************向服务器写的函数*****************************************/ 

ssize_t	 writen(int fd, const void *vptr, size_t n )
{
	uint16_t		nleft;
	uint16_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	
	while (nleft > 0) 
	{
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) 
		{
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}
		
		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
/**********************************读取服务器发回来的函数*********************************/
ssize_t readn(int fd, void *vptr, size_t n)
{
	size_t	nleft;
	ssize_t	nread;
	char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) 
	{
		if ( (nread = read(fd, ptr, nleft)) < 0) 
		{
			if (errno == EINTR)
				nread = 0;		/* and call read() again */
			else
				return(-1);
		} 
		else if (nread == 0)
		{
			JT808_PRT("mei you shu ju ke yi du le ...\n");
			break;				/* EOF */
		}
		nleft -= nread;
		ptr   += nread;
		JT808_PRT("the nread value=%d \n",nread);
	}
	return(n - nleft);		/* return >= 0 */
}
static char txbuf[MAX_CACHE_POOL]={0};
/************************************************************
 * @file
 * @brief: tx data to pc 
 * @author wxg
 * @date 2015-06-12
 * @version 0.1
 */

static void *data_tx(void *arg)
{
	int n_bytes =0;
	int real_len =0;
	JT808_TX_NODEDATA *tmpstr;
	tmpstr =NULL;
	while(thread_statu.tx)
	{
		
		real_len = list_data_proc(txbuf,&tmpstr);
		if(real_len!=0)
		{
			outprint_hex("msgsend",txbuf,real_len);
			sys_time();
		again:
			JT808_PRT("linkstatus %d\n",linkstatus.mian_ip);
			n_bytes = writen(tmpstr->linkno,txbuf, real_len);
			if(n_bytes==-1)
			{
				//logsave("tx data failed...\n");
				JT808_PRT("tx data failed...\n");
				sleep(2);//碰到中断干扰，3s后再次写入数据
				goto again;

			}
			//JT808_PRT("run here %d %d\n",n_bytes,real_len);
			if(n_bytes == real_len)
			{
				//;
				tmpstr->state = WAIT_ACK;
				tmpstr->timeout_tick =now_time();
				rxdata.linkfd =tmpstr->linkno;
				JT808_PRT("tcp data tx sucess...%d (%d)>>%d\n",n_bytes,tmpstr->state,tmpstr->timeout_tick);
				bzero(txbuf,sizeof(txbuf));
				
			}

		}
	
	usleep(5000);
	}
	 return NULL;
	
}



char rxbuf[MAX_CACHE_POOL];
/************************************************************
 * @file
 * @brief: rx data which from pc 
 * @author wxg
 * @date 2015-06-12
 * @version 0.1
 * 
 */

static void *data_rx(void *arg)
{
	struct timeval  timeout;
	fd_set rfd;
	int n_bytes =0;
	//int rx_num =0;
	int err =-1;
	int length =0;
	timeout.tv_sec  = 2;
	timeout.tv_usec = 0;
	while(thread_statu.rx)
	{
		FD_ZERO(&rfd);
		FD_SET(rxdata.linkfd,&rfd);
		if(select(1+rxdata.linkfd,&rfd,NULL,NULL,&timeout)>0)
		{
			if(FD_ISSET(rxdata.linkfd,&rfd))

			{
				err = ioctl(rxdata.linkfd, FIONREAD, &length);
				if(!err)
				{
					if(length!=0)
					{
						JT808_PRT("rxdata.linkfd =%d\n",rxdata.linkfd);
						if((n_bytes = readn(rxdata.linkfd,rxbuf,length))>0);
						{
							rxdata.length = n_bytes;
							rxdata.str = rxbuf;
							outprint_hex("msgrx",rxbuf,n_bytes);
							jt808_rx_proc( rxbuf, n_bytes, rxdata.linkfd);
							bzero(rxbuf,n_bytes);
						}
					}
					
				}
			}
		
		}
	}
	return NULL;
}	
/************************************************************
 * @file
 * @brief: start the thread "socket_rx_data" 
 * @author wxg
 * @date 2015-06-12
 * @version 0.1
 * @return if this thread sucess ,retun 0
 */

int socket_rx_data(void)
{
	thread_statu.rx = DF_TURE;
	return pthread_create(&sock_rx_pid, 0, data_rx, NULL);	
}
/************************************************************
 * @file
 * @brief: start the thread "socket_tx_data" 
 * @author wxg
 * @date 2015-06-12
 * @version 0.1
 * @return if this thread sucess ,retun 0
 */

int socket_tx_data(void)
{
	thread_statu.tx = DF_TURE;
	return pthread_create(&sock_tx_pid, 0, data_tx, NULL);
}
/***********************************************************
 * @file
 * @brief: contral the thread "socket_rx_data" 
 * @author wxg
 * @date 2015-06-12
 * @version 0.1
 */

int socket_rx_data_contl(char input)
{
    if ((DF_TURE == thread_statu.rx)&&(DF_TURE==input))
    {
        thread_statu.rx = DF_FALSE;
        
    }
	pthread_join(sock_rx_pid, 0);
    return DF_SUCCESS;
}
/************************************************************
 * @file
 * @brief: contral the thread "socket_tx_data" 
 * @author wxg
 * @date 2015-06-12
 * @version 0.1
 */

int socket_tx_data_contl(char input)
{
	if ((DF_TURE == thread_statu.tx)&&(DF_TURE==input))
    {
        thread_statu.tx = DF_FALSE;
       
    }
	 pthread_join(sock_rx_pid, 0);
    return DF_SUCCESS;	
}
/************************************************************
 * @file check_modem_status(void)
 * @brief: check tne modem online or offline(the file "modem.sh" must in /etc/ppp/peer) 
 * @author wxg
 * @date 2015-06-21
 * @ return 0 online other offline
 * @version 0.1
 */

int check_modem_status(void)
{
	int result;
	result = my_system("/etc/ppp/peers/modem.sh");
	return result;
}



#ifdef __cplusplus
}; //end of extern "C" {
#endif



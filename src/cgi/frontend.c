#include <fcgi_stdio.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "frontend.h"

extern int bl_downlink2anyconn_hook(const void *inbuf, int inlength, void *outbuf, int *outlength);
extern int bl_anyconn2uplink_hook(const void *inbuf, int inlength, void *outbuf, int *outlength);


#define PORT 12345 /*This is the port for the client server connection*/

static int g_is_opened = 0; //0/!0: no/yes

char uplink_buf[UP_LINK_BUF_SIZE], downlink_buf[DOWN_LINK_BUF_SIZE];
static int on = RPC_INIT_IO_MODE;
static int count = 0;




int fe_is_opened()
{
	return g_is_opened;
}

//>=0/<0: valid handler/failed
int fe_open(void) //return a handle
{
	int rc, on = RPC_INIT_IO_MODE;
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(PORT);
	if(-1 == connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
	{
		COMPOSE_UPLINK_MSG(NAK_TERM, "");
		return -1;
	}

	rc = ioctl(sock, FIONBIO, (char *)&on);
	if (rc < 0)
	{
		COMPOSE_UPLINK_MSG(NAK_TERM, "");
		close(sock);
		return -1;
	}

	g_is_opened = !0;

	return sock;

}

int fe_write(int hdl, const char *buf, int length, int (*cb)(void *))
{
	//to be simple, i use data as downlink buffer
	int numbytes = length, rc = 0;
	char *data = buf;
	
	numbytes = strlen(data)+1;
	
	memset(data, 0, SERVER_DL_HEADER_LENGTH); //preparing to be re-filled with frontend instructions
	((server_dl_t *)data)->flag = FE_ASYNC_WRITE;


	
	//payload data xcoding
	bl_downlink2anyconn_hook(&data[DL_PAYLOAD], numbytes-DL_PAYLOAD, &data[DL_PAYLOAD], &numbytes);
	numbytes += DL_PAYLOAD;
	
	if(!on)
	{
		rc = send(hdl, data, numbytes, 0);
		if(rc > 0)
		{
			DBG_CGI("%s:%d:%d\n", __FUNCTION__, __LINE__, count++); 	
			return 0;
		}
		else
		{
			if ((rc == -1) && (errno == EAGAIN||errno == EWOULDBLOCK||errno == EINTR)) 
			{
				COMPOSE_UPLINK_MSG(NAK_RNDM, "");
				DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
				//printf("%s, count=%d", uplink_buf, count++);
				return 0;
			}
			else
			{
				COMPOSE_UPLINK_MSG(NAK_TERM, "");
				DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
				close(hdl);
				return -1;
			}
		}
	}
	else 
	{
		rc = send(hdl, data, numbytes, 0);
		if (rc <= 0)
		{
			COMPOSE_UPLINK_MSG(NAK_TERM, "");
			DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
			close(hdl);
			return -1;
		}
		else
		{
			DBG_CGI("%s:%d:%d\n", __FUNCTION__, __LINE__, count++);
			return 0;
		}
	}


}

//timeout was set by ioctl interface, not here
int fe_read(int hdl, const char *in_buf, int in_length, char **out_buf, int *out_length, int flag)
{
	int rc;

	char *data = in_buf;
	int sockfd = hdl, numbytes = 0;
	
	numbytes = strlen(data)+1;
	memset(data, 0, SERVER_DL_HEADER_LENGTH); //preparing to be re-filled with frontend instructions
	((server_dl_t *)data)->flag = flag;

	memset(uplink_buf, 0, UP_LINK_BUF_SIZE);
	
	if(!on) //which means sync reading for status query from device
	{
		rc = send(sockfd, data, numbytes, 0);
		if(rc > 0)
		{					
			rc = (int)recv(sockfd, uplink_buf, UP_LINK_BUF_SIZE, 0);
			if( rc >0 )
			{
				int out_len = 0;
				bl_anyconn2uplink_hook(uplink_buf, rc, uplink_buf, &out_len);
				*out_buf = uplink_buf;
				*out_length = out_len;
				return 0;
				//COMPOSE_UPLINK_MSG(ACK_SYNC, "%s", uplink_buf); //readback data
				//DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
			}
			else
			{
				if((rc<0) && (errno == EAGAIN||errno == EWOULDBLOCK||errno == EINTR))
				{
					COMPOSE_UPLINK_MSG(NAK_RNDM, "");
					DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
					return 0;
				}
				else
				{
					COMPOSE_UPLINK_MSG(NAK_TERM, "");
					DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
					close(sockfd);
					return -1;
				}	
			}
		}
		else
		{
			if ((rc == -1) && (errno == EAGAIN||errno == EWOULDBLOCK||errno == EINTR)) 
			{
				COMPOSE_UPLINK_MSG(NAK_RNDM, "");
				DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
				//printf("%s, count=%d", uplink_buf, count++);
				return 0;
			}
			else
			{
				COMPOSE_UPLINK_MSG(NAK_TERM, "");
				DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
				close(sockfd);
				return -1;
			}
		}
	}
	else //which means async reading for status query from device
	{
		rc = send(sockfd, data, numbytes, 0);
		if (rc <= 0)
		{
			COMPOSE_UPLINK_MSG(NAK_TERM, "");
			close(sockfd);
			return -1;
		}
		else
		{
			rc = (int)recv(sockfd, uplink_buf, UP_LINK_BUF_SIZE, 0);
			if (rc < 0)
			{
				//DBG("%s:%d:rc=%d\n", __FUNCTION__, __LINE__, rc);
				//hongao: this fd must be unblocked!
				if (errno != EWOULDBLOCK)
				{
					COMPOSE_UPLINK_MSG(NAK_TERM, "");
					DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
					close(sockfd);
					return -1;
				}
				else
				{
					COMPOSE_UPLINK_MSG(NAK_RNDM, "");
					DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
					return 0;
				}
	
			}
	
			/*****************************************************/
			/* Check to see if the connection has been			 */
			/* closed by the client 							 */
			/*****************************************************/
			if (rc == 0)
			{
				COMPOSE_UPLINK_MSG(NAK_TERM, "");
				close(sockfd);
				return -1;
			}
	
			{
				int out_len = 0;
				bl_anyconn2uplink_hook(uplink_buf, rc, uplink_buf, &out_len);
				*out_buf = uplink_buf;
				*out_length = out_len;
				return 0;
			}
			
		}
	}

}

int fe_ioctl(int hdl, void *arg)
{
	int sockfd = hdl;
	char *data = (char *)arg;
	int rc;

	(data[DL_PAYLOAD_SYNC] == 'S') ? (on=0) : (on=!0); //data[P_OFFSET_DL_PAYLOAD] = 'S' for blocking io: on=0. o/w on=!0  
	/* Sets or clears nonblocking input/output for a socket. The arg parameter is a pointer to an integer. 
	If the integer is 0, nonblocking input/output on the socket is cleared. 
	Otherwise, the socket is set for nonblocking input/output.*/
	rc = ioctl(sockfd, FIONBIO, (char *)&on);
	if (rc < 0)
	{
		COMPOSE_UPLINK_MSG(NAK_TERM, "");
		DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
		close(sockfd);
		return -1;
	}
	
	if(!on) //on=0 means blocking mode
	{
		struct timeval timeout;
	
		//setting send timeout
		timeout.tv_sec = (data[DL_PAYLOAD_TXTO]-'0')*RPC_SYNC_TIMO_RESOLUTION/1000;
		timeout.tv_usec = (data[DL_PAYLOAD_TXTO]-'0')*RPC_SYNC_TIMO_RESOLUTION%1000*1000;
	
		if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		{
			COMPOSE_UPLINK_MSG(NAK_TERM, "");
			DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
			close(sockfd);
			return -1;
		}
	
		//setting receive timeout
		timeout.tv_sec = (data[DL_PAYLOAD_RXTO]-'0')*RPC_SYNC_TIMO_RESOLUTION/1000;
		timeout.tv_usec = (data[DL_PAYLOAD_RXTO]-'0')*RPC_SYNC_TIMO_RESOLUTION%1000*1000;
	
		if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
		{
			COMPOSE_UPLINK_MSG(NAK_TERM, "");
			DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
			close(sockfd);
			return -1;
		}
	
		DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
	}

}

int fe_close(int hdl)
{
	return close(hdl);
}


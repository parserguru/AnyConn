/*!**************************************************************************
 *  \copyright (c) 2016 Sinbest International Trading PTE LTD., all rights reserved
 *
 *  \brief Requirements Covered:
 *   - Fixed ip of devices are required;
 *   - Local communicaions via tcp server are fixed to 127.0.0.1
 *
 *  \cond
 *   Revision History:
 *   Date        Author      Description
 *   ------      --------    --------------
 *   16/11/17    Lu Hongao    Created
 *   20/11/17    Lu Hongao    Constructed a static link map for staic link mapping
 *                            by hooking connection events. So no need to take care 
 *                            of link sequence anymore.
 *  \endcond
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

#include <netdb.h>
#include <arpa/inet.h>

#include "frontend.h"

#define DEBUG_PRINTF

#ifdef DEBUG_PRINTF
#define DBG(fmt,arg...) printf(fmt,##arg)
#else
#define DBG(fmt,arg...) do{}while(0);
#endif

//cgi will get status from here
//please add more details design for product development,
//this is jz a demo only!
static char latest_uplink_status_buf[1024] = {0};

typedef struct t_fds_info
{
	char name[100];
	unsigned long ip;
	int port;
	int fds;
	int status;
	
} fds_info_t;

//this enum is for O(0) algorithm of switch settings
enum t_link_index_name {
	ID_485_ETH_DEVICE_1 = 0,
	ID_LOCAL_FCGI_1,
	ID_OUT_OF_RANGE
	
} link_index_name_t;

//this is a fixed map for static connection management
//for future security enhancement, status and presettings could be split to two parts and stored in different place
static fds_info_t link_map[] = {
	{"ID_485_ETH_DEVICE_1", 0x3c3ca8c0, 12345, -1, -1}, //ID_485_ETH_DEVICE_1 addr:192.168.60.60(0x3c3ca8c0), port:12345(0x3930)
	{"ID_LOCAL_FCGI_1", 0x100007f, 12345, -1, -1}, //ID_LOCAL_FCGI_1 addr:127.0.0.1(0x100007f), port:12345(0x3930)
};

static void put_link_map()
{
	for(int cnt1=0; cnt1<ID_OUT_OF_RANGE; cnt1++)
	{
		DBG("%s:[%d], ip=0x%08x, port=%d, fds=%d, status=%d\n", link_map[cnt1].name, cnt1, link_map[cnt1].ip, link_map[cnt1].port, link_map[cnt1].fds, link_map[cnt1].status);
	}
}

//monitor new connections
//assume the first few connection are all from 485/eth module
//status: 0/-1 on/off
void connection_hook(struct pollfd *fds, int status)
{		
	struct sockaddr_in addr_in;
	
		
	
	socklen_t len = sizeof(addr_in);
	getsockname(fds->fd, (struct sockaddr *)&addr_in, &len);
	if (addr_in.sin_family != AF_INET) {
		printf("Not an Internet socket.\n");
		return;
	}
	
	DBG("fd=%d, status=%s\n", fds->fd, (0==status)?"ON":"OFF");
	
	for(int cnt=0; cnt<ID_OUT_OF_RANGE; cnt++)
	{
		if(addr_in.sin_addr.s_addr == link_map[cnt].ip)
		{
			link_map[cnt].fds = fds->fd;
			link_map[cnt].status = status;
			break;
		}
	}
	
#ifdef DEBUG_PRINTF
	put_link_map();
#endif
}

static int g_read_barrier = 0;
//this is hooked up at socket data receving point
//customize switch: device uplink to status buffer, while local cgi downlink directly pass to device
int switch_hook(int sockfd, char *buffer, int length)
{
	//this is an active uplink from device to browser. we jz updata status buffer for this design
	if((sockfd == link_map[ID_485_ETH_DEVICE_1].fds) && (0 == link_map[ID_485_ETH_DEVICE_1].status))
	{
		if(!g_read_barrier) //to collect async events from device
		{
			if(length > sizeof(latest_uplink_status_buf))
				length = sizeof(latest_uplink_status_buf);

			memcpy(latest_uplink_status_buf, buffer, length);
		}
		else //directly write through to the blocking browser
		{
			int rc = send(link_map[ID_LOCAL_FCGI_1].fds, buffer, length, 0);
			if (rc < 0)
			{
				perror("  send() failed");
				g_read_barrier = 0;
				return -1;
			}
			
			g_read_barrier = 0;
		}
			
	}
	//this is an active downlink from browser to device. we directly pass the data to the target device
	else if((sockfd == link_map[ID_LOCAL_FCGI_1].fds) && (0 == link_map[ID_LOCAL_FCGI_1].status))
	{
		int rc = send(link_map[ID_485_ETH_DEVICE_1].fds, buffer+SERVER_DL_HEADER_LENGTH, length-SERVER_DL_HEADER_LENGTH, 0);
		if (rc < 0)
		{
			perror("  send() failed");
			return -1;
		}

		if(FE_SYNC_READ == ((server_dl_t *)buffer)->flag)
		{
		    int rc = send(link_map[ID_LOCAL_FCGI_1].fds, latest_uplink_status_buf, sizeof(latest_uplink_status_buf), 0);
			if (rc < 0)
			{
				perror("  send() failed");
				return -1;
			}
			
		}
		else if(FE_SYNC_READ_HARD == ((server_dl_t *)buffer)->flag)
		{
			g_read_barrier = !0; //read barrier is setting to force the next writeback through(uplink for sync read output)
		}
	}
	else
	{
		perror("unknown socket!\n");
		return -1;
	}
		
	return 0;
}
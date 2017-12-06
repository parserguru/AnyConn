/*!**************************************************************************
 *  \copyright (c) 2016 Sinbest International Trading PTE LTD., all rights reserved
 *
 *  \brief Requirements Covered:
 *   A high performance but compact Remote Procedure Call handler
 *
 *  \cond
 *   Revision History:
 *   Date        Author      Description
 *   ------      --------    --------------
 *   17/11/17    Lu Hongao    Created
 *  \endcond
 *
 *****************************************************************************/
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

#include "rpc.h"





#define QCMD_LENGTH 3

//payload of rpc interface usually in xml/json format because of the nature of http, 
//but rpc/anyconn we use memory interface
//@inbuf and @outbuf could be the same buffer passed by callers, so please takecare
int bl_downlink2anyconn_hook(const void *inbuf, int inlength, void *outbuf, int *outlength)
{
	//add more for payload format transferring, e.g json decoding
	//...



	//get output. you may customize it
	*outlength = inlength;

	return 0;
}

//@inbuf and @outbuf could be the same buffer passed by callers, so please takecare
int bl_anyconn2uplink_hook(const void *inbuf, int inlength, void *outbuf, int *outlength)
{

    //add more for payload format transferring, e.g json encoding
    //...

	//get output. you may customize it
	*outlength = inlength;

	return 0;
}

void rpc_handler()
{
	char *data;
	int numbytes = 0;

	int rc;
	//fe opened handler
	int g_fe_handle = -1;


	g_fe_handle = fe_open();

	
	while(FCGI_Accept() >= 0) {  
		data = getenv("QUERY_STRING");
	
		switch(data[DL_TYPE])
		{
			case 'C':
				fe_ioctl(g_fe_handle, data);
				break;

			case 'D':
				//not supported yet
				COMPOSE_UPLINK_MSG(DBG_NANA, "");
				//DBG_CGI("%s:%d:%d:on=%d\n", __FUNCTION__, __LINE__, count++, on);
				break;
				
			case 'G': 
				{
					char *out_buf;
					int out_length;
					fe_read(g_fe_handle, data, strlen(data)+1, &out_buf, &out_length, FE_SYNC_READ);
					COMPOSE_UPLINK_MSG(ACK_SYNC, "%s", out_buf); //readback data
				}
				break;

			case 'P':
				fe_write(g_fe_handle, data, strlen(data)+1, NULL);
				break;

			default:
				break;
		}
	}

	fe_close(g_fe_handle);
	return;
				
}

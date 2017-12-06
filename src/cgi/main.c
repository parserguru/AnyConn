/*!**************************************************************************
 *  \copyright (c) 2016 Sinbest International Trading PTE LTD., all rights reserved
 *
 *  \brief Requirements Covered:
 *
 *
 *  \cond
 *   Revision History:
 *   Date        Author      Description
 *   ------      --------    --------------
 *   17/11/17    Lu Hongao    Created
 *  \endcond
 *
 *****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcgi_stdio.h>
#include <sys/ioctl.h>

#include "rpc.h"




extern void rpc_handler();

//int main(int argc, char *argv[])
void main(void)
{

		

	rpc_handler();
	

}




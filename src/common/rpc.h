/*!**************************************************************************
 *  \copyright (c) 2016 Sinbest International Trading PTE LTD., all rights reserved
 *
 *  \brief Requirements Covered:
 *   A header of the high performance but compact Remote Procedure Call handler
 *
 *  \cond
 *   Revision History:
 *   Date        Author      Description
 *   ------      --------    --------------
 *   17/11/17    Lu Hongao    Created
 *  \endcond
 *
 *****************************************************************************/
#ifndef __RPC_H__
#define __RPC_H__

extern int fe_open(void); //return a handle
extern int fe_write(int hdl, const char *buf, int length, int (*cb)(void *));
extern int fe_read(int hdl, const char *in_buf, int in_length, char **out_buf, int *out_length, int flag);
extern int fe_close(int hdl);


//IO init presetting to !0, non-blocking mode
#define RPC_INIT_IO_MODE (!0)
#define RPC_SYNC_TIMO_RESOLUTION 200


typedef enum t_uplink_msg_type
{
	NAK_EMRG = 0, /* emergency: anyconn server is unusable, need to reboot the server */
	NAK_TERM, /* terminated: connection is abort, need to reconnect */
	NAK_TIMO, /* timeout: request timeout, need to retry */
	NAK_RNDM, /* random: random err like interrupt, need to retry */	
	ACK_ASYN, /* asyn send: async data sending succeed, followed by status of sending if any */
	ACK_SYNC, /* sync read: sync data reading succeed, followed by readback messages if any */

	DBG_NANA,
		
} uplink_msg_type_t;


//UPLINK INTERFACE
//cgi->pc format: "hash|src|dst|status|cnt|return|payload"
//@hash(8): ('RRRRRRRR'eserved) crc32 for data integrity. to compose: http://www.sha1-online.com/
//@src(4): source id, for anyconn firewall filtering
//@dst(4): destination id, for anyconn firewall filtering
//@status(1): ('R'eserved) for flow control, events report, etc.
//@cnt(1): message count for synchronization on non-blocking interface, ASC 0->9->0...(0x30->0x39->0x30...)
//@payload(v): output parameters, msg body
const static char *g_report_table[] = {
	"RRRRRRRR|0000|0000|R|0|NAK_EMRG|",
	"RRRRRRRR|0000|0000|R|0|NAK_TERM|",
	"RRRRRRRR|0000|0000|R|0|NAK_TIMO|",
	"RRRRRRRR|0000|0000|R|0|NAK_RNDM|",
	"RRRRRRRR|0000|0000|R|0|ACK_ASYN|",
	"RRRRRRRR|0000|0000|R|0|ACK_SYNC|",
	"RRRRRRRR|0000|0000|R|0|DBG_NANA|",
};

//compose the uplink in html format
#define COMPOSE_UPLINK_MSG(rpc_err,fmt,arg...) do { \
	printf("Content-Type: text/html\r\n"); \
	printf("\r\n"); \
	printf("[%03d]%s: ", rpc_err, g_report_table[rpc_err]); \
	printf(fmt,##arg); \
} while(0)


//DOWNLINK INTERFACE
//pc->cgi format: "hash|src|dst|type|cnt|payload"
//@hash(8): ('RRRRRRRR'eserved) crc32 for data integrity. to compose: http://www.sha1-online.com/
//@src(4): source id, for anyconn firewall filtering
//@dst(4): destination id, for anyconn firewall filtering
//@type(1): P/G/C/D:Post/Get/IO Control/Debug
//@cnt(1): message count for async interface, ASC 0->9->0...(0x30->0x39->0x30...)
//@payload(v): input parameters
//	when type=P/G, payload=Downlink command; 
//	while type=C, payload[0]=S/A:blocking io/non-blocking io
//                	payload[1] ASCtoINT 0->9 for blocking send() timeout, resolution is RPC_SYNC_TIMO_RESOLUTIONms
//					payload[2] ASCtoINT 0->9 for blocking recv() timeout, resolution is RPC_SYNC_TIMO_RESOLUTIONms

//downlink message data structure is defined as below:
#define DL_HASH 0
#define DL_SRC (DL_HASH+8+1)
#define DL_DST (DL_SRC+4+1)
#define DL_TYPE (DL_DST+4+1)
#define DL_CNT (DL_TYPE+1+1)
#define DL_PAYLOAD (DL_CNT+1+1)
#define DL_PAYLOAD_SYNC (DL_PAYLOAD)
#define DL_PAYLOAD_TXTO (DL_PAYLOAD_SYNC+1)
#define DL_PAYLOAD_RXTO (DL_PAYLOAD_TXTO+1)

//examples:
//http://192.168.60.60/cgi-bin/rpc.fcgi?RRRRRRRR|0000|0000|C|0|S99      //IO Control, blocking io, rw timeout=9*RPC_SYNC_TIMO_RESOLUTIONms
//http://192.168.60.60/cgi-bin/rpc.fcgi?RRRRRRRR|0000|0000|C|0|A      //IO Control, non-blocking io
//http://192.168.60.60/cgi-bin/rpc.fcgi?RRRRRRRR|0000|0000|P|0|ASYNC WRITE CMD      //POST
//http://192.168.60.60/cgi-bin/rpc.fcgi?RRRRRRRR|0000|0000|G|0|READ(A/S MODE BY IO CONTROL SETTINGS)      //GET



#define DEBUG_PRINTF



#ifdef DEBUG_PRINTF
#define DBG_CGI(fmt,arg...) COMPOSE_UPLINK_MSG(DBG_NANA,fmt,##arg)
#else
#define DBG_CGI(fmt,arg...) do{}while(0)
#endif

//temporary put here

typedef enum t_fe_flag
{
	FE_ASYNC_WRITE = 0,
	FE_SYNC_READ, //read from latest status buffer
	FE_SYNC_READ_HARD, //blocking untill h/w response

};





#endif



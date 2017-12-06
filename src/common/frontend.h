#ifndef __FRONTEND_H__
#define __FRONTEND__

#include "rpc.h"

int fe_open(void); //return a handle
int fe_write(int hdl, const char *buf, int length, int (*cb)(void *));
int fe_read(int hdl, const char *in_buf, int in_length, char **out_buf, int *out_length, int flag);
int fe_close(int hdl);


#define DOWN_LINK_BUF_SIZE 1024
#define UP_LINK_BUF_SIZE 1024

typedef struct t_server_dl
{
	unsigned long hash;
	unsigned short src_id;
	unsigned short dst_id;
	char flag;
	unsigned char cnt;
	
}__attribute__((packed)) server_dl_t; //for network purpose

#define SERVER_DL_HEADER_LENGTH DL_PAYLOAD //reuse RPC buffer



typedef struct t_server_ul
{
	unsigned long hash;
	unsigned short src_id;
	unsigned short dst_id;
	char flag;
	unsigned char cnt;
	
}__attribute__((packed)) server_ul_t; //for network purpose










#endif





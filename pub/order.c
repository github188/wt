/*****************************************************
 *
 * 命令操作函数
 *
 *****************************************************/

#include "header.h"

/** 
 *@brief  平台地址下发
 *@param  sockfd	类型 int			要发送到设备的socket套接字
 *@param  msg_head  类型 msg_head_st*	已填充设备基本信息的msg_head_st指针
 *@return succ 0 failed -1
 */
int send_addr_set(int sockfd, msg_head_st* msg_head, int order, char* addr, int port)
{
	int size = sizeof(msg_head_st) + sizeof(addr_set_st) + 1;
	char* buf = malloc(size);
	memset(buf, 0, size);
	msg_head_st* head = (msg_head_st*)buf;
	addr_set_st* msg = (addr_set_st*)( buf + sizeof(msg_head_st) );

	//包头
	memcpy(head, msg_head, sizeof(msg_head_st));
	head->order = order;
	head->mode	= 2;
	head->len	= size - 1;
	
	snprintf(msg->addr, sizeof(msg->addr), "%s", addr);
	msg->port = port;
	
	//xyprintf_addr_set(msg);

	//发送
	if( send_addr_set_msg(sockfd, buf) ){
		free(buf);
		return -1;
	}

	if(order == msg_order_ac_addr_set){
		xyprintf(0, "%u:--> --> Send AC addr set msg to router success --> -->", msg_head->router_id);
	}
	else if(order == msg_order_acac_addr_set){
		xyprintf(0, "%u:--> --> Send ACAC addr set msg to router success --> -->", msg_head->router_id);
	}
	else{
		xyprintf(0, "%u:--> --> Send GUIDE addr set msg to router success --> -->", sockfd);
	}
	
	free(buf);
	return 0;
}

/*****************************************************
 *
 * 网络操作函数
 *
 *****************************************************/

#include "auth_header.h"

/** 
 *@brief  接收路由器用户包体数据 并装换为本地字节序
 *@param  sockfd	类型 int				要接收数据的socket套接字
 *@param  msg		类型 user_msg_st*		接收到的数据存放地址
 *@param  len		类型 int				接收数据长度
 *@return success 0 failed -1
 */
int recv_user_msg_head(int sockfd, user_msg_st *msg, int len)
{
	if( wt_recv_block( sockfd, (char*)msg, len)){
		return -1;
	}

	big_little16(&(msg->auth_len));
	big_little32(&(msg->speed[0]));
	big_little32(&(msg->speed[1]));
	big_little32(&(msg->auth_time));
	big_little32(&(msg->flow[0]));
	big_little32(&(msg->flow[1]));
	
	return 0;
}

/** 
 *@brief  发送用户数据包到目标路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int				目标路由器的socket套接字
 *@param  msg_head	类型 msg_head_st*		要发送的数据包包头数据
 *@param  msg		类型 user_msg_st*		要发送的数据包包体数据
 *@param  rj_addr	类型 char*				跳转地址，没有跳转地址时传空
 *@return success 0 failed -1
 */
int send_sep_user_msg_head(int sockfd, msg_head_st *msg_head, user_msg_st *msg, char* rj_addr)
{
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	big_little16(&(msg->auth_len));
	big_little32(&(msg->speed[0]));
	big_little32(&(msg->speed[1]));
	big_little32(&(msg->auth_time));
	big_little32(&(msg->flow[0]));
	big_little32(&(msg->flow[1]));
	
	int len = sizeof(msg_head_st) + sizeof(user_msg_st);
	if(rj_addr){
		len += strlen(rj_addr);
	}
	
	void *buf = malloc(len);
	
	memcpy(buf,							msg_head,	sizeof(msg_head_st));
	memcpy(buf + sizeof(msg_head_st),	msg,		sizeof(user_msg_st) );
	
	if( rj_addr ){
		memcpy(buf + sizeof(msg_head_st) + sizeof(user_msg_st), rj_addr, strlen(rj_addr));
	}
	
	int res = wt_send_block( sockfd, buf, len);
	free(buf);
	return res;
}

/** 
 *@brief  发送用户数据包到目标路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int	目标路由器的socket套接字
 *@param  full_msg	类型 void*	要发送的数据包包头和包体内容
 *@return success 0 failed -1
 */
int send_full_user_msg_head(int sockfd, void* full_msg){
	msg_head_st *msg_head = full_msg;						//发往设备的数据包头
	user_msg_st *msg	  = full_msg + sizeof(msg_head_st);	//发往设备的数据包体
	int len = msg_head->len;	

	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	big_little16(&(msg->auth_len));
	big_little32(&(msg->speed[0]));
	big_little32(&(msg->speed[1]));
	big_little32(&(msg->auth_time));
	big_little32(&(msg->flow[0]));
	big_little32(&(msg->flow[1]));
	return wt_send_block( sockfd, full_msg, len );
}

/** 
 *@brief  发送设置白名单数据包到目标路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int				目标路由器的socket套接字
 *@param  msg		类型 void*				要发送的数据包包头和包体内容
 *@return success 0 failed -1
 */
int send_dns_msg_head(int sockfd, void* msg)
{
	msg_head_st	*msg_head = msg;
	white_dns_msg_st *dns_msg = msg + sizeof(msg_head_st);
	
	int len = msg_head->len;

	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	big_little16(&(dns_msg->url_len));

	return wt_send_block( sockfd, msg, len );
}


/** 
 *@brief  接收路由器心跳包体数据 并装换为本地字节序
 *@param  sockfd	类型 int				要接收数据的socket套接字
 *@param  msg		类型 heart_msg_st*		接收到的数据存放地址
 *@param  len		类型 int				接收数据长度
 *@return success 0 failed -1
 */
int recv_heart_msg_head(int sockfd, heart_msg_st *msg, int len)
{
	if( wt_recv_block( sockfd, (char*)msg, len) ){
		return -1;
	}

	big_little32(&(msg->router_id));
	big_little32(&(msg->user_num));
	return 0;
}

/** 
 *@brief  回复路由器发来的心跳包 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int				目标路由器的socket套接字
 *@param  msg_head	类型 msg_head_st*		要发送的数据包包头数据
 *@param  msg		类型 heart_msg_st*		要发送的数据包包体数据
 *@param  len		类型 int				数据包包体的长度
 *@return success 0 failed -1
 */
int send_heart_msg_head(int sockfd, msg_head_st* msg_head, heart_msg_st *msg, int len)
{
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);
	
	big_little32(&(msg->router_id));
	big_little32(&(msg->user_num));

	int size = sizeof(*msg_head) + len;
	__u8 *buf = malloc(size);
		
	memcpy(buf, msg_head, sizeof(*msg_head));
	memcpy(buf + sizeof(*msg_head), msg, len);
	
	int res = wt_send_block( sockfd, buf, size );

	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	free(buf);
	return res;
}

/** 
 *@brief  接收路由器QQ信息包体数据 并装换为本地字节序
 *@param  sockfd	类型 int				要接收数据的socket套接字
 *@param  msg		类型 third_qq_msg_st*	接收到的数据存放地址
 *@param  len		类型 int				接收数据长度
 *@return success 0 failed -1
 */
int recv_third_qq_msg_head(int sockfd, third_qq_msg_st *msg, int len)
{
	if( wt_recv_block( sockfd, (char*)msg, len)){
		return -1;
	}

	big_little32(&(msg->qq_num));
	big_little16(&(msg->qq_type));
	return 0;
}

/** 
 *@brief  接收路由器自动升级回复包体数据 并装换为本地字节序
 *@param  sockfd	类型 int						要接收数据的socket套接字
 *@param  msg		类型 auto_upgrade_result_st*	接收到的数据存放地址
 *@param  len		类型 int						接收数据长度
 *@return success 0 failed -1
 */
int recv_auto_upgrade_res(int sockfd, auto_upgrade_result_st *msg, int len )
{
	if( wt_recv_block( sockfd, (char*)msg, len)){
		return -1;
	}

	big_little16(&(msg->result));
	big_little16(&(msg->len));
	return 0;
}

/** 
 *@brief  发送富媒体设置数据包到目标服务器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int		目标路由器的socket套接字
 *@param  gg_msg	类型 void*		数据包包头和包体
 *@return success 0 failed -1
 */
int send_simple_gg_msg_head(int sockfd, void *gg_msg)
{
	msg_head_st *msg_head = gg_msg;
	int len = msg_head->len;
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	simple_gg_msg_st *gg = (simple_gg_msg_st*)(gg_msg + sizeof(msg_head_st));

	big_little32( &(gg->en) );
	big_little32( &(gg->js_len) );
	big_little32( &(gg->dns_cnt) );
	big_little32( &(gg->data_len) );
	
	return wt_send_block( sockfd, gg_msg, len );
}

/** 
 *@brief  发送版本号获取数据包到目标服务器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg_head	类型 msg_head_st*	数据包包头
 *@return success 0 failed -1
 */
int send_version_get_msg(int sockfd, msg_head_st *msg_head)
{
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);
	
	int res = wt_send_block(sockfd, (unsigned char*)msg_head, sizeof(msg_head_st) );

	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	return res;
}

/** 
 *@brief  发送自动升级数据包到目标服务器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg		类型 void*			数据包包头和包体
 *@return success 0 failed -1
 */
int send_auto_upgrade_msg(int sockfd, void *msg)
{
	msg_head_st *msg_head = msg;
	int len = msg_head->len;
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	big_little16(&((auto_upgrade_msg_st*)(msg + sizeof(msg_head_st)))->url_len);
	
	return wt_send_block( sockfd, msg, len );
}

/** 
 *@brief  发送配置获取报文 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg_head	类型 msg_head_st*	数据包包头
 *@return success 0 failed -1
 */
int send_wireless_config_get_msg(int sockfd, msg_head_st *msg_head)
{
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);
	
	int res = wt_send_block(sockfd, (unsigned char*)msg_head, sizeof(msg_head_st) );
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	return res;
}

/** 
 *@brief  接收路由器配置报文 并装换为本地字节序
 *@param  sockfd	类型 int						要接收数据的socket套接字
 *@param  msg		类型 wifi_base_conf_msg_st*		接收到的数据存放地址
 *@return success 0 failed -1
 */
int recv_wifi_base_config(int sockfd, wifi_base_conf_msg_st *msg)
{
	if( wt_recv_block( sockfd, (char*)msg, sizeof(wifi_base_conf_msg_st))){
		return -1;
	}

	int i;
	for(i = 0; i < 5; i++){
		if(msg->list[i].security_mode == 4 || 
			msg->list[i].security_mode == 5 || 
			msg->list[i].security_mode == 6 || 
			msg->list[i].security_mode == 7 || 
			msg->list[i].security_mode == 8 || 
			msg->list[i].security_mode == 9 ){
			
			big_little32(&(msg->list[i].key.wpa.rekeyinterval));
			big_little32(&(msg->list[i].key.wpa.radius_server));
			big_little32(&(msg->list[i].key.wpa.session_timeout));
			big_little16(&(msg->list[i].key.wpa.radius_port));
			big_little16(&(msg->list[i].key.wpa.PMKCachePeriod));
			big_little16(&(msg->list[i].key.wpa.PreAuthentication));
		}
	}//end for
	return 0;
}

/** 
 *@brief  发送路由器配置报文 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int					目标路由器的socket套接字
 *@param  msg_head	类型 msg_head_st*			数据包包头
 *@param  msg		类型 wifi_base_conf_msg_st*	数据包包体
 *@return success 0 failed -1
 */
int send_wifi_base_config(int sockfd, msg_head_st *msg_head, wifi_base_conf_msg_st *msg)
{
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	int i;
	for(i = 0; i < 5; i++){
		if(msg->list[i].security_mode == 4 || 
			msg->list[i].security_mode == 5 || 
			msg->list[i].security_mode == 6 || 
			msg->list[i].security_mode == 7 || 
			msg->list[i].security_mode == 8 || 
			msg->list[i].security_mode == 9 ){
			
			big_little32(&(msg->list[i].key.wpa.rekeyinterval));
			big_little32(&(msg->list[i].key.wpa.radius_server));
			big_little32(&(msg->list[i].key.wpa.session_timeout));
			big_little16(&(msg->list[i].key.wpa.radius_port));
			big_little16(&(msg->list[i].key.wpa.PMKCachePeriod));
			big_little16(&(msg->list[i].key.wpa.PreAuthentication));
		}
	}//end for
	
	int len = sizeof(msg_head_st) + sizeof(wifi_base_conf_msg_st);
	char *buf = malloc( len );
	memcpy(buf, msg_head, sizeof(msg_head_st));
	memcpy(buf + sizeof(msg_head_st), msg, sizeof(wifi_base_conf_msg_st));
	int ret = wt_send_block( sockfd, buf, len );
	free(buf);
	return ret;
}

/** 
 *@brief  发送重启指令到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg		类型 void*			数据包包头和包体
 *@return success 0 failed -1
 */
int send_reboot_msg(int sockfd, void *msg)
{
	msg_head_st *msg_head = msg;
	int len = msg_head->len;
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	reboot_msg_st *r_msg = msg + sizeof(msg_head_st);			// 发往设备的数据包体
	big_little32(&r_msg->type);
	
	return wt_send_block( sockfd, msg, len );
}

/** 
 *@brief  发送简单缓存指令到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg		类型 void*			数据包包头和包体
 *@return success 0 failed -1
 */
int send_simple_cache_msg(int sockfd, void *msg)
{
	msg_head_st *msg_head = msg;
	int len = msg_head->len;
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	simple_cache_msg_st *cache = msg + sizeof(msg_head_st);			// 发往设备的数据包体
	big_little16(&cache->url_len);
	
	return wt_send_block( sockfd, msg, len );
}

/** 
 *@brief  发送USB缓存指令到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg		类型 void*			数据包包头和包体
 *@return success 0 failed -1
 */
int send_usb_simple_cache_msg(int sockfd, void *msg)
{
	msg_head_st *msg_head = msg;
	int len = msg_head->len;
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	usb_simple_cache_st *cache = msg + sizeof(msg_head_st);			// 发往设备的数据包体
	big_little16(&cache->opt);
	big_little16(&cache->len);
	
	return wt_send_block( sockfd, msg, len );
}

/** 
 *@brief  发送阻止用户微信域名重定向报文到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg		类型 void*			数据包包头和包体
 *@return success 0 failed -1
 */
int send_domain_redirect_msg(int sockfd, void *msg)
{
	msg_head_st *msg_head = msg;
	int len = msg_head->len;
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	return wt_send_block( sockfd, msg, len );
}

/** 
 *@brief  发送行为管理 关键词过滤指令到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg		类型 void*			数据包包头和包体
 *@return success 0 failed -1
 */
int send_web_keyword_msg(int sockfd, void *msg)
{
	msg_head_st *msg_head = msg;
	int len = msg_head->len;
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	return wt_send_block( sockfd, msg, len );
}

/** 
 *@brief  发送行为管理 url重定向指令到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg		类型 void*			数据包包头和包体
 *@return success 0 failed -1
 */
int send_url_redirect_msg(int sockfd, void *msg)
{
	msg_head_st *msg_head = msg;
	int len = msg_head->len;
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	return wt_send_block( sockfd, msg, len );
}

/** 
 *@brief  发送行为管理 流媒体识别指令到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg		类型 void*			数据包包头和包体
 *@return success 0 failed -1
 */
int send_mx_app_filter_msg(int sockfd, void *msg)
{
	msg_head_st *msg_head = msg;
	int len = msg_head->len;
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	return wt_send_block( sockfd, msg, len );
}

/** 
 *@brief  发送微信分享设置报文到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg		类型 void*			数据包包头和包体
 *@return success 0 failed -1
 */
int send_weixin_share_set_msg(int sockfd, void *msg)
{
	msg_head_st *msg_head = msg;
	int len = msg_head->len;
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	weixin_share_set_st* wsss = msg + sizeof(msg_head_st);
	big_little16(&wsss->enable);
	big_little16(&wsss->html_len);

	return wt_send_block( sockfd, msg, len );
}

/** 
 *@brief  发送USB路径文件列表获取报文到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg		类型 void*			数据包包头和包体
 *@return success 0 failed -1
 */
int send_usb_file_get_msg(int sockfd, void *msg)
{
	msg_head_st *msg_head = msg;
	int len = msg_head->len;
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	return wt_send_block( sockfd, msg, len );
}

/** 
 *@brief  发送路由器速度获取数据包到目标服务器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg_head	类型 msg_head_st*	数据包包头
 *@return success 0 failed -1
 */
int send_head_get_msg(int sockfd, msg_head_st *msg_head)
{
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);
	
	int res = wt_send_block(sockfd, (unsigned char*)msg_head, sizeof(msg_head_st) );

	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);

	return res;
}

/** 
 *@brief  接收路由器实时速度报文 并转换为本地字节序
 *@param  sockfd	类型 int						要接收数据的socket套接字
 *@param  msg		类型 third_speed_st*		接收到的数据存放地址
 *@return success 0 failed -1
 */
int recv_speed_get_msg(int sockfd, third_speed_st *msg)
{
	if( wt_recv_block( sockfd, (char*)msg, sizeof(third_speed_st))){
		return -1;
	}
	
	big_little32(&msg->speed_up);
	big_little32(&msg->speed_down);

	return 0;
}

/** 
 *@brief  发送简单广告2报文到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg		类型 void*			数据包包头和包体
 *@return success 0 failed -1
 */
int send_simple_gg2_head_st(int sockfd, void *msg)
{
	msg_head_st *msg_head = msg;
	int len = msg_head->len;
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);
	
	simple_gg2_head_st* sg2 = msg + sizeof(msg_head_st);

	int i;
	simple_gg2_st *data = (simple_gg2_st*)(sg2->data);
	for(i = 0; i < sg2->cnt; i++){
		big_little32(&data->replace_pos_flag);
		big_little32(&data->dns_len);
		big_little32(&data->js_len);
		int data_len = data->data_len;
		big_little32(&data->data_len);
		
		data = (simple_gg2_st*)( ((void*)data) + sizeof(simple_gg2_st) + data_len );
	}

	big_little32(&sg2->en);
	big_little32(&sg2->cnt);
	big_little32(&sg2->data_len);
	
	return wt_send_block( sockfd, msg, len );
}

/** 
 *@brief  发送探针设置报文到路由器 并在发送前将数据转换成网络字节序
 *@param  sockfd	类型 int			目标路由器的socket套接字
 *@param  msg		类型 void*			数据包包头和包体
 *@return success 0 failed -1
 */
int send_monitor_set_st_msg(int sockfd, void *msg)
{
	msg_head_st *msg_head = msg;
	int len = msg_head->len;
	
	big_little16(&msg_head->order);
	big_little16(&msg_head->len);
	big_little16(&msg_head->sec_flag);
	big_little32(&msg_head->crc);
	big_little32(&msg_head->router_id);
	
	monitor_set_st* mss = msg + sizeof(msg_head_st);

	big_little16(&mss->state);
	big_little16(&mss->port);
	big_little16(&mss->time);
	big_little16(&mss->timeout);
	
	return wt_send_block( sockfd, msg, len );
}

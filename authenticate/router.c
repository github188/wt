/*****************************************************
 *
 * 设备接入 管理 等
 *
 *****************************************************/

#include "auth_header.h"

/** 
 *@brief  将对应认证服务器连接置错,在下个epoll循环中删除
 *@param  shebei_pkid	类型 int	网络连接套接字
 *@return nothing
 */
void inline socklist_shebeipkid_err(int shebei_pkid){
	struct list_head* pos;
	//遍历查找对应的出错设备
	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	for( pos = gv_sock_list_head.next; pos != &gv_sock_list_head; pos = pos->next ){
		if( shebei_pkid == ((sock_list*)pos)->shebei_pkid){
			((sock_list*)pos)->stat = SOCK_STAT_DEL;//置为-1 备下次添加时 删除
			break;
		}	
	}
	pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
}

/** 
 *@brief  将对应认证服务器连接置错,在下个epoll循环中删除
 *@param  sockfd	类型 int	网络连接套接字
 *@return nothing
 */
void inline socklist_sockfd_err(int sockfd){
	struct list_head* pos;
	//遍历查找对应的出错设备
	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	for( pos = gv_sock_list_head.next; pos != &gv_sock_list_head; pos = pos->next ){
		if( sockfd == ((sock_list*)pos)->sockfd){
			((sock_list*)pos)->stat = SOCK_STAT_DEL;//置为-1 备下次添加时 删除
			break;
		}	
	}
	pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
}

/** 
 *@brief  添加设备到监听列表 并更新当前时间 和 心跳次数
 *@param  last_heart_time	类型 time_t	最后一次的活动时间
 *@param  heart_count		类型 int	增加的心跳次数
 *@return null
 */
void inline socklist_sockfd_add(__u32 rid, time_t last_heart_time, int heart_count){
	struct list_head* pos;
	//遍历查找对应的出错设备
	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	for( pos = gv_sock_list_head.next; pos != &gv_sock_list_head; pos = pos->next ){
		if(((sock_list*)pos)->router_id == rid){
			((sock_list*)pos)->stat = SOCK_STAT_ADD;//置为-1 备下次添加时 删除
			((sock_list*)pos)->last_heart_time = last_heart_time;
			((sock_list*)pos)->heart_count += heart_count;
			break;
		}	
	}
	pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
}

/** 
 *@brief  根据设备rid获取设备信息
 *@param  rid	类型 __u32		设备rid
 *@param  sock	类型 sock_list*	返回参数 设备信息
 *@return 找到 0 没有找到 -1
 */
int inline socklist_rid_call_all(__u32 rid, sock_list* sock){
	struct list_head* pos;
	int find_flag = -1;
	//查找发送报文的设备 并将设备信息复制出来
	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	list_for_each(pos,&gv_sock_list_head){
		if(((sock_list*)pos)->router_id == rid){
			memcpy(sock, pos, sizeof(sock_list));
			find_flag = 0;
			break;
		}
	}
	pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
	return find_flag;
}

/** 
 *@brief  根据设备id获取设备信息
 *@param  rid	类型 __u32		设备id
 *@param  sock	类型 sock_list*	返回参数 设备信息
 *@return 找到 0 没有找到 -1
 */
int inline socklist_id_call_all(__u32 id, sock_list* sock){
	struct list_head* pos;
	int find_flag = -1;
	//查找发送报文的设备 并将设备信息复制出来
	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	list_for_each(pos,&gv_sock_list_head){
		if(((sock_list*)pos)->shebei_pkid == id){
			memcpy(sock, pos, sizeof(sock_list));
			find_flag = 0;
			break;
		}
	}
	pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
	return find_flag;
}

/** 
 *@brief  根据商户id获取设备信息
 *@param  id	类型 __u32		商户id
 *@param  sock	类型 sock_list*	返回参数 设备信息
 *@param  num	类型 __u32		第几个 0.1.2.3.4.5.6
 *@return 找到 0 没有找到 -1
 */
int inline socklist_sid_call_all(__u32 id, int num, sock_list* sock){
	struct list_head* pos;
	int find_flag = -1;
	int count = 0;
	//查找发送报文的设备 并将设备信息复制出来
	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	list_for_each(pos,&gv_sock_list_head){
		if(((sock_list*)pos)->shanghuid == id){
			if(count == num){
				memcpy(sock, pos, sizeof(sock_list));
				find_flag = 0;
				break;
			}
			count++;
		}
	}
	pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
	return find_flag;
}

/** 
 *@brief  接收数据并处理 被epoll添加到线程池
 *@param  arg		类型 void*				epoll到的有可读数据的sockfd
 *@param  handle    类型 wt_sql_handle*		数据库操作资源集合
 *@return 数据库出错返回WT_SQL_ERROR 其他返回0
 */
void* msg_recv_process( void* arg, wt_sql_handle* handle )
{
	int sockfd = (int)(long)arg;
	//申请报文头空间
	msg_head_st		*msg_head = malloc( sizeof(msg_head_st) );
	memset(msg_head, 0, sizeof(sizeof(msg_head_st)));
	//接收报文头
	if( recv_msg_head(sockfd, msg_head) ){
		xyprintf(0, "SOCK_ERROR:%d %s %d -- Get router's massage head error!", sockfd, __FILE__, __LINE__);
		goto HEAD_ERR;
	}
	//xyprintf_msg_head(msg_head);
	
	//计算报文体长度
	int body_size = msg_head->len - sizeof(msg_head_st);

	if(body_size <= 0){
		goto HEAD_ERR;
	}

	//接收报文体 如果是上下线 或者回复 用包装函数接收 其他的直接接收
	void *buf;
	if( msg_head->order == msg_order_user_req || msg_head->order == msg_order_user_result ){
		buf = malloc( body_size + 1 );
		memset(buf, 0, body_size + 1 );
		if( recv_user_msg_head(sockfd, (user_msg_st*)buf, body_size ) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Get router's user massage error!", sockfd, __FILE__, __LINE__);
			goto MSG_ERR;
		}
	}
	else if(msg_head->order == msg_order_heart){
		buf = malloc( body_size );
		memset(buf, 0, body_size);
		if( recv_heart_msg_head(sockfd, (heart_msg_st*)buf, body_size ) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Get router's heart massage error!", sockfd, __FILE__, __LINE__);
			goto MSG_ERR;
		}
	}
	else if(msg_head->order == msg_order_third_qq){
		buf = malloc( body_size );
		memset(buf, 0, body_size);
		if( recv_third_qq_msg_head(sockfd, (third_qq_msg_st*)buf, body_size ) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Get router's qq massage error!", sockfd, __FILE__, __LINE__);
			goto MSG_ERR;
		}
	}
	else if(msg_head->order == msg_order_version_send){
		buf = malloc( body_size + 1 );	//多申请一个字节 做为版本号的结束符
		memset(buf, 0, body_size + 1);
		if( wt_recv_block(sockfd, buf, body_size) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Get router's version massage error!", sockfd, __FILE__, __LINE__);
			goto MSG_ERR;
		}
	}
	else if(msg_head->order == msg_order_auto_upgrade){
		buf = malloc( body_size + 1 );
		memset(buf, 0, body_size + 1 );
		if( recv_auto_upgrade_res(sockfd, (auto_upgrade_result_st*)buf, body_size ) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Get router's auto upgrade massage error!", sockfd, __FILE__, __LINE__);
			goto MSG_ERR;
		}
	}
	else if(msg_head->order == msg_order_mx_wireless_config_send){
		buf = malloc( body_size );
		memset(buf, 0, body_size);
		if(body_size != sizeof(wifi_base_conf_msg_st)){
			xyprintf(0, "DATA_ERROR:%d %s %d -- body_size != sizeof(wt_wifi_base_conf_so)!", sockfd, __FILE__, __LINE__);
			goto MSG_ERR;
		}
		if( recv_wifi_base_config(sockfd, (wifi_base_conf_msg_st*)buf ) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Get router's wireless config error!", sockfd, __FILE__, __LINE__);
			goto MSG_ERR;
		}
	}
	else if(msg_head->order == msg_order_mx_wireless_config_5g_send){
		buf = malloc( body_size );
		memset(buf, 0, body_size);
		if(body_size != sizeof(wifi_base_conf_msg_st)){
			xyprintf(0, "DATA_ERROR:%d %s %d -- body_size != sizeof(wt_wifi_base_conf_so)!", sockfd, __FILE__, __LINE__);
			goto MSG_ERR;
		}
		if( recv_wifi_base_config(sockfd, (wifi_base_conf_msg_st*)buf ) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Get router's wireless config error!", sockfd, __FILE__, __LINE__);
			goto MSG_ERR;
		}
	}
	else if(msg_head->order == msg_order_speed_get_result){
		buf = malloc( body_size );
		memset(buf, 0, body_size);
		if(body_size != sizeof(third_speed_st)){
			xyprintf(0, "DATA_ERROR:%d %s %d -- body_size != sizeof(third_speed_st)!", sockfd, __FILE__, __LINE__);
			goto MSG_ERR;
		}
		if( recv_speed_get_msg(sockfd, (third_speed_st*)buf ) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Get router's wireless config error!", sockfd, __FILE__, __LINE__);
			goto MSG_ERR;
		}
	}
	else {
		buf = malloc( body_size + 1 );
		memset(buf, 0, body_size + 1 );
		if( wt_recv_block(sockfd, buf, body_size) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Get router's other massage error!", sockfd, __FILE__, __LINE__);
			goto MSG_ERR;
		}
	}

	// 查找设备信息 并复制出来
	sock_list* sock = malloc(sizeof(sock_list));
	memset(sock, 0, sizeof(sock_list));
	if( socklist_rid_call_all(msg_head->router_id, sock) ){
		xyprintf(0, "%d:DATA_ERROR: %s %d data error!", msg_head->router_id, __FILE__, __LINE__);
		goto SOCK_ERR;
	}
	
	//处理时间测试 -- 开始时间
	struct timeval start, end;
	gettimeofday( &start, NULL );

	//处理
	switch(msg_head->order){
		case msg_order_user_req://路由器发出的用户信息，比如上下线，对应 struct user_msg_st;
		{
			user_msg_st *msg = buf;
			if(msg->cmd == user_cmd_req_on){//上线信息处理
#if ROUTER_MSG_DEBUG
				xyprintf(0, "%u: ↑ ↑ ↑ ↑  mac = %02x%02x%02x%02x%02x%02x ↑ ↑ ↑ ↑ ", msg_head->router_id, 
					msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
#endif
				__u32 shangmac_id = 0;
				__u32 mac_id = 0;

				unsigned int user_ret =
					wt_sql_user_on_first_half( handle, msg_head, msg, sock->shebei_pkid, sock->shanghuid, sock->user_manyou_time, &shangmac_id, &mac_id);

				if(user_ret & USER_IN_WHITE) {	//在白名单中 不需要认证
					msg->cmd        = user_init_pass;
					msg->result		= user_cur_unknown;
					msg->auth_len   = 0;                            // auth_addr 域的长度
					msg->speed[0]	= 0;
					msg->speed[1]	= 0;

#if ROUTER_MSG_DEBUG
					xyprintf(0, "%u: --> --> send user_msg(Res online -- pass(in white) ) mac = %02x%02x%02x%02x%02x%02x --> -->", msg_head->router_id,
							msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
#endif
					//修改数据包头
					msg_head->mode			= 2;									//方向
					msg_head->order			= msg_order_user_op;					//包体类型
					sprintf(msg->user_id, "%c|%u", USER_ID_ONLINE, shangmac_id);
						
					//xyprintf_msg_head(msg_head);
					//xyprintf_user_msg_head(msg);
					if( send_sep_user_msg_head(sock->sockfd, msg_head, msg, NULL ) ){
						xyprintf(0, "SOCK_ERROR:%d %s %d -- Send online pass message head to router is error!", sock->sockfd, __FILE__, __LINE__);
						goto SOCK_ERR;
					}
				}
				else if( (user_ret & USER_CAN_MANYOU) || (user_ret & USER_SQL_ERROR) ){
					//在漫游中 不需要认证
					//如果sql错误 也让用户上网
					msg->cmd        = user_init_pass;
					msg->result		= user_cur_unknown;
					msg->auth_len   = 0;                            // auth_addr 域的长度
					msg->speed[0]	= sock->setbw_up;
					msg->speed[1]	= sock->setbw_down;

#if ROUTER_MSG_DEBUG
					xyprintf(0, "%u: --> --> send user_msg(Res online -- pass(can manyou) ) mac = %02x%02x%02x%02x%02x%02x --> -->", msg_head->router_id,
							msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
#endif
					//修改数据包头
					msg_head->mode			= 2;									//方向
					msg_head->order			= msg_order_user_op;					//包体类型
					sprintf(msg->user_id, "%c|%u", USER_ID_ONLINE, shangmac_id);
						
					//xyprintf_msg_head(msg_head);
					//xyprintf_user_msg_head(msg);
					if( send_sep_user_msg_head(sock->sockfd, msg_head, msg, NULL ) ){
						xyprintf(0, "SOCK_ERROR:%d %s %d -- Send online pass message head to router is error!", sock->sockfd, __FILE__, __LINE__);
						if( user_ret & USER_SQL_ERROR ){
							goto SQL_ERR;
						}
						goto SOCK_ERR;
					}

					if( user_ret & USER_SQL_ERROR ){
						goto SQL_ERR;
					}
				}
				else {//需要认证
					char rj_addr[256] = {0};
					msg->cmd        = user_init_rj;
					msg->result		= user_cur_unknown;
					sprintf(rj_addr, "%s?Rid=%u&macid=%u", sgv_trezhengurl, sock->shebei_pkid, shangmac_id);         //跳转地址
					msg->auth_len   = strlen(rj_addr);								// auth_addr 域的长度

#if ROUTER_MSG_DEBUG
					xyprintf(0, "%u: --> --> send user_msg(Res online -- rj) mac = %02x%02x%02x%02x%02x%02x --> -->", msg_head->router_id,
						msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
#endif
					//修改数据包头
					msg_head->mode			= 2;									// 方向
					msg_head->order			= msg_order_user_op;					// 包体类型
					sprintf(msg->user_id, "%c|%u", USER_ID_ONLINE, shangmac_id);
					msg_head->len += msg->auth_len;
							
					//xyprintf_msg_head(msg_head);
					//xyprintf_user_msg_head(msg);
					if( send_sep_user_msg_head(sock->sockfd, msg_head, msg, rj_addr ) ){
						xyprintf(0, "SOCK_ERROR:%d %s %d -- Send online rj message to router is error!", sock->sockfd, __FILE__, __LINE__);
						goto SOCK_ERR;
					}
				}
				
				//执行上线的下半部分 修改数据库数据
				if( wt_sql_user_on_after_half(handle, sock->shebei_pkid, sock->shanghuid, sock->user_manyou_time, shangmac_id, mac_id, user_ret) ){
					goto SQL_ERR;
				}
			}
			else if(msg->cmd == user_cmd_req_off){//下线信息处理
#if ROUTER_MSG_DEBUG
				xyprintf(0, "%u: ↓ ↓ ↓ ↓  mac = %02x%02x%02x%02x%02x%02x ↓ ↓ ↓ ↓ ", msg_head->router_id,
						msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
#endif
				//xyprintf_msg_head(msg_head);
				//xyprintf_user_msg_head(msg);
						
				if ( wt_sql_user_off( handle, msg_head, msg, sock->user_manyou_time)){
					goto SQL_ERR;
				}
			}
			else {
#if ROUTER_MSG_DEBUG
				xyprintf(0, "%u: ↑ ↓ ↑ ↓ message date error! ↑ ↓ ↑ ↓ ", msg_head->router_id);
				xyprintf_msg_head(msg_head);
				xyprintf_user_msg_head(msg);
#endif
			}

			break;
		}
		case msg_order_heart://心跳包，对应 struct heart_msg_st;
		{
			heart_msg_st *msg = buf;
			big_little32(&msg->router_id);
	
			msg_head->mode = 2;

			//xyprintf_msg_head(msg_head);
			//return heart包
			if( send_heart_msg_head(sock->sockfd, msg_head, msg, sizeof(heart_msg_st)) ){
				xyprintf(0, "SOCK_ERROR:%d %s %d -- Result heart message to router is error!", sock->sockfd, __FILE__, __LINE__);
				goto SOCK_ERR;
			}
#if ROUTER_MSG_DEBUG
			//xyprintf(0, "%u: --> --> Res heart_msg success --> --> count = %u", sock->router_id, sock->heart_count );
#endif
			
			sock->heart_count ++;
		
			if( sock->heart_count == 2){			// 第二三四次心跳下发 内存缓存 usb缓存 关键词过滤
				if( send_all_web_keyword(handle, sock) ){
					goto SQL_ERR;
				}
			}
/*
			else if( sock->heart_count == 3){
				if( send_all_usb_simple_cache(handle, sock, msg_order_memory_cache_set) ){
					goto SQL_ERR;
				}
			}
			else if( sock->heart_count == 4){
				if( send_all_usb_simple_cache(handle, sock, msg_order_usb_cache) ){
					goto SQL_ERR;
				}
			}
*/

			// 每tdatasize获取设备当前流量
			if(sock->heart_count % (sgv_tdatasize * 2) == 0){
				if( send_speed_get(sock->sockfd, msg_head) ){
					xyprintf(0, "SOCK_ERROR:%d %s %d -- Get speed message to router is error!", sock->sockfd, __FILE__, __LINE__);
					goto SOCK_ERR;
				}
			}
			//每 tupdatelist次 更新数据库
			if( sock->heart_count % sgv_tupdatelist == 0 ){
				if( wt_sql_heart( handle, sock->shebei_pkid, sgv_tupdatelist, sock->user_manyou_time) ){
					goto SQL_ERR;
				}
#if ROUTER_MSG_DEBUG
				xyprintf(0, "%u: ❤ ♥ ❤ ♥  last_heart_time = %ld, user_num = %u ❤ ♥ ❤ ♥ ", sock->router_id, sock->last_heart_time, msg->user_num);
#endif
				
				//在特定的时间内获取版本号 根据版本号判断是否需要升级
				time_t now;
				struct tm *p;
				time(&now);
				p = localtime(&now);
			    //xyprintf(0, "%d%d%d %d:%d:%d", (1900+p->tm_year), (p->tm_mon + 1), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
				//if( sock->dailiid == 42 && ( p->tm_hour == 3 || p->tm_hour == 4 ) ){	//3:00:00 - 4:59:59
				if( p->tm_hour == 3 || p->tm_hour == 4 ){	//3:00:00 - 4:59:59
					if( send_version_get(sock->sockfd, msg_head) ){
						xyprintf(0, "SOCK_ERROR:%d %s %d -- Send version get message to router is error!", sock->sockfd, __FILE__, __LINE__);
						goto SQL_ERR;
					}
				}
			}

			// 更新设备信息
			socklist_sockfd_add(sock->router_id, time(NULL), 1);
		
			// 提前退出
			if(sock){
				free(sock);
			}
			if(buf){
				free(buf);
			}
			if(msg_head){
				free(msg_head);
			}
			return (void*)0;
			break;
		}
		case msg_order_user_result://路由器回复用户操作的结果，对应 struct user_msg_st;
		{
			user_msg_st *msg = buf;
#if ROUTER_MSG_DEBUG
			//xyprintf(0, "%u: ✪ ✪ ✪ ✪  mac = %02x%02x%02x%02x%02x%02x ✪ ✪ ✪ ✪ ", msg_head->router_id,
			//		msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
			//xyprintf_msg_head(msg_head);
			//xyprintf_user_msg_head(msg);
#endif
			if(msg->user_id[0] == USER_ID_SCANSQL || msg->user_id[0] == USER_ID_TAKEOUT || msg->user_id[0] == USER_ID_TIMEOUT){
				if( wt_sql_result( handle, msg_head, msg) ){
					goto SQL_ERR;
				}
			}
			break;
		}
		case msg_order_third_qq://路由器上传的qq信息
		{
			third_qq_msg_st *msg = buf;
#if ROUTER_MSG_DEBUG
			//xyprintf(0, "%u: QQQQQQQQQQ mac = %02x%02x%02x%02x%02x%02x QQ:%u, type:%u ", msg_head->router_id,
			//		msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5], msg->qq_num, msg->qq_type);
			//xyprintf_msg_head(msg_head);
			//xyprintf_third_qq_head(msg);
#endif
			if( wt_sql_third_qq( handle, msg_head, msg, sock->shebei_pkid) ){
				goto SQL_ERR;
			}
			break;
		}
/*		case msg_order_simple_cache:	//下发简单缓存返回信息
		{
			simple_cache_result_msg_st *msg = buf;
			big_little16(&msg->result);
			big_little16(&msg->len);
			if(msg->result == auto_upgrade_config_failed){
				if(msg->len){
					xyprintf(0, "%u: ❂ ❂ ❂ ❂  Simple cache config failed! result data: %s", msg_head->router_id, msg->data);
				}
				else {
					xyprintf(0, "%u: ❂ ❂ ❂ ❂  Simple cache config failed!", msg_head->router_id);
				}
			}
			else if(msg->result == auto_upgrade_config_suss){
				if(msg->len){
					xyprintf(0, "%u: ❂ ❂ ❂ ❂  Simple cache config success! result data: %s", msg_head->router_id, msg->data);
				}
				else {
					xyprintf(0, "%u: ❂ ❂ ❂ ❂  Simple cache config success!", msg_head->router_id);
				}
			}
			break;
		}
*/
		case msg_order_usb_cache_result:			// 下发usb缓存返回信息
		case msg_order_memory_cache_result:			// 下发内存缓存返回信息
		case msg_order_web_keyword_result:			// 下发关键词过滤设置返回信息
		case msg_order_url_redirect_result:			// 下发URL重定向设置返回信息
		case msg_order_mx_app_filter_result:		// 下发流媒体控制返回信息
		{
			result_msg_st* msg = buf;
			big_little16(&(msg->result));
			big_little16(&(msg->len));
#if ROUTER_MSG_DEBUG
			xyprintf_result(msg);
#endif
			break;
		}
		case msg_order_simple_gg:					// 富媒体返回报文
		case msg_order_mx_wireless_config_result:	// 下发路由器配置返回报文
		case msg_order_mx_wireless_config_5g_result:// 下发路由器配置返回报文
		case msg_order_reboot_result:				// 重启命令返回报文
		case msg_order_weixin_share_set_result:		// 微信分享命令返回报文
		case msg_order_ac_addr_result:				// 集中管理平台地址设置返回报文
		case msg_order_acac_addr_result:			// 集中管理平台地址设置返回报文
		case msg_order_wifi_monitor_result:
		{
			result_msg_st* msg = buf;
			big_little16(&(msg->result));
			big_little16(&(msg->len));
			//xyprintf_msg_head(msg_head);
			//xyprintf_result(msg);
			if(msg->result){
				xyprintf(0, "%u: ❂ ❂ ❂ ❂  Order is %u's result success!", msg_head->router_id , msg_head->order);
			}
			else {
				if(msg->len){
					xyprintf(0, "%u: ❂ ❂ ❂ ❂  Order is %u's result failed, error data(%d): %s", msg_head->router_id , msg_head->order, msg->len, msg->data);
				}
				else {
					xyprintf(0, "%u: ❂ ❂ ❂ ❂  Order is %u's result failed!", msg_head->router_id , msg_head->order);
				}
			}
			break;
		}
		case msg_order_simple_gg2_result:			// 简单广告2 下发回复
		{
			result_msg_st* msg = buf;
			big_little16(&(msg->result));
			big_little16(&(msg->len));
			//xyprintf_msg_head(msg_head);
			//xyprintf_result(msg);
			if(msg->result){
				if(msg->len){
					xyprintf(0, "%u: ❂ ❂ ❂ ❂  Order is %u's result failed, error data(%d): %s", msg_head->router_id , msg_head->order, msg->len, msg->data);
				}
				else {
					xyprintf(0, "%u: ❂ ❂ ❂ ❂  Order is %u's result failed!", msg_head->router_id , msg_head->order);
				}
			}
			else {
				xyprintf(0, "%u: ❂ ❂ ❂ ❂  Order is %u's result success!", msg_head->router_id , msg_head->order);
			}
			break;
		}
		case msg_order_usb_file_get_result:			// USB文件列表获取返回信息
		{
			result_msg_st* msg = buf;
			big_little16(&(msg->result));
			big_little16(&(msg->len));
#if ROUTER_MSG_DEBUG
			xyprintf_result(msg);
#endif
			break;
		}
		case msg_order_version_send:				// 获取版本号
		{
			router_version_msg_st* msg = buf;
			xyprintf(0, "%u: ❂ ❂ ❂ ❂  Get version success! ❂ ❂ ❂ ❂  -- %s", msg_head->router_id , msg->version);
			//更新数据库当前设备版本号
			if( wt_sql_version_refresh( handle, msg->version, sock->shebei_pkid) ){
				goto SQL_ERR;
			}

			//获取到版本号后 判断是否需要升级 但第一次获取的时候不判断 因为这是设备启动时候获取的
			if( sock->heart_count >= 1 ){
				
				int i = 0;
				pthread_mutex_lock(&gv_up_addr_lock);
				for(; i < gv_router_type_num; i++){
					
					if(sock->xinghaoid == sgv_up_addr[i].id){//根据设备型号查找最新设备版本号
						
						if( strcmp(msg->version, sgv_up_addr[i].version) ){//判断版本号是否相同 不同 则下发升级指令
							xyprintf(0, "%u: ❂ ❂ ❂ ❂  Send auto upgrade! ❂ ❂ ❂ ❂  -- %s", msg_head->router_id , sgv_up_addr[i].version);
							if( send_auto_upgrade(sock->sockfd, msg_head, sock->xinghaoid, sgv_up_addr[i].codeurl) ){
								pthread_mutex_unlock(&gv_up_addr_lock);
								xyprintf(0, "SOCK_ERROR:%d %s %d -- Send upgrade message to router is error!", sock->sockfd, __FILE__, __LINE__);
								goto SOCK_ERR;
							}
						}//end if
						
						break;
					}//end if

				}//end for

				pthread_mutex_unlock(&gv_up_addr_lock);
			}//end if
			else {
				if( sock->shebei_pkid != sock->router_id ){
					// 如果连接的不是自己的管理平台 下发修改管理平台地址
					
					// 判断是否需要下发修改acac地址的报文
					int i = 0;
					pthread_mutex_lock(&gv_up_addr_lock);
					for(; i < gv_router_type_num; i++){
						if(sock->xinghaoid == sgv_up_addr[i].id){
							
							if( sgv_up_addr[i].ishaveacserver == 1 && strcmp(msg->version, sgv_up_addr[i].version) == 0 ){
								
								// 该类型设备有acac的功能 并且是最新版本
								if( send_addr_set( sock->sockfd, msg_head, msg_order_acac_addr_set, sgv_acacserverurl, sgv_acacserverport) ){
									pthread_mutex_unlock(&gv_up_addr_lock);
									xyprintf(0, "SOCK_ERROR:%d %s %d -- Send ACAC addr change message to router is error!", sock->sockfd, __FILE__, __LINE__);
									goto SOCK_ERR;
								}

							}//end if
							
							break;
						}//end if
					}//end for
					pthread_mutex_unlock(&gv_up_addr_lock);
	
					// 修改ac地址	
					if( send_addr_set( sock->sockfd, msg_head, msg_order_ac_addr_set, sgv_acserverurl, sgv_acserverport) ){
						goto SOCK_ERR;
					}
				}
				else {
					// 判断版本号
					if( !strcmp(msg->version, "MX-60-2.06.198") ||
						!strcmp(msg->version, "MX-30-2.06.456") ||
						!strcmp(msg->version, "MX-40-2.06.459") ||
						!strcmp(msg->version, "MX-2001-2.06.460") ||
						!strcmp(msg->version, "MX-8001-2.06.461") ||
						!strcmp(msg->version, "MX-8003-2.06.462") ){
						if( send_gg2(sock->sockfd, sock->shanghuid, msg_head) ){
							xyprintf(0, "SOCK_ERROR:%d %s %d -- Send gg2 message to router is error!", sock->sockfd, __FILE__, __LINE__);
							goto SOCK_ERR;
						}
					}
					
					// 是否要下发探针配置
					if( !strncmp(msg->version, "MXP-4001", 8) ){
						int ret = send_monitor_set_st(sock->sockfd, msg_head, sock->router_id, handle);
						if( ret == WT_SQL_ERROR){
							goto SQL_ERR;
						}
						else if(ret){
							xyprintf(0, "SOCK_ERROR:%d %s %d -- Send gg2 message to router is error!", sock->sockfd, __FILE__, __LINE__);
							goto SOCK_ERR;
						}
					}
				}
			}
			break;
		}
		case msg_order_auto_upgrade:				// 自动升级返回
		{
			auto_upgrade_result_st* msg = buf;
			xyprintf(0, "%u: ❂ ❂ ❂ ❂  Get auto upgrade's result! ❂ ❂ ❂ ❂ ", msg_head->router_id);
			xyprintf_auto_upgrade_result(msg);
			if( msg->result == auto_upgrade_failed || msg->result == auto_upgrade_config_failed ){//升级操作失败 升级失败
				xyprintf(0, "%u:❂ ❂ ❂ ❂  Upgrade failed", msg_head->router_id);
				if( wt_sql_version_refresh( handle, "Upgrade failed", sock->shebei_pkid) ){
					goto SQL_ERR;
				}
			}
			else if( msg->result == auto_upgrade_config_suss ){		//升级操作成功
				xyprintf(0, "%u:❂ ❂ ❂ ❂  Upgrading...", msg_head->router_id);
				if( wt_sql_version_refresh( handle, "Upgrading...", sock->shebei_pkid) ){
					goto SQL_ERR;
				}
			}
			else if( msg->result == auto_upgrade_suss ){		//升级成功
				xyprintf(0, "%u:❂ ❂ ❂ ❂  Upgrade succ", msg_head->router_id);
				if( wt_sql_version_refresh( handle, "Upgrade succ", sock->shebei_pkid) ){
					goto SQL_ERR;
				}
			}
			break;
		}
		case msg_order_mx_wireless_config_send://获取路由器配置返回报文
		{
			wifi_base_conf_msg_st* msg = buf;
			//xyprintf(0, "%u: ❂ ❂ ❂ ❂  Get router's config result! ❂ ❂ ❂ ❂ ", msg_head->router_id );
			//xyprintf_wt_wifi_base_conf_so(msg);
			int ret = wt_sql_wireless_config(handle, msg_head, msg, sock->shebei_pkid, sock->shanghuid, 0);
			if(ret == -1){
				goto SQL_ERR;
			}
			else if (ret == 1){//发生更改 需要重新下发
				xyprintf(0, "%u: ❂ ❂ ❂ ❂  Router's wireless config has change! ❂ ❂ ❂ ❂ ", msg_head->router_id );
				msg_head->order = msg_order_mx_wireless_config_set;
				msg_head->mode = 2;
#if ROUTER_MSG_DEBUG
				//xyprintf_msg_head(msg_head);
				//xyprintf_wt_wifi_base_conf_so(msg);
#endif
				xyprintf(0, "%u: --> --> send change wireless config to router --> -->", msg_head->router_id);
				if( send_wifi_base_config(sock->sockfd, msg_head, msg) ){
					xyprintf(0, "SOCK_ERROR:%d %s %d -- Send change wireless config message to router is error!", sock->sockfd, __FILE__, __LINE__);
					goto SOCK_ERR;
				}
			}
			else{
				xyprintf(0, "%u: ❂ ❂ ❂ ❂  Router's wireless config don't change! ❂ ❂ ❂ ❂ ", msg_head->router_id );
			}
			break;
		}
		case msg_order_mx_wireless_config_5g_send://获取路由器配置返回报文
		{
			wifi_base_conf_msg_st* msg = buf;
			//xyprintf(0, "%u: ❂ ❂ ❂ ❂  Get router's config result! ❂ ❂ ❂ ❂ ", msg_head->router_id );
			//xyprintf_wt_wifi_base_conf_so(msg);
			int ret = wt_sql_wireless_config(handle, msg_head, msg, sock->shebei_pkid, sock->shanghuid, 1);
			if(ret == -1){
				goto SQL_ERR;
			}
			else if (ret == 1){//发生更改 需要重新下发
				xyprintf(0, "%u: ❂ ❂ ❂ ❂  Router's 5G wireless config has change! ❂ ❂ ❂ ❂ ", msg_head->router_id );
				msg_head->order = msg_order_mx_wireless_config_5g_set;
				msg_head->mode = 2;
#if ROUTER_MSG_DEBUG
				//xyprintf_msg_head(msg_head);
				//xyprintf_wt_wifi_base_conf_so(msg);
#endif
				xyprintf(0, "%u: --> --> send change 5G wireless config to router --> -->", msg_head->router_id);
				if( send_wifi_base_config(sock->sockfd, msg_head, msg) ){
					xyprintf(0, "SOCK_ERROR:%d %s %d -- Send change 5G wireless config message to router is error!", sock->sockfd, __FILE__, __LINE__);
					goto SOCK_ERR;
				}
			}
			else{
				xyprintf(0, "%u: ❂ ❂ ❂ ❂  Router's 5G wireless config don't change! ❂ ❂ ❂ ❂ ", msg_head->router_id );
			}
			break;
		}
		case msg_order_speed_get_result:
		{
			third_speed_st* msg = buf;
			//xyprintf_speed_st(msg);
			if( wt_sql_speed_insert(handle, msg, sock->shebei_pkid, sock->shanghuid) ){
				goto SQL_ERR;
			}
			break;
		}
		case msg_order_httpd_pwd_send:
		{
			httpd_pwd_st* msg = buf;
			xyprintf_http_pw_st(msg);
			break;
		}
		default:
		{
			xyprintf(0, "%u: ↑ ↓ ♥ ✪  Message date error! ↑ ↓ ❤ ✪ ", msg_head->router_id);
			xyprintf_msg_head(msg_head);
			if(msg_head->len - sizeof(msg_head_st) > 6){
				char* msg = buf;
				xyprintf(0, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", msg[0], msg[1], msg[2], msg[3], msg[4], msg[5]);
			}
			break;
		}
	}//end switch

	//处理时间 -- 结束时间
	gettimeofday( &end, NULL );
	if( end.tv_sec - start.tv_sec > 2 ){
		xyprintf(0, "SQL_TIME_WARNING:Msg Order is 0x%x, Spend time %d us!", msg_head->order, 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec -start.tv_usec);
	}

	// 更新设备信息
	socklist_sockfd_add(sock->router_id, time(NULL), 0);
	if(sock){
		free(sock);
	}
	if(buf){
		free(buf);
	}
	if(msg_head){
		free(msg_head);
	}
	return (void*)0;
SQL_ERR:
	socklist_sockfd_add(sock->router_id, time(NULL), 0);
	if(sock){
		free(sock);
	}
	if(buf){
		free(buf);
	}
	if(msg_head){
		free(msg_head);
	}
	return (void*)WT_SQL_ERROR;

SOCK_ERR:
	if(sock){
		free(sock);
	}
MSG_ERR:
	if(buf){
		free(buf);
	}
HEAD_ERR:
	if(msg_head){
		free(msg_head);
	}
	socklist_sockfd_err(sockfd);
	return (void*)0;
}

/** 
 *@brief  设备下线处理函数 在epoll之前被添加到线程池
 *@param  arg		类型 void*				要关闭设备的sock_list
 *@param  handle    类型 wt_sql_handle*		数据库操作资源集合
 *@return 数据库出错返回WT_SQL_ERROR 其他返回0
 */
void* router_offline_fun( void* arg, wt_sql_handle* handle )
{
	pthread_detach(pthread_self());
	sock_list* sock	= (sock_list*)arg;

	xyprintf(0, "%d: ╮ ( ╯ ▽ ╰ ) ╭ Router offline thread is run in %lu !!!", sock->router_id, pthread_self() );

	//修改设备表记录
#ifdef ADVERTISING
	sprintf(handle->sql_str, "UPDATE mx_shebei SET lastdotime = GETDATE(), \
			doingtime = doingtime + DATEDIFF(n, lastdotime, GETDATE()) WHERE id = %u",
			sock->shebei_pkid);
#else
	sprintf(handle->sql_str, "UPDATE mx_shebei SET lastdotime = GETDATE(),lastsetdoingtime = GETDATE(), isonlien = 0, \
			doingtime = doingtime + DATEDIFF(n, lastdotime, GETDATE()) WHERE id = %u",
			sock->shebei_pkid);
#endif
	if(wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto STR_ERR;
	}
	//更新设备表记录表do时间
#ifdef ADVERTISING
	sprintf(handle->sql_str, "UPDATE mx_shebei_log_last SET zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), \
			lastdotime = GETDATE(), islast = 0 WHERE rid = %u AND islast = 1",
			sock->shebei_pkid);
#else
	sprintf(handle->sql_str, "UPDATE mx_shenbei_log_last SET zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), \
			lastdotime = GETDATE(), islast = 0 WHERE rid = %u AND islast = 1",
			sock->shebei_pkid);
#endif
	if(wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto STR_ERR;
	}
	//更新用户mac表do时间
#ifdef ADVERTISING
	sprintf(handle->sql_str, "UPDATE mx_maclist SET zonlinetime = zonlinetime + DATEDIFF(n, lastdotime, GETDATE()), \
			lastdotime = GETDATE(), isonline = 0 \
			WHERE lastrid = %u AND isonline = 1",
			sock->shebei_pkid);
#else
	sprintf(handle->sql_str, "UPDATE mx_maclist SET zonlinetime = zonlinetime + DATEDIFF(n, lastdotime, GETDATE()), \
			lastdotime = GETDATE(), lastsetdoingtime = GETDATE(), isonline = 0 \
			WHERE lastrid = %u AND isonline = 1",
			sock->shebei_pkid);
#endif
	if(wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto STR_ERR;
	}
	//更新shang maclist表do时间
#ifdef ADVERTISING
	sprintf(handle->sql_str, "UPDATE mx_shanghu_maclist SET zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), \
			lastdotime = GETDATE(), isonline = 0, outmanyou = DATEADD(n, %u, GETDATE() ) \
			WHERE id IN ( SELECT id FROM mx_shanghu_maclist WHERE lastrid = %u AND isonline = 1)",
		sock->user_manyou_time, sock->shebei_pkid);
#else
	sprintf(handle->sql_str, "UPDATE mx_shang_maclist SET zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), \
			lastdotime = GETDATE(), lastsetdoingtime = GETDATE(), isonline = 0, ispop = 0, outmanyou = DATEADD(n, %u, GETDATE() ) \
			WHERE id IN ( SELECT id FROM mx_shang_maclist WHERE lastrid = %u AND isonline = 1)",
		sock->user_manyou_time, sock->shebei_pkid);
#endif
	if(wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto STR_ERR;
	}
	//更新用户上网记录表do时间
#ifdef ADVERTISING
	sprintf(handle->sql_str, "UPDATE mx_mac_log_last SET zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), \
			lastdotime = GETDATE(), islast = 0 WHERE id IN( SELECT id FROM mx_mac_log_last WHERE rid = %u AND islast = 1)",
			sock->shebei_pkid);
#else
	sprintf(handle->sql_str, "UPDATE mx_mac_log_last SET zongtime = zongtime + DATEDIFF(n, lastdotime, GETDATE()), \
			lastdotime = GETDATE(),lastsetdoingtime = GETDATE(), islast = 0 WHERE id IN( SELECT id FROM mx_mac_log_last WHERE rid = %u AND islast = 1)",
			sock->shebei_pkid);
#endif
	if(wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto STR_ERR;
	}
	
	wt_close_sock( &(sock->sockfd) );								//关闭socket
	free( sock );
	xyprintf(0, "╮ ( ╯ ▽ ╰ ) ╭ %lu's router offline thread is over!!!", pthread_self());
	return (void*)0;
STR_ERR:
	wt_close_sock( &(sock->sockfd) );								//关闭socket
	free( sock );
	xyprintf(0, "╮ ( ╯ ▽ ╰ ) ╭ %lu's router offline thread is over, has error!!!", pthread_self() );
	return (void*)WT_SQL_ERROR;
}

/** 
 *@brief  epoll线程
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* router_epoll_thread(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Epoll thread is running!!!");

	int ret, i;
	struct list_head *pos;	//设备list遍历变量
	
	int epfd = epoll_create( MAX_EPOLL_NUM );		//创建epoll
	if(epfd == -1){
		xyprintf(errno, "EPOLL_ERROR: %s %d -- epoll_create()", __FILE__, __LINE__);
		goto CREATE_ERR;
	}
	
	struct epoll_event *events;
	events = (struct epoll_event*)malloc (1024 * sizeof(struct epoll_event) );	//一次最多获得1024条结果
	memset(events, 0, 1024 * sizeof(struct epoll_event));

	while(1){
		pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
		for( pos = gv_sock_list_head.next; pos != &gv_sock_list_head; pos = pos->next ){//遍历设备list 添加或删除设备到epoll队列中

			if( time(0) - ((sock_list*)pos)->last_heart_time > 60){
				((sock_list*)pos)->stat = SOCK_STAT_DEL;
			}
			else if( ((sock_list*)pos)->stat == SOCK_STAT_ADD ){		//需要添加的设备
				struct epoll_event event;
				event.data.fd = ((sock_list*)pos)->sockfd;
				event.events = EPOLLIN | EPOLLRDHUP;
				ret = epoll_ctl(epfd, EPOLL_CTL_ADD, ((sock_list*)pos)->sockfd, &event);
				if(ret == -1){
					xyprintf(errno, "EPOLL_ERROR: %s %d -- epoll_ctl()", __FILE__, __LINE__);
					((sock_list*)pos)->stat = SOCK_STAT_DEL;
				}
				else {
					((sock_list*)pos)->stat = SOCK_STAT_ADDED;
				}
			}
		
			if( ((sock_list*)pos)->stat == SOCK_STAT_DEL ) {
				struct epoll_event event;
				event.data.fd = ((sock_list*)pos)->sockfd;
				event.events = EPOLLIN | EPOLLRDHUP;
				ret = epoll_ctl(epfd, EPOLL_CTL_DEL, ((sock_list*)pos)->sockfd, &event);	//从监听列表中删除
				if(ret == -1){
					xyprintf(errno, "EPOLL_ERROR: %s %d -- epoll_ctl()", __FILE__, __LINE__);
				}
				
				//从链表中删除
				struct list_head *temp = pos;
				pos = temp->prev;
				list_del( temp );
				gv_sock_count--;
				xyprintf(0, "☠ ☠ ☠ ☠  -- router_id is %u, sock_count = %d", ((sock_list*)temp)->router_id, gv_sock_count);
			
				pool_add_worker( router_offline_fun, (void*)temp );	//添加到接收并处理任务队列
			}
		}
		pthread_mutex_unlock( &gv_list_lock );//⊙﹏⊙

		ret = epoll_wait(epfd, events, 1024, 100);
		for(i = 0; i < ret; i++){
			if( (events[i].events & EPOLLRDHUP ) || (events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || 
					( !(events[i].events & EPOLLIN) ) ){ //判断是否出错
				socklist_sockfd_err(events[i].data.fd);
			}
			else {
				if(events[i].data.fd > 0){
					//msg_recv(events[i].data.fd);
					struct epoll_event event;
					event.data.fd = events[i].data.fd;
					event.events = EPOLLIN | EPOLLRDHUP;
					ret = epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, &event);	//从监听列表中删除
					if(ret == -1){
						xyprintf(errno, "EPOLL_ERROR: %s %d -- epoll_ctl()", __FILE__, __LINE__);
					}
					pool_add_worker( msg_recv_process, (void*)(long) (events[i].data.fd) );	//添加到接收并处理任务队列
				}
				else {
					xyprintf(0,"EPOLL_ERROR:%s %d -- fd is %d", __FILE__, __LINE__, events[i].data.fd);
				}
			}
		}
	}
CTL_ERR:
	close(epfd);
	free(events);
CREATE_ERR:
	xyprintf(0, "EPOLL_ERROR:%s %d -- Epoll pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  设备连接认证函数 在accept_fun内被添加
 *@param  fd		类型 void*				连接设备的sockfd
 *@param  handle    类型 wt_sql_handle*		数据库操作资源集合
 *@return 数据库出错返回WT_SQL_ERROR 其他返回0
 */
void* authenticate_process(void *fd, wt_sql_handle *handle)
{
	xyprintf(0, "O(∩ _∩ )O ~~ A new router request Authenticate!!!");

	int				client_sockfd	= (int)(long)fd;
	random_pair		random_num;
	
	msg_head_st		*msg_head	= (msg_head_st*)malloc( sizeof(msg_head_st) );
	memset(msg_head, 0, sizeof(msg_head_st));

	//接收第一包数据
	if( recv_msg_head(client_sockfd, msg_head) ){//获取数据包头
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Recv router's massage head error!", client_sockfd, __FILE__, __LINE__);
		goto HEAD_ERR;
	}

	//判断
	if( msg_head->mode != 1 || msg_head->order != msg_order_auth || msg_head->len < sizeof(msg_head_st) || msg_head->ver != 4){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Router's message head date error!", client_sockfd, __FILE__, __LINE__);
		xyprintf_msg_head(msg_head);
		goto HEAD_ERR;
	}
	
	//取包体长度
	int body_size = msg_head->len - sizeof(msg_head_st);
	cer_msg_st		*msg	= malloc( body_size );
	
	if(msg == NULL){
		xyprintf(0,"ROUTER_ERROR: %s %d malloc error!", __FILE__, __LINE__);
		goto HEAD_ERR;
	}
	memset(msg, 0, body_size);

	//接收包体
	if( recv_cer_msg_head(client_sockfd, msg, body_size) ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Recv router's cer massage error!", client_sockfd, __FILE__, __LINE__);
		goto MSG_ERR;
	}
	//判断
	if(msg->order != 1 || msg->srv_type != srv_type_auth){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Router's cer message date error!", client_sockfd, __FILE__, __LINE__);
		xyprintf_cer_msg_head(msg);
		goto MSG_ERR;
	}
	//xyprintf(0, "%u: <-- <-- Recv frist cer_msg success <-- <--", msg_head->router_id );

	//保存随机码 并回复第一包数据
	random_num.router_seq	= msg->router_seq;
	random_num.srv_seq		= random_num.router_seq << 6;
	msg->srv_seq			= random_num.srv_seq;
	msg->order++;
	msg_head->mode			= 2;
	//发送回复包
	if( send_cer_msg_head(client_sockfd, msg_head, msg, body_size) ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Send cer message to router is error!", client_sockfd, __FILE__, __LINE__);
		goto MSG_ERR;
	}
	//xyprintf(0, "%u: --> --> Res frist cer_msg success --> -->", rid);

	//接收第二包数据
	if( recv_msg_head(client_sockfd, msg_head) ){//获取数据包头
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Recv router's message head error!", client_sockfd, __FILE__, __LINE__);
		goto MSG_ERR;
	}
	//判断	
	if( msg_head->mode != 1 || msg_head->order != msg_order_auth ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Router's message head data error!", client_sockfd, __FILE__, __LINE__);
		xyprintf_msg_head(msg_head);
		goto MSG_ERR;
	}
	
	//取包体长度
	body_size = msg_head->len - sizeof(msg_head_st);
	msg			= (cer_msg_st*)realloc( msg, body_size );
	
	if(msg == NULL){
		xyprintf(0,"ROUTER_ERROR: %s %d realloc error!", __FILE__, __LINE__);
		goto MSG_ERR;
	}
	
	//收包体
	if( recv_cer_msg_head(client_sockfd, msg, body_size) ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Recv router's cer message error!", client_sockfd, __FILE__, __LINE__);
		goto MSG_ERR;
	}
	
	//判断
	if( msg->order != 3 || msg->srv_type != srv_type_auth ||
		msg->router_seq != random_num.router_seq || msg->srv_seq != random_num.srv_seq ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Router's cer message data error!", client_sockfd, __FILE__, __LINE__);
		xyprintf_cer_msg_head(msg);
		goto MSG_ERR;
	}

	//xyprintf(0, "%u: <-- <-- Recv second cer_msg success <-- <--", rid);

	//3.2绑定变量和SQL语句查询结果
	__u32 id = 0, rid = 0, shanghuid = 0, isok = 0, xinghaoid = 0, setbw_up = 0, setbw_down = 0, iscandoing = 0, dailiid = 0;
	char driverversion[64] = { 0 };
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, &id,				20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2, SQL_C_ULONG, &rid,				20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3, SQL_C_ULONG, &shanghuid,		20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 4, SQL_C_ULONG, &isok,			20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 5, SQL_C_CHAR,  driverversion,	64, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 6, SQL_C_ULONG, &xinghaoid,		20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 7, SQL_C_ULONG, &setbw_up,		20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 8, SQL_C_ULONG, &setbw_down,		20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 9, SQL_C_ULONG, &iscandoing,		20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle,10, SQL_C_ULONG, &dailiid,			20, &handle->sql_err);

	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT TOP 1 id, rid, shanghuid, isok, driverversion, xinghaoid, \
			setbw_up, setbw_down, iscandoing, dailiid FROM mx_shebei WHERE Hard_seq = '%s'", msg->hard_seq);
	if(wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	if(handle->sql_ret == SQL_NO_DATA){
		xyprintf(0, "ROUTER_ERROR:%s %d -- Not has router's message in datebase!", __FILE__, __LINE__);
		goto SQLED_ERR;
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标//清空上次执行的数据 有数据会返回0 没有数据100 出错-1 这并不是一个好做法 应该还有其他的解决方法

	if( rid != msg_head->router_id ){
		xyprintf(0, "ROUTER_ERROR:%s %d -- Router's id is error!",	__FILE__, __LINE__);
		goto SQLED_ERR;
	}
	if( isok == 0 ){
		xyprintf(0, "ROUTER_ERROR:%s %d -- Router's isok status is error!",	__FILE__, __LINE__);
		goto SQLED_ERR;
	}
	if( iscandoing == 0 ){
		xyprintf(0, "ROUTER_ERROR:%s %d -- Router's iscandoing status is error!",	__FILE__, __LINE__);
		goto SQLED_ERR;
	}
	if( shanghuid == 0 ){
		xyprintf(0, "ROUTER_ERROR:%s %d -- Router's shanghuid is error!",  __FILE__, __LINE__);
		goto SQLED_ERR;
	}

	//从商户表查询漫游时间的设定值 和 是否开启的认证方式 和 单独的白名单域名
	char	gotodomain[1024] = {0};
	unsigned int reusermanyou = 0;
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG,	&reusermanyou,	20,		&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2, SQL_C_CHAR,	gotodomain,		1024,	&handle->sql_err);
#ifdef ADVERTISING
	sprintf(handle->sql_str, "SELECT TOP 1 reusermanyou, gotodomain FROM mx_shanghu WHERE id = %u", shanghuid);
#else
	char	retype[128] = {0};
	__u8 hycardiswx = 0;
	__u8 ishycard = 0;
	SQLBindCol(handle->sqlstr_handle, 3, SQL_C_CHAR,	retype + 1,		120,	&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 4, SQL_C_ULONG,	&hycardiswx,	20,		&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 5, SQL_C_ULONG,	&ishycard,		20,		&handle->sql_err);
	sprintf(handle->sql_str, "SELECT TOP 1 reusermanyou, gotodomain, retype, hycardiswx, ishycard FROM mx_shanghu WHERE id = %u", shanghuid);
#endif
	if(wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	if(handle->sql_ret == SQL_NO_DATA){
		xyprintf(0, "ROUTER_ERROR:%s %d -- Not has shanghu's data in datebase!", __FILE__, __LINE__);
		goto SQLED_ERR;
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	//判断用户是否开启微信 qq weibo 认证
	__u8 ios_authenticate		= 0;
	__u8 weixin_authenticate	= 0;
	__u8 qq_authenticate		= 0;
	__u8 sina_authenticate		= 0;
#ifdef ADVERTISING
#else
	retype[0] = ',';
	retype[ strlen(retype) ] = ',';
	//xyprintf(0, "retype is %s", retype);
	// 判断是否开启微信认证 或者微信分享 或者开启了会员功能并且会员开启了微信认证并且开启了用户名挡路
	if( strstr(retype, ",2,") || strstr(retype, ",19,") || ( hycardiswx && ishycard && strstr(retype, ",3,") ) ){
		weixin_authenticate = 1;
	}
	//判断是否开启qq认证
	if(	strstr(retype, ",5,") ){
		qq_authenticate = 1; 
	}
	//判断是否开启新浪微博认证
	if( strstr(retype, ",6,") ){
		sina_authenticate = 1;
	}
	//判断是否开启摇一摇
	if( strstr(retype, ",21,") ){
		ios_authenticate = 1;
	}
#endif

	//查询商户开启的其他白名单
	unsigned int isqqdm = 0, isweixindm = 0, isweibodm = 0;
	char otherdm[2048] = { 0 };
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG,	&isqqdm,		20,		&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2, SQL_C_ULONG,	&isweixindm,	20,		&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3, SQL_C_ULONG,	&isweibodm,		20,		&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 4, SQL_C_CHAR,	otherdm,		2048,	&handle->sql_err);
	sprintf(handle->sql_str, "SELECT TOP 1 isqqdm, isweixindm, isweibodm, otherdm FROM mx_shanghu_domain WHERE shanghuid = %u", shanghuid);
	//xyprintf(0, "%s", handle->sql_str);
	if(wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}

	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	if(handle->sql_ret != SQL_NO_DATA){
		if(isqqdm){
			qq_authenticate = 1; 
		}
		if(isweixindm){
			weixin_authenticate = 1;
		}
		if(isweibodm){
			sina_authenticate = 1;
		}
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	//xyprintf(0, "gotodomain is %s\n\t\totherdm is %s", gotodomain, otherdm);


	//下发无线设置 如果设备有无线的话
	int i = 0;
	pthread_mutex_lock(&gv_up_addr_lock);
	for(; i < gv_router_type_num; i++){
		if(xinghaoid == sgv_up_addr[i].id){//根据设备型号查找最新设备版本号
			if( sgv_up_addr[i].ishavewifi ){//判断版本号是否相同
				if( send_wireless_config_get(client_sockfd, msg_head, msg_order_mx_wireless_config_get) ){
					pthread_mutex_unlock(&gv_up_addr_lock);
					goto SQLED_ERR;
				}

				if( !strncmp(sgv_up_addr[i].version, "MX-980", 6) ){
					if( send_wireless_config_get(client_sockfd, msg_head, msg_order_mx_wireless_config_5g_get) ){
						pthread_mutex_unlock(&gv_up_addr_lock);
						goto SQLED_ERR;
					}
				}
			}
			break;
		}
	}
	pthread_mutex_unlock(&gv_up_addr_lock);

	//发送白名单 分是否开启微信认证 或微信分享
	if( send_white(client_sockfd, ios_authenticate, weixin_authenticate, qq_authenticate, sina_authenticate, msg_head, gotodomain, otherdm) ){
		goto SQLED_ERR;
	}

	//下发获取版本号
	if( send_version_get(client_sockfd, msg_head) ){
		goto SQLED_ERR;
	}
	
	// 获取登录名和密码 测试
//	if( send_http_passwd_get(client_sockfd, msg_head) ){
//		goto SQLED_ERR;
//	}

	//下发富媒体设置
	if( send_gg(client_sockfd, shanghuid, msg_head) ){
		goto SQLED_ERR;
	}

	if ( weixin_authenticate ){
		// 如果开启了微信认证 下发微信认证设置报文
		if( send_weixin_share_set( client_sockfd, msg_head, id) ){
			goto SQLED_ERR;
		}
	}
	

	//将设备的上次的在线记录的islast修改为0
#ifdef ADVERTISING
	sprintf(handle->sql_str, "UPDATE mx_shebei_log_last SET islast = 0 WHERE rid = %u and islast = 1", id);
#else
	sprintf(handle->sql_str, "UPDATE mx_shenbei_log_last SET islast = 0 WHERE rid = %u and islast = 1", id);
#endif
	if(wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	
	//添加新的设备上线记录
#ifdef ADVERTISING
	sprintf( handle->sql_str, "INSERT INTO mx_shebei_log_last (shanghuid, rid, zongtime, lastdotime, createdate, islast) \
			VALUES (%u, %u, 0, GETDATE(), GETDATE(), 1)", shanghuid, id );
#else
	sprintf( handle->sql_str, "INSERT INTO mx_shenbei_log_last (shanghuid, rid, zongtime, lastdotime, createdate, islast) \
			VALUES (%u, %u, 0, GETDATE(), GETDATE(), 1)", shanghuid, id );
#endif
	if(wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	
	//更新用户mac表do时间
	sprintf(handle->sql_str, "UPDATE mx_maclist SET isonline = 0 WHERE isonline = 1 AND lastrid = %u", id);
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}

	//更新shang maclist表do时间
#ifdef ADVERTISING
	sprintf(handle->sql_str, "UPDATE mx_shanghu_maclist SET isonline = 0 WHERE id IN \
			( SELECT id FROM mx_shanghu_maclist WHERE isonline = 1 AND lastrid = %u)", id);
#else
	sprintf(handle->sql_str, "UPDATE mx_shang_maclist SET isonline = 0, ispop = 0 WHERE id IN \
			( SELECT id FROM mx_shang_maclist WHERE isonline = 1 AND lastrid = %u)", id);
#endif
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	
	//更新设备表
#ifdef ADVERTISING
	sprintf(handle->sql_str, "UPDATE mx_shebei SET isonlien = 1, lastdotime = GETDATE(), reservid = %u WHERE id = %u", gv_auth_code, id);
#else
	sprintf(handle->sql_str, "UPDATE mx_shebei SET isonlien = 1, lastdotime = GETDATE(), \
			lastsetdoingtime = GETDATE(), reservid = %u WHERE id = %u", gv_auth_code, id);
#endif
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}

	struct list_head *pos;	//设备list遍历变量
	int find_flag = 0;
	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	for( pos = gv_sock_list_head.next; pos != &gv_sock_list_head; pos = pos->next ){//遍历设备list 添加或删除设备到epoll队列中
		if( ((sock_list*)pos)->shebei_pkid == id){
			find_flag = 1;
			xyprintf(0, "♨ ♨ ♨ ♨  -- sockfd is %d, router_id is %u, sock_count = %d", client_sockfd, msg_head->router_id, gv_sock_count);
			
			sock_list *sock_node = (sock_list*)pos;
		
			wt_close_sock( &(sock_node->sockfd) );						//关闭原来的socket

			sock_node->stat			= SOCK_STAT_ADD;
			sock_node->sockfd		= client_sockfd;
			sock_node->ver			= msg_head->ver;					//版本,初始为1; 添加 third_id 后改为2, 目前为4 2.3没有设备序列号
			sock_node->heart_count	= 0;								//心跳计数器	
			sock_node->sec_flag		= msg_head->sec_flag;				//数据加密方式,0: 不加密；其它可扩展
			sock_node->router_id	= msg_head->router_id;
			memcpy( sock_node->hard_seq, msg_head->hard_seq, 64);		//设备序列号
			//sock_node->shebei_pkid	= id;
			sock_node->shanghuid	= shanghuid;
			sock_node->dailiid		= dailiid;
			time( &( sock_node->last_heart_time ) );
			sock_node->xinghaoid	= xinghaoid;
			sock_node->setbw_up		= setbw_up;
			sock_node->setbw_down	= setbw_down;
	
			sock_node->weixin_flag  = weixin_authenticate;
			sock_node->ios_flag		= ios_authenticate;
			sock_node->qq_flag		= qq_authenticate;
			sock_node->sina_flag	= sina_authenticate;
#ifdef ADVERTISING
				sock_node->hycardiswx  = 0;
				sock_node->ishycard  = 0;
#else
			// 会员卡其否启用微信关注
			if(hycardiswx){
				sock_node->hycardiswx  = 1;
			}
			else {
				sock_node->hycardiswx  = 0;
			}
		
			// 是否启用会员卡
			if(ishycard){
				sock_node->ishycard  = 1;
			}
			else {
				sock_node->ishycard  = 0;
			}
#endif

			//商户设置了漫游时间 就使用商户的 商户没有设置 就使用默认的
			if( reusermanyou ){
				sock_node->user_manyou_time = reusermanyou;
			}
			else {
				sock_node->user_manyou_time = sgv_tfree_manyou_time;
			}
			break;
		}
	}
	pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙








	if( !find_flag ){
		//认证完毕 添加到连接设备链表
		sock_list *sock_node = malloc(sizeof(sock_list));
		memset(sock_node, 0, sizeof(sizeof(sock_list)));
	
		sock_node->stat			= SOCK_STAT_ADD;
		sock_node->sockfd		= client_sockfd;
		sock_node->ver			= msg_head->ver;					//版本,初始为1; 添加 third_id 后改为2, 目前为4 2.3没有设备序列号
		sock_node->heart_count	= 0;								//心跳计数器	
		sock_node->sec_flag		= msg_head->sec_flag;				//数据加密方式,0: 不加密；其它可扩展
		sock_node->router_id	= msg_head->router_id;
		memcpy( sock_node->hard_seq, msg_head->hard_seq, 64);		//设备序列号
		sock_node->shebei_pkid	= id;
		sock_node->shanghuid	= shanghuid;
		sock_node->dailiid		= dailiid;
		time( &( sock_node->last_heart_time ) );
		sock_node->xinghaoid	= xinghaoid;
		sock_node->setbw_up		= setbw_up;
		sock_node->setbw_down	= setbw_down;
	
		sock_node->weixin_flag  = weixin_authenticate;
		sock_node->ios_flag		= ios_authenticate;
		sock_node->qq_flag		= qq_authenticate;
		sock_node->sina_flag	= sina_authenticate;

#ifdef ADVERTISING
			sock_node->hycardiswx  = 0;
			sock_node->ishycard  = 0;
#else
		// 会员卡其否启用微信关注
		if(hycardiswx){
			sock_node->hycardiswx  = 1;
		}
		else {
			sock_node->hycardiswx  = 0;
		}
		
		// 是否启用会员卡
		if(ishycard){
			sock_node->ishycard  = 1;
		}
		else {
			sock_node->ishycard  = 0;
		}
#endif
		//商户设置了漫游时间 就使用商户的 商户没有设置 就使用默认的
		if( reusermanyou ){
			sock_node->user_manyou_time = reusermanyou;
		}
		else {
			sock_node->user_manyou_time = sgv_tfree_manyou_time;
		}
	
		pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙

		list_add(&(sock_node->node), &gv_sock_list_head);
		gv_sock_count++;
		xyprintf(0, "♨ ♨ ♨ ♨  -- sockfd is %d, router_id is %u, sock_count = %d", client_sockfd, msg_head->router_id, gv_sock_count);
		pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
	}

	xyprintf(0, "%u: O(∩ _∩ )O ~~ Authenticate is over!!!", msg_head->router_id);
	if(msg_head){
		free( msg_head );
	}
	if(msg){
		free( msg );
	}
	return (void*)0;

SQLED_ERR:
	if(msg){
		free( msg );
	}
	if(msg_head){
		free( msg_head );
	}
	wt_close_sock( &client_sockfd );
	return (void*)WT_SQL_ERROR;
MSG_ERR:
	if(msg){
		free( msg );
	}
HEAD_ERR:
	if(msg_head){
		free( msg_head );
	}
ERR:
	wt_close_sock( &client_sockfd );
	xyprintf(0, "ROUTER_ERROR:%s %d -- Authenticate pthread is unnatural deaths!!!", __FILE__, __LINE__);
	return (void*)0;
} 

/** 
 *@brief  accept线程
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* router_conn_thread(void* fd)
{
	pthread_detach( pthread_self() );
	xyprintf(0, "** O(∩ _∩ )O ~~ Accept thread is running!!!");
	int sockfd;
	unsigned int accpet_close_count  = 0;
	if( wt_sock_init( &sockfd, cgv_authenticate_port, MAX_EPOLL_NUM ) ){
		goto ERR;
	}
	struct sockaddr_in client_address;//存放客户端地址信息
	int client_len = sizeof( client_address );
	while(1){
		int client_sockfd;
		client_sockfd = accept(sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);  
		if(client_sockfd == -1){
			xyprintf(errno, "SOCK_ERROR:%d %s %d -- accept()", sockfd, __FILE__, __LINE__);
			continue;
		}

		if( get_cur_queue_size() > 100 ){
			xyprintf(0, "benben: accpet_close_count = %u", ++accpet_close_count);
			close(client_sockfd);
		}
		else {
			xyprintf(0, "♨ ♨ ♨ ♨  -- router %s connection, sockfd is %d", inet_ntoa( client_address.sin_addr) , client_sockfd);
			pool_add_worker( authenticate_process, (void*)((long)client_sockfd));
		}
	}
ERR:
	close( sockfd );
	xyprintf(0, "✟ ✟ ✟ ✟ -- %s %d:Accept pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit( NULL );
}

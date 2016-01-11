/*****************************************************
 *
 * 服务器之间交互
 *
 *****************************************************/

#include "auth_header.h"

#define SERVER_MUTUAL_DEBUG		0

/** 
 *@brief  重新读取升级地址
 *@return succ 0 failed -1
 */
void* change_upurl(void* arg, wt_sql_handle* handle)
{
	if( wt_sql_get_upurl(handle) ){
		return (void*)WT_SQL_ERROR;
	}
	return (void*)0;
}

/** 
 *@brief  修改其他全局配置
 *@return succ 0 failed -1
 */
void* change_other(void* arg, wt_sql_handle* handle)
{
	if( wt_sql_get_other(handle) ){
		return (void*)WT_SQL_ERROR;
	}
	if( wt_sql_get_weixin(handle) ){
		return (void*)WT_SQL_ERROR;
	}
	return (void*)0;
}

/** 
 *@brief  修改富媒体内容
 *@return succ 0 failed -1
 */
void* change_gg(void* arg, wt_sql_handle* handle)
{
	if( wt_sql_get_gg(handle) ){
		return (void*)WT_SQL_ERROR;
	}
	if( wt_sql_get_gg2(handle) ){
		return (void*)WT_SQL_ERROR;
	}
/*	
	//生成数据包头
	msg_head_st *msg_head = malloc(sizeof(msg_head_st));
	memset(msg_head, 0, sizeof(msg_head_st));

	struct list_head *pos;						// 连接设备链表遍历变量
	//遍历发送所有设备
	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	list_for_each(pos,&gv_sock_list_head){
		msg_head->ver			= ((sock_list*)pos)->ver;							//版本
		msg_head->sec_flag		= ((sock_list*)pos)->sec_flag;						//加密方式
		msg_head->router_id		= ((sock_list*)pos)->router_id;						//设备id
		memcpy(msg_head->hard_seq, ((sock_list*)pos)->hard_seq, 64);				//序列号
	
		if( send_gg(((sock_list*)pos)->sockfd, ((sock_list*)pos)->shanghuid, msg_head) ){
			((sock_list*)pos)->stat = SOCK_STAT_DEL;
		}
	}
	pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
	free(msg_head);
*/
	return (void*)0;
}

/**
 *@brief  修改白名单内容
 *@return succ 0 failed -1
 */
void* change_white(void* arg, wt_sql_handle* handle)
{
	if( wt_sql_get_white(handle) ){
		return (void*)WT_SQL_ERROR;
	}

	//生成数据包头
	msg_head_st *msg_head = malloc(sizeof(msg_head_st));
	memset(msg_head, 0, sizeof(msg_head_st));

	struct list_head *pos;						// 连接设备链表遍历变量

	// 商户单独的白名单
	char	gotodomain[1024]	= {0};
	char    otherdm[2048]		= {0};
	
	sock_list *sock;
	//遍历发送所有设备 重新下发白名单
	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	list_for_each(pos,&gv_sock_list_head){
		sock = (sock_list*)pos;
	
		// 查询商户单独白名单
		memset(gotodomain, 0, 1024);
		SQLBindCol( handle->sqlstr_handle, 1, SQL_C_CHAR, gotodomain, 1024, &handle->sql_err );//将变量和查询结果绑定
		sprintf( handle->sql_str, "SELECT TOP 1 gotodomain FROM mx_shanghu WHERE id = %u", sock->shanghuid );
		if (wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
			return (void*)WT_SQL_ERROR;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
		if( handle->sql_ret == SQL_NO_DATA ){
			xyprintf(0, "DATA_ERROR:%s %d -- Get retype is error! \n\thandle->sql_str:%s", __FILE__, __LINE__, handle->sql_str);
			sock->stat = SOCK_STAT_DEL;
			continue;
		}
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
		
		// 查询其他白名单
		memset(otherdm, 0, 2048);
		SQLBindCol( handle->sqlstr_handle, 1, SQL_C_CHAR, otherdm, 2048, &handle->sql_err );//将变量和查询结果绑定
		sprintf( handle->sql_str, "SELECT TOP 1 otherdm FROM mx_shanghu_domain WHERE shanghuid = %u", sock->shanghuid );
		//xyprintf(0, "%s", handle->sql_str);
		if (wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
			return (void*)WT_SQL_ERROR;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
		//if( handle->sql_ret == SQL_NO_DATA ){
		//	xyprintf(0, "DATA_ERROR:%s %d -- Get retype is error! \n\thandle->sql_str:%s", __FILE__, __LINE__, handle->sql_str);
		//}
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
		//xyprintf(0, "otherdm is %s", otherdm);
	
		msg_head->ver			= sock->ver;							//版本
		msg_head->sec_flag		= sock->sec_flag;						//加密方式
		msg_head->router_id		= sock->router_id;						//设备id
		memcpy(msg_head->hard_seq, sock->hard_seq, 64);					//序列号
		
		if( send_white(sock->sockfd, sock->ios_flag, sock->weixin_flag, sock->qq_flag, sock->sina_flag, msg_head, gotodomain, otherdm) ){
			sock->stat = SOCK_STAT_DEL;
		}
	}
	pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
	free(msg_head);
	
	return (void*)0;
}

/**
 *@brief  mac踢出
 *@return succ 0 failed -1
 */
void* mac_takeout(void* arg, wt_sql_handle* handle)
{
	if(!arg){
		xyprintf(0, "GUIDE_ERROR: %s -- %s -- %d", __FILE__, __func__, __LINE__);
		return (void*)0;
	}

	unsigned int shebeiid	= *( (unsigned int*)( arg ));
	unsigned int shang_pkid = *( (unsigned int*)( arg + sizeof(unsigned int) ) );
	char		 mac[16] = {0};
	snprintf(mac, sizeof(mac), "%s", (char*)(arg + sizeof(unsigned int) * 2));
#if SERVER_MUTUAL_DEBUG	
	xyprintf(0, "GUIDE:Has a order of mac takeout, shebeiid is %u, shang_pkid is %u, mac is %s",
			shebeiid, shang_pkid, mac);
#endif

	//数据包头
	void* full_msg = malloc( sizeof(msg_head_st) + sizeof(user_msg_st) + 256 );
	memset(full_msg, 0, sizeof(msg_head_st) + sizeof(user_msg_st) + 256 );
	msg_head_st *msg_head = full_msg;								// 发往设备的数据包头
	user_msg_st *msg = full_msg + sizeof(msg_head_st);				// 发往设备的数据包体
	char* rj_addr = full_msg + sizeof(msg_head_st) + sizeof(user_msg_st);
	sock_list *sock = malloc(sizeof(sock_list));
	

#if SERVER_MUTUAL_DEBUG	
	xyprintf(0, "GUIDE:router -- %u ┗|｀O′|┛ 嗷~~ Are you here~", shebeiid);
#endif
	
	if( !socklist_id_call_all(shebeiid, sock) ){
#if SERVER_MUTUAL_DEBUG	
		xyprintf(0, "GUIDE:%u (～o￣▽￣)～o ~。。。 I'm here!", shebeiid);
#endif
		//生成数据包头
		msg_head->ver			= sock->ver;				// 版本
		msg_head->mode			= 2;						// 方向
		msg_head->order			= msg_order_user_op;		// 包体类型
		msg_head->sec_flag		= sock->sec_flag;			// 加密方式
		//msg_head->crc;									// 校验和
		msg_head->router_id		= sock->router_id;			// 设备id
		//msg_head->third_id[64];							// 保留字段
		memcpy(msg_head->hard_seq, sock->hard_seq, 64);		// 序列号
			
		//生成数据包体
		msg->cmd		= user_init_rj;										// 数据类型
		msg->result		= user_cur_unknown;
		msg->auth_len	= 0;												// auth_addr 域的长度
		//msg->user_ip	= inet_addr(userip);								// 路由器发出的用户的IP
		string2x(msg->mac, mac, 6);											// mac地址
		//msg->	//__u32 speed[2];											// 速度控制
		sprintf(msg->user_id, "%c|%u", USER_ID_TAKEOUT, shang_pkid);			// 认证服务器回复用户的标识
		//msg->	__u32 auth_time;											// 免重复认证的时间
		//msg->	//__u32 flow[2];											// 流量控制
		//msg->__u8 auth_addr[0];
		sprintf(rj_addr, "%s?Rid=%u&macid=%u", sgv_trezhengurl, shebeiid, shang_pkid);		//跳转地址
		msg->auth_len		= strlen(rj_addr);												//auth_addr 域的长度
		msg_head->len		= sizeof(msg_head_st) + sizeof(user_msg_st) + msg->auth_len;	//数据包长度
			
		//xyprintf_msg_head(msg_head);
		//xyprintf_user_msg_head(msg);
		xyprintf(0, "%u: --> --> send user_msg(takeout) mac = %02x%02x%02x%02x%02x%02x --> -->", msg_head->router_id,
			msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
		if( send_full_user_msg_head(sock->sockfd, full_msg ) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Send takeout message head to router is error!", sock->sockfd, __FILE__, __LINE__);
			socklist_sockfd_err(sock->sockfd);
		}
	}//end if

	if(sock){
		free(sock);
	}
	if(full_msg){
		free(full_msg);
	}
	if(arg){
		free(arg);
	}
	return (void*)0;
}

/**
 *@brief  设备重启
 *@return succ 0 failed -1
 */
void* routers_restart(void* arg, wt_sql_handle* handle)
{
	if(!arg){
		xyprintf(0, "GUIDE_ERROR: %s -- %s -- %d", __FILE__, __func__, __LINE__);
		return (void*)0;
	}

	int num = *( (unsigned int*)( arg + sizeof(unsigned int) ) );
	unsigned int* routers = (unsigned int*)( arg + sizeof(unsigned int) * 2);
				
#if SERVER_MUTUAL_DEBUG	
	xyprintf(0, "GUIDE:Has a order of router restart, num is %d", num);
#endif
				
	//数据包头
	void* full_msg = malloc( sizeof(msg_head_st) + sizeof(reboot_msg_st));
	memset(full_msg, 0, sizeof(msg_head_st) + sizeof(reboot_msg_st) );
	msg_head_st *msg_head = full_msg;								// 发往设备的数据包头
	reboot_msg_st *msg = full_msg + sizeof(msg_head_st);			// 发往设备的数据包体
	sock_list *sock = malloc(sizeof(sock_list));
	
	int i;
	for(i = 0; i < num; i++){
		
#if SERVER_MUTUAL_DEBUG	
		xyprintf(0, "GUIDE:router %d -- %u ┗|｀O′|┛ 嗷~~ Are you here~", i, routers[i]);
#endif
		memset(msg_head, 0, sizeof(msg_head_st));
		memset(sock, 0, sizeof(sock_list));
		
		if( !socklist_id_call_all(routers[i], sock) ){
#if SERVER_MUTUAL_DEBUG	
			xyprintf(0, "GUIDE:%u (～o￣▽￣)～o ~。。。 I'm here!", routers[i]);
#endif
			
			msg_head->ver			= sock->ver;			// 版本
			msg_head->mode			= 2;					// 方向
			msg_head->order			= msg_order_reboot;		// 包体类型
			msg_head->sec_flag		= sock->sec_flag;		// 加密方式
			msg_head->router_id		= sock->router_id;		// 设备id
			memcpy(msg_head->hard_seq, sock->hard_seq, 64);	// 序列号

			//数据包体
			msg->type = 1;
			msg_head->len = sizeof(msg_head_st) + sizeof(reboot_msg_st);
			xyprintf(0, "%u: --> --> send reboot_msg(order from platform) --> -->", msg_head->router_id);
			if( send_reboot_msg(sock->sockfd, full_msg ) ){
				xyprintf(0, "SOCK_ERROR:%d %s %d -- Send reboot message to router is error!", sock->sockfd, __FILE__, __LINE__);
				socklist_sockfd_err(sock->sockfd);
			}

		}// end if
	}// end for

	if(sock){
		free(sock);
	}
	if(full_msg){
		free(full_msg);
	}
	if(arg){
		free(arg);
	}

	return (void*)0;
}

/**
 *@brief  设备过滤关键字
 *@return succ 0 failed -1
 */
void* routers_keyword(void* arg, wt_sql_handle* handle)
{
	if(!arg){
		xyprintf(0, "GUIDE_ERROR: %s -- %s -- %d", __FILE__, __func__, __LINE__);
		return (void*)0;
	}

	unsigned int shanghuid = *((unsigned int*)( arg ));

#if SERVER_MUTUAL_DEBUG	
	xyprintf(0, "GUIDE:Has a order of router keyword, shanghuid is %d", shanghuid);
#endif
	
	//数据包头
	sock_list *sock = malloc(sizeof(sock_list));
	
	int i;
	for(i = 0; ; i++){
		
#if SERVER_MUTUAL_DEBUG	
		xyprintf(0, "GUIDE:shanghu %u's number %d ┗|｀O′|┛ 嗷~~ Do you have", shanghuid, i);
#endif
		memset(sock, 0, sizeof(sock_list));
		
		if( !socklist_sid_call_all(shanghuid, i, sock) ){
#if SERVER_MUTUAL_DEBUG	
			xyprintf(0, "GUIDE:shanghu %u's number %d router NO.%u (～o￣▽￣)～o ~。。。 I'm here!", shanghuid, i, sock->shebei_pkid);
#endif
			if( send_all_web_keyword(handle, sock) ){
				goto SQL_ERR;
			}
		}
		else {
			break;
		}// end if
	}// end for

	if(sock){
		free(sock);
	}
	if(arg){
		free(arg);
	}

	return (void*)0;
SQL_ERR:
	if(sock){
		free(sock);
	}
	if(arg){
		free(arg);
	}
	return (void*)WT_SQL_ERROR;
}

/**
 *@brief  设备usb缓存修改
 *@return succ 0 failed -1
 */
void* routers_wireless(void* arg, wt_sql_handle* handle)
{
	if(!arg){
		xyprintf(0, "GUIDE_ERROR: %s -- %s -- %d", __FILE__, __func__, __LINE__);
		return (void*)0;
	}

	int num = *( (unsigned int*)( arg + sizeof(unsigned int) ) );
	unsigned int* routers = (unsigned int*)( arg + sizeof(unsigned int) * 2);
				
#if SERVER_MUTUAL_DEBUG	
	xyprintf(0, "GUIDE:Has a order of router wireless, num is %d", num);
#endif
				
	//数据包头
	msg_head_st *msg_head = malloc(sizeof(msg_head_st));
	sock_list *sock = malloc(sizeof(sock_list));
	
	int i;
	for(i = 0; i < num; i++){
		
#if SERVER_MUTUAL_DEBUG	
		xyprintf(0, "GUIDE:router %d:NO.%u ┗|｀O′|┛ 嗷~~ Are you here", i, routers[i]);
#endif
		memset(msg_head, 0, sizeof(msg_head_st));
		memset(sock, 0, sizeof(sock_list));
		
		if( !socklist_id_call_all(routers[i], sock) ){
#if SERVER_MUTUAL_DEBUG	
			xyprintf(0, "GUIDE:%u (～o￣▽￣)～o ~。。。 I'm here!", routers[i]);
#endif
			
			msg_head->ver			= sock->ver;			// 版本
			msg_head->sec_flag		= sock->sec_flag;		// 加密方式
			msg_head->router_id		= sock->router_id;		// 设备id
			memcpy(msg_head->hard_seq, sock->hard_seq, 64);	// 序列号
		
			if( send_wireless_config_get(sock->sockfd, msg_head, msg_order_mx_wireless_config_get) ){
				xyprintf(0, "SOCK_ERROR:%u %s %d -- Send wireless config get message to router is error!", sock->router_id, __FILE__, __LINE__);
				socklist_sockfd_err(sock->sockfd);
			}
			
			if( send_wireless_config_get(sock->sockfd, msg_head, msg_order_mx_wireless_config_5g_get) ){
				xyprintf(0, "SOCK_ERROR:%u %s %d -- Send wireless config get message to router is error!", sock->router_id, __FILE__, __LINE__);
				socklist_sockfd_err(sock->sockfd);
			}
		}// end if
	}// end for

	if(sock){
		free(sock);
	}
	if(msg_head){
		free(msg_head);
	}
	if(arg){
		free(arg);
	}

	return (void*)0;
}


/**
 *@brief  设备usb缓存修改
 *@return succ 0 failed -1
 */
void* routers_cacheusb(void* arg, wt_sql_handle* handle)
{
	if(!arg){
		xyprintf(0, "GUIDE_ERROR: %s -- %s -- %d", __FILE__, __func__, __LINE__);
		return (void*)0;
	}

	unsigned int shanghuid = *((unsigned int*)( arg ));

#if SERVER_MUTUAL_DEBUG	
	xyprintf(0, "GUIDE:Has a order of router cacheusb, shanghuid is %d", shanghuid);
#endif
	
	//数据包头
	sock_list *sock = malloc(sizeof(sock_list));
	
	int i;
	for(i = 0; ; i++){
		
#if SERVER_MUTUAL_DEBUG	
		xyprintf(0, "GUIDE:shanghu %u's number %d ┗|｀O′|┛ 嗷~~ Do you have", shanghuid, i);
#endif
		memset(sock, 0, sizeof(sock_list));
		
		if( !socklist_sid_call_all(shanghuid, i, sock) ){
#if SERVER_MUTUAL_DEBUG	
			xyprintf(0, "GUIDE:shanghu %u's number %d router NO.%u (～o￣▽￣)～o ~。。。 I'm here!", shanghuid, i, sock->shebei_pkid);
#endif
			if( send_all_usb_simple_cache(handle, sock, msg_order_usb_cache) ){
				goto SQL_ERR;
			}
		}
		else {
			break;
		}// end if
	}// end for

	if(sock){
		free(sock);
	}
	if(arg){
		free(arg);
	}

	return (void*)0;
SQL_ERR:
	if(sock){
		free(sock);
	}
	if(arg){
		free(arg);
	}
	return (void*)WT_SQL_ERROR;
}

/**
 *@brief  设备内存缓存修改
 *@return succ 0 failed -1
 */
void* routers_cache(void* arg, wt_sql_handle* handle)
{
	if(!arg){
		xyprintf(0, "GUIDE_ERROR: %s -- %s -- %d", __FILE__, __func__, __LINE__);
		return (void*)0;
	}

	unsigned int shanghuid = *((unsigned int*)( arg ));

#if SERVER_MUTUAL_DEBUG	
	xyprintf(0, "GUIDE:Has a order of router cache, shanghuid is %d", shanghuid);
#endif
	
	//数据包头
	sock_list *sock = malloc(sizeof(sock_list));
	
	int i;
	for(i = 0; ; i++){
		
#if SERVER_MUTUAL_DEBUG	
		xyprintf(0, "GUIDE:shanghu %u's number %d ┗|｀O′|┛ 嗷~~ Do you have", shanghuid, i);
#endif
		memset(sock, 0, sizeof(sock_list));
		
		if( !socklist_sid_call_all(shanghuid, i, sock) ){
#if SERVER_MUTUAL_DEBUG	
			xyprintf(0, "GUIDE:shanghu %u's number %d router NO.%u (～o￣▽￣)～o ~。。。 I'm here!", shanghuid, i, sock->shebei_pkid);
#endif

			if( send_all_usb_simple_cache(handle, sock, msg_order_memory_cache_set) ){
				goto SQL_ERR;
			}
		}
		else {
			break;
		}// end if
	}// end for

	if(sock){
		free(sock);
	}
	if(arg){
		free(arg);
	}

	return (void*)0;
SQL_ERR:
	if(sock){
		free(sock);
	}
	if(arg){
		free(arg);
	}
	return (void*)WT_SQL_ERROR;
}

/**
 *@brief  设备断开指令处理
 *@return succ 0 failed -1
 */
void* routers_disconn(void* arg, wt_sql_handle* handle)
{
	if(!arg){
		xyprintf(0, "GUIDE_ERROR: %s -- %s -- %d", __FILE__, __func__, __LINE__);
		return (void*)0;
	}

	int num = *(unsigned int*)( arg + sizeof(unsigned int) );
	unsigned int* routers = (unsigned int*)( arg + sizeof(unsigned int) * 2);
				
	int i;
	for(i = 0; i < num; i++){
		xyprintf(0, "GUIDE:router disconn %d:NO.%u", i, routers[i]);
		socklist_shebeipkid_err(routers[i]);
	}// end for
	
	if(arg){
		free(arg);
	}

	return (void*)0;
}

/**
 *@brief  AC地址修改
 *@return succ 0 failed -1
 */
void* ac_change(void* arg, wt_sql_handle* handle)
{
	if(!arg){
		xyprintf(0, "GUIDE_ERROR: %s -- %s -- %d", __FILE__, __func__, __LINE__);
		return (void*)0;
	}

	int num = *( (unsigned int*)( arg + sizeof(unsigned int) ) );
	unsigned int* routers = (unsigned int*)( arg + sizeof(unsigned int) * 2);
				
#if SERVER_MUTUAL_DEBUG	
	xyprintf(0, "GUIDE:Has a order of router ac change, num is %d", num);
#endif
				
	//数据包头
	msg_head_st *msg_head = malloc(sizeof(msg_head_st));
	sock_list *sock = malloc(sizeof(sock_list));
	
	int i;
	for(i = 0; i < num; i++){
		
#if SERVER_MUTUAL_DEBUG	
		xyprintf(0, "GUIDE:router %d:NO.%u ┗|｀O′|┛ 嗷~~ Are you here", i, routers[i]);
#endif
		memset(msg_head, 0, sizeof(msg_head_st));
		memset(sock, 0, sizeof(sock_list));
		
		if( !socklist_id_call_all(routers[i], sock) ){
#if SERVER_MUTUAL_DEBUG	
			xyprintf(0, "GUIDE:%u (～o￣▽￣)～o ~。。。 I'm here!", routers[i]);
#endif
			msg_head->ver			= sock->ver;			//版本
			msg_head->sec_flag		= sock->sec_flag;		//加密方式
			msg_head->router_id		= sock->router_id;		//设备id
			memcpy(msg_head->hard_seq, sock->hard_seq, 64);	//序列号

			// 修改acac地址
			if( send_addr_set( sock->sockfd, msg_head, msg_order_acac_addr_set, sgv_acacserverurl, sgv_acacserverport) ){
				xyprintf(0, "SOCK_ERROR:%d %s %d -- Send ACAC addr change message to router is error!", sock->sockfd, __FILE__, __LINE__);
				socklist_shebeipkid_err(sock->shebei_pkid);
			}
	
			msg_head->ver			= sock->ver;			//版本
			msg_head->sec_flag		= sock->sec_flag;		//加密方式
			msg_head->router_id		= sock->router_id;		//设备id
			memcpy(msg_head->hard_seq, sock->hard_seq, 64);	//序列号
			// 修改ac地址	
			if( send_addr_set( sock->sockfd, msg_head, msg_order_ac_addr_set, sgv_acserverurl, sgv_acserverport) ){
				xyprintf(0, "SOCK_ERROR:%d %s %d -- Send AC addr change message to router is error!", sock->sockfd, __FILE__, __LINE__);
				socklist_shebeipkid_err(sock->shebei_pkid);
			}
		}
	}// end for

	if(sock){
		free(sock);
	}
	if(msg_head){
		free(msg_head);
	}
	if(arg){
		free(arg);
	}

	return (void*)0;
}

/**
 *@brief  设备升级指令处理
 *@return succ 0 failed -1
 */
void* routers_update(void* arg, wt_sql_handle* handle)
{
	if(!arg){
		xyprintf(0, "GUIDE_ERROR: %s -- %s -- %d", __FILE__, __func__, __LINE__);
		return (void*)0;
	}

	int num = *( (unsigned int*)( arg + sizeof(unsigned int) ) );
	unsigned int* routers = (unsigned int*)( arg + sizeof(unsigned int) * 2);
				
#if SERVER_MUTUAL_DEBUG	
	xyprintf(0, "GUIDE:Has a order of router update, num is %d", num);
#endif
				
	//数据包头
	msg_head_st *msg_head = malloc(sizeof(msg_head_st));
	sock_list *sock = malloc(sizeof(sock_list));
	
	int i;
	for(i = 0; i < num; i++){
		
#if SERVER_MUTUAL_DEBUG	
		xyprintf(0, "GUIDE:router %d:NO.%u ┗|｀O′|┛ 嗷~~ Are you here", i, routers[i]);
#endif
		memset(msg_head, 0, sizeof(msg_head_st));
		memset(sock, 0, sizeof(sock_list));
		
		if( !socklist_id_call_all(routers[i], sock) ){
#if SERVER_MUTUAL_DEBUG	
			xyprintf(0, "GUIDE:%u (～o￣▽￣)～o ~。。。 I'm here!", routers[i]);
#endif
			msg_head->ver			= sock->ver;			//版本
			msg_head->sec_flag		= sock->sec_flag;		//加密方式
			msg_head->router_id		= sock->router_id;		//设备id
			memcpy(msg_head->hard_seq, sock->hard_seq, 64);	//序列号
			// 获取版本号 在获取后 判断是否需要升级
			if( send_version_get( sock->sockfd, msg_head) ){
				xyprintf(0, "GUIDE_ERROR:%d - Get version from router error, %s -- %d!",
						sock->router_id, __FILE__, __LINE__);
				socklist_shebeipkid_err(sock->shebei_pkid);
			}
		}
	}// end for

	if(sock){
		free(sock);
	}
	if(msg_head){
		free(msg_head);
	}
	if(arg){
		free(arg);
	}

	return (void*)0;
}

/**
 *@brief  获取服务器的当前状态 并发送给引导服务器
 *@return succ 0 failed -1
 */
int res_stat(int sockfd){
	ag_msg_stat msg_stat;
	msg_stat.head.head = 0x68;
	msg_stat.head.order = ag_stat_report;
	msg_stat.head.len = sizeof(ag_msg_stat);
		
	//获取状态
	get_stat(&msg_stat);
	msg_stat.router_num = gv_sock_count;
				
	msg_stat.cur_queue_size = get_cur_queue_size();
				
	//打印系统信息
	xyprintf(0, "☻ ☺ ☻ ☺  sock_count is %u, cur_queue_size is %u\n\t\ttotal\t\tused\t\tfree\t\tbuffers\t\tcached\nMem:\t\t%uM\t\t%uM\t\t%uM\t\t%uM\t\t%uM\n-/+ buffers/cache:\t\t%uM\t\t%uM\n",
	msg_stat.router_num, msg_stat.cur_queue_size,
	msg_stat.total_mem, msg_stat.used_mem,
	msg_stat.free_mem, msg_stat.buffers_mem, msg_stat.cached_mem,
	msg_stat.used_mem - msg_stat.buffers_mem - msg_stat.cached_mem,
	msg_stat.free_mem + msg_stat.buffers_mem + msg_stat.cached_mem);
		
	//发送
	if( wt_send_block( sockfd, (unsigned char*)&msg_stat, msg_stat.head.len) ){
		return -1;
	}
	return 0;
}

/**
 *@brief  连接引导服务器 并做初始认证
 *@return succ 创建的sockfd failed -1
 */
int conn_guide()
{
	int sockfd;
    struct sockaddr_in addr;	
	
	//socket初始化
	while(( sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		xyprintf(errno, "GUIDE_ERROR:%s %d -- socket()", __FILE__, __LINE__);
		sleep(10);
	}
	
	//连接引导服务器 失败的话 休眠3秒 重新连接
	addr.sin_family			= AF_INET;
	addr.sin_port			= htons( cgv_report_port );
	addr.sin_addr.s_addr	= inet_addr( cgv_report_addr );
	while( connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1){
		xyprintf(errno, "GUIDE_ERROR:%d %s %d -- connect()", sockfd, __FILE__, __LINE__);
		sleep(3);
	}
		
	//发送连接认证信息 报文头 + PASSWORD + 认证端口
	ag_msg_conn msg;
	msg.head.head	= AG_HEAD_HEAD;
	msg.head.order	= ag_authenticate;
	msg.head.len	= sizeof(msg);
	msg.passwd		= AG_PASS;
	msg.port		= cgv_authenticate_port;
		
	if( wt_send_block( sockfd, &msg, msg.head.len) ){
		xyprintf(errno, "GUIDE_ERROR:%d %s %d -- Send authenticate message to guide error!", sockfd, __FILE__, __LINE__);
		goto SOCK_ERR;
	}

	//接收引导服务器返回的信息并判断
	ag_msg_conn_res	res;
	if( wt_recv_block( sockfd, &res, sizeof(res)) ){
		xyprintf(errno, "GUIDE_ERROR:%d %s %d -- Recv message from guide error!", sockfd, __FILE__, __LINE__);
		goto SOCK_ERR;
	}
	if(res.head.head != 0x68 || res.head.order != ag_authenticate_res || res.head.len != sizeof(res)){
		xyprintf(errno, "GUIDE_ERROR:%d %s %d -- Guide result 's data error!", sockfd, __FILE__, __LINE__);
		goto SOCK_ERR;
	}

	if( res.conn_res != AG_CONN_SUCC ){
		xyprintf(0,"Can't connection Guide, res code is 0x%x!", res.conn_res);
		sleep(5);
		goto SOCK_ERR;
	}

	xyprintf(0, "** O(∩ _∩ )O ~~ Coneection Guide success, code is %u, platform_port is %u", res.auth_code, res.platform_port);

	// 启动其他线程
	pthread_mutex_lock(&gv_guide_flag_lock);
	gv_guide_flag = 1;
	gv_auth_code = res.auth_code;
	gv_platform_port = res.platform_port;
	pthread_mutex_unlock(&gv_guide_flag_lock);

	return sockfd;
SOCK_ERR:
	close( sockfd );		//出现错误关闭sockfd 并重新连接
	sockfd = -1;
	return -1;
}


/**
 *@brief  接收引导服务器的指令 并验证
 *@return succ 0 failed -1
 */
int recv_guide_data(int sockfd, char** data, int* data_len)
{
	//设置超时时间
	struct timeval tv;	// 超时时间结构体
	tv.tv_sec = 120;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	ag_msg_head	res;
	if( recv( sockfd, (unsigned char*)&res, sizeof(res), 0) <= 0 ){
		xyprintf(errno, "GUIDE_ERROR:Recv message from guide error, %s -- %d!", __FILE__, __LINE__);
		goto SOCK_ERR;
	}
	
	if(res.head != 0x68 || res.len < sizeof(res) ){
		xyprintf(errno, "GUIDE_ERROR:Guide result 's data error, %s -- %d!", __FILE__, __LINE__);
		goto SOCK_ERR;
	}

	//接收数据 如果有的话
	if( res.len > sizeof(res) ){
		*data_len = res.len - sizeof(res);
		*data = malloc(*data_len);
		memset(*data, 0 , *data_len);
		
		if( recv( sockfd, *data, *data_len, 0) != *data_len ){
			xyprintf(errno, "GUIDE_ERROR:Recv message from guide error, %s -- %d!", __FILE__, __LINE__);
			free(*data);
			*data = NULL;
			goto SOCK_ERR;
		}
	
		// 数据内容效验
		if(res.order == ag_mac_takeout || res.order == ag_mac_online){
			if( res.len != sizeof(ag_msg_mac) ){
				xyprintf(0, "GUIDE_ERROR: %s -- %s -- %d", __FILE__, __func__, __LINE__);
				free(*data);
				*data = NULL;
				goto SOCK_ERR;
			}
		}
		else if(res.order == ag_router_update ||	res.order == ag_router_disconn ||
				res.order == ag_router_cache ||		res.order == ag_router_cacheusb ||
				res.order == ag_router_wireless ||	res.order == ag_router_keyword ||
				res.order == ag_router_restart){
			// 数据内容效验 商户id + 设备数量 + 设备ids
			int num = *(unsigned int*)( *data + sizeof(unsigned int) );
			if( (num + 2) * sizeof(unsigned int) != *data_len ){
				xyprintf(0, "GUIDE_ERROR: %s -- %s -- %d", __FILE__, __func__, __LINE__);
				free(*data);
				*data = NULL;
				goto SOCK_ERR;
			}
		}
	}
	else {
		// 如果是下面的命令之一 却没有数据 差评
		if(res.order == ag_router_update ||
			res.order == ag_router_disconn ||
			res.order == ag_router_cache ||
			res.order == ag_router_cacheusb ||
			res.order == ag_router_wireless ||
			res.order == ag_router_keyword ||
			res.order == ag_router_restart ||
			res.order == ag_mac_takeout ||
			res.order == ag_mac_online){
				
			xyprintf(0, "GUIDE_ERROR: %s -- %s -- %d", __FILE__, __func__, __LINE__);
			goto SOCK_ERR;
		}
	}
	
	return res.order;

SOCK_ERR:
	return -1;
}

/** 
 *@brief  引导服务器指令处理线程
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* guide_mutual_thread(void* fd)
{
	pthread_detach( pthread_self() );
	xyprintf(0, "** O(∩ _∩ )O ~~ Status report thread is running!!!");
	int sockfd;
	while(1){
		
		pthread_mutex_lock(&gv_guide_flag_lock);
		gv_guide_flag = 0;
		gv_auth_code = 0;
		gv_platform_port = 0;
		pthread_mutex_unlock(&gv_guide_flag_lock);
		
		sockfd = conn_guide();
		if(sockfd == -1){
			continue;
		}
		
		if( res_stat(sockfd) ){
			xyprintf(errno, "GUIDE_ERROR:%d %s %d -- Send status message to guide error!", sockfd, __FILE__, __LINE__);
			close(sockfd);
			continue;
		}
		
		//循环接收引导服务器的指令
		while(1){
			char* data = NULL;
			int data_len = 0;

			int order = recv_guide_data(sockfd, &data, &data_len);	// 接收报文

#if SERVER_MUTUAL_DEBUG	
			//xyprintf(0, "data addr is %p, data_len is %d", data, data_len);
#endif			
			// 处理
			if( order == -1){						// 接收出错
				close(sockfd);
				break;
			}
			else if(order == ag_get_stat_report){	// 获取认证服务器状态
				if( res_stat(sockfd) ){
					xyprintf(errno, "GUIDE_ERROR:%d %s %d -- Send status message to guide error!", sockfd, __FILE__, __LINE__);
					close(sockfd);
					break;
				}
			}
			else if(order == ag_gg_change){			// 修改富媒体内容
				pool_add_worker( change_gg, NULL );
			}
			else if(order == ag_white_change){		// 修改白名单内容
				pool_add_worker( change_white, NULL );
			}
			else if(order == ag_upurl_change){		// 修改升级地址
				pool_add_worker( change_upurl, NULL );
			}
			else if(order == ag_other_change){		// 修改其他全局配置
				pool_add_worker( change_other, NULL );
			}
			else if(order == ag_router_update){		// 设备升级
				if(data_len >= 3 * sizeof(unsigned int)){
					pool_add_worker( routers_update, data );
				}
			}
			else if(order == ag_router_disconn){	// 重新连接
				if(data_len >= 3 * sizeof(unsigned int)){
					pool_add_worker( routers_disconn, data );
				}
			}
			else if(order == ag_router_cache){		// 修改内存缓存
				if(data_len >= 3 * sizeof(unsigned int)){
					pool_add_worker( routers_cache, data );
				}
			}
			else if(order == ag_router_cacheusb){	// 修改usb缓存
				if(data_len >= 3 * sizeof(unsigned int)){
					pool_add_worker( routers_cacheusb, data );
				}
			}
			else if(order == ag_router_wireless){	// 修改无线
				if(data_len >= 3 * sizeof(unsigned int)){
					pool_add_worker( routers_wireless, data );
				}
			}
			else if(order == ag_router_keyword){	// 修改过滤关键词
				if(data_len >= 3 * sizeof(unsigned int)){
					pool_add_worker( routers_keyword, data );
				}
			}
			else if(order == ag_router_restart){	// 重启
				if(data_len >= 3 * sizeof(unsigned int)){
					pool_add_worker( routers_restart, data );
				}
			}
			else if(order == ag_mac_takeout){		// mac踢出
				if(data_len == sizeof(ag_msg_mac) - sizeof(ag_msg_head)){
					pool_add_worker( mac_takeout, data);
				}
			}
			else if(order == ag_ac_change){		// ac地址修改
				if(data_len >= 3 * sizeof(unsigned int)){
					pool_add_worker( ac_change, data );
				}
			}
			else {									// 错误报文
				xyprintf(0, "GUIDE_ERROR:%d %s %d -- Order is error!", sockfd, __FILE__, __LINE__);
			}
		
		}// end while(1)
	}// end while(1)
ERR:
	xyprintf(0, "GUIDE_ERROR:%s %d -- Status report pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit( NULL );
}

/** 
 *@brief  处理平台发来的对mac的操作
 *@param  json			类型 int	json全文
 *@param  order_json	类型 int	json中的order部分
 *@return succ 0 failed -1
 */
int platform_mac_process(int sockfd, cJSON *json, cJSON *order_json)
{
	int i;
	//取商户id
	cJSON *shang_json = cJSON_GetObjectItem(json,"shang_pkid");
	if(!shang_json){
		xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto ERR;
	}
	
	cJSON *shebei_json = cJSON_GetObjectItem(json,"shebeiid");
	if(!shebei_json){
		xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto ERR;
	}
	
	cJSON *mac_json = cJSON_GetObjectItem(json,"mac");
	if(!mac_json){
		xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto ERR;
	}

#if SERVER_MUTUAL_DEBUG	
	xyprintf(0, "PLATFORM:Has a order of mac online, shebeiid is %u, shang_pkid is %u, mac is %s",
			shebei_json->valueint, shang_json->valueint, mac_json->valuestring);
#endif

	//数据包头
	void* full_msg = malloc( sizeof(msg_head_st) + sizeof(user_msg_st) );
	memset(full_msg, 0, sizeof(msg_head_st) + sizeof(user_msg_st) );
	msg_head_st *msg_head = full_msg;								// 发往设备的数据包头
	user_msg_st *msg = full_msg + sizeof(msg_head_st);				// 发往设备的数据包体
	
	sock_list *sock = malloc(sizeof(sock_list));
	

#if SERVER_MUTUAL_DEBUG	
	xyprintf(0, "PLATFORM:router -- %u ┗|｀O′|┛ 嗷~~ Are you here~", shebei_json->valueint);
#endif
	
	if( !socklist_id_call_all(shebei_json->valueint, sock) ){
#if SERVER_MUTUAL_DEBUG	
		xyprintf(0, "PLATFORM:%u (～o￣▽￣)～o ~。。。 I'm here!", shebei_json->valueint);
#endif
		//生成数据包头
		msg_head->ver			= sock->ver;									// 版本
		msg_head->mode			= 2;											// 方向
		msg_head->order			= msg_order_user_op;							// 包体类型
		msg_head->len			= sizeof(msg_head_st) + sizeof(user_msg_st);	// 数据包长度
		msg_head->sec_flag		= sock->sec_flag;								// 加密方式
		//msg_head->crc;														// 校验和
		msg_head->router_id		= sock->router_id;								// 设备id
		//msg_head->third_id[64];												// 保留字段
		memcpy(msg_head->hard_seq, sock->hard_seq, 64);							// 序列号
					
		//生成数据包体
		msg->cmd		= user_init_pass;										// 数据类型
		msg->result		= user_cur_log_in;
		msg->auth_len	= 0;													// auth_addr 域的长度
		//msg->user_ip	= inet_addr(userip);									// 路由器发出的用户的IP
		string2x(msg->mac, mac_json->valuestring, 6);							// mac地址
		msg->speed[0]	= sock->setbw_up;										// 上行速度
		msg->speed[1]	= sock->setbw_down;										// 下行速度
		snprintf(msg->user_id, sizeof(msg->user_id), "%c|%u|%u", USER_ID_SCANSQL, shang_json->valueint, sockfd);
		//msg->	__u32 auth_time;												// 免重复认证的时间
		//msg->	//__u32 flow[2];												// 流量控制
		//msg->__u8 auth_addr[0];
		
		//xyprintf_msg_head(msg_head);
		//xyprintf_user_msg_head(msg);
		xyprintf(0, "%u: --> --> send user_msg(Res online -- Platform) mac = %02x%02x%02x%02x%02x%02x --> -->", msg_head->router_id,
			msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
		if( send_full_user_msg_head(sock->sockfd, full_msg ) ){
			xyprintf(0, "SOCK_ERROR:%d %s %d -- Send takeout message head to router is error!", sock->sockfd, __FILE__, __LINE__);
			socklist_sockfd_err(sock->sockfd);
		}
	}//end if
	else {
		char *res = "3";
		if( send(sockfd, res, strlen(res), 0) <= 0){
			xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Res platform's massage error!", sockfd, __FILE__, __LINE__);
		}
	}

	if(sock){
		free(sock);
	}
	if(full_msg){
		free(full_msg);
	}
	return 0;
ERR:
	return -1;
}

/** 
 *@brief  平台命令处理函数 在platform_fun内被调用
 *@param  fd		类型 void*				连接设备的sockfd
 *@return 无意义
 */
void* platform_process(void *fd)
{
	int			sockfd	= (int)(long)fd;		//64bits下 void* 要先转换成long 然后再转换成int
	int			ret = 0;
	int			i;

	//报文体存放位置
	char buf[1024] = { 0 };

	//接收
	if( recv(sockfd, buf, 1024, 0) <= 0){
		xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Recv platform's massage error!", sockfd, __FILE__, __LINE__);
		goto DATA_ERR;
	}

#if SERVER_MUTUAL_DEBUG
	//晒一下
	xyprintf(0, "PLATFORM:Platform's msg: %s", buf);
#endif

	//比较是否是json头部
	if(strncmp( buf,"php-stream:",11)) {
		xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto DATA_ERR;
	}
	
	//解析json用到的临时变量
	cJSON *json, *order_json = NULL;

	// 主体json
	json=cJSON_Parse( buf + 11 );
	if (!json){
		xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto DATA_ERR;
	}

	//判断操作类型
	order_json = cJSON_GetObjectItem(json,"order");
	if (!order_json){
		xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto JSON_ERR;
	}
	
	if( !strcmp(order_json->valuestring, "mac_online")){		// 上线
		if( platform_mac_process(sockfd, json, order_json) ){
			xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
			goto JSON_ERR;
		}
	}
	else {
		xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto JSON_ERR;
	}
	
	cJSON_Delete(json);
	return (void*)0;
	
	//错误处理 使用内核中常用的goto模式～
JSON_ERR:
	cJSON_Delete(json);
DATA_ERR:
	wt_close_sock( &sockfd );
	xyprintf(0, "PLATFORM_ERROR:%s %s %d -- Request pthread is unnatural deaths!!!", __func__, __FILE__, __LINE__);
	return (void*)0;
} 

/** 
 *@brief  平台连接监听线程函数
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* platform_conn_thread(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Platform connection thread is running!!!");
	while(1){
		int sockfd;
		if( wt_sock_init( &sockfd, gv_platform_port, MAX_EPOLL_NUM) ){		//初始化监听连接
			xyprintf(errno, "PLATFORM_ERROR:0 %s %d -- wt_sock_init()", __FILE__, __LINE__);
			continue;
		}
		struct sockaddr_in client_address;				//存放客户端地址信息
		int client_len = sizeof(client_address);		//存放客户端地址信息结构体长度
		
		while(1){										//开始循环监听
			
			int client_sockfd;
			client_sockfd = accept(sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);
			
			if(client_sockfd == -1){					//监听出错
				xyprintf(errno, "PLATFORM_ERROR:%d %s %d -- accept()", sockfd, __FILE__, __LINE__);
				break;
			}
			
#if SERVER_MUTUAL_DEBUG
			//监听到一个连接 先打印一下
			xyprintf(0, "PLATFORM:O(∩ _∩ )O ~~ platform %s connection, sockfd is %d", inet_ntoa( client_address.sin_addr) , client_sockfd);
#endif
			pool_add_worker( platform_process, (void*)((long)client_sockfd));
		}

		close(sockfd);									//如果出错了 就关掉连接 重新初始化
	}

	// 远方～～～
	xyprintf(0, "PLATFORM_ERROR:✟ ✟ ✟ ✟  -- %s %d:Platform pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

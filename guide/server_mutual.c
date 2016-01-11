/*****************************************************
 *
 * 引导服务器 与 其他服务器的交互
 *
 *****************************************************/
#include "guide_header.h"

#define SERVER_MUTUAL_DEBUG			0

/** 
 *@brief  将对应认证服务器连接置错,在下个epoll循环中删除
 *@param  sockfd	类型 int	网络连接套接字
 *@return nothing
 */
void inline authenticate_list_err(int sockfd){
	struct list_head* pos;
	pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
	for( pos = gv_authenticate_list_head.next; pos != &gv_authenticate_list_head; pos = pos->next ){
		if( sockfd == ((authenticate_node*)pos)->sockfd){
			((authenticate_node*)pos)->stat = -1;//置为-1 备下次添加时 删除
			break;
		}	
	}
	pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙
}

/** 
 *@brief  处理平台发来的对mac的操作
 *@param  json			类型 int	json全文
 *@param  order_json	类型 int	json中的order部分
 *@return succ 0 failed -1
 */
int platform_mac_process(cJSON *json, cJSON *order_json)
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

	//申请给authenticate的报文空间
	int size = sizeof(ag_msg_mac);
	ag_msg_mac *msg = malloc( size );
	memset(msg, 0, size);
	
	//报文头赋值
	msg->head.head = AG_HEAD_HEAD;
	msg->head.len  = size;
		
	// 报文头order赋值
	if( !strcmp(order_json->valuestring, "mac_takeout") ){
		msg->head.order = ag_mac_takeout;
	}else if( !strcmp(order_json->valuestring, "mac_online") ){
		msg->head.order = ag_mac_online;
	}

	//设备id
	msg->shebeiid   = shebei_json->valueint;
	// shang_pkid
	msg->shang_pkid = shang_json->valueint;
	// mac
	snprintf(msg->mac, sizeof(msg->mac), "%s", mac_json->valuestring);

#if SERVER_MUTUAL_DEBUG
	// 打印一下 接收过来的数据
	xyprintf(0, "PLATFORM:** order is %s", order_json->valuestring);
	xyprintf(0, "PLATFORM:** shebeiid is %u", msg->shebeiid);
	xyprintf(0, "PLATFORM:** shang_pkid is %u", msg->shang_pkid);
	xyprintf(0, "PLATFORM:** mac is %s", msg->mac);
	xyprintf(0, "PLATFORM:** sizeof(msg->mac) is %d", sizeof(msg->mac));
	//发送报文
	xyprintf(0, "PLATFORM:--> --> --> Send mac process message! --> --> -->");
#endif

	struct list_head* pos;
	pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
	for( pos = gv_authenticate_list_head.next; pos != &gv_authenticate_list_head; pos = pos->next ){//遍历设备list 添加或删除设备到epoll队列中
		pthread_mutex_lock( &( ((authenticate_node*)pos)->lock ) );	//加authenticate sockfd的互斥锁
		if( wt_send_block(((authenticate_node*)pos)->sockfd, msg, size ) ){	//发送
			xyprintf(0, "PLATFORM_ERROR:%s %s %d : Send mac process massage to %d error!", __func__, __FILE__, __LINE__, ((authenticate_node*)pos)->sockfd);
			((authenticate_node*)pos)->stat = -1;//置为-1 备下次添加时 删除
		}
		pthread_mutex_unlock( &( ((authenticate_node*)pos)->lock ) );
	}
	pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙

	// 处理结束 删除申请的报文空间
	if(msg){
		free(msg);
	}
	return 0;
	// 如果在申请报文空间后 出现错误 跳转到此 结束
DATA_ERR:
	if(msg){
		free(msg);
	}
ERR:
	return -1;
}

/** 
 *@brief  处理平台发来的对设备的操作
 *@param  json			类型 int	json全文
 *@param  order_json	类型 int	json中的order部分
 *@return succ 0 failed -1
 */
int platform_router_process(cJSON *json, cJSON *order_json)
{
	int i;
	//取商户id
	cJSON *shanghu_json = cJSON_GetObjectItem(json,"shanghuid");
	if(!shanghu_json){
		xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto ERR;
	}
	
	cJSON *shebei_json = cJSON_GetObjectItem(json,"shebeiid");
	if(!shebei_json){
		xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto ERR;
	}
	
	//计算设备数量
	int router_num = cJSON_GetArraySize(shebei_json);

	//申请给authenticate的报文空间
	int size = sizeof(ag_msg_router) + router_num * sizeof(unsigned int);
	ag_msg_router *msg = malloc( size );
	memset(msg, 0, size);
	
	//报文头赋值
	msg->head.head = AG_HEAD_HEAD;
	msg->head.len  = size;
		
	// 报文头order赋值
	if( !strcmp(order_json->valuestring, "router_update") ){
		msg->head.order = ag_router_update;
	}else if( !strcmp(order_json->valuestring, "router_disconn") ){
		msg->head.order = ag_router_disconn;
	}else if( !strcmp(order_json->valuestring, "router_cache") ){
		msg->head.order = ag_router_cache;
	}else if( !strcmp(order_json->valuestring, "router_cacheusb") ){
		msg->head.order = ag_router_cacheusb;
	}else if( !strcmp(order_json->valuestring, "router_wireless") ){
		msg->head.order = ag_router_wireless;
	}else if( !strcmp(order_json->valuestring, "router_keyword") ){
		msg->head.order = ag_router_keyword;
	}else if( !strcmp(order_json->valuestring, "router_restart") ){
		msg->head.order = ag_router_restart;
	}else if( !strcmp(order_json->valuestring, "shebei_type") ){
		msg->head.order = ag_upurl_change;
	}else if( !strcmp(order_json->valuestring, "fumeiti") ){
		msg->head.order = ag_gg_change;
	}else if( !strcmp(order_json->valuestring, "white_list") ){
		msg->head.order = ag_white_change;
	}else if( !strcmp(order_json->valuestring, "other_change") ){
		msg->head.order = ag_other_change;
	}else if( !strcmp(order_json->valuestring, "ac_change") ){
		msg->head.order = ag_ac_change;
	}

	//商户id
	msg->shanghuid   = shanghu_json->valueint;
	// 设备数量
	msg->router_num = router_num;

#if SERVER_MUTUAL_DEBUG
	// 打印一下 接收过来的数据
	xyprintf(0, "PLATFORM:** order is %s", order_json->valuestring);
	xyprintf(0, "PLATFORM:** shanghuid is %u", msg->shanghuid);
	xyprintf(0, "PLATFORM:** shebei num is %d", msg->router_num);
#endif
	//将json中的设备id放到报文体内
	for (i = 0; i < router_num; i++){
		cJSON* sjson = cJSON_GetArrayItem(shebei_json, i);
		if(sjson){
			msg->routers[i] = atoi(sjson->valuestring);
#if SERVER_MUTUAL_DEBUG
			xyprintf(0, "** %d -- shebeiid is %u", i + 1, msg->routers[i]);
#endif
		}
		else {
			xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
			goto DATA_ERR;
		}
	}

	//发送报文
#if SERVER_MUTUAL_DEBUG
	xyprintf(0, "PLATFORM:--> --> --> Send router process message! --> --> -->");
#endif
	struct list_head* pos;
	pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
	for( pos = gv_authenticate_list_head.next; pos != &gv_authenticate_list_head; pos = pos->next ){//遍历设备list 添加或删除设备到epoll队列中
		pthread_mutex_lock( &( ((authenticate_node*)pos)->lock ) );	//加authenticate sockfd的互斥锁
		if( wt_send_block(((authenticate_node*)pos)->sockfd, msg, size ) ){	//发送
			xyprintf(0, "PLATFORM_ERROR:%s %s %d : Send report process massage to %d error!", __func__, __FILE__, __LINE__, ((authenticate_node*)pos)->sockfd);
			((authenticate_node*)pos)->stat = -1;//置为-1 备下次添加时 删除
		}
		pthread_mutex_unlock( &( ((authenticate_node*)pos)->lock ) );
	}
	pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙

	// 处理结束 删除申请的报文空间
	if(msg){
		free(msg);
	}
	return 0;
	// 如果在申请报文空间后 出现错误 跳转到此 结束
DATA_ERR:
	if(msg){
		free(msg);
	}
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
#endif
	xyprintf(0, "PLATFORM:Platform's msg: %s", buf);

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

	if( !strcmp(order_json->valuestring, "router_update") ||	// 升级
		!strcmp(order_json->valuestring, "router_disconn") ||	// 需要重新连接
		!strcmp(order_json->valuestring, "router_cache") ||		// 修改内存缓存
		!strcmp(order_json->valuestring, "router_cacheusb") ||	// 修改usb缓存
		!strcmp(order_json->valuestring, "router_wireless") ||	// 修改无线设置
		!strcmp(order_json->valuestring, "router_keyword") ||	// 修改过滤关键词
		!strcmp(order_json->valuestring, "ac_change") ||		// 修改AC地址
		!strcmp(order_json->valuestring, "shebei_type") ||		// 修改设备类型
		!strcmp(order_json->valuestring, "fumeiti") ||			// 修改富媒体
		!strcmp(order_json->valuestring, "white_list") ||		// 修改白名单
		!strcmp(order_json->valuestring, "other_change") ||		// 修改其他配置
		!strcmp(order_json->valuestring, "router_restart")){	// 重启
		//设备操作
		if( platform_router_process(json, order_json) ){
			xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
			goto JSON_ERR;
		}
		else {
			// 发送返回值
			char *res = "1";
			//xyprintf(0, "** res -- %d -- %s", strlen(res), res);
			if( send(sockfd, res, strlen(res), 0) <= 0){
				xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Res platform's massage error!", sockfd, __FILE__, __LINE__);
				goto JSON_ERR;
			}
		}
#if IS_CONN_BOSS
		// 设备信息上报
		if( !strcmp(order_json->valuestring, "shebei_type") ){
			SEND_RTS();
		}
#endif
	} else if( !strcmp(order_json->valuestring, "mac_takeout") ||		// 踢出
			 !strcmp(order_json->valuestring, "mac_online")){		// 上线
		//mac操作
		if( platform_mac_process(json, order_json) ){
			xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
			goto JSON_ERR;
		}
		else {
			// 发送返回值
			char *res = "1";
			//xyprintf(0, "** res -- %d -- %s", strlen(res), res);
			if( send(sockfd, res, strlen(res), 0) <= 0){
				xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Res platform's massage error!", sockfd, __FILE__, __LINE__);
				goto JSON_ERR;
			}
		}
	}
#if IS_CONN_BOSS
	else if( !strcmp(order_json->valuestring, "sys_config") ){
		SEND_BASIS_SN(sockfd);
		cJSON_Delete(json);
		return (void*)0;
	}
#endif
	else {
		xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto JSON_ERR;
	}
	
	cJSON_Delete(json);
	wt_close_sock( &sockfd );
#if SERVER_MUTUAL_DEBUG
	xyprintf(0, "PLATFORM:Request completion of the platform!!!");
#endif
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
void* platform_conn_thread(void *fd){
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Platform connection thread is running!!!");
	while(1){
		int sockfd;
		if( wt_sock_init( &sockfd, cgv_platform_port, MAX_EPOLL_NUM) ){		//初始化监听连接
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
			
			pthread_t thread;//创建线程维护与第三方监听程序的连接 
			if(pthread_create(&thread, NULL, platform_process, (void*)(long)client_sockfd) != 0){//创建子线程
				xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
				break;
			}

		}

		close(sockfd);									//如果出错了 就关掉连接 重新初始化
	}

	//到不了的地方～～～
	xyprintf(0, "PLATFORM_ERROR:✟ ✟ ✟ ✟  -- %s %d:Platform pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  第三方连接处理线程函数
 *@param  fd		类型 void*	线程启动参数,用来传送第三方socket套接字
 *@return nothing
 */
void* third_process(void* fd)
{
	pthread_detach(pthread_self());
	int sockfd = (int)(long)fd;

	gv_third_count++;

	//接收连接报文头并判断
	ag_msg_head	head;
	if( wt_recv_block( sockfd, &head, sizeof(ag_msg_head) ) ){
		xyprintf(0, "THIRD_ERROR:%d %s %d -- Recv report massage head error!", sockfd, __FILE__, __LINE__);
		goto SOCK_ERR;
	}
	if(head.head != AG_HEAD_HEAD || head.order != ag_third || head.len != ( sizeof(ag_msg_head) + 4 ) ){
		xyprintf(0, "THIRD_ERROR:%d %s %d -- Report massage data error!", sockfd, __FILE__, __LINE__);
		goto SOCK_ERR;
	}
	//接收连接密码 并判断
	int pass;
	if( wt_recv_block( sockfd, &pass, 4 ) ){
		xyprintf(0, "THIRD_ERROR:%d %s %d -- Recv report massage password error!", sockfd, __FILE__, __LINE__);
		goto SOCK_ERR;
	}
	if( pass != AG_PASS ){
		xyprintf(0, "THIRD_ERROR:%d %s %d : Password error!", sockfd, __FILE__, __LINE__);
		goto SOCK_ERR;
	}
	//回复连接报文
	head.order = ag_third_res;
	head.len   = sizeof(head);
	if( wt_send_block( sockfd, &head, sizeof(head) ) ){
		xyprintf(0, "THIRD_ERROR:%d %s %d : Send report res massage error!", sockfd, __FILE__, __LINE__);
		goto SOCK_ERR;
	}
	
	xyprintf(0, "THIRD:gv_third_count = %u", gv_third_count);

	void *temp;

	//进入循环 接收第三方程序的请求
	while(1){
		//设置超时时间
		struct timeval tv;  // 超时时间结构体
		tv.tv_sec = 120;
		tv.tv_usec = 0;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		
		//接收报文头
		if( recv( sockfd, &head, sizeof(ag_msg_head), 0 ) <= 0 ){
			xyprintf(0, "THIRD_ERROR:%d %s %d -- Recv report massage head error!", sockfd, __FILE__, __LINE__);
			goto SOCK_ERR;
		}
		if(head.head != AG_HEAD_HEAD ){
			xyprintf(0, "THIRD_ERROR:%d %s %d -- Report massage data error!", sockfd, __FILE__, __LINE__);
			goto SOCK_ERR;
		}
		
		//接收报文 如果有的话
		if(head.len - sizeof(ag_msg_head) > 0){
			void *temp = malloc(head.len - sizeof(ag_msg_head));
			if(temp == NULL){
				xyprintf(0, "THIRD_ERROR: %s %d malloc error!", __FILE__, __LINE__);
				goto SOCK_ERR;
			}
			if( recv( sockfd, &head, sizeof(ag_msg_head), 0 ) <= 0 ){
				xyprintf(0, "THIRD_ERROR:%d %s %d -- Recv report massage head error!", sockfd, __FILE__, __LINE__);
				goto SOCK_ERR;
			}
		}
		
		struct list_head* pos;

		//判断第三方程序的请求并处理
		if(head.order == ag_get_stat_report){	//获取状态
			
#if SERVER_MUTUAL_DEBUG
			xyprintf(0, "THIRD:<-- <-- <-- Third get status! <-- <-- <-- gv_third_count = %u", gv_third_count);
#endif
			
			//回复报文格式 报文头 + 状态报文体 * 设备数
			pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
			
			int size = sizeof(ag_msg_head) + gv_authenticate_count * sizeof(authenticate_stat);
			void* buf = malloc(size);
			if(!buf){
				pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙
				xyprintf(0, "THIRD:%s %s %d -- malloc() error!!!", __FILE__, __func__, __LINE__);
				goto SOCK_ERR;
			}

			ag_msg_head *head = buf;
			head->head = AG_HEAD_HEAD;
			head->order = ag_stat_report;
			head->len = size;
			int i = 0;
			authenticate_stat *stat = buf + sizeof(ag_msg_head);
			authenticate_node *node;

			//遍历设备表 将设备信息填充到报文内
			for( pos = gv_authenticate_list_head.next; pos != &gv_authenticate_list_head; pos = pos->next ){
				node = (authenticate_node*)pos;
				stat[i].ip					= node->sin_addr.s_addr;
				stat[i].router_num			= node->router_num;					//已连接设备数
				stat[i].cur_queue_size		= node->cur_queue_size;				//未处理报文数
				stat[i].cpu_usage			= node->cpu_usage;					//cpu使用率
				stat[i].total_mem			= node->total_mem;					//总内存
				stat[i].used_mem			= node->used_mem;					//已使用内存
				stat[i].free_mem			= node->free_mem;					//空闲内存
				//stat[i].shared_mem		= node->shared_mem;
				stat[i].buffers_mem			= node->buffers_mem;
				stat[i].cached_mem			= node->cached_mem;
				stat[i].swap				= node->swap;
				i++;
			}
			
			pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙
			
			if( wt_send_block( sockfd, buf, size ) ){
				xyprintf(0, "THIRD_ERROR:%d %s %d : Send report massage error!", sockfd, __FILE__, __LINE__);
				free(buf);
				goto SOCK_ERR;
			}
			free(buf);
		}
		else if(head.order == ag_gg_change){	//富媒体内容修改
			//生成富媒体修改报文 下发给认证服务器
			ag_msg_head head;
			head.head = AG_HEAD_HEAD;
			head.order = ag_gg_change;
			head.len   = sizeof(head);
			
			pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
			xyprintf(0, "THIRD:--> --> --> Send gg change message! --> --> -->");
			for( pos = gv_authenticate_list_head.next; pos != &gv_authenticate_list_head; pos = pos->next ){//遍历设备list 添加或删除设备到epoll队列中
				pthread_mutex_lock( &( ((authenticate_node*)pos)->lock ) );
				if( wt_send_block(((authenticate_node*)pos)->sockfd, &head, sizeof(head) ) ){
					xyprintf(0, "AUTHENTICATE_ERROR:%d %s %d : Send report res massage error!", ((authenticate_node*)pos)->sockfd, __FILE__, __LINE__);
					((authenticate_node*)pos)->stat = -1;//置为-1 备下次添加时 删除
				}
				pthread_mutex_unlock( &( ((authenticate_node*)pos)->lock ) );
			}
			pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙
		}
		else if(head.order == ag_white_change){	//白名单修改
			//生成白名单修改报文 下发给认证服务器
			ag_msg_head head;
			head.head = AG_HEAD_HEAD;
			head.order = ag_white_change;
			head.len   = sizeof(head);
			
			pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
			xyprintf(0, "THIRD:--> --> --> Send white change message! --> --> -->");
			for( pos = gv_authenticate_list_head.next; pos != &gv_authenticate_list_head; pos = pos->next ){//遍历设备list 添加或删除设备到epoll队列中
				pthread_mutex_lock( &( ((authenticate_node*)pos)->lock ) );
				if( wt_send_block(((authenticate_node*)pos)->sockfd, &head, sizeof(head) ) ){
					xyprintf(0, "AUTHENTICATE_ERROR:%d %s %d : Send report res massage error!", ((authenticate_node*)pos)->sockfd, __FILE__, __LINE__);
					((authenticate_node*)pos)->stat = -1;//置为-1 备下次添加时 删除
				}
				pthread_mutex_unlock( &( ((authenticate_node*)pos)->lock ) );
			}
			pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙
		}
		else if(head.order == ag_upurl_change){	//升级地址修改
			//生成升级地址修改报文 下发给认证服务器
			ag_msg_head head;
			head.head = AG_HEAD_HEAD;
			head.order = ag_upurl_change;
			head.len   = sizeof(head);
			
			pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
			xyprintf(0, "THIRD:--> --> --> Send upurl change message! --> --> -->");
			for( pos = gv_authenticate_list_head.next; pos != &gv_authenticate_list_head; pos = pos->next ){//遍历设备list 添加或删除设备到epoll队列中
				pthread_mutex_lock( &( ((authenticate_node*)pos)->lock ) );
				if( wt_send_block(((authenticate_node*)pos)->sockfd, &head, sizeof(head) ) ){
					xyprintf(0, "AUTHENTICATE_ERROR:%d %s %d : Send report res massage error!", ((authenticate_node*)pos)->sockfd, __FILE__, __LINE__);
					((authenticate_node*)pos)->stat = -1;//置为-1 备下次添加时 删除
				}
				pthread_mutex_unlock( &( ((authenticate_node*)pos)->lock ) );
			}
			pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙
		}
		else if(head.order == ag_other_change){	//其他配置项修改
			//生成其他配置项 报文下发给认证服务器
			ag_msg_head head;
			head.head = AG_HEAD_HEAD;
			head.order = ag_other_change;
			head.len   = sizeof(head);
			
			pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
			xyprintf(0, "THIRD:--> --> --> Send other change message! --> --> -->");
			for( pos = gv_authenticate_list_head.next; pos != &gv_authenticate_list_head; pos = pos->next ){//遍历设备list 添加或删除设备到epoll队列中
				pthread_mutex_lock( &( ((authenticate_node*)pos)->lock ) );
				if( wt_send_block(((authenticate_node*)pos)->sockfd, &head, sizeof(head) ) ){
					xyprintf(0, "AUTHENTICATE_ERROR:%d %s %d : Send report res massage error!", ((authenticate_node*)pos)->sockfd, __FILE__, __LINE__);
					((authenticate_node*)pos)->stat = -1;//置为-1 备下次添加时 删除
				}
				pthread_mutex_unlock( &( ((authenticate_node*)pos)->lock ) );
			}
			pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙
		}
		else{	//报文错误
			xyprintf(0, "THIRD_ERROR:%d %s %d -- Order is error, order = %u!", sockfd, __FILE__, __LINE__, head.order);
		}
	}
	
SOCK_ERR:
	close(sockfd);
	gv_third_count--;
	xyprintf(0, "THIRD:✟ ✪ ✟ ✪ ✟  -- %s %d:Third fun pthread is unnatural deaths!!! gv_third_count = %u", __FILE__, __LINE__, gv_third_count);
	pthread_exit(NULL);
}

/** 
 *@brief  第三方程序连接监听线程函数
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* third_conn_thread(void *fd){
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Third connection thread is running!!!");
	while(1){
		int sockfd;
		if( wt_sock_init( &sockfd, cgv_third_port, 2) ){
			xyprintf(errno, "THIRD_ERROR:0 %s %d -- wt_sock_init()", __FILE__, __LINE__);
			continue;
		}
		struct sockaddr_in client_address;//存放客户端地址信息
		int client_len = sizeof(client_address);
		while(1){
			int client_sockfd;
			client_sockfd = accept(sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);  
			if(client_sockfd == -1){ 
				xyprintf(errno, "THIRD_ERROR:%d %s %d -- accept()", sockfd, __FILE__, __LINE__);
				break;
			}

			xyprintf(0, "THIRD:third %s connection, sockfd is %d", inet_ntoa( client_address.sin_addr) , client_sockfd);

			pthread_t thread;//创建线程维护与第三方监听程序的连接 
			if(pthread_create(&thread, NULL, third_process, (void*)(long)client_sockfd) != 0){//创建子线程
				xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
				break;
			}
		}
		close(sockfd);
	}
	xyprintf(0, "THIRD:✟ ✟ ✟ ✟  -- %s %d:THird_start pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  接收认证服务器发来的状态报告
 *@param  sockfd	类型 int	epoll到有数据可接收的socket套接字
 *@return nothing
 */
void auth_packet_process(int sockfd)
{
	ag_msg_stat stat;
	//接收认证服务器发来的状态报文
	if( wt_recv_block(sockfd, &stat, sizeof(ag_msg_stat) ) ){
		xyprintf(0, "AUTHENTICATE_ERROR:%d %s %d -- Recv report massage error!", sockfd, __FILE__, __LINE__);
		goto ERR;
	}
	//判断认证服务器发来的状态报文头信息
	if(stat.head.head != AG_HEAD_HEAD || stat.head.order != ag_stat_report || stat.head.len != sizeof(ag_msg_stat) ){
		xyprintf(0, "AUTHENTICATE_ERROR:%d %s %d -- Report massage data error!", sockfd, __FILE__, __LINE__);
		goto ERR;
	}

	struct list_head* pos;
	//遍历查找对应的认证服务器 然后更新状态信息
	pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
	for( pos = gv_authenticate_list_head.next; pos != &gv_authenticate_list_head; pos = pos->next ){
		if( sockfd == ((authenticate_node*)pos)->sockfd){		//更新数据
			((authenticate_node*)pos)->router_num		= stat.router_num;
			((authenticate_node*)pos)->cur_queue_size	= stat.cur_queue_size;
			((authenticate_node*)pos)->cpu_usage		= stat.cpu_usage;
			((authenticate_node*)pos)->total_mem		= stat.total_mem;
			((authenticate_node*)pos)->used_mem			= stat.used_mem;
			((authenticate_node*)pos)->free_mem			= stat.free_mem;
			//((authenticate_node*)pos)->shared_mem		= stat.shared_mem;
			((authenticate_node*)pos)->buffers_mem		= stat.buffers_mem;
			((authenticate_node*)pos)->cached_mem		= stat.cached_mem;
			((authenticate_node*)pos)->swap				= stat.swap;
			time( &( ((authenticate_node*)pos)->last_time) );
			((authenticate_node*)pos)->load_weight		= stat.used_mem * 100 / stat.total_mem;	//重新计算负载情况
			
			break;
		}
	}
	pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙
	return;
ERR:
	authenticate_list_err(sockfd);
}

/** 
 *@brief  epoll线程 接收认证服务器状态报告
 *@param  fd		类型 void*		线程启动参数,未使用
 *@return nothing
 */
void* auth_epoll_thread(void *fd)
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
	events = (struct epoll_event*)malloc (16 * sizeof(struct epoll_event) );	//一次最多获得16条结果
	if(events == NULL){
		xyprintf(0, "EPOLL_ERROR: %s %d malloc error!", __FILE__, __LINE__);
		goto CREATE_ERR;
	}

	while(1){
		pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
		for( pos = gv_authenticate_list_head.next; pos != &gv_authenticate_list_head; pos = pos->next ){//遍历设备list 添加或删除设备到epoll队列中

			//将两个请求间隔 没有收到回复报文的认证服务器 置错
			if( time(0) - ((authenticate_node*)pos)->last_time > cgv_report_interval * 2){
				((authenticate_node*)pos)->stat = -1;
			}

			if( ((authenticate_node*)pos)->stat == 1 ){		//需要添加到epoll列表的认证服务器
				struct epoll_event event;
				event.data.fd = ((authenticate_node*)pos)->sockfd;
				event.events = EPOLLIN | EPOLLRDHUP;
				ret = epoll_ctl(epfd, EPOLL_CTL_ADD, ((authenticate_node*)pos)->sockfd, &event);
				if(ret == -1){
					xyprintf(errno, "EPOLL_ERROR: %s %d -- epoll_ctl()", __FILE__, __LINE__);
				}
				((authenticate_node*)pos)->stat = 0;
			}
			else if( ((authenticate_node*)pos)->stat == -1 ) {	//需要从epoll列表中删除 并关闭连接的认证服务器
				struct epoll_event event;
				event.data.fd = ((authenticate_node*)pos)->sockfd;
				event.events = EPOLLIN | EPOLLRDHUP;
				ret = epoll_ctl(epfd, EPOLL_CTL_DEL, ((authenticate_node*)pos)->sockfd, &event);	//从监听列表中删除
				if(ret == -1){
					xyprintf(errno, "EPOLL_ERROR:%s %d epoll_ctl()", __FILE__, __LINE__);
				}
				wt_close_sock( &(((authenticate_node*)pos)->sockfd) );								//关闭socket
				pthread_mutex_destroy( &(((authenticate_node*)pos)->lock) );						//销毁锁
				

				xyprintf(0, "AUTHENTICATE_ERROR:ip is %s, gv_authenticate_count = %d", inet_ntoa(((authenticate_node*)pos)->sin_addr), gv_authenticate_count);
				
				//从链表中删除
				struct list_head *temp = pos;
				pos = temp->prev;
				list_del( temp );
				if(temp){
					free( temp );
				}
				gv_authenticate_count--;
			}
		}
		pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙

		//epoll_wait
		ret = epoll_wait(epfd, events, 1024, 100);
		for(i = 0; i < ret; i++){
			if( (events[i].events & EPOLLRDHUP ) || (events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || 
					( !(events[i].events & EPOLLIN) ) ){ //判断是否出错
				authenticate_list_err(events[i].data.fd);	//将连接置错
			}
			else {
				auth_packet_process( events[i].data.fd );			//接收信息
			}
		}
	}
CTL_ERR:
	close(epfd);
	if(events){
		free(events);
	}
CREATE_ERR:
	xyprintf(0, "AUTHENTICATE_ERROR:✟ ✟ ✟ ✟  -- %s %d:Epoll pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  刷新状态线程 在一定的间隔时间内 向认证服务器下发获取状态报告报文
 *@param  fd		类型 void*		线程启动参数,未使用
 *@return nothing
 */
void* auth_stat_refresh_thread(void* fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Refresh authenticate stat thread is running!!!");
	struct list_head *pos;	//设备list遍历变量
	ag_msg_head head;
	head.head = AG_HEAD_HEAD;
	head.order = ag_get_stat_report;
	head.len   = sizeof(head);
	
	while(1){
		
		pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
		if(gv_authenticate_count > 0){
#if SERVER_MUTUAL_DEBUG
			xyprintf(0, "AUTHENTICATE:--> --> --> Call authenticate send stat! --> --> -->");
#endif
			for( pos = gv_authenticate_list_head.next; pos != &gv_authenticate_list_head; pos = pos->next ){//遍历设备list 添加或删除设备到epoll队列中
				pthread_mutex_lock( &( ((authenticate_node*)pos)->lock ) );
				if( wt_send_block(((authenticate_node*)pos)->sockfd, &head, sizeof(head) ) ){
					xyprintf(0, "AUTHENTICATE_ERROR:%d %s %d : Send report res massage error!", ((authenticate_node*)pos)->sockfd, __FILE__, __LINE__);
					((authenticate_node*)pos)->stat = -1;//置为-1 备下次添加时 删除
				}
				pthread_mutex_unlock( &( ((authenticate_node*)pos)->lock ) );
			}
		}
		pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙
		
		sleep(cgv_report_interval);
	}
ERR:
	xyprintf(0, "AUTHENTICATE:✟ ✟ ✟ ✟  -- %s %d:Refresh status pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  认证服务器连接处理线程函数
 *@param  fd		类型 void*		线程启动参数,传递认证服务器信息authenticate_node
 *@return nothing
 */
void* auth_conn_process(void *fd){
	pthread_detach(pthread_self());
	authenticate_node *node = fd;

	//接收连接报文头 并判断
	ag_msg_conn msg;
	if( wt_recv_block(node->sockfd, &msg, sizeof(ag_msg_conn) ) ){
		xyprintf(0, "AUTHENTICATE_ERROR:%d %s %d -- Recv report massage head error!", node->sockfd, __FILE__, __LINE__);
		goto ERR;
	}
	if(msg.head.head != AG_HEAD_HEAD || msg.head.order != ag_authenticate || msg.head.len != sizeof(ag_msg_conn) ||
		msg.passwd != AG_PASS ){
		xyprintf(0, "AUTHENTICATE_ERROR:%d %s %d -- Report massage data error!", node->sockfd, __FILE__, __LINE__);
		goto ERR;
	}
	
	//回复连接报文
	ag_msg_conn_res res;

	//TODO:认证服务器信息验证
	wt_sql_handle	handle;
	if( wt_sql_init(&handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		res.conn_res = AG_CONN_FAIL_ERR;	
	}
	else {
		unsigned int id, isok, platform_port;
		SQLBindCol( handle.sqlstr_handle, 1, SQL_C_ULONG, &id,				20, &handle.sql_err);//将变量和查询结果绑定
		SQLBindCol( handle.sqlstr_handle, 2, SQL_C_ULONG, &isok,			20, &handle.sql_err);//将变量和查询结果绑定
		SQLBindCol( handle.sqlstr_handle, 3, SQL_C_ULONG, &platform_port,	20, &handle.sql_err);//将变量和查询结果绑定
	
		//3.3执行SQL语句
		sprintf(handle.sql_str, "SELECT id, isok, onlinemacport FROM mx_sys_reserver WHERE servip = '%s'", inet_ntoa( node->sin_addr ) );
		if(wt_sql_exec(&handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
			wt_sql_destroy(&handle);
			res.conn_res = AG_CONN_FAIL_ERR;	
		}
		else {
			//4.3处理查询结果
			handle.sql_ret = SQLFetch(handle.sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
			if(handle.sql_ret == SQL_NO_DATA){
				res.conn_res = AG_CONN_FAIL_NONE;
				xyprintf(0, "There is no information of this server - %s!", inet_ntoa( node->sin_addr ) );
			}
			else if(!isok){
				res.conn_res = AG_CONN_FAIL_ISOK;
				xyprintf(0, "The server is not ok! - %s", inet_ntoa( node->sin_addr ) );
			}
			else {
				res.conn_res	= AG_CONN_SUCC;
				res.auth_code	= id;
				res.platform_port = platform_port;
				node->auth_code	= id;
			}
			wt_sql_destroy(&handle);
		}
	}
	
	res.head.head  = AG_HEAD_HEAD;
	res.head.order = ag_authenticate_res;
	res.head.len   = sizeof(res);
	if( wt_send_block(node->sockfd, &res, sizeof(res) ) ){
		xyprintf(0, "AUTHENTICATE_ERROR:%d %s %d : Send report res massage error!", node->sockfd, __FILE__, __LINE__);
		goto ERR;
	}

	if(res.conn_res != AG_CONN_SUCC){
		goto ERR;
	}

	//接收状态报告报文
	ag_msg_stat stat;
	if( wt_recv_block(node->sockfd, &stat, sizeof(ag_msg_stat) ) ){
		xyprintf(0, "AUTHENTICATE_ERROR:%d %s %d -- Recv report massage error!", node->sockfd, __FILE__, __LINE__);
		goto ERR;
	}
	if(stat.head.head != AG_HEAD_HEAD || stat.head.order != ag_stat_report || stat.head.len != + sizeof(ag_msg_stat) ){
		xyprintf(0, "AUTHENTICATE_ERROR:%d %s %d -- Report massage data error!", node->sockfd, __FILE__, __LINE__);
		goto ERR;
	}

	//填充状态 并将连接添加到链表内
	node->port			= msg.port;
	node->router_num	= stat.router_num;
	node->cur_queue_size= stat.cur_queue_size;
	node->cpu_usage		= stat.cpu_usage;
	node->total_mem		= stat.total_mem;
	node->used_mem		= stat.used_mem;
	node->free_mem		= stat.free_mem;
	//node->shared_mem	= stat.shared_mem;
	node->buffers_mem	= stat.buffers_mem;
	node->cached_mem	= stat.cached_mem;
	node->swap			= stat.swap;
	time(&(node->last_time));
	node->stat			= 1;
	node->load_weight  = stat.used_mem * 100 / stat.total_mem;
	pthread_mutex_init( &node->lock, 0 );

	pthread_mutex_lock(&gv_authenticate_list_lock);
	list_add(&(node->node), &gv_authenticate_list_head);
	gv_authenticate_count++;
	xyprintf(0, "AUTHENTICATE:ip is %s, gv_authenticate_count = %d", inet_ntoa(node->sin_addr), gv_authenticate_count);
	pthread_mutex_unlock(&gv_authenticate_list_lock);
	pthread_exit(NULL);

ERR:
	close(node->sockfd);
	xyprintf(0, "AUTHENTICATE_ERROR:✟ ✟ ✟ ✟  -- %s %d:Report_fun pthread is unnatural deaths, socfd = %d!!", __FILE__, __LINE__, node->sockfd);
	if(node){
		free(node);
	}
	pthread_exit(NULL);
}

/** 
 *@brief  认证服务器连接监听线程函数
 *@param  fd		类型 void*		线程启动参数,未使用
 *@return nothing
 */
void* auth_conn_thread(void *fd){
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Authenticate connection thread is running!!!");
	while(1){
		int sockfd;
		if( wt_sock_init( &sockfd, cgv_report_port, MAX_EPOLL_NUM ) ){
			xyprintf(errno, "AUTHENTICATE_ERROR:0 %s %d -- wt_sock_init()", __FILE__, __LINE__);
			sleep(5);
			continue;
		}
		struct sockaddr_in client_address;//存放客户端地址信息
		int client_len = sizeof(client_address);
		while(1){
			int client_sockfd;
			client_sockfd = accept(sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);  
			if(client_sockfd == -1){ 
				xyprintf(errno, "AUTHENTICATE_ERROR:%d %s %d -- accept()", sockfd, __FILE__, __LINE__);
				break;
			}

			xyprintf(0, "AUTHENTICATE:Authenticate server of ip is %s connection, sockfd is %d", inet_ntoa( client_address.sin_addr) , client_sockfd);
			authenticate_node *node = malloc(sizeof(authenticate_node));
			if(node == NULL){
				xyprintf(0, "AUTHENTICATE_ERROR: %s %d malloc error!", __FILE__, __LINE__);
				break;
			}
			node->sockfd = client_sockfd;
			memcpy(&(node->sin_addr), &(client_address.sin_addr), sizeof(client_address.sin_addr) );

			pthread_t thread;//创建不同的子线程以区别不同的客户端  
			if(pthread_create(&thread, NULL, auth_conn_process, node) != 0){//创建子线程
				xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
				break;
			}
		}
		close(sockfd);
	}
	xyprintf(0, "AUTHENTICATE:✟ ✟ ✟ ✟  -- %s %d:Report_start pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

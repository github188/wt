/*****************************************************
 *
 * 对设备的引导
 *
 *****************************************************/
#include "guide_header.h"

/** 
 *@brief  设备引导处理线程函数
 *@param  fd		类型 void*		线程启动参数,传递设备的socket套接字
 *@return nothing
 */
void* guide_process(void *fd){
	pthread_detach(pthread_self());

	int	client_sockfd = (int)(long)fd;

#if IS_CONN_BOSS
	// BOSS 是否可用
	pthread_mutex_lock(&gv_boss_flag_lock);
	if(!gv_boss_flag){
		pthread_mutex_unlock(&gv_boss_flag_lock);
		xyprintf(0, "0(:3　)～ ('､3_ヽ)_");
		sleep(10);
		wt_close_sock(&client_sockfd);
		pthread_exit(NULL);
	}
	pthread_mutex_unlock(&gv_boss_flag_lock);
#endif

	gv_guide_router_count++;
	//xyprintf(0, "ROUTER:** O(∩ _∩ )O ~~ A new router guide thread running, guide_router_count = %d!", gv_guide_router_count);

	//如果没有认证服务器
	pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
	if( list_empty(&gv_authenticate_list_head) ){
		pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙
		xyprintf(0, "0(:3　)～ ('､3_ヽ)_ -- Not have authenticate!");
		sleep(10);
		wt_close_sock(&client_sockfd);
		pthread_exit(NULL);
	}
	pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙

	int				body_size;
	int				router_id;
	msg_head_st		*msg_head		= NULL;
	cer_msg_st		*msg			= NULL;
	random_pair		random_num;
	
	msg_head = (msg_head_st*)malloc( sizeof(msg_head_st) );
	if(msg_head == NULL){
		xyprintf(0, "ROUTER_ERROR: %s %d malloc error!", __FILE__, __LINE__);
		goto SOCK_ERR;
	}

	//接收第一包数据 并判断
	if( recv_msg_head(client_sockfd, msg_head) ){//获取数据包头
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- : Recv router's massage head error!", client_sockfd, __FILE__, __LINE__);
		goto SOCK_ERR;
	}
	if( msg_head->mode != 1 || msg_head->order != msg_order_auth || msg_head->ver != 4){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- : Router's massage head data error!", client_sockfd, __FILE__, __LINE__);
		xyprintf_msg_head(msg_head);
		goto SOCK_ERR;
	}

	body_size	= msg_head->len - sizeof(msg_head_st);
	
	msg			= (cer_msg_st*)malloc( body_size );
	if(msg == NULL){
		xyprintf(0, "ROUTER_ERROR: %s %d malloc error!", __FILE__, __LINE__);
		goto SOCK_ERR;
	}

	//接收包体
	if( recv_cer_msg_head(client_sockfd, msg, body_size) ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- : Recv router's cer massage error!", client_sockfd, __FILE__, __LINE__);
		goto SOCK_ERR;
	}
	//判断
	if(msg->order != 1 || msg->srv_type != srv_type_main){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- : Router's cer massage data error!", client_sockfd, __FILE__, __LINE__);
		xyprintf_cer_msg_head(msg);
		goto SOCK_ERR;
	}
	
	//xyprintf(0, "%u: <-- <-- Recv frist cer_msg success <-- <--", msg_head->router_id);

	//数据库操作所需参数
	wt_sql_handle	handle;
	
	//初始化数据库连接
	if( wt_sql_init(&handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		goto SOCK_ERR;
	}

#if IS_CONN_BOSS
	// 判断设备数量
	int boss_falg;

	pthread_mutex_lock(&gv_boss_flag_lock);
	boss_falg = gv_num_falg;
	pthread_mutex_unlock(&gv_boss_flag_lock);
	
	if( boss_falg == BG_NUM_FLAG_TOTAL ){
		// 总数量的判断
		int count_id;
		SQLBindCol( handle.sqlstr_handle, 1, SQL_C_ULONG, &count_id,		20, &handle.sql_err);
		sprintf(handle.sql_str, "SELECT COUNT(id) FROM mx_shebei WHERE isonlien = 1");
		if(wt_sql_exec(&handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
			goto SQLED_ERR;
		}
		handle.sql_ret = SQLFetch(handle.sqlstr_handle);
		SQLFreeStmt(handle.sqlstr_handle, SQL_CLOSE);
	
		// 进行判断
		pthread_mutex_lock(&gv_boss_flag_lock);
		if( count_id >= gv_total_num ){
			pthread_mutex_unlock(&gv_boss_flag_lock);
			xyprintf(0, "ERROR:%s %d", __FILE__, __LINE__);
			goto SQLED_ERR;
		}
		pthread_mutex_unlock(&gv_boss_flag_lock);
	} else if (boss_falg == BG_NUM_FLAG_TYPE ){
	} else {
		xyprintf(0, "ERROR:%s %d", __FILE__, __LINE__);
		goto SQLED_ERR;
	}

#endif


	//3.2绑定变量和SQL语句查询结果
	__u32 id, shanghuid, rid, isok, iscandoing, xinghaoid;
	char xinghao[16];
	SQLBindCol( handle.sqlstr_handle, 1, SQL_C_ULONG, &id,				20, &handle.sql_err);
	SQLBindCol( handle.sqlstr_handle, 2, SQL_C_ULONG, &shanghuid,		20, &handle.sql_err);
	SQLBindCol( handle.sqlstr_handle, 3, SQL_C_ULONG, &rid,				20, &handle.sql_err);
	SQLBindCol( handle.sqlstr_handle, 4, SQL_C_ULONG, &isok,			20, &handle.sql_err);
	SQLBindCol( handle.sqlstr_handle, 5, SQL_C_ULONG, &iscandoing,		20, &handle.sql_err);
	SQLBindCol( handle.sqlstr_handle, 6, SQL_C_ULONG, &xinghaoid,		20, &handle.sql_err);
	SQLBindCol( handle.sqlstr_handle, 7, SQL_C_CHAR,  xinghao,			16, &handle.sql_err);

	//3.3执行SQL语句
	sprintf(handle.sql_str, "SELECT TOP 1 id, shanghuid, rid, \
			isok, iscandoing, xinghaoid, xinghao FROM mx_view_useshebei WHERE Hard_seq = '%s'", msg->hard_seq);
	if(wt_sql_exec(&handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
		goto SQLED_ERR;
	}
	//4.3处理查询结果
	handle.sql_ret = SQLFetch(handle.sqlstr_handle);
	if(handle.sql_ret == SQL_NO_DATA){
		xyprintf(0, "ROUTER_ERROR: %s %d -- Not has router's message in datebase, msg->hard_seq = %s!", __FILE__, __LINE__, msg->hard_seq);
		goto SQLED_ERR;
	}

	if(shanghuid == 0){
		xyprintf(0, "ROUTER_ERROR: %s %d -- Shanghu id is error, shanghuid = %u, msg->hard_seq = %s!", __FILE__, __LINE__, shanghuid, msg->hard_seq);
		goto SQLED_ERR;
	}
	//xyprintf(0, "iscandoing = %u, startdate_res = %d, enddate_res = %u", iscandoing, startdate_res, enddate_res);
	if(iscandoing == 0){
		xyprintf(0, "ROUTER_ERROR: %s %d -- Router not candoing or is expired, shanghuid = %u, msg->hard_seq = %s!", __FILE__, __LINE__, shanghuid, msg->hard_seq);
		goto SQLED_ERR;
	}
	SQLFreeStmt(handle.sqlstr_handle, SQL_CLOSE);

#if IS_CONN_BOSS
	if( boss_falg == BG_NUM_FLAG_TYPE ){
		// 设备数量的判断
		int count_id;
		SQLBindCol( handle.sqlstr_handle, 1, SQL_C_ULONG, &count_id,		20, &handle.sql_err);
		sprintf(handle.sql_str, "SELECT COUNT(id) FROM mx_shebei WHERE isonlien = 1 AND xinghaoid = %u", xinghaoid);
		if(wt_sql_exec(&handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
			goto SQLED_ERR;
		}
		handle.sql_ret = SQLFetch(handle.sqlstr_handle);
		SQLFreeStmt(handle.sqlstr_handle, SQL_CLOSE);

		if( RTS_NUM_PROCESS( xinghao, count_id) ){
			xyprintf(0, "GUIDE_ERROR:%s %d", __FILE__, __LINE__);
			goto SQLED_ERR;
		}
	}
#endif
	
	router_id = msg_head->router_id;

	//保存随机码 并回复第一包数据
	random_num.router_seq	= msg->router_seq;
	random_num.srv_seq		= random_num.router_seq << 6;
	msg->srv_seq			= random_num.srv_seq;
	msg->order++;
	msg_head->mode			= 2;
	//回复第一包数据
	if( send_cer_msg_head(client_sockfd,msg_head, msg, body_size) ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Send message to router is error!", client_sockfd, __FILE__, __LINE__);
		goto SQLED_ERR;
	}
	
	//xyprintf(0, "%u: --> --> Res frist cer_msg success --> -->", router_id);

	//接收第二包数据
	if( recv_msg_head(client_sockfd, msg_head) ){//获取数据包头
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Recv router's message head error!", client_sockfd, __FILE__, __LINE__);
		goto SQLED_ERR;
	}
	
	//判断	
	if( msg_head->mode != 1 || msg_head->order != msg_order_addr ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Router's message head data error!", client_sockfd, __FILE__, __LINE__);
		xyprintf_msg_head(msg_head);
		goto SQLED_ERR;
	}
	
	//取包体长度
	body_size = msg_head->len - sizeof(msg_head_st);
	msg = (cer_msg_st*)realloc( msg, body_size );
	
	if(msg == NULL){
		xyprintf(0, "ROUTER_ERROR: %s %d realloc error!", __FILE__, __LINE__);
		goto SOCK_ERR;
	}
	
	//收包体
	if( recv_cer_msg_head(client_sockfd, msg, body_size) ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Recv router's cer message error!", client_sockfd, __FILE__, __LINE__);
		goto SQLED_ERR;
	}
	
	//判断
	if(msg->order != 3 || msg->srv_type != srv_type_main ||
		msg->router_seq != random_num.router_seq || msg->srv_seq != random_num.srv_seq ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Router's cer message data error!", client_sockfd, __FILE__, __LINE__);
		xyprintf_cer_msg_head(msg);
		goto SQLED_ERR;
	}
	//xyprintf(0, "%u: <-- <-- Recv second cer_msg success <-- <--", router_id);

	//遍历认证服务器链表 找到负载最小的
	struct list_head*	pos;
	authenticate_node*	node;
	int					min_load_weight;		// 内存
	int					min_router_num;			// 设备数量
	int					min_cur_queue_size;		// 未处理报文数量
	struct in_addr		sin_addr;				// 地址
	int					port;					// 端口
	unsigned int		auth_code;				// 服务器代码

	pthread_mutex_lock(&gv_authenticate_list_lock);//⊙﹏⊙
	
	//取第一个服务器的值
	pos = gv_authenticate_list_head.next;
	
	node = (authenticate_node*)pos;
	min_load_weight		= node->load_weight;
	min_router_num		= node->router_num;
	min_cur_queue_size	= node->cur_queue_size;
	memcpy(&sin_addr, &(node->sin_addr), sizeof(struct in_addr));
	port				= node->port;
	auth_code			= node->auth_code;

	// 对比 找出负载最小的那个
	pos = pos->next;
	for( ; pos != &gv_authenticate_list_head; pos = pos->next ){
		//if( ((authenticate_node*)pos)->load_weight < min_load_weight ){
		//TODO 算法需要优化
		if( ((authenticate_node*)pos)->cur_queue_size < min_cur_queue_size && ((authenticate_node*)pos)->router_num < min_router_num ){
			node = (authenticate_node*)pos;
			min_load_weight		= node->load_weight;
			min_router_num		= node->router_num;
			min_cur_queue_size	= node->cur_queue_size;
			memcpy(&sin_addr, &(node->sin_addr), sizeof(struct in_addr));
			port				= node->port;
			auth_code			= node->auth_code;
		}
	}
	pthread_mutex_unlock(&gv_authenticate_list_lock);//⊙﹏⊙


	//回复第二包数据
	msg->order++;
	msg_head->mode = 2;
	sprintf(msg->auth_srv, "%s:%d", inet_ntoa(sin_addr), port);
	if( send_cer_msg_head(client_sockfd, msg_head, msg, body_size) ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Send cer message error!", client_sockfd, __FILE__, __LINE__);
		goto SQLED_ERR;
	}
	
	//xyprintf(0, "%u: --> --> Res second cer_msg success --> -->", router_id);

	//如果设备id发生变化 将表内rid为原rid的清空
	if( router_id != rid){
		sprintf(handle.sql_str, "UPDATE mx_shebei SET rid = 0 WHERE rid = %u", router_id);
		if(wt_sql_exec(&handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
			goto SQLED_ERR;
		}
	}

	//修改登陆设备的数据表记录 判断设备id是否变化 是否激活
	if( router_id != rid){
		//xyprintf(0, "ROUTER:Router's rid change to %u", id);
		if(isok){
			sprintf(handle.sql_str, "UPDATE mx_shebei SET rid = %d WHERE id = %u",
					router_id, id);
			if(wt_sql_exec(&handle)){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
				goto SQLED_ERR;
			}
		}
		else {
			//xyprintf(0, "ROUTER:Router activate, id is %u", id);
			sprintf(handle.sql_str, "UPDATE mx_shebei SET rid = %d, isok = 1, jihuodate = getdate() WHERE id = %u",
					router_id, id);
			if(wt_sql_exec(&handle)){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
				goto SQLED_ERR;
			}
			
			//修改商户表的设备激活数量
			sprintf(handle.sql_str, "UPDATE mx_shanghu SET usenum = usenum + 1, unusenum = devicenum - usenum - 1 WHERE id = %u",shanghuid);
			if(wt_sql_exec(&handle)){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
				goto SQLED_ERR;
			}
		}
	}
	else {
		if(isok){
			//sprintf(handle.sql_str, "UPDATE mx_shebei SET isonlien = 1, lastdotime = getdate(), lastsetdoingtime = getdate(), \
					reservid = %u WHERE id = %u", auth_code, id);
			//if(wt_sql_exec(&handle)){
			//	xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
			//	goto SQLED_ERR;
			//}
		}
		else {
			//xyprintf(0, "ROUTER:Router activate, id is %u", id);
			sprintf(handle.sql_str, "UPDATE mx_shebei SET isok = 1, jihuodate = getdate() WHERE id = %u", id);
			if(wt_sql_exec(&handle)){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
				goto SQLED_ERR;
			}
			
			//修改商户表的设备激活数量
			sprintf(handle.sql_str, "UPDATE mx_shanghu SET usenum = usenum + 1, unusenum = devicenum - usenum - 1  WHERE id = %u",shanghuid);
			if(wt_sql_exec(&handle)){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
				goto SQLED_ERR;
			}
		}
	}


	xyprintf(0, "ROUTER:** O(∩ _∩ )O ~~ A router is guide over, guide for %s load_weight is %d, guide_router_count = %d!\n", msg->auth_srv, min_load_weight, gv_guide_router_count);
	wt_sql_destroy(&handle);
	if(msg_head){
		free( msg_head );
	}
	if(msg){
		free( msg );
	}
	wt_close_sock(&client_sockfd);
	pthread_exit(NULL);

SQLED_ERR:
	wt_sql_destroy(&handle);
SOCK_ERR:
	if(msg_head){
		free( msg_head );
	}
	if(msg){
		free( msg );
	}
ERR:
	sleep(60);	//休眠60秒后 断开设备连接
	wt_close_sock(&client_sockfd);
	xyprintf(0, "ROUTER_ERROR:✟ ✟ ✟ ✟  -- %s %d:Guide pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}  

/** 
 *@brief  路由设备引导监听函数
 *@return error -1
 */
int guide_conn_thread()
{
	int sockfd;
	//socket 初始化
	if( wt_sock_init( &sockfd, cgv_guide_port, MAX_EPOLL_NUM ) ){
		return -1;
	}
	struct sockaddr_in client_address;//存放客户端地址信息
	int client_len = sizeof(client_address);

	//持续监听
	while(1){
		int client_sockfd;
		client_sockfd = accept(sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);  
		if(client_sockfd == -1){ 
			xyprintf(errno, "ROUTER_ERROR:%d %s %d -- accept()", sockfd, __FILE__, __LINE__);
			break;
		}
		xyprintf(0, "ROUTER:♨ ♨ ♨ ♨  -- Router %s connection, sockfd is %d", inet_ntoa( client_address.sin_addr) , client_sockfd);
		
		//创建新线程以引导设备
		pthread_t thread;
		if(pthread_create(&thread, NULL, guide_process, (void*)(long)client_sockfd) != 0){
			xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
			break;
		}
	}
	close(sockfd);
}

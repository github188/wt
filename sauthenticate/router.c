/*****************************************************
 *
 * 设备接入 管理 等
 *
 *****************************************************/

#include "sauth_header.h"

/** 
 *@brief  设备连接认证函数 在accept_fun内被添加
 *@param  fd		类型 void*				连接设备的sockfd
 *@param  handle    类型 wt_sql_handle*		数据库操作资源集合
 *@return 数据库出错返回WT_SQL_ERROR 其他返回0
 */
void* authenticate_process(void *fd)
{
	pthread_detach( pthread_self() );
	
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
	xyprintf(0, "%d: <-- <-- Recv frist cer_msg success <-- <--", client_sockfd );

	// 建立数据库连接并查询设备对应的代理商
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		free(handle);
		goto MSG_ERR;
	}
	__u32 userid;
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, &userid,				20, &handle->sql_err);
	
	sprintf(handle->sql_str, "SELECT TOP 1 userid FROM mx_s_shebei WHERE Hard_seq = '%s'", msg->hard_seq);
	if(wt_sql_exec(handle)){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		goto SQLED_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	if(handle->sql_ret == SQL_NO_DATA){
		xyprintf(0, "ROUTER_ERROR:%s %d -- Not has router's message in datebase!", __FILE__, __LINE__);
		goto SQLED_ERR;
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);

	// 判断 引导服务器id 并 获取引导服务器地址和端口
	char url[GUIDE_URL_LEN] = {0};
	unsigned int port = 0;
	if( userid != 0 && guide_list_get(userid, url, &port) ){
		xyprintf(0, "ROUTER_ERROR:%s %d -- Router's userid is error!",	__FILE__, __LINE__);
		goto SQLED_ERR;
	}

	//保存随机码 并回复第一包数据
	random_num.router_seq	= msg->router_seq;
	random_num.srv_seq		= random_num.router_seq << 6;
	msg->srv_seq			= random_num.srv_seq;
	msg->order++;
	msg_head->mode			= 2;
	//发送回复包
	if( send_cer_msg_head(client_sockfd, msg_head, msg, body_size) ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Send cer message to router is error!", client_sockfd, __FILE__, __LINE__);
		goto SQLED_ERR;
	}
	xyprintf(0, "%d: --> --> Res frist cer_msg success --> -->", client_sockfd);

	//接收第二包数据
	if( recv_msg_head(client_sockfd, msg_head) ){//获取数据包头
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Recv router's message head error!", client_sockfd, __FILE__, __LINE__);
		goto SQLED_ERR;
	}
	//判断	
	if( msg_head->mode != 1 || msg_head->order != msg_order_auth ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Router's message head data error!", client_sockfd, __FILE__, __LINE__);
		xyprintf_msg_head(msg_head);
		goto SQLED_ERR;
	}
	
	//取包体长度
	body_size = msg_head->len - sizeof(msg_head_st);
	msg			= (cer_msg_st*)realloc( msg, body_size );
	
	if(msg == NULL){
		xyprintf(0,"ROUTER_ERROR: %s %d realloc error!", __FILE__, __LINE__);
		goto SQLED_ERR;
	}
	
	//收包体
	if( recv_cer_msg_head(client_sockfd, msg, body_size) ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Recv router's cer message error!", client_sockfd, __FILE__, __LINE__);
		goto SQLED_ERR;
	}
	
	//判断
	if( msg->order != 3 || msg->srv_type != srv_type_auth ||
		msg->router_seq != random_num.router_seq || msg->srv_seq != random_num.srv_seq ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Router's cer message data error!", client_sockfd, __FILE__, __LINE__);
		xyprintf_cer_msg_head(msg);
		goto SQLED_ERR;
	}
	
	xyprintf(0, "%d: <-- <-- Recv second cer_msg success <-- <--", client_sockfd);

	// 下发修改引导服务器指令
	if( send_addr_set(client_sockfd, msg_head, msg_order_guide_addr_set, url, port) ){
		goto SQLED_ERR;
	}
	
	xyprintf(0, "%d: O(∩ _∩ )O ~~ Authenticate is over!!!", client_sockfd);
	
SQLED_ERR:
	wt_sql_destroy(handle);
	if(handle){
		free(handle);
	}
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
	pthread_exit( NULL );
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
	pthread_t pt;
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
		
		xyprintf(0, "♨ ♨ ♨ ♨  -- router %s connection, sockfd is %d", inet_ntoa( client_address.sin_addr) , client_sockfd);
		if( pthread_create(&pt, NULL, authenticate_process, (void*)(long)client_sockfd) != 0 ){
			xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
		}
		
		usleep( 100 * 1000 );
	}
ERR:
	close( sockfd );
	xyprintf(0, "✟ ✟ ✟ ✟ -- %s %d:Accept pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit( NULL );
}

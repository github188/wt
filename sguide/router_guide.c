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

	gv_guide_router_count++;
	//xyprintf(0, "ROUTER:** O(∩ _∩ )O ~~ A new router guide thread running, guide_router_count = %d!", gv_guide_router_count);

	int				body_size;
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
	
	//数据库操作所需参数
	wt_sql_handle	handle;
	
	//初始化数据库连接
	if( wt_sql_init(&handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		goto SOCK_ERR;
	}

	//3.2绑定变量和SQL语句查询结果
	__u32 userid;
	SQLBindCol( handle.sqlstr_handle, 1, SQL_C_ULONG, &userid,				20, &handle.sql_err);

	//3.3执行SQL语句
	sprintf(handle.sql_str, "SELECT TOP 1 userid FROM mx_s_shebei WHERE Hard_seq = '%s'", msg->hard_seq);
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

	if(userid == 0){
		xyprintf(0, "ROUTER_ERROR: %s %d -- This router is not distribution agents, msg->hard_seq = %s!", __FILE__, __LINE__, msg->hard_seq);
		goto SQLED_ERR;
	}
	SQLFreeStmt(handle.sqlstr_handle, SQL_CLOSE);

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
	//回复第二包数据
	msg->order++;
	msg_head->mode = 2;
	sprintf(msg->auth_srv, "%s:%d", AUTHENTICATE_ADDR, cgv_authenticate_port);
	if( send_cer_msg_head(client_sockfd, msg_head, msg, body_size) ){
		xyprintf(0, "ROUTER_ERROR:%d %s %d -- Send cer message error!", client_sockfd, __FILE__, __LINE__);
		goto SQLED_ERR;
	}
	
	xyprintf(0, "ROUTER:** O(∩ _∩ )O ~~ A router is guide over, guide_router_count = %d!\n", gv_guide_router_count);
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

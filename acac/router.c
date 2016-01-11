/*****************************************************
 *
 * 设备接入 管理 等
 *
 *****************************************************/

#include "acac_header.h"

#define ROUTER_DEBUG			0

/** 
 *@brief  将对应设备连接置错,在下个epoll循环中删除
 *@param  sockfd	类型 int	网络连接套接字
 *@return nothing
 */
void inline socklist_sockfd_err(int sockfd)
{
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

int inline socklist_rid_call_all(unsigned int rid, sock_list *sock)
{
	//查找平台要求控制的设备是否存在 如果存在 就将数据拷贝出来
	struct list_head* pos;
	int find_flag = -1;
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
 *@brief  更新对应设备信息,并复制出来
 *@param  sockfd	类型 int	网络连接套接字
 *@return succ 0, failed -1
 */
int inline socklist_sockfd_stat_call_all(int sockfd, sock_list* sock)
{
	//查找平台要求控制的设备是否存在 如果存在 就将数据拷贝出来
	struct list_head* pos;
	int find_flag = -1;
	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	list_for_each(pos,&gv_sock_list_head){
		if(((sock_list*)pos)->sockfd == sockfd){
			((sock_list*)pos)->stat = SOCK_STAT_ADD;
			time( &((sock_list*)pos)->last_heart_time );
			((sock_list*)pos)->heart_count ++;
			memcpy(sock, pos, sizeof(sock_list));
			find_flag = 0;
			break;
		}
	}
	pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
	return find_flag;
}

/** 
 *@brief  更新aplist表信息
 *@return succ 0, failed 
 */
int update_aplist(ac_state_st *msg, sock_list *sock, wt_sql_handle* handle)
{
	big_little32(&msg->deviceID);
	big_little32(&msg->state);
	big_little32(&msg->count);
	
	big_little_stat_ap(msg);
	
#if ROUTER_DEBUG
	xyprintf_ac_state(msg);
	xyprintf(0, "%d:Update aplist!!", msg->deviceID);
#endif
	
	int i;
	for(i = 0; i < msg->count; i++){
		// 查询ap信息在数据库中是否存在
		
		char name[32] = { 0 };
		char group[32] = { 0 };
		int j;
		for(j = 0; j < 31; j++){
			if(msg->ap[i].base_info.ap_name[j] == '>' && msg->ap[i].base_info.ap_name[ j + 1 ] == '>'){
				memcpy(name, msg->ap[i].base_info.ap_name, j);
				memcpy(group, msg->ap[i].base_info.ap_name + j + 2, 30 - j);
				break;
			}
		}

#if ROUTER_DEBUG
		xyprintf(0, "name = %s\n\tgroup = %s", name, group);
#endif

		unsigned int id;
		SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, &id,			20, &handle->sql_err);
		sprintf(handle->sql_str, "SELECT TOP 1 id from mx_ac_ap WHERE rid = %u and apsncode = %u", sock->router_id, msg->ap[i].device_id);
		if(wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			goto SQLED_ERR;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
		if(handle->sql_ret == SQL_NO_DATA){
			// 没有 插入
			SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			sprintf(handle->sql_str, "INSERT INTO mx_ac_ap(dailiid, shanghuid, rid, \
					apsncode, apsncodex, apname, apgroup, apssid, apdriverversion, macnum, isonline, lastdotime, createdate) \
					VALUES(%u, %u, %u, \
					%u, '%X', '%s', '%s', '%s', '%s', %u, 1, GETDATE(), GETDATE())",
				sock->dailiid, sock->shanghuid, sock->router_id,
				msg->ap[i].device_id, msg->ap[i].device_id, name, group, msg->ap[i].wifi_base_info.list[0].ssid,
				msg->ap[i].base_info.ap_version, msg->ap[i].user.user_num );
			if(wt_sql_exec(handle)){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}
		}
		else {
			// 有 更新
			SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			sprintf(handle->sql_str, "UPDATE mx_ac_ap set apname = '%s', apgroup = '%s', apssid = '%s', \
					apdriverversion = '%s', macnum = %u, isonline = 1, lastdotime = GETDATE() WHERE id = %u",
				name, group, msg->ap[i].wifi_base_info.list[0].ssid,
				msg->ap[i].base_info.ap_version, msg->ap[i].user.user_num, id);
			if(wt_sql_exec(handle)){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
				goto SQLED_ERR;
			}
		}
	}
	return 0;
SQLED_ERR:
	return WT_SQL_ERROR;
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
	
	ac_head_st	*msg_head	= (ac_head_st*)malloc( sizeof(ac_head_st) );
	memset(msg_head, 0, sizeof(ac_head_st));

	//接收包头
	if( recv_ac_head(sockfd, msg_head) ){//获取数据包头
		xyprintf(0, "ROUTER_ERROR:%s %s %d -- Recv message head form router of sockfd is %d error!", __func__, __FILE__, __LINE__,sockfd);
		goto HEAD_ERR;
	}

	//包头数据判断
	if( msg_head->ver != 4 || msg_head->datalen <= sizeof(ac_head_st) ){
		xyprintf(0, "ROUTER_ERROR:%s %s %d -- The message head date of Router's sockfd is %d error!", __func__, __FILE__, __LINE__, sockfd);
		xyprintf_ac_head(msg_head);
		goto HEAD_ERR;
	}
#if ROUTER_DEBUG
	xyprintf_ac_head(msg_head);
#endif

	//申请包体空间
	int size = msg_head->datalen - sizeof(ac_head_st);
	void *buf = (ac_state_st*)malloc( size );
	memset(buf, 0, size );


	if( msg_head->cmdID == AC_CMD_UPLOAD_APINFO ){//状态报告信息
		ac_state_st *msg = buf;
		if( recv_ac_state(sockfd, msg, size) ){
			xyprintf(0, "ROUTER_ERROR:%s %s %d -- Recv message form router of sockfd is %d error!", __func__, __FILE__, __LINE__, sockfd);
			goto MSG_ERR;
		}
#if ROUTER_DEBUG
		xyprintf_ac_state(msg);
		xyprintf(0, "%u:<-- <-- Recv the state message success <-- <--, ap count is %u", msg->deviceID, msg->count);
#endif
	
		//回复状态报告信息
		msg_head->cmdID		= AC_CMD_UPLOAD_APINFO_ACK;
		msg_head->mode		= 1;
		//msg_head->deviceID	= ;
		
		int i;
		for(i = 0; i < msg->count; i++){
			msg->ap[i].flag = 0;
		}
	
		if( send_ac_state(sockfd, msg_head, msg) ){
			xyprintf(0, "ROUTER_ERROR:%s %s %d -- Return the message to router of sockfd is %d is error!", __func__, __FILE__, __LINE__, sockfd);
			goto MSG_ERR;
		}

		// 更新链表中的状态信息 和 数据库
		sock_list *sock = malloc(sizeof(sock_list));
		memset(sock, 0, sizeof(sock_list));

		int find_flag = socklist_sockfd_stat_call_all(sockfd, sock);
		int ret = 0;
		if(!find_flag){
#if ROUTER_DEBUG
			xyprintf(0, "%u:--> --> Return the state message success --> -->, count = %u", sock->router_id, sock->heart_count);
#endif
			ret = update_aplist(msg, sock, handle);
		}
		else {
			xyprintf(0, "ROUTER_ERROR:%s %s %d -- Error should not happen, sockfd is %d!!!", __func__, __FILE__, __LINE__, sockfd);
		}

		if(sock){
			free(sock);
		}
		if(buf){
			free(buf);
		}
		if(msg_head){
			free(msg_head);
		}
		return (void*)(long)ret;
	}
	else if(msg_head->cmdID == AC_CMD_PROXY_QUEST_ACK) {//代理请求回复
		ac_proxy_st *msg = buf;

		//接收报文体
		if( recv_ac_proxy(sockfd, msg) ){
			xyprintf(0, "ROUTER_ERROR:%s %s %d -- Recv message form router of sockfd is %d error!", __func__, __FILE__, __LINE__, sockfd);
			goto MSG_ERR;
		}

		//各种晒
		//xyprintf_ac_proxy(msg);
		
		if( msg->enable == 78 ){
			//准备数据 回复platform
			unsigned char buf[256] = { 0 };
			if(msg->port){		//根据路由器返回的成功或失败不同而返回给平台不同的数据
				snprintf(buf, sizeof(buf), "{\"res\":\"1\",\"data\":\"%s:%d\"}\n", msg->ctrlURL, msg->port);
			}
			else{
				snprintf(buf, sizeof(buf), "{\"res\":\"2\",\"data\":\"%s\"}\n", msg->msg);		//不要用 sizeof(temp) 只会返回系统位数对应的值
			}
			
			if( wt_send_block( msg->socket_php, buf, strlen(buf) ) ){		//发送给平台
				xyprintf(0, "PLATFORM_ERROR:Send proxy res msg to platform error, sockfd is %d!!!", msg->socket_php);
				xyprintf_ac_proxy(msg);
			}
			else {
				xyprintf(0, "PLATFORM:proxy res to platform(sockfd:%d) success : %s", msg->socket_php, buf);
			}

			close(msg->socket_php);	//关闭连接 因为没用了
		}
	}
	else if(msg_head->cmdID == AC_CMD_LOGIN){
		ac_login_st *msg = buf;
		if( recv_ac_login(sockfd, msg) ){
			xyprintf(0, "ROUTER_ERROR:%s %s %d -- Recv message form router of sockfd is %d error!", __func__, __FILE__, __LINE__, sockfd);
			goto MSG_ERR;
		}
		xyprintf(0, "%u:<-- <-- Recv login msg success again <-- <--", msg->deviceID);
		//xyprintf_ac_login(msg);
	}
		
	//将sockfd添加到epoll列表
	struct list_head* pos;
	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	list_for_each(pos,&gv_sock_list_head){
		if(((sock_list*)pos)->sockfd == sockfd){
			((sock_list*)pos)->stat = SOCK_STAT_ADD;
			break;
		}
	}
	pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙

	if(buf){
		free(buf);
	}
	if(msg_head){
		free(msg_head);
	}
	return (void*)0;
MSG_ERR:
	if( buf ){
		free( buf );
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

	xyprintf(0, "%d: ╮ ( ╯ ▽ ╰ ) ╭ Router offline thread is running!!!", sock->router_id);
	//关闭sockfd
	wt_close_sock( &(sock->sockfd) );								//关闭socket
	
	free( sock );
	//xyprintf(0, "╮ ( ╯ ▽ ╰ ) ╭ Router offline thread is over!!!");
	return (void*)0;
STR_ERR:
	free( sock );
	xyprintf(0, "╮ ( ╯ ▽ ╰ ) ╭ Router offline thread is over, has error!!!");
	return (void*)WT_SQL_ERROR;
}

/** 
 *@brief  epoll线程
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* epoll_thread(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Epoll thread is running!!!");

	int ret, i;
	struct list_head *pos;	//设备list遍历变量
	
	int epfd = epoll_create( MAX_EPOLL_NUM );		//创建epoll
	if(epfd == -1){
		xyprintf(errno, "EPOLL_ERROR:%s %s %d -- epoll_create()", __func__, __FILE__, __LINE__);
		goto CREATE_ERR;
	}
	
	struct epoll_event *events;
	events = (struct epoll_event*)malloc (1024 * sizeof(struct epoll_event) );	//一次最多获得1024条结果
	memset(events, 0, 1024 * sizeof(struct epoll_event));

	while(1){
		pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
		for( pos = gv_sock_list_head.next; pos != &gv_sock_list_head; pos = pos->next ){//遍历设备list 添加或删除设备到epoll队列中

			sock_list *sock = (sock_list*)pos;

			if( time(0) - sock->last_heart_time > sgv_acac_interval + 60 ){	//设备的最后活动时间 超过两次间隔时间则在下面删除
				sock->stat = SOCK_STAT_DEL;
			}

			if( sock->stat == SOCK_STAT_ADD ){		//需要添加的设备
				struct epoll_event event;
				event.data.fd = sock->sockfd;
				event.events = EPOLLIN | EPOLLRDHUP;
				ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sock->sockfd, &event);
				if(ret == -1){
					xyprintf(errno, "EPOLL_ERROR:%s %s %d -- epoll_ctl()", __func__, __FILE__, __LINE__);
					sock->stat = SOCK_STAT_DEL;
				}
				else {
					sock->stat = SOCK_STAT_ADDED;
				}
			}
		
			if( sock->stat == SOCK_STAT_DEL ) {		//需要在列表中删除的设备
				struct epoll_event event;
				event.data.fd = sock->sockfd;
				event.events = EPOLLIN | EPOLLRDHUP;
				ret = epoll_ctl(epfd, EPOLL_CTL_DEL, sock->sockfd, &event);	//从监听列表中删除
				if(ret == -1){
					xyprintf(errno, "EPOLL_ERROR:%s %s %d -- epoll_ctl()", __func__, __FILE__, __LINE__);
				}
				
				//从链表中删除
				struct list_head *temp = pos;
				pos = temp->prev;
				list_del( temp );
				gv_sock_count--;
				xyprintf(0, "%u:☠ ☠ ☠ ☠  -- sock_count = %d", ((sock_list*)temp)->router_id, gv_sock_count);
			
				pool_add_worker( router_offline_fun, (void*)temp );	//添加到接收并处理任务队列
			}
		}
		pthread_mutex_unlock( &gv_list_lock );//⊙﹏⊙

		ret = epoll_wait(epfd, events, 1024, 100);		//epoll_wait
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
						xyprintf(errno, "EPOLL_ERROR:%s %s %d -- epoll_ctl()", __func__, __FILE__, __LINE__);
					}
					pool_add_worker( msg_recv_process, (void*)(long) (events[i].data.fd) );	//添加到接收并处理任务队列
				}
				else {
					xyprintf(0,"EPOLL_ERROR:%s %s %d", __func__, __FILE__, __LINE__);
				}//end if
			}//end if
		}//end for
	}
CTL_ERR:
	close(epfd);
	free(events);
CREATE_ERR:
	xyprintf(0, "EPOLL_ERROR:%s %s %d -- Epoll pthread is unnatural deaths!!!", __func__, __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  设备连接认证函数 在accept_fun内被添加
 *@param  fd		类型 void*				连接设备的sockfd
 *@param  handle    类型 wt_sql_handle*		数据库操作资源集合
 *@return 数据库出错返回WT_SQL_ERROR 其他返回0
 */
void* ac_process_fun(void *fd, wt_sql_handle *handle)
{
	xyprintf(0, "O(∩ _∩ )O ~~ A new router request Management!!!");

	int				client_sockfd	= (int)(long)fd;

	ac_head_st	*msg_head	= (ac_head_st*)malloc( sizeof(ac_head_st) );
	memset(msg_head, 0, sizeof(ac_head_st));

	//接收包头
	if( recv_ac_head(client_sockfd, msg_head) ){//获取数据包头
		xyprintf(0, "ROUTER_ERROR:%s %s %d -- Recv router's(sockfd:%d) massage head error!", __func__, __FILE__, __LINE__, client_sockfd);
		goto HEAD_ERR;
	}

	//判断
	if( msg_head->ver != 4 || msg_head->cmdID != AC_CMD_LOGIN ||
		msg_head->datalen != sizeof(ac_head_st) + sizeof(ac_login_st) ){
		xyprintf(0, "ROUTER_ERROR:%s %s %d -- Router's(sockfd:%d) message head date error!", __func__, __FILE__, __LINE__, client_sockfd);
		xyprintf_ac_head(msg_head);
		goto HEAD_ERR;
	}
#if ROUTER_DEBUG
	xyprintf_ac_head(msg_head);
#endif
	
	ac_login_st	*msg = (ac_login_st*)malloc( sizeof(ac_login_st) );
	memset(msg, 0, sizeof(ac_login_st) );
	//接收登陆信息
	if( recv_ac_login(client_sockfd, msg) ){
		xyprintf(0, "ROUTER_ERROR:%s %s %d -- Recv router's(sockfd:%d) massage head error!", __func__, __FILE__, __LINE__, client_sockfd);
		goto MSG_ERR;
	}
#if ROUTER_DEBUG
	xyprintf(0, "%u: <-- <-- Recv login msg success <-- <--", msg->deviceID );
	xyprintf_ac_login(msg);
#endif

	__u32 id = 0, shanghuid = 0, dailiid = 0;
	
	// 如果是设备AC的话 就判断是否正确
	if( strncmp(msg->sn, "RTAC_", 5) == 0 && msg_head->device_type == 1 ){
		//3.2绑定变量和SQL语句查询结果
		SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG, &id,			20, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 2, SQL_C_ULONG, &shanghuid,	20, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 3, SQL_C_ULONG, &dailiid,		20, &handle->sql_err);

		//3.3执行SQL语句
		sprintf(handle->sql_str, "SELECT TOP 1 id, shanghuid, dailiid from mx_shebei WHERE hard_seq = '%s'", msg->sn + 5 );
#if ROUTER_DEBUG
		xyprintf(0, "%s", handle->sql_str);
#endif
		if(wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			goto SQLED_ERR;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
		if(handle->sql_ret == SQL_NO_DATA){
			xyprintf(0, "ROUTER_ERROR:%s %d -- Not has router's message in datebase, hard_seq = %s", __FILE__, __LINE__, msg->sn);
			msg->state = AC_LOGIN_NOUSER;
		}
		else {
			msg->state = AC_LOGIN_LOGINOK;
		}
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

		if(msg->deviceID != id){
			xyprintf(0, "ROUTER: -> <- Change deviceID ( %u -> %u ) -> <-", msg->deviceID, id);
			msg->deviceID		= id;
			msg->randID			= shanghuid;
		}
	}
	else {
		xyprintf(0, "ROUTER_ERROR:SN is error -- %s", msg->sn);
		msg->state = AC_LOGIN_NOUSER;
	}

	// 回复登陆报文
	msg_head->cmdID		= AC_CMD_LOGIN_ACK;
	msg_head->mode		= 1;
	msg->hearttime		= sgv_acac_interval;
	memset(msg->msg, 0, sizeof(msg->msg));
	
	//xyprintf_ac_head(msg_head);
	//xyprintf_ac_login(msg);
	if( send_ac_login(client_sockfd, msg_head, msg) ){
		xyprintf(0, "ROUTER_ERROR:%s %s %d -- Return router's(sockfd:%d) massage error!", __func__, __FILE__, __LINE__, client_sockfd);
		goto MSG_ERR;
	}
	
	big_little32(&msg->deviceID);
	
#if ROUTER_DEBUG
	xyprintf(0, "%u: --> --> Return login msg success --> -->", msg->deviceID );
#endif
	if(msg->state != AC_LOGIN_LOGINOK){
		goto MSG_ERR;
	}

	//认证完毕 添加到连接设备链表
	sock_list *sock_node = malloc(sizeof(sock_list));
	memset(sock_node, 0, sizeof(sizeof(sock_list)));
	
	sock_node->stat			= SOCK_STAT_ADD;
	sock_node->sockfd		= client_sockfd;
	sock_node->router_id	= id;
	sock_node->shanghuid	= shanghuid;
	sock_node->dailiid		= dailiid;
	sock_node->ver			= msg_head->ver;
	big_little32(&msg_head->device_type);
	sock_node->device_type	= msg_head->device_type;	
	time( &sock_node->last_heart_time );
	sock_node->heart_count  = 0;

	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	list_add(&(sock_node->node), &gv_sock_list_head);
	gv_sock_count++;
	xyprintf(0, "%u:Logged!!! -- sockfd is %d, sock_count = %u", msg->deviceID, client_sockfd, gv_sock_count);
	pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
	
	if(msg){
		free(msg);
	}
	if(msg_head){
		free(msg_head);
	}

	//xyprintf(0, "ROUTER:O(∩ _∩ )O ~~ Request is over!!!");
	return (void*)0;

SQLED_ERR:
	if(msg){
		free(msg);
	}
	if(msg_head){
		free(msg_head);
	}
	wt_close_sock( &client_sockfd );
	return (void*)WT_SQL_ERROR;
MSG_ERR:
	if(msg){
		free(msg);
	}
HEAD_ERR:
	if(msg_head){
		free(msg_head);
	}
ERR:
	wt_close_sock( &client_sockfd );
	xyprintf(0, "✟ ✟ ✟ ✟ -- %s %d:Request pthread is unnatural deaths!!!", __FILE__, __LINE__);
	return (void*)0;
} 

/** 
 *@brief  accept线程
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* accept_thread(void* fd)
{
	pthread_detach( pthread_self() );
	xyprintf(0, "** O(∩ _∩ )O ~~ Accept thread is running!!!");
	int sockfd;
	if( wt_sock_init( &sockfd, sgv_acac_port, MAX_EPOLL_NUM ) ){
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
		xyprintf(0, "ROUTER:♨ ♨ ♨ ♨  -- router %s connection, sockfd is %d", inet_ntoa( client_address.sin_addr) , client_sockfd);
		pool_add_worker( ac_process_fun, (void*)((long)client_sockfd));
		usleep( 50 * 1000 );
	}
ERR:
	close( sockfd );
	xyprintf(0, "✟ ✟ ✟ ✟ -- %s %d:Accept pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit( NULL );
}


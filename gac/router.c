/*****************************************************
 *
 * 设备接入 管理 等
 *
 *****************************************************/

#include "gac_header.h"

#define ROUTER_DEBUG		0

/** 
 *@brief  将对应认证服务器连接置错,在下个epoll循环中删除
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
		xyprintf(0, "sockfd:<-- <-- Recv the state message success <-- <--, ap count is %u", sockfd, msg->count);
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

		big_little32(&msg_head->deviceID);

		__u32 heart_count = 0;

		//将sockfd添加到epoll列表 更新设备最后活动时间
		struct list_head* pos;
		pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
		list_for_each(pos,&gv_sock_list_head){
			if(((sock_list*)pos)->sockfd == sockfd){
				((sock_list*)pos)->stat = SOCK_STAT_ADD;
				time( &((sock_list*)pos)->last_heart_time );
				((sock_list*)pos)->heart_count ++;
				heart_count = ((sock_list*)pos)->heart_count;
				break;
			}
		}
		pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙

#if ROUTER_DEBUG
		xyprintf(0, "%d:--> --> Return the state message success --> -->, count = %u", sockfd, heart_count);
#endif
		
		if(buf){
			free(buf);
		}
		if(msg_head){
			free(msg_head);
		}
		return (void*)0;
	}
	else if(msg_head->cmdID == AC_CMD_LOGIN){
		ac_login_st *msg = buf;
		if( recv_ac_login(sockfd, msg) ){
			xyprintf(0, "ROUTER_ERROR:%s %s %d -- Recv message form router of sockfd is %d error!", __func__, __FILE__, __LINE__, sockfd);
			goto MSG_ERR;
		}
		xyprintf(0, "%d:<-- <-- Recv login msg success again <-- <--", sockfd);
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

	xyprintf(0, "%d: ╮ ( ╯ ▽ ╰ ) ╭ Router offline thread is running!!!", sock->sockfd);
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

			if( time(0) - sock->last_heart_time > cgv_gac_interval + 60 ){	//设备的最后活动时间 超过两次间隔时间则在下面删除
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
				xyprintf(0, "%d:☠ ☠ ☠ ☠  -- sock_count = %d", ((sock_list*)temp)->sockfd, gv_sock_count);
			
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
	xyprintf(0, "%d: <-- <-- Recv login msg success <-- <--", client_sockfd );
	xyprintf_ac_login(msg);
#endif

	msg->state = AC_LOGIN_LOGINOK;

	if(msg->deviceID != GAC_ID){
		xyprintf(0, "ROUTER: -> <- Change deviceID ( %u -> %u ) -> <-", msg->deviceID, GAC_ID);
		msg->deviceID		= GAC_ID;
		msg->randID			= 6612;
	}
	
	msg_head->cmdID		= AC_CMD_LOGIN_ACK;
	msg_head->mode		= 1;
	//msg_head->deviceID	= ;
	msg->hearttime		= cgv_gac_interval;
	memset(msg->msg, 0, sizeof(msg->msg));
	
	//xyprintf_ac_head(msg_head);
	//xyprintf_ac_login(msg);
	if( send_ac_login(client_sockfd, msg_head, msg) ){
		xyprintf(0, "ROUTER_ERROR:%s %s %d -- Return router's(sockfd:%d) massage error!", __func__, __FILE__, __LINE__, client_sockfd);
		goto MSG_ERR;
	}
	
	big_little32(&msg->deviceID);
	
#if ROUTER_DEBUG
	xyprintf(0, "%d: --> --> Return login msg success --> -->", client_sockfd );
#endif
	if(msg->state != AC_LOGIN_LOGINOK){
		goto MSG_ERR;
	}

	//认证完毕 添加到连接设备链表
	sock_list *sock_node = malloc(sizeof(sock_list));
	memset(sock_node, 0, sizeof(sizeof(sock_list)));
	
	sock_node->stat			= SOCK_STAT_ADD;
	sock_node->sockfd		= client_sockfd;
	time( &sock_node->last_heart_time );
	sock_node->heart_count  = 0;

	pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
	list_add(&(sock_node->node), &gv_sock_list_head);
	gv_sock_count++;
	xyprintf(0, "%d:Logged!!! -- sock_count = %u", client_sockfd, gv_sock_count);
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
	if( wt_sock_init( &sockfd, cgv_gac_port, MAX_EPOLL_NUM ) ){
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


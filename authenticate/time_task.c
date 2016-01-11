/*****************************************************
 *
 * 定时任务 凌晨6点 重启所有设备
 *
 *****************************************************/

#include "auth_header.h"

/** 
 *@brief  设备定时重启
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* timing_restart_thread(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Timing restart thread is running!!!");

	//定时到早晨6点 计算当前时间到早晨6点相差的秒数
	unsigned int sleep_time;
	time_t now;
	struct tm *p;
	time(&now);
	p = localtime(&now);
	if(p->tm_hour < 6){
		sleep_time = ( 6 - p->tm_hour - 1 ) * 60 * 60 + ( 60 - p->tm_min ) * 60;
	}
	else {
		sleep_time = ( 24 + 6 - p->tm_hour - 1) * 60 * 60 + ( 60 - p->tm_min ) * 60;
	}
	xyprintf(0, "** (～﹃～)~zZ ~~ Timing restart thread will sleep %u s!!!", sleep_time);

	// 申请重启命令报文的空间,其他命令发送报文不需要报文体或者不需要发送报文
	void* full_msg = malloc( sizeof(msg_head_st) + sizeof(reboot_msg_st));
	memset(full_msg, 0, sizeof(msg_head_st) + sizeof(reboot_msg_st) );
	msg_head_st *msg_head = full_msg;								// 发往设备的数据包头
	reboot_msg_st *msg = full_msg + sizeof(msg_head_st);			// 发往设备的数据包体
	
	struct list_head *pos;						// 连接设备链表遍历变量
	sock_list* sock;

	while(1){
		sleep( sleep_time );
		sleep_time = 24 * 60 * 60;				//再定时到24小时以后
		
		xyprintf(0, "TIMING_RESTART:** ( ﾉ > ω < )ﾉ ~~ Timing restart thread is get up!!!");
		
		//遍历所有设备
		list_for_each(pos,&gv_sock_list_head){
			sock = (sock_list*)pos;
			memset(msg_head, 0, sizeof(msg_head_st));
			msg_head->ver			= sock->ver;				// 版本
			msg_head->mode			= 2;						// 方向
			msg_head->order			= msg_order_reboot;			// 包体类型
			msg_head->sec_flag		= sock->sec_flag;			// 加密方式
			//msg_head->crc;									// 校验和
			msg_head->router_id		= sock->router_id;			// 设备id
			//msg_head->third_id[64];							// 保留字段
			memcpy(msg_head->hard_seq, sock->hard_seq, 64);		// 序列号
					
			//生成数据包体
			msg->type = 1;
			msg_head->len = sizeof(msg_head_st) + sizeof(reboot_msg_st);

			//发送重启命令
			xyprintf(0, "%u: --> --> send reboot_msg(Reboot -- ScanSQL) --> -->", msg_head->router_id);
			if( send_reboot_msg(sock->sockfd, full_msg ) ){
				xyprintf(0, "SOCK_ERROR:%d %s %d -- Send reboot message to router is error!", sock->sockfd, __FILE__, __LINE__);
				sock->stat = SOCK_STAT_DEL;
			}
		}
		xyprintf(0, "TIMING_RESTART:** (～﹃～)~zZ ~~ Timing restart thread will sleep %u s!!!", sleep_time);
	}
ERR:
	pthread_exit(NULL);
}

/** 
 *@brief  扫描数据库线程 查看数据 有没有更新的数据
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* scanSQL_takeout_thread(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ ScanSQL takeout thread is running!!!");

	//连接数据库相关资源
	wt_sql_handle	*handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	wt_sql_handle	*handle_update = malloc(sizeof(wt_sql_handle));
	memset(handle_update, 0, sizeof(wt_sql_handle));

	struct list_head *pos;						// 连接设备链表遍历变量
	sock_list* sock;

	void* full_msg = malloc( sizeof(msg_head_st) + sizeof(user_msg_st) + 256 );
	memset(full_msg, 0, sizeof(msg_head_st) + sizeof(user_msg_st) + 256 );

	msg_head_st *msg_head = full_msg;								// 发往设备的数据包头
	user_msg_st *msg = full_msg + sizeof(msg_head_st);				// 发往设备的数据包体
	char* rj_addr = full_msg + sizeof(msg_head_st) + sizeof(user_msg_st);

	__u32 shangmac_pkid, shebei_pkid, isonline;
	char mac[20] = {0};

	while(1){
		//连接数据库
		if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
			sleep(5);
			continue;
		}
		
		if( wt_sql_init(handle_update, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
			sleep(5);
			wt_sql_destroy(handle);
			continue;
		}
		//3.2绑定变量和SQL语句查询结果
		SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG,	&shangmac_pkid,	20, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 2, SQL_C_CHAR,	mac,			20, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 3, SQL_C_ULONG,	&shebei_pkid,	20, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 4, SQL_C_ULONG,	&isonline,  	20, &handle->sql_err);
		
		while(1){
			sleep( sgv_testmacuserout * 60 );
		
#ifdef ADVERTISING
			sprintf(handle->sql_str, "SELECT id, mac, lastrid, isonline FROM mx_view_shanghu_maclistiscanmanyou WHERE outtime IS NOT NULL AND outtime <= GETDATE()");
#else
			sprintf(handle->sql_str, "SELECT id, mac, lastrid, isonline FROM mx_view_shang_maclistiscanmanyou WHERE outtime IS NOT NULL AND outtime <= GETDATE()");
#endif
			if(wt_sql_exec(handle)){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
				goto SQL_ERR;
			}
			handle->sql_ret=SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。

			//对每条结果进行处理
			while(handle->sql_ret != SQL_NO_DATA){
				
				//xyprintf(0, "TAKEOUT:shangmac id is %u, router id is %u, mac is %s, isonline is %u", shangmac_pkid, shebei_pkid, mac, isonline);
				
				if(isonline == 0){
#ifdef ADVERTISING
					sprintf(handle_update->sql_str, "UPDATE mx_shanghu_maclist SET iscanmanyou = 0, outmanyou = GETDATE() WHERE id = %u", shangmac_pkid);
#else
					sprintf(handle_update->sql_str, "UPDATE mx_shang_maclist SET iscanmanyou = 0, outmanyou = GETDATE() WHERE id = %u", shangmac_pkid);
#endif
					if(wt_sql_exec(handle_update)){
						xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
						goto SQL_ERR;
					}
				}
				else {
					
					pthread_mutex_lock(&gv_list_lock);//⊙﹏⊙
					list_for_each(pos,&gv_sock_list_head){
						sock = (sock_list*)pos;
						if(sock->shebei_pkid == shebei_pkid){
							//生成数据包头
							memset(msg_head, 0, sizeof(msg_head_st));
							msg_head->ver			= sock->ver;								// 版本
							msg_head->mode			= 2;										// 方向
							msg_head->order			= msg_order_user_op;						// 包体类型
							msg_head->sec_flag		= sock->sec_flag;							// 加密方式
							//msg_head->crc;													// 校验和
							msg_head->router_id		= sock->router_id;							// 设备id
							//msg_head->third_id[64];											// 保留字段
							memcpy(msg_head->hard_seq, sock->hard_seq, 64);						// 序列号
					
							//生成数据包体
							memset(msg, 0, sizeof(user_msg_st));
							msg->cmd		= user_init_rj;										// 数据类型
							msg->result		= user_cur_unknown;
							msg->auth_len	= 0;												// auth_addr 域的长度
							//msg->user_ip	= inet_addr(userip);								// 路由器发出的用户的IP
							string2x(msg->mac, mac, 6);											// mac地址
							//msg->	//__u32 speed[2];											// 速度控制
							sprintf(msg->user_id, "%c|%u", USER_ID_TIMEOUT, shangmac_pkid);		// 认证服务器回复用户的标识
							//msg->	__u32 auth_time;											// 免重复认证的时间
							//msg->	//__u32 flow[2];											// 流量控制
							//msg->__u8 auth_addr[0];
							sprintf(rj_addr, "%s?Rid=%u&macid=%u", sgv_trezhengurl, shebei_pkid, shangmac_pkid);        //跳转地址
							msg->auth_len		= strlen(rj_addr);													//auth_addr 域的长度
							msg_head->len		= sizeof(msg_head_st) + sizeof(user_msg_st) + msg->auth_len;		//数据包长度
			
							//xyprintf_msg_head(msg_head);
							//xyprintf_user_msg_head(msg);
							xyprintf(0, "%u: --> --> send user_msg(Res timeout -- ScanSQL) mac = %02x%02x%02x%02x%02x%02x --> -->", msg_head->router_id,
									msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
							if( send_full_user_msg_head(sock->sockfd, full_msg ) ){
								xyprintf(0, "SOCK_ERROR:%d %s %d -- Send timeout message head to router is error!", sock->sockfd, __FILE__, __LINE__);
								sock->stat = SOCK_STAT_DEL;
							}
							break;
						}//end if
					}//end list_for_each(pos,&gv_sock_list_head){
					pthread_mutex_unlock(&gv_list_lock);//⊙﹏⊙
				
				}//end if(isonline == 0)
				handle->sql_ret = SQLFetch(handle->sqlstr_handle);		//取下一条记录
			}//while(handle->sql_ret != SQL_NO_DATA){
		}//while(1)
SQL_ERR:
		wt_sql_destroy(handle);
		wt_sql_destroy(handle_update);
	}//while(1)
ERR:
	free(handle_update);
	free(handle);
	free(full_msg);
	xyprintf(0, "TAKEOUT_ERROR:%s %d -- ScanSQL takeout pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

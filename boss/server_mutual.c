/*****************************************************
 *
 * BOSS服务器 与 其他服务器的交互
 *
 *****************************************************/
#include "boss_header.h"

#define SN_HOUSE_NUM	60
int sn_add_warehouse_count = SN_HOUSE_NUM;
int sn_del_warehouse_count = SN_HOUSE_NUM;

/** 
 *@brief  平台连接监听线程函数
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* sn_add_warehouse_thread(void *fd){
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ SN warehouse thread is running!!!");
	while(1){
		
		unsigned int id = 0;
		char sn[64] = {0};
		char type[15] = {0};
		
		// 初始化 数据库连接
		wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
		memset(handle, 0, sizeof(wt_sql_handle));
		if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
			xyprintf(0, "SQL_INIT_ERROR -- %s %d -- Datebase connect error!", __FILE__, __LINE__);
			free(handle);
			sleep(60);
			continue;
		}
		
		SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&id,			64,  &handle->sql_err);
		SQLBindCol( handle->sqlstr_handle, 2, SQL_C_CHAR,	sn,				64,  &handle->sql_err);
		SQLBindCol( handle->sqlstr_handle, 3, SQL_C_CHAR,	type,			15,  &handle->sql_err);

		while(1){
			if(sn_add_warehouse_count >= SN_HOUSE_NUM){
				// 需要入库的设备
				sprintf(handle->sql_str, "SELECT userid, hard_seq, xinghao FROM mx_s_shebei_temp");
				if ( wt_sql_exec(handle) ){
					xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
					break;
				}
		
				handle->sql_ret = SQLFetch(handle->sqlstr_handle);
				while(handle->sql_ret != SQL_NO_DATA){
					send_router_msg(id, BG_ROUTER_ADD, type, sn);
					handle->sql_ret = SQLFetch(handle->sqlstr_handle);
				}
				SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

				sn_add_warehouse_count = 0;							// 数据清空
			}
			else {
				sn_add_warehouse_count++;
				sleep(10);
			}
		}
		
		wt_sql_destroy(handle);
		if(handle){
			free(handle);
		}
	}
	//到不了的远方～～～
	xyprintf(0, "PLATFORM_ERROR:✟ ✟ ✟ ✟  -- %s %d:SN add warehouse pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  设备解绑操作
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* sn_del_warehouse_thread(void *fd){
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ SN del warehouse thread is running!!!");
	while(1){
		
		unsigned int id = 0;
		char sn[64] = {0};
		char type[15] = {0};
		
		// 初始化 数据库连接
		wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
		memset(handle, 0, sizeof(wt_sql_handle));
		if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
			xyprintf(0, "SQL_INIT_ERROR -- %s %d -- Datebase connect error!", __FILE__, __LINE__);
			free(handle);
			sleep(60);
			continue;
		}
		
		SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&id,			64,  &handle->sql_err);
		SQLBindCol( handle->sqlstr_handle, 2, SQL_C_CHAR,	sn,				64,  &handle->sql_err);
		SQLBindCol( handle->sqlstr_handle, 3, SQL_C_CHAR,	type,			15,  &handle->sql_err);

		while(1){
			if(sn_del_warehouse_count >= SN_HOUSE_NUM){
			
				// 要解绑的设备
				sprintf(handle->sql_str, "SELECT userid, hard_seq FROM mx_s_shebei_jiebangtemp");
				if ( wt_sql_exec(handle) ){
					xyprintf(0, "SQL_ERROR -- %s %d -- sql string is -- %s", __FILE__, __LINE__, handle->sql_str);
					break;
				}
				handle->sql_ret = SQLFetch(handle->sqlstr_handle);
				while(handle->sql_ret != SQL_NO_DATA){
					send_router_msg(id, BG_ROUTER_DEL, type, sn);
					handle->sql_ret = SQLFetch(handle->sqlstr_handle);
				}
				SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

				sn_del_warehouse_count = 0;							// 数据清空
			}
			else {
				sn_del_warehouse_count++;
				sleep(10);
			}
		}
		
		wt_sql_destroy(handle);
		if(handle){
			free(handle);
		}
	}
	//到不了的远方～～～
	xyprintf(0, "PLATFORM_ERROR:✟ ✟ ✟ ✟  -- %s %d:SN del warehouse pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  平台命令处理函数 在platform_fun内被调用
 *@param  fd		类型 void*				连接设备的sockfd
 *@return 无意义
 */
void* platform_process(void *fd)
{
	int			sockfd	= (int)(long)fd;		//64bits下 void* 要先转换成long 然后再转换成int

	//报文体存放位置
	char buf[1024] = { 0 };

	//接收
	if( recv(sockfd, buf, 1024, 0) <= 0){
		xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Recv platform's massage error!", sockfd, __FILE__, __LINE__);
		goto DATA_ERR;
	}

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
	
	if( !strcmp(order_json->valuestring, "num_change") ){		// 上线

		cJSON *user_json = cJSON_GetObjectItem(json,"shanghuid");
		if(!user_json){
			xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
			goto JSON_ERR;
		}
		
		xyprintf(0, "PLATFORM:num_change to user of %d", user_json->valueint);
		
		if( num_change( user_json->valueint ) ){
			char *res = "0";
			if( send(sockfd, res, strlen(res), 0) <= 0){
				xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Res platform's massage error!", sockfd, __FILE__, __LINE__);
				goto JSON_ERR;
			}
		}
		else {
			char *res = "1";
			if( send(sockfd, res, strlen(res), 0) <= 0){
				xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Res platform's massage error!", sockfd, __FILE__, __LINE__);
				goto JSON_ERR;
			}
		}
	}
	else if( !strcmp(order_json->valuestring, "sn_add_change") ){		// 上线
		xyprintf(0, "PLATFORM:sn_add_change!!!");
		sn_add_warehouse_count = SN_HOUSE_NUM;
		
		char *res = "1";
		if( send(sockfd, res, strlen(res), 0) <= 0){
			xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Res platform's massage error!", sockfd, __FILE__, __LINE__);
			goto JSON_ERR;
		}
	}
	else if( !strcmp(order_json->valuestring, "sn_del_change") ){		// 上线
		xyprintf(0, "PLATFORM:sn_del_change!!!");
		sn_del_warehouse_count = SN_HOUSE_NUM;
		
		char *res = "1";
		if( send(sockfd, res, strlen(res), 0) <= 0){
			xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Res platform's massage error!", sockfd, __FILE__, __LINE__);
			goto JSON_ERR;
		}
	}
	else {
		xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
		goto JSON_ERR;
	}
	
	cJSON_Delete(json);
	wt_close_sock( &sockfd );
	xyprintf(0, "PLATFORM:Request completion of the platform!!!");
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
			
			//监听到一个连接 先打印一下
			xyprintf(0, "PLATFORM:O(∩ _∩ )O ~~ platform %s connection, sockfd is %d", inet_ntoa( client_address.sin_addr) , client_sockfd);
			
			pthread_t thread;//创建线程维护与第三方监听程序的连接 
			if(pthread_create(&thread, NULL, platform_process, (void*)(long)client_sockfd) != 0){//创建子线程
				xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
				break;
			}
		}
		close(sockfd);									//如果出错了 就关掉连接 重新初始化
	}
	//到不了的远方～～～
	xyprintf(0, "PLATFORM_ERROR:✟ ✟ ✟ ✟  -- %s %d:Platform pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

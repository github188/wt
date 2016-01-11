/*****************************************************
 *
 * 服务器之间交互
 *
 *****************************************************/

#include "ac_header.h"

#define SERVER_MUTUAL_DEBUG		0

/** 
 *@brief  平台命令处理函数 在platform_fun内被添加
 *@param  fd		类型 void*				连接设备的sockfd
 *@param  handle    类型 wt_sql_handle*		数据库操作资源集合
 *@return 数据库出错返回WT_SQL_ERROR 其他返回0
 */
void* platform_process(void *fd, wt_sql_handle *handle)
{
	xyprintf(0, "O(∩ _∩ )O ~~ A new request of platform!!!");

	int			sockfd	= (int)(long)fd;		//64bits下 void* 要先转换成long 然后再转换成int
	int			ret = 0;

	//报文体存放位置
	char buf[1024] = { 0 };

	//接收
	if( recv(sockfd, buf, 1024, 0) <= 0){
		xyprintf(errno, "PLATFORM_ERROR:%s %s %d -- Recv platform's massage error, sockfd is %s!", __func__, __FILE__, __LINE__, sockfd);
		goto DATA_ERR;
	}

	//晒一下
	xyprintf(0, "PLATFORM:Platform's msg: %s", buf);

	//比较是否是json头部
	if(strncmp( buf,"php-stream:",11)) {
		goto DATA_ERR;
	}

	//解析json用到的临时变量
	cJSON *json, *rjson = NULL;
	//解析出来的数据内容
	int acid, apid, enable;

	json=cJSON_Parse( buf + 11 );
	
	if (json){
		rjson = cJSON_GetObjectItem(json,"ac_id");
		if (rjson){
			acid = rjson->valueint;
		}
		else {
			goto DATA_ERR;
		}

		rjson = cJSON_GetObjectItem(json,"ap_id");
		if (rjson){
			apid = rjson->valueint;
		}
		else {
			goto DATA_ERR;
		}

		rjson = cJSON_GetObjectItem(json,"enable");
		if (rjson){
			enable = rjson->valueint;
		}
		else{
			goto DATA_ERR;
		}

		cJSON_Delete(json);
	}
	
	//嗮一下
	xyprintf(0, "PLATFORM:acid = %d, apid = %d, enable = %d", acid, apid, enable);

	sock_list sock;
	memset(&sock, 0, sizeof(sock));
	//查找平台要求控制的设备是否存在 如果存在 就将数据拷贝出来
	int find_flag = socklist_rid_call_all(acid, &sock);

	//判断是否在循环中找到
	//设备 找到则发送控制请求报文；没有找到，则进入错误处理。
	if(find_flag){
		xyprintf(0, "PLATFORM:Not found the router, id is %d", acid);

		//错误提示信息，并转换编码格式
		unsigned char utf8[128] = { 0 };
		unsigned char gb2312[128] = { 0 };
		snprintf(utf8, sizeof(utf8), "设备未找到，可能是设备的版本太低，不支持远程控制！");
	
		//转换编码
		if( code_convert("UTF-8", "GB2312", utf8, strlen(utf8), gb2312, 128) ){
			xyprintf(0, "PLATFORM_ERROR:%s %s %d -- code_convert() failed!", __func__, __FILE__, __LINE__);
		}
	
		//按照格式，回复platform
		unsigned char buf[256] = { 0 };
		snprintf(buf, sizeof(buf), "{\"res\":\"2\",\"data\":\"%s\"}\n", gb2312);
		
		//发送
		if( wt_send_block( sockfd, buf, strlen(buf) ) ){		//发送给平台
			xyprintf(0, "PLATFORM_ERROR:%s %s %d -- Send the return message of proxy to platform error!!!", __func__, __FILE__, __LINE__);
			close(sockfd);
			goto DATA_ERR;
		}

		//发送以后 关闭socket 跳出函数
		xyprintf(0, "PLATFORM:Retrun message to platform of sockfd is %d success : %s -- %s", sockfd, buf, utf8);
		close(sockfd);
		goto DATA_ERR;
	}

	//组成下发给设备的控制请求报文的报文头
	ac_head_st msg_head;
	msg_head.ver		= sock.ver;
	msg_head.cmdID		= AC_CMD_PROXY_QUEST;
	msg_head.mode		= 1;
	msg_head.device_type= sock.device_type;
	msg_head.deviceID	= acid;
	msg_head.session	= 6612;
	msg_head.datalen	= sizeof(ac_head_st) + sizeof(ac_proxy_st);

	//组成下发给设备的控制请求报文的报文体
	ac_proxy_st proxy;
	memset(&proxy, 0, sizeof(proxy));
	proxy.socket_php = sockfd;
	proxy.ac_id = acid;
	proxy.ap_id = apid;
	proxy.enable = enable;
	proxy.port = cgv_proxy_port;
	snprintf( proxy.ctrlURL, sizeof(proxy.ctrlURL), "%s", cgv_proxy_addr);
	
	//晒一下报文 有一天要注释掉这里
	//xyprintf_ac_head(&msg_head);
	//xyprintf_ac_proxy(&proxy);

	//发送～～～
	if( send_ac_proxy(sock.sockfd, &msg_head, &proxy) ){
		xyprintf(0, "PLATFORM_ERROR:%s %s %d -- Return to router of sockfd is %d massage error!", __func__, __FILE__, __LINE__, sock.sockfd);
		goto DATA_ERR;
	}

	//功成 得瑟下 收功～～～
	xyprintf(0, "PLATFORM:O(∩ _∩ )O ~~ Request is over!!!");
	return (void*)0;

	//错误处理 使用内核中常用的goto模式～
DATA_ERR:
	wt_close_sock( &sockfd );
	xyprintf(0, "PLATFORM:%s %s %d -- Request pthread is unnatural deaths!!!", __func__, __FILE__, __LINE__);
	return (void*)0;
} 

/** 
 *@brief  平台连接监听线程函数
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* platform_thread(void *fd){
	pthread_detach(pthread_self());
	while(1){
		int sockfd;
		if( wt_sock_init( &sockfd, cgv_platform_port, MAX_EPOLL_NUM) ){		//初始化监听连接
			xyprintf(0, "PLATFORM_ERROR:%s %s %d -- wt_sock_init()", __func__, __FILE__, __LINE__);
			continue;
		}
		struct sockaddr_in client_address;				//存放客户端地址信息
		int client_len = sizeof(client_address);		//存放客户端地址信息结构体长度
		
		while(1){										//开始循环监听
			
			int client_sockfd;
			client_sockfd = accept(sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);
			
			if(client_sockfd == -1){					//监听出错
				xyprintf(errno, "PLATFORM_ERROR:%s %s %d -- accept()", __func__, __FILE__, __LINE__);
				break;
			}
			
			//监听到一个连接 先打印一下
			xyprintf(0, "PLATFORM:Platform %s connection, sockfd is %d", inet_ntoa( client_address.sin_addr) , client_sockfd);
			
			//然后将sockfd作为参数 添加到线程池任务队列
			pool_add_worker( platform_process, (void*)((long)client_sockfd));

		}

		close(sockfd);									//如果出错了 就关掉连接 重新初始化
	}

	//到不了的地方～～～
	xyprintf(0, "PLATFORM_ERROR:%s %s %d -- Platform pthread is unnatural deaths!!!", __func__, __FILE__, __LINE__);
	pthread_exit(NULL);
}

/*****************************************************
 *
 * 服务器之间交互
 *
 *****************************************************/

#include "sauth_header.h"

/** 
 *@brief  平台命令处理函数 在platform_fun内被调用
 *@param  fd		类型 void*				连接设备的sockfd
 *@return 无意义
 */
void* platform_process(void *fd)
{
	pthread_detach(pthread_self());
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

	//晒一下
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
	
	if( !strcmp(order_json->valuestring, "guide_change")){		// 上线
		if( get_guide_list() == 0 ){
			xyprintf(0, "PLATFORM:Guide list change success!!!!");
			char *res = "1";
			if( send(sockfd, res, strlen(res), 0) <= 0){
				xyprintf(0, "PLATFORM_ERROR:%d %s %d -- Res platform's massage error!", sockfd, __FILE__, __LINE__);
			}
		}
	}
	else {
		xyprintf(0,"PLATFORM_ERROR:JSON's data error -- %s -- %d!!!", __FILE__, __LINE__);
	}
	
JSON_ERR:
	cJSON_Delete(json);
DATA_ERR:
	wt_close_sock( &sockfd );
	pthread_exit(NULL);
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
	int sockfd;
	pthread_t pt;
	while( wt_sock_init( &sockfd, cgv_platform_port, MAX_EPOLL_NUM) ){		//初始化监听连接
		xyprintf(errno, "PLATFORM_ERROR:0 %s %d -- wt_sock_init()", __FILE__, __LINE__);
		sleep(60);
		continue;
	}
	struct sockaddr_in client_address;				//存放客户端地址信息
	int client_len = sizeof(client_address);		//存放客户端地址信息结构体长度
	while(1){										//开始循环监听
		int client_sockfd;
		client_sockfd = accept(sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);
			
		if(client_sockfd == -1){					//监听出错
			xyprintf(errno, "PLATFORM_ERROR:%d %s %d -- accept()", sockfd, __FILE__, __LINE__);
			continue;
		}
			
		xyprintf(0, "PLATFORM:O(∩ _∩ )O ~~ platform %s connection, sockfd is %d", inet_ntoa( client_address.sin_addr) , client_sockfd);
		if( pthread_create(&pt, NULL, platform_process, (void*)(long)client_sockfd) != 0 ){
			xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
		}

		usleep( 100 * 1000 );
	}

	// 远方～～～
	close(sockfd);
	xyprintf(0, "PLATFORM_ERROR:✟ ✟ ✟ ✟  -- %s %d:Platform pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

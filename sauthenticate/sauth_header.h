/*****************************************************
 *
 * 头文件
 *
 *****************************************************/
#ifndef AUTH_HEADER_H
#define AUTH_HEADER_H

#include "list.h"
#include "cJSON.h"

#define GUIDE_URL_LEN	128

// router.c
extern void* router_epoll_thread(void *fd);
extern void* router_conn_thread(void* fd);

// server_mutual.c
extern void* platform_conn_thread(void *fd);

//引导服务器
typedef struct guide_node{
	struct list_head	node;
	unsigned int		id;								// 代理商id
	char				url[ GUIDE_URL_LEN ];			// 代理商引导服务器地址
	unsigned int		port;							// 代理商引导服务器端口
}guide_node;

//配置文件内读出的配置参数
extern char				cgv_sql_name[32];				// 数据库名
extern char				cgv_sql_user[32];				// 数据库登陆用户名
extern char				cgv_sql_pass[32];				// 数据库登陆密码
extern unsigned int		cgv_authenticate_port;			// 认证端口,开放给设备的

// 引导服务器链表
extern struct list_head		gv_guide_head;			// 引导服务器链表头
extern pthread_mutex_t		gv_guide_lock;			// 引导服务器链表互斥锁

// 平台连接端口
extern unsigned int			cgv_platform_port;		// 平台端口,开放给平台的

extern void* router_conn_thread(void* fd);
extern void* platform_conn_thread(void *fd);
#endif // AUTH_HEADER_H

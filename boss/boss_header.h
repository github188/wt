/*****************************************************
 *
 * 头文件
 *
 *****************************************************/
#ifndef BOSS_HEADER_H
#define BOSS_HEADER_H

#include "list.h"
#include "cJSON.h"

//认证服务器链表节点结构体 -- 本地维护
typedef struct guide_node{
	struct list_head	node;
	int					sockfd;
	struct in_addr		sin_addr;			// guide ip
	unsigned int		userid;
	time_t				last_time;
	int					stat;				// 1 需要添加监听 0 已添加 -1 需要删除
	pthread_mutex_t		lock;
	int					get_shanghu_flag;
	int					get_shebei_type_flag;
}guide_node;

//全局变量 读取自配置文件
extern char cgv_sql_name[32];					// sql name
extern char cgv_sql_user[32];					// sql user
extern char cgv_sql_pass[32];					// sql password
extern int	cgv_platform_port;					// 开放给平台的端口

extern unsigned int	sgv_trnum;				// 初始设备数量
extern unsigned int	sgv_tupdate;			// 心跳间隔

// 全局变量
extern struct list_head gv_guide_list_head;		// 引导服务器链表头
extern pthread_mutex_t	gv_guide_list_lock;		// 引导服务器链表互斥锁
extern unsigned int		gv_guide_count;			// 引导服务器数量
extern int				gv_time_ed_flag;		// 定时任务完成标志

// guide.c
extern void* guide_epoll_thread(void *fd);
extern void* guide_conn_thread(void *fd);
// server_mutual.c
extern void* platform_conn_thread(void *fd);
extern void* sn_add_warehouse_thread(void *fd);
extern void* sn_del_warehouse_thread(void *fd);

// time_tasks.c
extern void* get_ss_thread(void *fd);
extern void* tfree_thread(void *fd);
#endif // BOSS_HEADER_H

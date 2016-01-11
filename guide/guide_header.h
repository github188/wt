/*****************************************************
 *
 * 头文件
 *
 *****************************************************/
#ifndef GUIDE_HEADER_H
#define GUIDE_HEADER_H

#include "list.h"
#include "cJSON.h"

#define IS_CONN_BOSS		1

//认证服务器链表节点结构体 -- 本地维护
typedef struct authenticate_node{
	struct list_head	node;
	int					sockfd;
	pthread_mutex_t		lock;
	struct in_addr		sin_addr;			//设备ip
	int					port;				//开放给路由器的端口
	int					stat;				//1 需要添加监听 0 已添加 -1 需要删除
	unsigned int router_num;				//已连接设备数
	unsigned int cur_queue_size;			//未处理报文数
	unsigned int cpu_usage;					//cpu使用率
	unsigned int total_mem;					//总内存
	unsigned int used_mem;					//已使用内存
	unsigned int free_mem;					//空闲内存
//	unsigned int shared_mem;
	unsigned int buffers_mem;
	unsigned int cached_mem;
	unsigned int swap;
	time_t		 last_time;					//最后状态报告时间
	int			 load_weight;				//负载状态 使用认证服务器发来的状态用算法算出
	unsigned int auth_code;
}authenticate_node;

// 认证服务器链表节点结构体 -- 发送给第三方
typedef struct authenticate_stat{
	unsigned int ip;						//设备ip
	unsigned int router_num;				//已连接设备数
	unsigned int cur_queue_size;			// 等待队列中的任务数目
	unsigned int cpu_usage;					//cpu使用率
	unsigned int total_mem;					//总内存
	unsigned int used_mem;					//已使用内存
	unsigned int free_mem;					//空闲内存
//	unsigned int shared_mem;
	unsigned int buffers_mem;
	unsigned int cached_mem;
	unsigned int swap;
}authenticate_stat;

// 设备类型链表数量
typedef struct rt_node{
	struct list_head	node;
	char				name[32];
	unsigned int		num;
}rt_node;

//全局变量 读取自配置文件
extern char cgv_sql_name[32];					// sql name
extern char cgv_sql_user[32];					// sql user
extern char cgv_sql_pass[32];					// sql password
extern int	cgv_guide_port;						// 引导服务器开放给设备的端口
extern int  cgv_report_port;					// 引导服务器开放给认证服务器的状态报告端口
extern int  cgv_report_interval;				// 引导服务器与认证的状态报告报文间隔
extern int  cgv_third_port;						// 开放给第三方程序的监听接口
extern int	cgv_platform_port;					// 开放给平台的端口

// 全局变量 读取自数据库
extern unsigned int sgv_status_update_interval;			// 状态更新线程执行间隔时间
extern unsigned int sgv_tfree_interval;					// tfree更新线程执行间隔时间
extern unsigned int sgv_ac_interval;

// 全局变量
extern struct list_head gv_authenticate_list_head;		// 已启动认证服务器链表头
extern pthread_mutex_t	gv_authenticate_list_lock;		// 已启动认证服务器链表互斥锁
extern unsigned int		gv_authenticate_count;			// 已启动认证服务器数量
extern unsigned int		gv_third_count;					// 第三方监听程序连接计数器
extern unsigned int		gv_guide_router_count;			// 引导过的设备计数器
extern pthread_mutex_t	gv_task_lock;					// 数据库定时任务互斥锁

extern pthread_mutex_t	gv_boss_flag_lock;
extern int				gv_boss_flag;
extern int				gv_heart_interval;
extern int				gv_num_falg;
extern int				gv_total_num;
extern int				gv_rt_num;
extern struct list_head gv_boss_rt_head;

// server_mutual.c
extern void* platform_conn_thread(void *fd);
extern void* third_conn_thread(void *fd);
extern void* auth_epoll_thread(void *fd);
extern void* auth_stat_refresh_thread(void *fd);
extern void* auth_conn_thread(void *fd);

extern void* BOSS_PROCESS_THREAD(void* fd);

// time_task.c
extern void* INVALID_DATA_CLEAN_THREAD(void *fd);
extern void* TFREE_THREAD(void *fd);
extern void* LOG_METASTASIS_THREAD(void *fd);
extern void* STATISTICS_THREAD(void *fd);
extern void* LOG_LAST_TO_LOG_THREAD(void *fd);
extern void* DAILY_REPORT_THREAD(void *fd);
#endif // GUIDE_HEADER_H

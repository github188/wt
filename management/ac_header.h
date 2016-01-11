/*****************************************************
 *
 * 头文件
 *
 *****************************************************/
#ifndef AC_HEADER_H
#define AC_HEADER_H

#include "list.h"
#include "cJSON.h"

#define SOCK_STAT_ADD		 1
#define SOCK_STAT_ADDED		 0
#define SOCK_STAT_DEL		-1

//配置文件内读出的配置参数
extern char	cgv_sql_name[32];
extern char	cgv_sql_user[32];
extern char	cgv_sql_pass[32];
extern int	cgv_platform_port;
extern int	cgv_proxy_port;
extern char	cgv_proxy_addr[32];

extern int	sgv_ac_port;
extern int	sgv_ac_interval;

//连接设备链表结构
typedef struct sock_list {
	struct list_head	node;
	int					stat;						// 1 需要添加监听 0 已添加 -1 需要删除
	int					sockfd;
	//pthread_mutex_t		send_lock;
	int					router_id;					// 设备rid 即设备表主键ID
	unsigned int		ver;						// 版本
	unsigned int		device_type;				// 1为AC，2为无线路由，3为不带无线路由
	//__u8				sn[128];					// 设备序列号（唯一性）
	time_t				last_heart_time;			// 最后心跳时间
	__u32				heart_count;
}sock_list;

extern struct list_head gv_sock_list_head;				//连接设备链表头
extern pthread_mutex_t	gv_list_lock;					//连接设备链读写锁
extern __u32			gv_sock_count;				//连接设备计数器

extern void* platform_thread(void *fd);
extern void* epoll_thread(void *fd);
extern void* accept_thread(void* fd);

#endif // AUTH_HEADER_H

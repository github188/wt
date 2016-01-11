/*****************************************************
 *
 * 头文件
 *
 *****************************************************/
#ifndef AC_HEADER_H
#define AC_HEADER_H

#include "list.h"
#include "cJSON.h"

#define GAC_ID				0x7FFFFFFF

#define SOCK_STAT_ADD		 1
#define SOCK_STAT_ADDED		 0
#define SOCK_STAT_DEL		-1

//配置文件内读出的配置参数
extern char	cgv_sql_name[32];
extern char	cgv_sql_user[32];
extern char	cgv_sql_pass[32];
extern int	cgv_gac_port;
extern int	cgv_gac_interval;

//连接设备链表结构
typedef struct sock_list {
	struct list_head	node;
	int					stat;						// 1 需要添加监听 0 已添加 -1 需要删除
	int					sockfd;
	//pthread_mutex_t		send_lock;
	//__u8				sn[128];					// 设备序列号（唯一性）
	time_t				last_heart_time;			// 最后心跳时间
	__u32				heart_count;
}sock_list;

extern struct list_head gv_sock_list_head;				//连接设备链表头
extern pthread_mutex_t	gv_list_lock;					//连接设备链读写锁
extern __u32			gv_sock_count;				//连接设备计数器

extern void* epoll_thread(void *fd);
extern void* accept_thread(void* fd);

#endif // AUTH_HEADER_H

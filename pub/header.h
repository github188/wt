/*****************************************************
 *
 * 头文件 宏
 *
 *****************************************************/
#ifndef WT_HEADER_H
#define WT_HEADER_H

#define PROFRAM_VERSION			"5.1.17"

#define LOGS_MAX_SIZE			( 5 * 1024 * 1024 )	//单个log文件大小 1M
#define WRITE_LOG				1					//是否写log文件

#define MAX_EPOLL_NUM			65536				//epoll 最大数量

#define CONFIG_FILE_NAME		"config.ini"		//配置文件名

#define WT_SQL_ERROR			6l					// 全局数据库错误标识码

#define PRINT_USER_LOG			1					// 是否打印user操作的log 比如上下线等

#define SOCK_STAT_ADD		 1				// 设备需要添加到epoll列表
#define SOCK_STAT_ADDED		 0				// 设备已添加到epoll列表
#define SOCK_STAT_DEL		-1				// 设备出错,应从当前链表删除

//#define ADVERTISING			1				// 广告系统Advertising

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>
#include <stdarg.h>

#include <signal.h>

#include <errno.h>
#include <unistd.h>  
#include <fcntl.h>
#include <assert.h>

#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/wait.h>

#include <iconv.h>

#include <pthread.h>  

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;

extern char cgv_sql_name[32];
extern char cgv_sql_user[32];
extern char cgv_sql_pass[32];

//数据库操作所需参数
typedef struct wt_sql_handle{
	SQLHENV		env_handle;						// Handle ODBC environment 环境句柄
	SQLHDBC		conn_handle;					// Handle connection 连接句柄
	SQLHSTMT	sqlstr_handle;					// 数据库执行语句句柄
	SQLRETURN	sql_ret;
	char		sql_stat[12];					// Status SQL
	SQLLEN		sql_err;
	SQLSMALLINT	sql_mlen;
	char		err_msg[200];
	char		sql_str[1024];
}wt_sql_handle;

/********************** guide & authenticate whit router  ******************************/

//数据类型标识
//根据struct msg_head_st::order 的取值,后接各自所对应的数据结构
enum msg_order_type {
	msg_order_unknown = 0, 
	msg_order_auth,							//01 连接认证,对应struct cer_msg_st;
	msg_order_addr,							//02 获取认证、审计服务器地址,对应 struct cer_msg_st;
	msg_order_user_req,						//03 路由器发出的用户信息,比如上下线,对应 struct user_msg_st;
	msg_order_heart,						//04 心跳包,对应 struct heart_msg_st;
	msg_order_user_op,						//05 服务器发出的对用户的操作,对应struct user_msg_st;
	msg_order_user_result,					//06 路由器回复用户操作的结果,对应 struct user_msg_ead_st;
	msg_order_dns_white,					//07 路由器修改白名单, 对应 struct dns_white_list_st
	msg_order_auto_upgrade,					//08 设置路由器自动升级, 对应 struct auto_upgrade_st
	msg_order_simple_cache,					//09 用户路由器简单缓存页面, 对应 simple_cache_st
	msg_order_simple_gg,					//10 简单广告
	msg_order_third_qq,						//11 发送QQ信息
	msg_order_local_user_login,				//12 发送用户认证信息
	msg_order_local_php_config,				//13 下发认证页面信息
	msg_order_local_user_login_new,			//14 发送用户认证信息
	msg_order_usb_cache = 20,				//20 树熊下发USB 缓存
	msg_order_usb_cache_result,				//21 回复USB 缓存处理结果
	msg_order_reboot,						//22 下发重启操作 后接 reboot_st
	msg_order_reboot_result,				//23 回复重启操作结果, 需要先回复再重启23
	msg_order_sj_url,						//24 树熊需要的URL 信息
	//msg_order_user_qos_set = 33,			//33 开启限速功能报文
	//msg_order_user_qos_result = 34,		//34 开启限速功能回复报文
	//msg_order_wireless_config_set = 41,	//41 下发无线配置
	//msg_order_wireless_config_result,		//42 下发无线配置返回报文
	//msg_order_wireless_config_get,		//43 获取无线配置
	//msg_order_wireless_config_send,		//44 路由器返回无线配置
	msg_order_speed_get = 45,				//45 获取实时速度操作
	msg_order_speed_get_result,				//46 获取实时速度返回
	msg_order_wifi_monitor_set = 51,		//51 配置无线探针
	msg_order_wifi_monitor_result,		//52 配置无线探针回复
	msg_order_wifi_monitor_get,			//53 获取无线探针设置
	msg_order_wifi_monitor_send,			//54 获取无线探针设置回复
	msg_order_version_get = 55,				//55 获取版本号 不需要后接内容
	msg_order_version_send,					//56 返回版本号 后接struct wt_router_version_st
	msg_order_web_keyword_set,				//57 设置行为管理过滤关键词 后接struct web_keyword_st
	msg_order_web_keyword_result,			//58 设置行为管理过滤关键词回复
	msg_order_url_redirect_set,				//59 行为管理URL重定向
	msg_order_url_redirect_result,			//60 行为管理URL重定向回复
	msg_order_mx_app_filter_set,			//61 流媒体识别控制
	msg_order_mx_app_filter_result,			//62 流媒体识别控制回复
	msg_order_memory_cache_set = 63,		//63 下发缓存到内存
	msg_order_memory_cache_result,			//64 下发缓存到内存回复
	msg_order_mx_wireless_config_set = 68,	//68 下发无线信息配置, struct wt_mx_wifi_base_conf_so
	msg_order_mx_wireless_config_result,	//69 回复
	msg_order_mx_wireless_config_get,		//70 获取无线配置
	msg_order_mx_wireless_config_send,		//71 回复
	//msg_order_mx_domain_redirect_set = 80,//80 阻止用户微信域名重定向 不用了
	//msg_order_mx_domain_redirect_result,	//81 阻止用户微信域名重定向回复 不用了
	msg_order_ac_addr_set = 82,				//82 集中管理平台地址下发
	msg_order_ac_addr_result,				//83 集中管理平台地址回复
	msg_order_usb_file_get,					//84 USB路径文件列表获取
	msg_order_usb_file_get_result,			//85 USB路径文件列表获取回复
	msg_order_weixin_share_set,				//86 微信分享下发,weixin_share_st
	msg_order_weixin_share_set_result,		//87 微信分享下发回复, 
	msg_order_acac_addr_set = 91,			//91 ACAC的地址端口修改
	msg_order_acac_addr_result,				//92 ACAC的地址端口修改回复
	msg_order_guide_addr_set = 95,			//95 引导地址端口修改
	msg_order_guide_addr_result,			//96 引导地址端口修改回复
	msg_order_mx_wireless_config_5g_set = 97,//97 下发5G无线信息配置97 , struct wt_wifi_base_conf_so
	msg_order_mx_wireless_config_5g_result,	//98 回复98
	msg_order_mx_wireless_config_5g_get,	//99 获取5G无线配置99
	msg_order_mx_wireless_config_5g_send,	//100 回复100
	//msg_order_get_php_do = 100,			//100 用户上线,  GET PHP 操作
	msg_order_simple_gg2_set = 103,			//103 简单广告2 下发
	msg_order_simple_gg2_result,			//104 简单广告2 下发回复
	msg_order_simple_gg2_get,				//105 获取简单广告2参数
	msg_order_simple_gg2_send,				//106 获取简单广告2参数回复
	msg_order_httpd_pwd_set,				//107 设置httpd 登录用户名和密码
	msg_order_httpd_pwd_result,				//108 回复
	msg_order_httpd_pwd_get,				//109 获取httpd 登录用户名和密码
	msg_order_httpd_pwd_send,				//110 回复

	msg_order_max,
};

//发送数据的类型
enum mode_type {
	mode_unknown = 0, 
	mode_tosvr,						//1 路由器发往服务器的数据包
	mode_fromsvr,					//2 服务器发往路由器的数据包
	mode_max,
};

//服务器类型, 连接认证时, 如果两个服务器不能区分, 可用此状态区分
enum srv_type {
	srv_type_unknown = 0,
	srv_type_main,					//1 引导服务器
	srv_type_auth,					//2 认证服务器
	srv_type_max,
};

//公共的数据包头
typedef struct msg_head_st
{
	__u8	ver;					// 版本,初始为1; 添加 third_id 后改为2, 目前为4 2.3没有设备序列号
	__u8	mode;					// 方向,1: 路由器发往服务器；2: 服务器发往路由器
	__u16	order;					// 用于标识该结构体后的数据的类型,取值见enum msg_order_type
	__u16	len;					// 数据包的长度
	__u16	sec_flag;				// 数据加密方式,0: 不加密；其它可扩展
	__u32	crc;					// 校验和,全包检验
	__u32	router_id;				// 设备id,由主服务器分配 
	//TODO:此ID 在路由器页面上配置,  如果业务上需要次ID  则需要相应的路由器版本
	__u8	third_id[64];			// 保留字段,可根据需要解释其意义,比如用于第三方认证服务器生成的ID
	__u8	hard_seq[64];			// 设备序列号
}msg_head_st;

//路由器与引导、认证服务器间的连接认证数据包
//服务端返回的auth_srv如果是空的,则不开启认证策略,不需要和任何认证服务器做交互,全部开放。
//路由器与主、认证服务器间的认证数据包及审计、认证服务器地址
//对应struct msg_head_st::order = msg_order_auth, msg_order_addr
typedef struct cer_msg_st
{
	__u16	order;					// 表示连接认证时的交互顺序,由1开始逐1地递增。路由器与服务器间共享同一个order,比如：路由器order = 1, 服务器order = 2, 路由器order = 3, 目前最多到4
	__u8	srv_type;				// 连接服务器的类型,1: 引导服务器；2: 认证服务器,可参考enum srv_type
	__u8	pad;
	__u32	router_seq;				// 路由器产生的随机数,用于加密、预防copy 攻击
	__u32	srv_seq;				// 服务器产生的随机数,用于加密、预防copy 攻击
	__u32	time;					// 路由器与认证服务器间的心跳时间,单位秒,默认为30秒
	__u32	hard_id;				// 设备硬件型号
	__u8	hard_seq[64];			// 设备序列号
	__u8	router_addr[128];		// 设备地址
	__u8	router_cont[32];		// 联系人
	__u8	router_tel[32];			// 联系电话
	__u8	router_dinate[32];		// 设备坐标
	__u8	router_agents[32];		// 代理商ID
	__u8	dev_name[32];			// 设备标识
	__u8	auth_srv[64];			// 认证服务器地址,比如: www.auth_srv.com:8080
	__u8	sj_srv[64];				// 审计服务器,目前暂时保留不用
	__u8	mng_srv[64];			// 集中管理服务器,目前暂时保留不用
	__u8	def_redirect[128];		// 服务器提供的默认跳转地址,在struct user_cmd_req_type 中没有指定跳转地址时使用
	__u16	cs_type;				// 场所类型,取值参考enum cs_type, 如,酒店、药店、小区等等
	__u16	url_len;				// 域名白名单长度
	__u8	url_data[0];			// 域名白名单,域名间以分号分隔
}cer_msg_st;

// 平台地址下发
typedef struct addr_set_st
{
	unsigned char  addr[30];		// 集中管理服务器地址	
	unsigned short port;			// 端口, 修改后会重启集中管理和认证
}addr_set_st;

// 引导和认证时候 随机数
typedef struct random_pair
{
	int router_seq;
	int srv_seq;
}random_pair;











/********************** guide with authenticate with third  ******************************/

// 认证服务器与引导服务器之间的通信类型
enum ag_enum
{
	ag_unknow = 0,
	ag_authenticate,				//1 认证服务器连接
	ag_authenticate_res,			//2 认证服务器连接返回
	ag_third,						//3 第三方程序连接
	ag_third_res,					//4 第三方程序连接返回
	ag_get_stat_report,				//5 获取状态报告
	ag_stat_report,					//6 状态报告
	ag_white_change,				//7 白名单更改
	ag_gg_change,					//8 富媒体更改
	ag_upurl_change,                //9 升級地址更改
	ag_other_change,				//10 其他参数更改
	ag_router_update,				//11 设备升级
	ag_router_disconn,				//12 重新连接
	ag_router_cache,				//13 修改内存缓存
	ag_router_cacheusb,				//14 修改usb缓存
	ag_router_wireless,				//15 修改无线
	ag_router_keyword,				//16 修改过滤关键词
	ag_router_restart,				//17 重启
	ag_mac_takeout,					//18 mac踢出
	ag_ac_change,					//18 ac修改
	ag_mac_online,					//19 mac上线 允许上网
	ag_max,
};


#define AG_PASS					0x427				// 认证连接引导时的认证码
#define AG_HEAD_HEAD		0x68

// 认证服务器与引导之间的通信报文头
typedef struct ag_msg_head{
	unsigned char head;				// 头
	unsigned char order;			// 报文类型
	unsigned short len;				// 报文长度
}ag_msg_head;

// 认证与引导之间通信报文体 -- 连接请求
typedef struct ag_msg_conn{
	ag_msg_head head;
	int passwd;						// 请求码
	unsigned int port;				// 开放的端口
}ag_msg_conn;

#define AG_CONN_SUCC			0x2A	// 连接成功 并要求上报状态信息
#define AG_CONN_FAIL_TIME		0x2B	// 连接失败 不在可用时间内 
#define AG_CONN_FAIL_NONE		0x2C	// 连接失败 没有这个认证服务器
#define AG_CONN_FAIL_ISOK		0x2D	// 连接失败 isok状态错误
#define AG_CONN_FAIL_SQL		0x2E	// sql错误 
#define AG_CONN_FAIL_ERR		0x2F	// 连接失败 其他错误

// 认证与引导之间通信报文体 -- 连接回复
typedef struct ag_msg_conn_res{
	ag_msg_head head;
	unsigned int conn_res;				// 回复代码
	unsigned int auth_code;				// 认证服务器代码
	unsigned int platform_port;			// 认证服务器开放给平台的端口
}ag_msg_conn_res;

// 认证与引导之间通信报文体 -- 设备操作
typedef struct ag_msg_router{
	ag_msg_head  head;
	unsigned int shanghuid;
	unsigned int router_num;
	unsigned int routers[0];
}ag_msg_router;

// 认证与引导之间通信报文体 -- mac操作
typedef struct ag_msg_mac{
	ag_msg_head  head;
	unsigned int shebeiid;
	unsigned int shang_pkid;
	char		 mac[16];
}ag_msg_mac;

// 认证与引导之间通信报文体 -- 状态报告
typedef struct ag_msg_stat{
	ag_msg_head	 head;
	unsigned int router_num;
	unsigned int cur_queue_size;			// 等待队列中的任务数目
	unsigned int cpu_usage;
	unsigned int total_mem;
	unsigned int used_mem;
	unsigned int free_mem;
//	unsigned int shared_mem;
	unsigned int buffers_mem;
	unsigned int cached_mem;
	unsigned int swap;
}ag_msg_stat;



/********************** boss with guide ******************************/
// boos和guide之间的通信命令
enum bg_enum
{
	bg_unknow = 0,
	bg_guide,				//1 服务器连接 && 发送平台信息 尤其是sn号
	bg_canuse_num,			//2 服务器连接返回 设备数量下发
	bg_heart,				//3 心跳
	bg_get_shebei_type,		//4 获取设备类型
	bg_shebei_type,			//5 设备类型 设备类型上报
	bg_get_shanghu,			//6 获取商户
	bg_shanghu,				//7 商户上报
	bg_daily_report,		//8 日报表
	bg_get_sn,				//9 获取sn号
	bg_router,				//10 设备操作
	bg_max,
};

#define BG_HEAD_VER				0x1
#define BG_HEAD_VER_1			0x1
#define BG_HEAD_HEAD			0x86
#define BG_BOSS_ADDR			"super.20080531.com"
//#define BG_BOSS_ADDR			"192.168.168.6"
#define BG_PORT					5612
#define SHANGHU_ONCE_NUM		50		// 单次发送的商户数量

// 报文头
typedef struct bg_msg_head
{
	unsigned char	ver;
	unsigned char	head;
	unsigned char	order;
	unsigned char	reserve;
	unsigned int	len;
	char	version[16];
}bg_msg_head;

// 连接请求
typedef struct bg_conn_msg
{
	int		php_sockfd;
	char	sn[64];						// 服务器sn号
	char	company_name[128];
	char	company_addr[256];
	char	fzr_name[32];
	char	fzr_tel[32];
	char	qq[16];
	int		is_streamline;
}bg_conn_msg;

#define BG_NUM_FLAG_TOTAL				1	//设备数量控制类型 -- 总数
#define BG_NUM_FLAG_TYPE				2	//设备数量控制类型 -- 类型数

// 可用设备数量报文
typedef struct bg_num_msg
{
	int		php_sockfd;
	int		guide_flag;			// -1 不可用 0 老用户 1 新用户
	int		heart_interval;		// 心跳频率
	int		num_flag;			// 数量控制标志
	int		total_num;			// 总数量
	int		rt_num;				// 设备类型数
	struct bg_msg_rt_num{
		char			name[32];
		unsigned int	num;
	}rts[0];					// 设备类型
}bg_num_msg;

// 心跳
typedef struct bg_heart_msg
{
	unsigned int	router_total_num;
	unsigned int	router_bind_num;
	unsigned int	router_online_num;
	unsigned int	ap_total_num;
	unsigned int	ap_online_num;
}bg_heart_msg;

// 设备类型信息

// 设备信息上报报文
typedef struct bg_rts_msg
{
	unsigned int	rt_num;
	struct bg_rt_node
	{
		unsigned int	id;
		unsigned int	curr_num;
		unsigned int	ishavewifi;
		unsigned int	ishaveacserver;
		char			name[16];
		char			driver_code[32];
		char			upurl[128];
		char			rarurl[128];
	}rts[0];

}bg_rts_msg;

// 商户信息上报报文
typedef struct bg_shanghu_msg
{
	unsigned int	shanghu_num;
	struct bg_shanghu_node
	{
		unsigned int	shanghu_id;
		char			isreopen;
		char			isok;
		unsigned short	router_num;
		unsigned short	use_num;
		unsigned short	unuse_num;
		unsigned int	ap_num;
		unsigned int	mac_num;
		char			name[128];
		char			retype[32];
		char			tel[32];
		char			mobile[16];
		char			address[256];
		char			fuzeren[32];
		char			zuobiao[32];
		char			modidate[32];
	}shanghus[0];
}bg_shanghu_msg;

// 日报表上报报文
typedef struct bg_daily_msg
{
	char		 yesterday[16];
	unsigned int router_used_num;
	unsigned int router_yo_num;
	unsigned int ap_total_num;
	unsigned int ap_yo_num;
	unsigned int shanghu_num;
}bg_daily_msg;

#define BG_ROUTER_ADD		1
#define BG_ROUTER_DEL		-1

// 设备操作报文
typedef struct bg_router_msg
{
	char	order;		// 1 添加 -1 删除
	char	type[15];
	char	sn[64];
}bg_router_msg;

/********************** ac and ac with platform  ******************************/
#define AC_CMD_LOGIN				0x01	// 设备登陆命令
#define AC_CMD_LOGIN_ACK			0x02	// 设备登陆命令(回复)
#define AC_CMD_UPLOAD_APINFO		0x03	// 设备上报AP信息给服务器
#define AC_CMD_UPLOAD_APINFO_ACK	0x04    // 服务返回
#define AC_CMD_DOWNLOAD_APINFO		0x05	// 服务器发送AP 信息给设备
#define AC_CMD_DOWNLOAD_APINFO_ACK	0x06	// 设备返回
#define AC_CMD_CTRLCMD				0x07	// 服务器发送控制命令给设备
#define AC_CMD_CTRLCMD_ACK			0x08	// 设备回复
#define AC_CMD_PROXY_QUEST			0x09	// 代理 请求
#define AC_CMD_PROXY_QUEST_ACK		0x0A	// 代理请求回复
#define AC_CMD_UPLOAD_ACINFO		0x0B	// 服务器下发设备信息到路由器
#define AC_CMD_UPLOAD_ACINFO_ACK	0x0C	// 路由器回复
#define AC_CMD_MAX					0x0D

enum ac_login_state{
	ERROR_NONE			=0x01,  // 未知错误
	AC_LOGIN_NOUSER		=0x02,  // 没有该用户
	AC_LOGIN_ERRPWD		=0x03,  // 密码错误
	AC_LOGIN_LOGGED		=0x04,  // 账号已登录
	AC_LOGIN_ILLEGAL	=0x05,  // 随机ID 错误，非法使用
	AC_LOGIN_LOGINOK	=0x07,  // 登陆成功
	AC_LOGIN_BLACK		=0xA0,  // 随机ID 错误，非法使用
};

typedef struct ac_head_st
{
	unsigned char	ver;			// 版本号（目前版本号为4）
	unsigned char	cmdID;			// 命令号，参考上述宏定义。
	unsigned char	mode;			// 0..客户机->主机，1..主机到客户机	
	unsigned char	res;			// 保留
	unsigned int	device_type;	// 1为AC，2为无线路由，3为不带无线路由	
	unsigned int	deviceID;		// 路由器ID，唯一性
	unsigned int	session;		// 会话ID
	unsigned int	datalen;		// 数据长度，包含数据头长度 
	unsigned char	tags[16];		// 校验和，全包检验
	unsigned char	data[0];		// 数据指针
}ac_head_st;//36字节

struct ac_base_info_st
{
	__u8  addr[128];				// 设备地址
	__u8  cont[32];					// 联系人
	__u8  tel[32];					// 联系电话
	__u8  dinate[32];				// 设备坐标
	__u32 type;						// 设备类型0.. 网吧,1..企业，2..小区3..KTV ，4..咖啡厅，5..公共场所，6..运营机构，7..酒店，8..连锁,9..酒吧
	__u32 offlinenotauth;			// 离线时用户不需要认证,65.. 设备离线时用户不需要认证即可上网，其他..关闭该功能
};

typedef struct ac_login_st
{
	__u32   userID;						// 用户ID（代理商ID)
	__u32	deviceID;					// 设备ID (唯一性，0..需要服务器生成后返回,其他..)
	__u32   randID;						// 随机ID (由服务器生成，不需要验证密码时，由设备ID 和randid 进行验证)
	__u32   needPWD;					// 0..不验证密码，77..请求验证密码(说明:deviceID=0,或deviceID 手动被修改时)
	__u8    devicePWD[32];				// 设备密码
	__u8    deviceName[64];				// 路由器名称	
	__u8    sn[128];					// 设备序列号（唯一性）
	__u32   hearttime;					// 心跳时间单位（范围 5-600 秒） 用于 路由器 多长时间 发送一次 在线的信息，服务器指定。单位秒，默认为60秒
	__u8	state;						// 7..表示成功，其他需要取 msg 看错误信息。
	__u8	msg[255];					// 表示state错误时的信息
	struct ac_base_info_st	baseinfo;	// 客户端与服务端同步
}ac_login_st;


struct ac_ap_user_st
{
	__u32 user_num;				// (网络字节序) 当前用户数
	__u8  user_ssid_num[8];		// 每个SSID下的用户数
	__u32 create_time;			// 创建时间，只是客户端为AC时才使用，表示AC刚识到AP的时间戳
};

struct ac_ap_wifi_wds_list_st
{
	__u8 mac[6];				// MAC地址
	__u8 encrypt_type;			// 加密类型，0:NONE,1:WEP,2:TKIP,3:AES
	__u8 key[65];				// 密钥
}; /*72字节*/

struct ac_ap_wifi_wds_conf_st
{
	__u8 wds_mode;								// WDS模式，0:关闭,2:桥接模式,3:中继模式,4:自学习模式
	__u8 phy_mode;								// PHY模式，0:CCK,1:OFDM,2:HTMIX
	__u8 pad[2];								// 保留
	struct ac_ap_wifi_wds_list_st list[4];	// 支持4个WDS 288字节
}; /*292字节*/

//路由器无线设置之wpa设置
struct ac_wpa_key_st
{
	unsigned char	key[64];
	unsigned int	rekeyinterval;			//密钥更新间隔(网络字节序)
	unsigned int	radius_server;			//(网络字节序)
	unsigned int	session_timeout;		//会话超时(网络字节序)
	unsigned short	radius_port;			//(网络字节序)
	unsigned short	PMKCachePeriod;			//PMK缓存周期(网络字节序)
	unsigned short	PreAuthentication;		//预认证0:关闭1:打开(网络字节序)
	unsigned char	encrypt_type;			//1:TKIP,2:AES,0:TKIP/AES
	unsigned char	pad;
};

//路由器无线设置之wep加密设置
struct ac_wep_key_st
{
	unsigned char defaultkey; /*1-4 用*/
	unsigned char encrypt_type; /*0:NONE,1:WEP*/
	unsigned char pad[2];
	unsigned char key1type; /*1:ASCII 0:HEX*/
	unsigned char key1[27];
	unsigned char key2type; /*1:ASCII 0:HEX*/
	unsigned char key2[27];
	unsigned char key3type; /*1:ASCII 0:HEX*/
	unsigned char key3[27];
	unsigned char key4type; /*1:ASCII 0:HEX*/
	unsigned char key4[27];
};

struct ac_ap_wifi_base_list_st
{
	__u8 ssid[32];			// SSID
	__u8 hide;				// 隐藏
	__u8 isolated;			// 隔离
	__u8 security_mode;		// 0:关闭,1:开放式,2:共享式,3:WEPAUTO,4:WPA个人,5:WPA2个人,6:WPA/WPA2个人,7:WPA,8:WPA2,9:WPA1WPA2,10:IEEE8021X(占时不做)
#if 1 //大端序
	__u8 ssid_charset:4;	// SSID编码,0:GB2312,1:UTF-8
	__u8 balanced:4;		// 均衡权值,0:关闭1:用户数,2:信号强度,3:流量
#else//小端序
	__u8 balanced:4;		// 均衡权值,0:关闭1:用户数,2:信号强度,3:流量
	__u8 ssid_charset:4;	// SSID编码,0:GB2312,1:UTF-8
#endif	
	union{
		struct ac_wpa_key_st wpa;
 		struct ac_wep_key_st wep;		
	}key;
}; /*148字节*/

struct ac_ap_wifi_base_conf_st
{
	__u8 enable;								// 无线开关1:开启,0:关闭
	__u8 mode;									// 网络模式0:11b/g 混合,1:仅支持11b,4:仅支持11g,9:11b/g/n混合,6:仅支持11n(2.4G)
	__u8 channel;								// 频道
	__u8 ssid_num;								// SSID数
	__u8 isolated;								// AP 隔离0:禁用,1:启用
	char rssi_down;								// 信号低于阀值主动踢下线
	__u8 user_auth;								// 用户认证上网0:禁用,1:启用
	__u8 pad;									// 保留
	struct ac_ap_wifi_base_list_st list[5];	// 5个SSID的配置 740字节
}; /*744字节*/


struct ac_ap_dhcp_conf_st
{
	__u8  enable;			// 开关0:关闭,1:普通设置,2:高级设置
	__u8  pad[3];			// 保留
	__u32 lease_time;		// 租期时间(秒)(网络字节序)
	__u32 start_ipaddr;		// 开始IP地址(网络字节序)
	__u32 end_ipaddr;		// 结束IP地址(网络字节序)
	__u32 gw;				// 网关(网络字节序)
	__u32 mask;				// 掩码(网络字节序)
	__u32 dns[2];			// DNS地址(网络字节序)
};  /*32字节*/


struct ac_ap_base_conf_st
{
	__u8  ap_name[32];		// 设备标识(数字英文中文)
	__u8  ap_info[32];		// 另它信息
	__u8  vlan_name[32];	// VLAN 名称
	__u16 vlan_id;			// VLAN ID (网络字节序)
	__u8  ap_bssid[6];		// 设备的MAC
	__u8 max_num;			// 最大用户数
	__u8 ap_power;			// 发射功率(1-100)
	__u8 ap_mode;			// AP模式0:普通AP模式,2:桥接模式3:中继模式4:自学习模式
	__u8 pad;				// 保留
	__u32 ipaddr;			// AP IP地址(网络字节序)
	__u32 netmask;			// AP 子网掩码(网络字节序)
	__u32 gateway;			// AP 网关(网络字节序)
	__u8  ap_version[32];	// AP 版本信息
	__u8  atime[50];		// 系统启动到现在的时间（以秒为单位）
	__u8  ftime[50];		// 系统空闲的时间（以秒为单位）
	__u32 mem_total;		// 总内存，单位Byte
	__u32 mem_free;			// 剩余内存，单位Byte
	__u32 ct_max;			// 连接数容量
	__u32 ct_num;			// 当前连接数	
	__u16 web_port;			// web端口(网络字节序)
	__u8 pad2[2];			// 保留	
	__u8 http_username[32];	// WEB管理账号
	__u8 http_passwd[32];	// WB管理密码
}; /*268字节*/


//struct ac_ap_conf_st 结构中flag的定义
enum{
	AC_CONF_FLAG_base=1,			// 表示ap信息的base_info有修改
	AC_CONF_FLAG_dhcp=2,			// 表示ap信息的dhcp_info有修改
	AC_CONF_FLAG_wifi_base=4,		// 表示ap信息的wifi_base_info有修改
	AC_CONF_FLAG_wifi_wds=8,		// 表示ap信息的wifi_wds_info有修改
};

struct ac_ap_conf_st
{
	__u32							device_id;			// AP设备的device ID,唯一性 (网络字节序)
	__u32							flag;				// 第1位设置为1表示base_info有参数。。。(网络字节序)，在服务端下发时才有用
	struct ac_ap_base_conf_st		base_info;			// AP基本信息
	struct ac_ap_dhcp_conf_st		dhcp_info;			// AP的DHCP信息
	struct ac_ap_wifi_base_conf_st	wifi_base_info;		// AP的无线基本信息
	struct ac_ap_wifi_wds_conf_st	wifi_wds_info;		// AP的WDS信息
	struct ac_ap_user_st			user; 
};

// 无线配置
typedef struct ac_state_st
{
	__u32	deviceID;					// 设备ID
	__u32   state;						// 状态,2:成功，3为失败，服务器下发配置参数时客户端回应
	__u8    msg[256];					// 状态说明，服务器下发配置参数时客户端回应
	__u32   count;						// ap个数
	struct ac_ap_conf_st ap[0];		// count个ap的数据
}ac_state_st;

// 远程控制报文内容
typedef struct ac_proxy_st
{
	__u32 socket_php;		// PHP socket,请不要改变，原来
	__u32 ac_id;			// 控制设备的deviceID
	__u32 ap_id;			// 控制设备下面的AP的device_id
	__u32 enable;			// 78 开启,非78 为关闭
	__u32 port;				// 代理端口
	__u8  ctrlURL[512];		// 控制服务端地址URL，如:www.aaa.com
	__u8  msg[64];			// 控制客户端连接控制服务端消息
}ac_proxy_st;

#endif //WT_HEADER_H

/*****************************************************
 *
 * 头文件
 *
 *****************************************************/
#ifndef AUTH_HEADER_H
#define AUTH_HEADER_H

#include "list.h"
#include "cJSON.h"

#define ROUTER_MSG_DEBUG	0				// 打印报文信息

#define DNS_WHITE_URL_SIZE		1024		// 白名单大小 后期要修改
#define GG_MAX_SIZE				4096		// 富媒体最大长度

//user_msg_head_st 中 user_id 的使用 
#define USER_ID_ONLINE			'0'			// 对用户的操作类型 如阻止上网 允许上网等
#define USER_ID_SCANSQL			's'			// 平台发来的允许上线
#define USER_ID_TAKEOUT			't'			// 踢出
#define USER_ID_TIMEOUT			'm'			// 上网时间到

// 用户上线 处理报文返回值
#define USER_SQL_ERROR			1<<0
#define USER_NEED_AUTH			1<<2		// 需要认证
#define USER_IN_WHITE			1<<3		// 在白名单内
#define USER_CAN_MANYOU			1<<4		// 在漫游时间内
#define USER_SHANG				1<<6		// 更新shangmac表
#define USER_SHANG_OPEN			1<<7		// 更新shangmac表(无感知)
#define USER_SHANG_OPEN_REN		1<<8		// 更新shangmac表(无感知未认证)
#define USER_MAC				1<<9		// 更新mac表
#define USER_MAC_REN			1<<10		// 更新mac表(无感知未认证)

// router.c
extern void* router_epoll_thread(void *fd);
extern void* router_conn_thread(void* fd);

// server_mutual.c
extern void* guide_mutual_thread(void* fd);
extern void* platform_conn_thread(void *fd);

// time_task.c
extern void* timing_restart_thread(void *fd);
extern void* scanSQL_takeout_thread(void *fd);
//extern void* scanSQL_online_thread(void *fd);

//升级地址结构体
typedef struct upgrade_addr{
	int		id;								// 设备型号id
	char	version[32];					// 最新版本号
	char	codeurl[128];					// 固件地址
	int		ishavewifi;						// 是否有无线功能
	int		ishaveacserver;
}upgrade_addr;

//简单广告2 结构体
typedef struct simple_gg2_node{
	struct list_head	node;
	int					location;
	char				categories[128];
	char				script1[ DNS_WHITE_URL_SIZE ];
	char				script2[ DNS_WHITE_URL_SIZE ];
	char				fudomain[ DNS_WHITE_URL_SIZE ];
}simple_gg2_node;

//连接设备链表结构
typedef struct sock_list {
	struct list_head	node;
	int					stat;				// 1 需要添加监听 0 已添加 -1 需要删除
	int					sockfd;				// socket连接
	//pthread_mutex_t		send_lock;
	__u8				ver;				// 版本,初始为1; 添加 third_id 后改为2, 目前为4 2.3没有设备序列号
	__u8				ios_flag;			// 是否开启微信认证或微信分享
	__u8				weixin_flag;		// 微信白名单域名
	__u8				hycardiswx;			// 会员卡是否启用微信关注
	__u8				ishycard;			// 是否启用会员卡
	__u8				qq_flag;			// 是否开启qq认证
	__u8				sina_flag;			// 是否开启微博认证
	__u8				reserve_1;
	__u16				sec_flag;			// 数据加密方式,0: 不加密；其它可扩展
	__u16				reserve_2;
	__u32				heart_count;		// 心跳计数器
	__u32				router_id;			// 设备id,由主服务器分配
	__u8				hard_seq[64];		// 设备序列号
	__u32				shebei_pkid;		// 设备表主键id
	__u32				shanghuid;			// 商户id
	__u32				dailiid;			// 代理id
	time_t				last_heart_time;	// 最后一次心跳时间 当一分钟没有收到心跳的时候 判定设备离线
	__u32				xinghaoid;			// 设备型号
	__u32				setbw_up;			// 限制单用户上行速度
	__u32				setbw_down;			// 限制单用户下行速度
	__u32				user_manyou_time;	// 用户漫游时间
}sock_list;

//配置文件内读出的配置参数
extern char			cgv_sql_name[32];								// 数据库名
extern char			cgv_sql_user[32];								// 数据库登陆用户名
extern char			cgv_sql_pass[32];								// 数据库登陆密码
extern char			cgv_report_addr[32];							// 运行状态报告地址,即引导服务器地址
extern unsigned int	cgv_report_port;								// 运行状态报告端口,即引导服务器开放给认证服务器的端口
extern unsigned int	cgv_authenticate_port;							// 认证端口,开放给设备的

//数据库读出的配置参数
extern unsigned int sgv_tfree_manyou_time;							// mx_shang_maclist iscanmanyou的更新时间
extern char			sgv_trezhengurl[256];							// 跳转界面url
extern unsigned int sgv_tupdatelist;								// 多少次心跳更新数据库
extern char			sgv_white_dns_url[ DNS_WHITE_URL_SIZE ];		// 域名白名单
extern char			sgv_white_dns_of_ios_url[ DNS_WHITE_URL_SIZE ];	// 域名白名单,开启微信认证时
extern char			sgv_white_dns_of_qq_url[ DNS_WHITE_URL_SIZE ];	// 域名白名单,开启qq认证时
extern char			sgv_white_dns_of_sina_url[ DNS_WHITE_URL_SIZE ];// 域名白名单,开启微博认证时
extern char			sgv_white_dns_of_weixin_url[ DNS_WHITE_URL_SIZE ];// 域名白名单,开启微信白名单时
extern char			sgv_rm_script1[ DNS_WHITE_URL_SIZE ];			// 富媒体Script代码1
extern char			sgv_rm_script2[ DNS_WHITE_URL_SIZE ];			// 富媒体Script代码2
extern char			sgv_rm_fudomain[ DNS_WHITE_URL_SIZE ];			// 富媒体域名
extern unsigned int	sgv_testmacuserout;								// 用户阻止下线扫描时间 单位：分
extern char			sgv_weixinhtml1[2048];							// 微信分享-替换html代码1
extern char			sgv_weixinhtml2[2048];							// 微信分享-替换html代码2
extern char			sgv_noweixingotourl[256];						// 微信分享-非微信url的跳转地址
extern char			sgv_acserverurl[256];							// ac集中管理平台地址
extern unsigned int sgv_acserverport;								// 集中管理平台端口
extern char			sgv_acacserverurl[256];							// ac集中管理平台地址
extern unsigned int sgv_acacserverport;								// 集中管理平台端口
extern unsigned int	sgv_tdatasize;									// 用户数据流量上传间隔 分钟

extern char			sgv_monitor_servurl[64];						// 探针服务器地址
extern unsigned int	sgv_monitor_servport;							// 探针服务器端口
extern unsigned int	sgv_monitor_time;								// 发送间隔时间
extern unsigned int	sgv_monitor_timeout;							// 客户端超时时间

// 设备型号
extern upgrade_addr			*sgv_up_addr;								// 升级地址结构指针头
extern unsigned int			gv_router_type_num;							// 设备型号数量计数器
extern pthread_mutex_t		gv_up_addr_lock;							// 升级地址互斥锁

// 设备链表
extern struct list_head		gv_sock_list_head;							// 连接设备链表头
extern pthread_mutex_t		gv_list_lock;								// 连接设备链读写锁
extern unsigned int			gv_sock_count;								// 连接设备计数器

// 简单广告2 列表
extern struct list_head		sgv_simple_gg2_list_head;
extern unsigned int			gv_simple_gg2_list_count;
extern pthread_mutex_t		gv_simple_gg2_list_lock;

// 连接引导服务器状态
extern int					gv_guide_flag;
extern pthread_mutex_t		gv_guide_flag_lock;
extern unsigned int			gv_auth_code;

// 平台连接端口
unsigned int				gv_platform_port;								// 平台端口,开放给平台的


/*************************** authenticate with router ******************************/

// 路由器发出的用户信息，对应struct msg_head_st::order = msg_order_user_req
enum user_cmd_req_type {
	user_cmd_req_unknonw = 0,
	user_cmd_req_on,				//1 用户上线
	user_cmd_req_off,				//2 用户下线
	user_cmd_req_flow,				//3 用户流量信息改变，服务器应根据流量的改变作出相应的处理
	user_cmd_req_max,
};

/*
	服务器发出的对用户信息的操作类型，对应struct msg_head_st::order = msg_order_user_op
		有四种操作类型:
			1. 直接通过：用户可以上网。路由也不再询问服务器。
				直到用户断开连接或者服务器通知到新的命令。
			2. 阻止上网：用户无法访问互联网，路由也不再询问服务器。
				直到用户断开或者服务器通知新的命令。
			3. 阻止并跳转：用户设备发起 http 请求被重定向至服务器返回的auth_addr。
				下次用户再次发生白名单之外网络行为，路由再次询问服务器，
				用户如果完成认证，服务器会返回直接通过，未完成认证，服务器仍然返回阻止并跳转。
			4. 通过并跳转：其他协议不阻拦，当发生 http 请求的时候重定向至服务器返回的auth_addr，
				后面的流程和阻止并跳转一样
	注: 
		此状态在用户连接路由器到离开路由器之前都不会改变
*/
enum user_cmd_init_type {
	user_init_unknown = 0,
	user_init_pass,					//1 直接通过
	user_init_reject,				//2 阻止上网
	user_init_rj,					//3 阻止并跳转
	user_init_pj,					//4 通过并跳转
	user_init_max,
};

// 用户状态
enum user_cur_type {
	user_cur_unknown = 0,
	user_cur_log_in,				//1 用户上线
	user_cur_log_out,				//2 用户下线
	user_cur_max,
};

// 路由器响应服务器的认证结果，对应struct msg_head_st::order = msg_order_user_result
enum user_result_type {
	user_result_unknown = 0, 
	user_result_succ,				//1 路由器处理成功
	user_result_fail,				//2 路由器处理失败，失败的原因，需另作说明
	user_result_max,
};

// 路由器响应服务器自动升级结果
enum auto_upgrade_result_type
{
	auto_upgrade_unknow = 0,
	auto_upgrade_config_failed,		//1 操作失败
	auto_upgrade_config_suss,		//2 操作成功
	auto_upgrade_failed,			//3 升级失败
	auto_upgrade_suss,				//4 升级成功
	auto_upgrade_max,
};



// 用户上下线认证的数据包 
typedef struct user_msg_st
{
	__u8 cmd;						// 分别参考: enum user_cmd_req_type, enum user_cmd_init_type
	__u8 result;					// 分别参考enum user_result_type, enum user_cur_type.若失败，原因通过下面的auth_addr 域说明
	__u16 auth_len;					// auth_addr 域的长度
	__u32 user_ip;					// 路由器发出的用户的IP
	__u8 mac[8];					// 用户的mac
	__u32 speed[2];					// 速度控制， [0], 表示上行速度; [1], 表示下行速度
	__u8 user_id[64];				// 认证服务器回复用户的标识
	__u32 auth_time;				// 免重复认证的时间，单位分  默认没活动时候 删除主机时间 传0 默认是10s
	/*
		关于流量的信息
			对于每一个用户，会对其上行、下行流量进行控制。
			流量的信息，若是由服务器发出，即表示该用户当前的剩余流量;
			若是由路由器发出，即表示该用户在这次登录后累计使用的流量。
			若服务器发来的流量信息为0，即表示该用户的流量无限制。
	*/
	__u32 flow[2];					// 流量控制，单位 MB. [0], 表示上行流量;[1], 表示下行流量
	__u8 auth_addr[ 0 ];			// 认证服务器发来的跳转地址或是路由器回复操作失败的原因 若没有指定跳转地址，则使用struct cer_msg_st::def_redirect
}user_msg_st;

// 路由器发往认证服务器的心跳包，对应struct msg_head_st::order = msg_order_heart
typedef struct heart_msg_st
{
	__u32 router_id;									// 设备分配id
	__u32 user_num;										// 在线用户数
}heart_msg_st;

// 路由器修改域名白名单时的结构
typedef struct white_dns_msg_st
{
	__u16 url_len;										// 域名白名单长度
	__u8  url_data[0];								// 域名白名单，域名间以分号分隔 最长4096， 最多64个
}white_dns_msg_st;


// 服务器发送路由器自动升级时的结构
typedef struct auto_upgrade_msg_st
{
	unsigned short url_len;								// 升级地址长度
	unsigned char  url_data[0];							// 升级地址
}auto_upgrade_msg_st;

// 路由器回复服务器自动升级操作结果的结构
typedef struct auto_upgrade_result_st
{
	unsigned short result;								// 升级操作状态, 对应enum auto_upgrade_result_type
	unsigned short len;									// 升级地址长度
	unsigned char  data[0];								// 升级地址
}auto_upgrade_result_st;

// 服务器发送缓存页面地址到路由器结构
typedef struct simple_cache_msg_st
{
	unsigned short url_len;								// 缓存地址总长度, 不超过2048
	unsigned char  url_data[0];							// 缓存地址, 格式"http://www.url.com<http://www.url.com/<http://www.url.com/xxx.jpg<http://www.url.com/123?456<"
}simple_cache_msg_st;

// 缓存操作回复报文
typedef struct simple_cache_result_msg_st
{
	unsigned short result;								// 缓存操作状态, 对应enum auto_upgrade_result_type
	unsigned short len;									// 回复内容长度
	unsigned char  data[0];								// 回复内容
}simple_cache_result_msg_st;

// 路由器发给服务器的qq号信息
typedef struct third_qq_msg_st
{
	unsigned int   qq_num;								// QQ号码, 网络序
	unsigned char  mac[6];
	unsigned short qq_type;								// 登录QQ类型, 0 手机QQ, 1 电脑QQ, 网络序
}third_qq_msg_st;

// 服务器下发的简单广告信息
typedef struct simple_gg_msg_st
{
	__u32 en;											// 功能开启状态 0:关闭 1:开启 网络序
	__u32 js_len;										// 传入的JS内容长度 网络序
	__u32 dns_cnt;										// 传入的DNS长度 网络序列
	__u32 data_len;										// 数据长度 为js_len + dns_cnt
	__u8  data[0];										// 数据内容 依次存放 js dns
}simple_gg_msg_st;

//下发简单广告 路由器发给服务器的回复包
typedef struct result_msg_st
{
	__u16 result;										// 操作状态, 对应enum auto_upgrade_result_type
	__u16 len;											// 回复内容长度
	__u8  data[0];										// 回复内容
}result_msg_st;

//路由器无线设置之wep加密设置
struct wep_key_st
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

//路由器无线设置之wpa设置
struct wpa_key_st
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
//路由器无线设置之单个ssid设置
struct wifi_base_list_st
{
	unsigned char ssid[32];					//SSID, 无论配置为GB2312还是UTF-8, 传入时, 都必须为GB2312
	unsigned char hide;						//隐藏
	unsigned char isolated;					//隔离
	unsigned char security_mode;			//0:关闭,1:开放式,2:共享式,3:WEPAUTO,4:WPA个人,5:WPA2个人,6:WPA/WPA2个人,7:WPA,8:WPA2,9:WPA1WPA2,10:IEEE8021X(占时不做)
	unsigned char ssid_charset;				//SSID编码,0:GB2312,1:UTF-8
	unsigned char balanced;					//暂不使用, 保留
	unsigned char pad[3];
	union{
		struct wpa_key_st wpa;
		struct wep_key_st wep;
	}key;
}; 

//路由器无线配置报文
typedef struct wifi_base_conf_msg_st
{
	unsigned char wifi_enable;				//无线开关1:开启,0:关闭
	unsigned char wifi_mode;				//网络模式0:11b/g 混合,1:仅支持11b,4:仅支持11g,9:11b/g/n混合,6:仅支持11n(2.4G) ,7:11g/n混合
	unsigned char wifi_channel;				//频道
	unsigned char wifi_ssid_num;			//SSID数
	unsigned char wifi_isolated;			//AP 隔离0:禁用,1:启用
	char wifi_rssi_down;					//信号低于阀值主动踢下线
	unsigned char wifi_txpower;				//无线发射功率
	unsigned char wifi_pad;					//填充
	
	unsigned char run_mode;					//运行模式1:增强,0:普通  //增强模式时候,扩展频道要设置为20M 频道模式设置为单频
	unsigned char HT_BW;					//带宽1:20/40MHz,0:20MHz
	unsigned char HT_EXTCHA;				//扩展频道,仅带宽为20/40MHz时有效
	unsigned char HT_BSSCoexistence;		//频道模式1:单频,0:双频

	struct wifi_base_list_st list[5];	//SSID个数
}wifi_base_conf_msg_st;

//获取版本号回复报文
typedef struct router_version_msg_st
{
	unsigned char version[32];		/* 固件版本号:  FBM-220W-1.0.215*/
}router_version_msg_st;

//设置重启命令报文
typedef struct reboot_msg_st
{
	int type;      /*1重启, 其它错误, 网络序*/
}reboot_msg_st;

//速度限制
typedef struct third_user_qos_msg_st
{
	unsigned int enable;		// 状态开关, 1, 为开启, 其它为关闭, 网络序
	unsigned int qos_up;		// 上传速度, 单位KB/S, 网络序,  0为不限速
	unsigned int qos_down;		// 下传速度, 单位KB/S, 网络序,  0为不限速
	unsigned int exp_ip_num;	// 控制例外的IP 个数, 网络序
	unsigned int exp_ip[0];		// 以整形表示的例外IP 的数组，网络序
}third_user_qos_msg_st;

#define USB_SIMPLE_CACHE_OPT_ADD		0
#define USB_SIMPLE_CACHE_OPT_MOD		1
#define USB_SIMPLE_CACHE_OPT_DEL		2
#define USB_SIMPLE_CACHE_OPT_DEL_ALL	3
#define USB_SIMPLE_CACHE_OPT_GET_LIST	4

//usb缓存
typedef struct usb_simple_cache_st
{
	unsigned short opt;      /*操作类型0, add, 1, mod, 2, del, 3,del_all, 4,get_list*/
	unsigned char  name[32]; /*规则描述符 */
	unsigned short len;      /*URL len 网络序*/
	unsigned char  url[0];   /*URL */    
}usb_simple_cache_st;

//行为管理-关键词过滤 设置报文
typedef struct web_keyword_st
{
	unsigned char opt[8];				// 操作def add mod del get 
	unsigned char def[4];				// 控制状态字符串, "1" 关闭, "0" 开启
	unsigned char log[4];				// 日志字符串, "0" 关闭, "1" 开启
	unsigned char block[4];				// 弹出警告提示字符串, "0" 关闭, "1" 开启
	unsigned char en[4];				// 字符串 "1" 启用,  "0"禁用 
	unsigned char name[32];				// 规则描述
	unsigned char keyword_gb2312[64];	// 关键字, GB2312 编码,  如:  你好 gb2312=%C4%E3%BA%C3 
	unsigned char keyword_utf8[64];		// 关键字, UTF8 编码,	   如:  你好 utf8=%E4%BD%A0%E5%A5%BD
}web_keyword_st;

//行为管理-URL重定向 设置
typedef struct url_redirect_st
{
	unsigned char en[4];			// 状态,  字符串"0", 禁用, "1", 启用
	unsigned char name[32];			// 描述gb2312
	unsigned char host[32];			// URL的主机名称		
	unsigned char host_flag[4];		// URL 的主机名称选项, 字符串 "0", 相同, "1", 包含
	unsigned char url[64];			// 目录网页(URL)	
	unsigned char url_flag[4];		// 目录网页(URL)选项,    字符串"0", 全部, "1", 相同, "2", 前相同
	unsigned char parm[64];			// 网页的参数	
	unsigned char parm_flag[4];		// 网页的参数选项,     字符串" 0", 全部, "1", 包含, "2", 包含指定参数, "3", 未包含指定参数
	unsigned char urlrd[128];		// 重写向到	
	unsigned char ld_en[4];			// 是否将被重定向的URL在末尾	字符串"0" , 不追加, "1" 追加
	unsigned char ips[64];			// 主机IP地址范围, 为空表示所有IP	
	unsigned char log[4];			// 日志字符串 "0", 关闭, "1", 开启
	unsigned char time[64];			// 基于时间控制, 如果不设置时间默认传"OFF"
	unsigned char opt[8];			// 操作 add  mod  del  get 
}url_redirect_st;

//行为管理-流媒体识别 设置
typedef struct mx_app_filter_st
{
	unsigned char en[4];			// 状态,  字符串"0", 禁用, "1", 启用
	unsigned char name[32];			// 描述gb2312
	unsigned char act[4];			// 控制方式字符串"0" 允许 "1"阻止
	unsigned char user_id[64];		// 主机IP地址范围可以为空,为空表示所有IP 
	unsigned char shibie_name[128];	// 识别ID  多个ID 用',' 逗号分隔01000000,02000000,03000000,04000000,05000000,06000000
	unsigned char log[4];			// 日志字符串, "1"开启 "0" 关闭
	unsigned char time[64];			// 基于时间控制, 如果不设置时间默认传"OFF"
	unsigned char opt[8];			// 操作 add  mod  del  get 
}mx_app_filter_st;

// 阻止用户微信域名重定向 已不用
typedef struct domain_redirect_st
{
	unsigned char domain_len;	// 域名长度, 不能大于63, 否则出错
	unsigned char domain[63];	// 域名/IP, 
}domain_redirect_st;

// 微信分享
typedef struct weixin_share_set_st
{
	unsigned short  enable;				// 状态开关, 1, 开启, 0, 关闭
	unsigned short  html_len;			// html 文件长度, 最大不超过7K 
	unsigned char   redirt_url[256];	// 跳转到指定的微信URL
	unsigned char	html_data[0];		// html 文件内容mac 用%s 代替	
}weixin_share_set_st;

//USB路径文件列表获取
typedef struct usb_file_get_st
{
	unsigned char  dir[512];			// 需要获文件目录, USB 的绝对路径,  如: /wifi/1 /wifi/1/2 
}usb_file_get_st;

typedef struct third_speed_st
{
	unsigned int speed_up;      // 上传速度, 网络序, 单位KB/S
	unsigned int speed_down;    // 下传速度, 网络序, 单位KB/S
}third_speed_st;

// 简单广告2 数据体
typedef struct simple_gg2_st
{
	int  replace_pos_flag;				// 0:追加在结尾，2:追加在关键字之前，3:追加在关键字之后
	unsigned char replace_pos[128];		// 关键字, 如<html>, </head>
	unsigned int  dns_len;				// DNS 个数,   多个以逗号',' 分隔,  > 0 && < 1020
	unsigned int  js_len;				// > 0 && < 4090
	unsigned int  data_len;				// dns_len + js_len 
	unsigned char data[0];				// js_len 长度JS内容 后接DNS 
}simple_gg2_st;

// 简单广告2 数据头
typedef struct simple_gg2_head_st
{
	unsigned int en;					// 开关, 0, 关, 非0 开
	unsigned int cnt;					// 规则条数, 最大16条
	unsigned int data_len;				// 数据长度, 暂时未使用
	unsigned char data[0];				// 数据struct wt_simple_gg_rule_st
}simple_gg2_head_st;

// 设备http登陆用户名账号密码设置 获取
typedef struct httpd_pwd_st
{
	unsigned char user_name[32];		// 登录 用户名
	unsigned char passwd[32];			// 登录 密码
}httpd_pwd_st;

typedef struct monitor_set_st
{
	unsigned short state;				// 功能状态开关, 1,开启, 其它关闭	, 网络序
	unsigned short port;				// 通信端口, 网络序
	unsigned short time;				// 发送间隔时间, 网络序
	unsigned short timeout;				// 客户端超时时间, 网络序
	unsigned char  address[64];			// 服务器地址, 网络序, 功能未开启时, 此值为"system nonsupport wifi_monitor, wifi_monitor did not open!!"
}monitor_set_st;






#endif // AUTH_HEADER_H

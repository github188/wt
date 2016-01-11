/*****************************************************
 *
 * 主函数 & 启程各线程
 *
 *****************************************************/
#include "auth_header.h"

//配置文件内读出的配置参数
char			cgv_sql_name[32];								// 数据库名
char			cgv_sql_user[32];								// 数据库登陆用户名
char			cgv_sql_pass[32];								// 数据库登陆密码
char			cgv_report_addr[32];							// 运行状态报告地址,即引导服务器地址
unsigned int	cgv_report_port;								// 运行状态报告端口,即引导服务器开放给认证服务器的端口
unsigned int	cgv_authenticate_port;							// 认证端口,开放给设备的

//数据库读出的配置参数
unsigned int	sgv_tfree_manyou_time;							// mx_shang_maclist iscanmanyou的更新时间
char			sgv_trezhengurl[256];							// 跳转界面url
unsigned int	sgv_tupdatelist;								// 多少次心跳更新数据库
char			sgv_white_dns_url[ DNS_WHITE_URL_SIZE ];		// 域名白名单
char			sgv_white_dns_of_ios_url[ DNS_WHITE_URL_SIZE ];	// 域名白名单,开启微信认证时
char			sgv_white_dns_of_qq_url[ DNS_WHITE_URL_SIZE ];	// 域名白名单,开启qq认证时
char			sgv_white_dns_of_sina_url[ DNS_WHITE_URL_SIZE ];// 域名白名单,开启微博认证时
char			sgv_white_dns_of_weixin_url[ DNS_WHITE_URL_SIZE ];// 域名白名单,开启微信白名单时
char			sgv_rm_script1[ DNS_WHITE_URL_SIZE ];			// 富媒体Script代码1
char			sgv_rm_script2[ DNS_WHITE_URL_SIZE ];			// 富媒体Script代码2
char			sgv_rm_fudomain[ DNS_WHITE_URL_SIZE ];			// 富媒体域名

unsigned int	sgv_testmacuserout;								// 用户阻止下线扫描时间 单位：分
char			sgv_weixinhtml1[2048];							// 微信分享-替换html代码1
char			sgv_weixinhtml2[2048];							// 微信分享-替换html代码2
char			sgv_noweixingotourl[256];						// 微信分享-非微信url的跳转地址
char			sgv_acserverurl[256];							// ac集中管理平台地址
unsigned int	sgv_acserverport;								// 集中管理平台端口
char			sgv_acacserverurl[256];							// ac集中管理平台地址
unsigned int	sgv_acacserverport;								// 集中管理平台端口
unsigned int	sgv_tdatasize;									// 用户数据流量上传间隔 分钟

char			sgv_monitor_servurl[64];						// 探针服务器地址
unsigned int	sgv_monitor_servport;							// 探针服务器端口
unsigned int	sgv_monitor_time;								// 发送间隔时间
unsigned int	sgv_monitor_timeout;							// 客户端超时时间

// 设备型号
upgrade_addr		*sgv_up_addr;								// 升级地址结构指针头
unsigned int		gv_router_type_num = 0;						// 设备型号数量计数器
pthread_mutex_t		gv_up_addr_lock;							// 升级地址互斥锁

// 设备链表
LIST_HEAD(			gv_sock_list_head);							// 连接设备链表头
pthread_mutex_t		gv_list_lock;								// 连接设备链读写锁
unsigned int		gv_sock_count = 0;							// 连接设备计数器

// 简单广告2 列表
LIST_HEAD(			sgv_simple_gg2_list_head);
unsigned int		gv_simple_gg2_list_count;
pthread_mutex_t		gv_simple_gg2_list_lock;

// 连接引导服务器状态
int					gv_guide_flag = 0;
pthread_mutex_t		gv_guide_flag_lock;
unsigned int		gv_auth_code = 0;

// 平台连接端口
unsigned int		gv_platform_port;								// 平台端口,开放给平台的

/** 
 *@brief  修复上次连接这台服务器的设备的数据信息
 *@return success 0 failed -1
 */
int auth_sql_repair(unsigned int auth_code)
{
	wt_sql_handle handle;
	memset(&handle, 0, sizeof(wt_sql_handle));
	
	if( wt_sql_init(&handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		goto ERR;
	}
	
	//更新用户mac表do时间
	sprintf(handle.sql_str, "UPDATE mx_maclist SET isonline = 0 WHERE isonline = 1 \
			AND lastrid in (SELECT id from mx_shebei WHERE isonlien = 1 and reservid = %u)", auth_code);
	if ( wt_sql_exec(&handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
		goto STR_ERR;
	}

	//更新shang maclist表do时间
#ifdef ADVERTISING
	sprintf(handle.sql_str, "UPDATE mx_shanghu_maclist SET isonline = 0 WHERE isonline = 1 \
			AND lastrid in (SELECT id from mx_shebei WHERE isonlien = 1 and reservid = %u)", auth_code);
#else
	sprintf(handle.sql_str, "UPDATE mx_shang_maclist SET isonline = 0, ispop = 0 WHERE isonline = 1 \
			AND lastrid in (SELECT id from mx_shebei WHERE isonlien = 1 and reservid = %u)", auth_code);
#endif
	if ( wt_sql_exec(&handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
		goto STR_ERR;
	}
	
	//修改设备表记录
	sprintf(handle.sql_str, "UPDATE mx_shebei SET isonlien = 0 WHERE isonlien = 1 and reservid = %u", auth_code);
	if ( wt_sql_exec(&handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle.sql_str);
		goto STR_ERR;
	}

	wt_sql_destroy(&handle);
	xyprintf(0, "** O(∩ _∩ )O ~~ Repair authenticate sql over!");
	return 0;

STR_ERR:
	wt_sql_destroy(&handle);
ERR:
	xyprintf(0, "✟ ✟ ✟ ✟ -- %s %d:Repair authenticate sql failed", __FILE__, __LINE__);
	return -1;
}

/** 
 *@brief  获取配置
 *@return success 0 failed -1
 */
int get_config()
{
	//读取配置文件内的配置
	int fd;
	char buf[1024];
	if( init_ini(CONFIG_FILE_NAME, &fd, buf, 1024) ){
		return -1;
	}
	if( get_ini(buf, "sql_name", cgv_sql_name) ){
		return -1;
	}
	if( get_ini(buf, "sql_user", cgv_sql_user) ){
		return -1;
	}
	if( get_ini(buf, "sql_pass", cgv_sql_pass) ){
		return -1;
	}
	if( get_ini(buf, "report_addr", cgv_report_addr) ){
		return -1;
	}
	char temp[32];
	if( get_ini(buf, "report_port", temp) ){
		return -1;
	}
	cgv_report_port = atoi( temp );
	if( get_ini(buf, "authenticate_port", temp) ){
		return -1;
	}
	cgv_authenticate_port = atoi( temp );
	destroy_ini( fd );

	//打印配置文件中读取的配置
	xyprintf(0, "** O(∩ _∩ )O ~~ Get config of config file is success!");
	xyprintf(0, "sql_name = %s\n\
			sql_user = %s\n\
			sql_pass = %s",
			cgv_sql_name, cgv_sql_user, cgv_sql_pass);
	xyprintf(0, "report_addr = %s\n\
			report_port = %u\n\
			authenticate_port = %u\n",
			cgv_report_addr, cgv_report_port, cgv_authenticate_port);


	//获取数据库内的配置内容
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		goto ERR;
	}
    
	if( wt_sql_get_gg(handle) ){
		goto STR_ERR;
	}
	if( wt_sql_get_gg2(handle) ){
		goto STR_ERR;
	}
    if( wt_sql_get_other(handle) ){
		goto STR_ERR;
	}
    if( wt_sql_get_white(handle) ){
		goto STR_ERR;
	}
    if( wt_sql_get_weixin(handle) ){
		goto STR_ERR;
	}
    if( wt_sql_get_upurl(handle) ){
		goto STR_ERR;
	}

	wt_sql_destroy(handle);
	free(handle);
	return 0;

STR_ERR:
	wt_sql_destroy(handle);
	free(handle);
ERR:
	return -1;
}

/** 
 *@brief  主体程序 由子进程运行
 */
void run()
{
	//关闭 父进程打开的log文件
	logs_destroy();
	//打开 子进程的log文件
	if( logs_init("logs") ){
		printf("DATA_ERROR:%s %d -- logs_init error!!!!\n",__FILE__, __LINE__);
		exit(-1);
	}

	//打印程序启动标志信息
	xyprintf(0, "\n\t\t\t*************************************************************************\n\
			*************************************************************************\n\
			****                                                                 ****\n\
			****                 The Authenticate is running!                    ****\n\
			****                          Pid is %d                            ****\n\
			****                             %s                              ****\n\
			****                                                                 ****\n\
			*************************************************************************\n\
			*************************************************************************\n",
			getpid(), PROFRAM_VERSION);

	//获取配置
	if( get_config() ){
		xyprintf(0, "DATA_ERROR:%s %d -- Get config of datebase is failed!", __FILE__, __LINE__);
		sleep(1000);
		exit(-1);
	}

	pthread_mutex_init(&gv_list_lock, 0);
	pthread_mutex_init(&gv_simple_gg2_list_lock, 0);
	pthread_mutex_init(&gv_up_addr_lock, 0);
	pthread_mutex_init(&gv_guide_flag_lock, 0);
	
	//信号屏蔽
	signal(SIGPIPE, SIG_IGN);
	
/*************************thread pool run*********************************************/
	pool_init(48, 192, 16, cgv_sql_name, cgv_sql_user, cgv_sql_pass);
	sleep(1);
/*************************与guide交互线程**********************************************/
	pthread_t guide_mutual_pt;
	if( pthread_create(&guide_mutual_pt, NULL, guide_mutual_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
/**************************************************************************************/

	while(1){
		pthread_mutex_lock(&gv_guide_flag_lock);
		if(gv_guide_flag){
			auth_sql_repair(gv_auth_code);
			pthread_mutex_unlock(&gv_guide_flag_lock);
			break;
		}
		pthread_mutex_unlock(&gv_guide_flag_lock);
		sleep(1);
	}

/*************************路由器连接监听线程******************************************/
	pthread_t router_conn_pt;
	if( pthread_create(&router_conn_pt, NULL, router_conn_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
/*************************路由器epoll线程*********************************************/
	pthread_t router_epoll_pt;
	if( pthread_create(&router_epoll_pt, NULL, router_epoll_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
/*************************mac上线 允许上网处理线程************************************/
	pthread_t platform_conn_thread_pt;
	if( pthread_create(&platform_conn_thread_pt, NULL, platform_conn_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	//pthread_t scanSQL_online_pt;
	//if( pthread_create(&scanSQL_online_pt, NULL, scanSQL_online_thread, NULL) != 0 ){
	//	xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	//}
/*************************mac踢出 处理线程********************************************/
	pthread_t scanSQL_takeout_pt;
	if( pthread_create(&scanSQL_takeout_pt, NULL, scanSQL_takeout_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
/*************************定时重启设备线程*********************************************/
	pthread_t timing_restart_pt;
	if( pthread_create(&timing_restart_pt, NULL, timing_restart_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
/**************************************************************************************/
	
	while(1){
		sleep(1000);
	}
	
	//执行不到的地方
	pthread_mutex_destroy(&gv_up_addr_lock);
	pthread_mutex_destroy(&gv_list_lock);
	pthread_mutex_destroy(&gv_simple_gg2_list_lock);
	pthread_mutex_destroy(&gv_guide_flag_lock);
}

/** 
 *@brief  主函数 启动子进程运行主体程序 并用父进程监控子进程的运行状况
 */
int main(int argc, char** argv)
{
	if( logs_init("wizard_logs") ){
		printf("DATA_ERROR:%s %d -- logs_init error!!!!\n",__FILE__, __LINE__);
		exit(-1);
	}

	// 守护进程（父进程）
	int status;
	for(;;){
		if(0 == fork() ){
			run();
		}
		xyprintf(0, "ELF_THREAD:The child process is running!");
	    waitpid(-1, &status, 0);
		xyprintf(0, "ELF_THREAD:The child process is dead!");
		if(WIFSIGNALED(status)) {
			xyprintf(0, "ELF_THREAD:The child process is dead! Signal is %d", WTERMSIG(status) );
        }
	}
}

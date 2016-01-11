/*****************************************************
 *
 * 主函数 & 启动
 *
 *****************************************************/
#include "guide_header.h"

//全局变量 读取自配置文件
char cgv_sql_name[32];					// sql name
char cgv_sql_user[32];					// sql user
char cgv_sql_pass[32];					// sql password
int	 cgv_guide_port;					// 引导服务器开放给设备的端口
int  cgv_report_port;					// 引导服务器开放给认证服务器的状态报告端口
int  cgv_report_interval;				// 引导服务器与认证的状态报告报文间隔
int  cgv_third_port;					// 开放给第三方程序的监听接口
int	 cgv_platform_port;					// 开放给平台的端口

// 全局变量 读取自数据库
unsigned int sgv_status_update_interval;			// 状态更新线程执行间隔时间
unsigned int sgv_tfree_interval;					// tfree更新线程执行间隔时间
unsigned int sgv_ac_interval;

// 全局变量
LIST_HEAD(			gv_authenticate_list_head);		// 已启动认证服务器链表头
pthread_mutex_t		gv_authenticate_list_lock;		// 已启动认证服务器链表互斥锁
unsigned int		gv_authenticate_count = 0;		// 已启动认证服务器数量

unsigned int		gv_third_count = 0;				// 第三方监听程序连接计数器

unsigned int		gv_guide_router_count = 0;		// 引导过的设备计数器

pthread_mutex_t		gv_task_lock;					// 数据库定时任务互斥锁

pthread_mutex_t		gv_boss_flag_lock;
int					gv_boss_flag = 0;				// boss控制的是否可以引导的标志
int					gv_heart_interval = 300;		// 心跳时间
int					gv_num_falg = 0;				// 数量控制falg
int					gv_total_num = 0;				// 设备数量
int					gv_rt_num = 0;					// 设备类型数量
LIST_HEAD(			gv_boss_rt_head);				// 设备类型链表

/** 
 *@brief  获取程序配置
 *@return success 0 failed -1
 */
int get_config()
{
	int fd;
	char buf[1024];
	//config文件初始化
	if( init_ini(CONFIG_FILE_NAME, &fd, buf, 1024) ){
		return -1;
	}
	//获取sql数据库 配置名
	if( get_ini(buf, "sql_name", cgv_sql_name) ){
		return -1;
	}
	//获取sql数据库 登陆用户名
	if( get_ini(buf, "sql_user", cgv_sql_user) ){
		return -1;
	}
	//获取sql数据库 登陆密码
	if( get_ini(buf, "sql_pass", cgv_sql_pass) ){
		return -1;
	}
	char temp[16];
	//获取 认证服务器连接端口
	if( get_ini(buf, "report_port", temp) ){
		return -1;
	}
	cgv_report_port = atoi(temp);
	//获取 认证服务器状态报告间隔时间
	if( get_ini(buf, "report_interval", temp) ){
		return -1;
	}
	cgv_report_interval = atoi(temp);
	//获取 设备引导连接端口
	if( get_ini(buf, "guide_port", temp) ){
		return -1;
	}
	cgv_guide_port = atoi(temp);
	//获取第三方程序连接端口
	if( get_ini(buf, "third_port", temp) ){
		return -1;
	}
	cgv_third_port = atoi(temp);
	//获取平台的连接端口
	if( get_ini(buf, "platform_guide_port", temp) ){
		return -1;
	}
	cgv_platform_port = atoi(temp);
	//关闭config文件
	destroy_ini( fd );

	//打印配置信息
	xyprintf(0, "** O(∩ _∩ )O ~~ Get config success!!\n\tsql_name = %s\n\tsql_user = %s\n\tsql_pass = %s\n\tguide_port = %d\n\treport_port = %d\n\treport_interval = %d\n\tplatfrom_port = %d",
			cgv_sql_name, cgv_sql_user, cgv_sql_pass, cgv_guide_port, cgv_report_port, cgv_report_interval, cgv_platform_port);

	//打印配置信息
	xyprintf(0, "third_port = %d", cgv_third_port);

	//获取数据库内的配置内容
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		free(handle);
		return -1;
	}
	
	//3.2绑定变量和SQL语句查询结果
	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_ULONG,	&sgv_status_update_interval,	20,		&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2,	SQL_C_ULONG,	&sgv_tfree_interval,			20,		&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3,	SQL_C_ULONG,	&sgv_ac_interval,				20,		&handle->sql_err);

	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT TOP 1 tupdatelist ,tupdatefree, acacservertime FROM mx_rconfig");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		wt_sql_destroy(handle);
		free(handle);
		return -1;
	}

	handle->sql_ret=SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	
	if(handle->sql_ret == SQL_NO_DATA){
		xyprintf(0, "DATA_ERROR:%s %d -- Not has config in datebase!", __FILE__, __LINE__);
		wt_sql_destroy(handle);
		free(handle);
		return -1;
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	if( !sgv_status_update_interval ){
		sgv_status_update_interval = 5;
	}

	if( !sgv_tfree_interval ){
		sgv_tfree_interval = 5;
	}

	// 将时间转换为秒
	sgv_status_update_interval *= 60;
	sgv_tfree_interval *= 60;

	wt_sql_destroy(handle);
	free(handle);
	
	xyprintf(0, "status_update_interval = %u\n\ttfree_interval = %u",
			sgv_status_update_interval, sgv_tfree_interval);
	
	xyprintf(0, "ac_interval = %u", sgv_ac_interval);
	return 0;
}

/** 
 *@brief  程序主体函数 由子进程运行
 */
void run()
{
	//关闭父进程创建的log文件
	logs_destroy();
	//创建子进程log文件
	if( logs_init("logs") ){
		printf("logs_init error!!!!\n");
		exit(-1);
	}
	
	//打印程序开启信息
	xyprintf(0, "\n\t\t\t*************************************************************************\n\
			*************************************************************************\n\
			****                                                                 ****\n\
			****                      The Guide is running!                      ****\n\
			****                          Pid is %d                            ****\n\
			****                             %s                              ****\n\
			****                                                                 ****\n\
			*************************************************************************\n\
			*************************************************************************\n",
			getpid(), PROFRAM_VERSION);
	
	
	//获取配置
	if( get_config() ){
		printf("get config error!!!!\n");
		sleep(1000);
		exit(-1);
	}

	//初始化认证服务器list信息互斥锁 和 定时任务 互斥锁
	pthread_mutex_init(&gv_authenticate_list_lock, 0);
	pthread_mutex_init(&gv_task_lock, 0);
	pthread_mutex_init(&gv_boss_flag_lock, 0);

	/****************平台连接监视线程**************************************************/
	pthread_t pt;
	if( pthread_create(&pt, NULL, platform_conn_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/**********************************************************************************/

#if IS_CONN_BOSS
	/**************BOSS平台控制线程****************************************************/
	if(pthread_create(&pt, NULL, BOSS_PROCESS_THREAD, 0) != 0){ 
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/**********************************************************************************/
	
	sleep(1);

	/**************向boss的日报表线程**************************************************/
	if(pthread_create(&pt, NULL, DAILY_REPORT_THREAD, 0) != 0){ 
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/**********************************************************************************/
#endif

	/****************认证服务器连接监听线程********************************************/
	if(pthread_create(&pt, NULL, auth_conn_thread, 0) != 0){ 
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************认证服务器报文epoll线程*******************************************/
	if(pthread_create(&pt, NULL, auth_epoll_thread, 0) != 0){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************认证服务器信息状态刷新线程****************************************/
	if(pthread_create(&pt, NULL, auth_stat_refresh_thread, 0) != 0){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************第三方连接监听线程************************************************/
	if(pthread_create(&pt, NULL, third_conn_thread, 0) != 0){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************LOG_LAST表数据向LOG表内转移***************************************/
	if( pthread_create(&pt, NULL, LOG_LAST_TO_LOG_THREAD, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}	
	/****************数据库数据更新线程************************************************/
	if( pthread_create(&pt, NULL, TFREE_THREAD, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************数据库更新线程 凌晨5点的log***************************************/
	if( pthread_create(&pt, NULL, LOG_METASTASIS_THREAD, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************数据库统计线程 凌晨2点********************************************/
	if( pthread_create(&pt, NULL, STATISTICS_THREAD, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************数据库无效数据清除线程 6点****************************************/
	if( pthread_create(&pt, NULL, INVALID_DATA_CLEAN_THREAD, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************设备监听线程******************************************************/
	while(1){
		guide_conn_thread();
	}

	//不会执行到的一步
	pthread_mutex_destroy(&gv_authenticate_list_lock);
	pthread_mutex_destroy(&gv_task_lock);
	pthread_mutex_destroy(&gv_boss_flag_lock);
}

/** 
 *@brief  主函数 启动子进程中运行程序主体 父进程监控主进程的运行状态
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
		if( 0 == fork() ){
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

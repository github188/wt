/*****************************************************
 *
 * 主函数 & 启动
 *
 *****************************************************/

#include "boss_header.h"

//全局变量 读取自配置文件
char cgv_sql_name[32];					// sql name
char cgv_sql_user[32];					// sql user
char cgv_sql_pass[32];					// sql password
int	 cgv_platform_port;					// 开放给平台的端口

// 全局变量 读取自数据库
unsigned int	sgv_trnum;				// 初始设备数量
unsigned int	sgv_tupdate;			// 心跳间隔

// 全局变量
LIST_HEAD(			gv_guide_list_head);	// 已启动认证服务器链表头
pthread_mutex_t		gv_guide_list_lock;		// 已启动认证服务器链表互斥锁
unsigned int		gv_guide_count = 0;		// 已启动认证服务器数量
int					gv_time_ed_flag = 0;	// 定时任务完成标志

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
		xyprintf(0, "CONFIG_ERROR:Init ini failed!");
		return -1;
	}
	//获取sql数据库 配置名
	if( get_ini(buf, "sql_name", cgv_sql_name) ){
		xyprintf(0, "CONFIG_ERROR:Read sql name failed!");
		return -1;
	}
	//获取sql数据库 登陆用户名
	if( get_ini(buf, "sql_user", cgv_sql_user) ){
		xyprintf(0, "CONFIG_ERROR:Read sql user name failed!");
		return -1;
	}
	//获取sql数据库 登陆密码
	if( get_ini(buf, "sql_pass", cgv_sql_pass) ){
		xyprintf(0, "CONFIG_ERROR:Read sql password failed!");
		return -1;
	}
	char temp[16];
	// 获取开放给平台的端口
	if( get_ini(buf, "platform_port", temp) ){
		xyprintf(0, "CONFIG_ERROR:Read the port for platform failed!");
		return -1;
	}
	cgv_platform_port = atoi(temp);
	destroy_ini( fd );

	//打印配置信息
	xyprintf(0, "** O(∩ _∩ )O ~~ Get config success!!\n\tsql_name = %s\n\tsql_user = %s\n\tsql_pass = %s\n\tplatfrom_port = %d",
			cgv_sql_name, cgv_sql_user, cgv_sql_pass, cgv_platform_port);

	//获取数据库内的配置内容
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		free(handle);
		return -1;
	}

	//3.3执行SQL语句
	sprintf(handle->sql_str, "UPDATE mx_s_user set isonline = 0");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		wt_sql_destroy(handle);
		free(handle);
		return -1;
	}

	SQLBindCol( handle->sqlstr_handle, 1, SQL_C_ULONG,	&sgv_trnum,			20,  &handle->sql_err);//将变量和查询结果绑定
	SQLBindCol( handle->sqlstr_handle, 2, SQL_C_ULONG,	&sgv_tupdate,		20,  &handle->sql_err);//将变量和查询结果绑定
	
	sprintf(handle->sql_str, "SELECT TOP 1 trnum, tupdate FROM mx_s_configsys");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		wt_sql_destroy(handle);
		free(handle);
		return -1;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
	wt_sql_destroy(handle);
	free(handle);
	
	xyprintf(0, "** O(∩ _∩ )O ~~ Get config success!!\n\tsgv_trnum = %u\n\tsgv_tupdate = %u",
			sgv_trnum, sgv_tupdate);

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
			****                      The Boss is running!                       ****\n\
			****                          Pid is %d                            ****\n\
			****                             %s                              ****\n\
			****                                                                 ****\n\
			*************************************************************************\n\
			*************************************************************************\n",
			getpid(), PROFRAM_VERSION);
	
	
	//获取配置
	if( get_config() ){
		xyprintf(0, "get config error!!!!\n");
		sleep(100);
		exit(-1);
	}
	
	pthread_mutex_init(&gv_guide_list_lock, 0);

	/*************************thread pool run**********************************/
	pool_init(4, 4, 4, cgv_sql_name, cgv_sql_user, cgv_sql_pass);
	/****************监听guide连接线程*************************************************/
	pthread_t guide_conn_pt;
	if(pthread_create(&guide_conn_pt, NULL, guide_conn_thread, 0) != 0){ 
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************GUIDE epoll线程***************************************************/
	pthread_t guide_epoll_pt;
	if(pthread_create(&guide_epoll_pt, NULL, guide_epoll_thread, 0) != 0){ 
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************平台连接监听线程**************************************************/
	pthread_t platform_conn_pt;
	if(pthread_create(&platform_conn_pt, NULL, platform_conn_thread, 0) != 0){ 
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************定时任务线程******************************************************/
	pthread_t get_ss_pt;
	if(pthread_create(&get_ss_pt, NULL, get_ss_thread, 0) != 0){ 
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************定时任务线程******************************************************/
	pthread_t tfree_pt;
	if(pthread_create(&tfree_pt, NULL, tfree_thread, 0) != 0){ 
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************定时任务线程******************************************************/
	pthread_t sn_add_pt;
	if(pthread_create(&sn_add_pt, NULL, sn_add_warehouse_thread, 0) != 0){ 
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/****************定时任务线程******************************************************/
	pthread_t sn_del_pt;
	if(pthread_create(&sn_del_pt, NULL, sn_del_warehouse_thread, 0) != 0){ 
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/**********************************************************************************/
	
	while(1){
		sleep(1000);
	}

	//不会执行到的一步
	pthread_mutex_destroy(&gv_guide_list_lock);
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

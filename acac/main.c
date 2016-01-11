/*****************************************************
 *
 * AC管理服务器
 *
 *****************************************************/

#include "acac_header.h"

//配置文件内读出的配置参数
char	cgv_sql_name[32];
char	cgv_sql_user[32];
char	cgv_sql_pass[32];
int		cgv_platform_port;
int		cgv_proxy_port;
char	cgv_proxy_addr[32];

int		sgv_acac_port;
int		sgv_acac_interval;

LIST_HEAD(			gv_sock_list_head);				//连接设备链表头
pthread_mutex_t		gv_list_lock;					//连接设备链读写锁
__u32				gv_sock_count = 0;				//连接设备计数器

/** 
 *@brief  线程启动函数
 *@return nothing
 */
int ac_start()
{
/*************************thread pool run**********************************/
	pool_init(4, 4, 4, cgv_sql_name, cgv_sql_user, cgv_sql_pass);
	sleep(1);
/*************************accept_thread************************************/
	pthread_t accept_thread_pt;
	if( pthread_create(&accept_thread_pt, NULL, accept_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
/*************************epoll_thread*************************************/
	pthread_t epoll_thread_pt;
	if( pthread_create(&epoll_thread_pt, NULL, epoll_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
/*************************platform_thread*************************************/
	pthread_t platform_thread_pt;
	if( pthread_create(&platform_thread_pt, NULL, platform_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
/**************************************************************************/
	while(1){
		sleep(1000);
	}
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
		xyprintf(0, "Get config from ini file is failed -- %s %d", __FILE__, __LINE__);
		return -1;
	}
	if( get_ini(buf, "sql_name", cgv_sql_name) ){
		xyprintf(0, "Get config from ini file is failed -- %s %d", __FILE__, __LINE__);
		return -1;
	}
	if( get_ini(buf, "sql_user", cgv_sql_user) ){
		xyprintf(0, "Get config from ini file is failed -- %s %d", __FILE__, __LINE__);
		return -1;
	}
	if( get_ini(buf, "sql_pass", cgv_sql_pass) ){
		xyprintf(0, "Get config from ini file is failed -- %s %d", __FILE__, __LINE__);
		return -1;
	}
	char temp[16];
	if( get_ini(buf, "proxy_port", temp) ){
		xyprintf(0, "Get config from ini file is failed -- %s %d", __FILE__, __LINE__);
		return -1;
	}
	cgv_proxy_port = atoi( temp );
	if( get_ini(buf, "proxy_addr", cgv_proxy_addr) ){
		xyprintf(0, "Get config from ini file is failed -- %s %d", __FILE__, __LINE__);
		return -1;
	}
	if( get_ini(buf, "platform_acac_port", temp) ){
		xyprintf(0, "Get config from ini file is failed -- %s %d", __FILE__, __LINE__);
		return -1;
	}
	cgv_platform_port = atoi( temp );
	destroy_ini( fd );
	
	//打印配置文件中读取的配置
	xyprintf(0, "Get config success!!\n\tsql_name = %s\n\tsql_user = %s\n\tsql_pass = %s\n\tplatform_port = %d\n\tproxy_port = %d\n\tproxy_addr = %s",
			cgv_sql_name, cgv_sql_user, cgv_sql_pass, cgv_platform_port, cgv_proxy_port, cgv_proxy_addr);


	//获取数据库内的配置内容
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		free(handle);
		return -1;
	}
	
	//3.2绑定变量和SQL语句查询结果
	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_ULONG,	&sgv_acac_port,		20,		&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2,	SQL_C_ULONG,	&sgv_acac_interval,	20,		&handle->sql_err);

	//3.3执行SQL语句
	sprintf(handle->sql_str, "SELECT TOP 1 acacserverport, acacservertime FROM mx_rconfig");
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
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标//清空上次执行的数据 有数据会返回0 没有数据100 出错-1 这并不是一个好做法 应该还有其他的解决方法

	if( !sgv_acac_interval ){
		sgv_acac_interval = 300;
	}

	xyprintf(0, "acac_port = %d\n\tacac_interval = %d", sgv_acac_port, sgv_acac_interval);

	wt_sql_destroy(handle);
	free(handle);

	if(sgv_acac_port <= 0){
		return -1;
	}

	return 0;
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
			****                      The ACAC is running!                       ****\n\
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
	
	//信号屏蔽
	signal(SIGPIPE, SIG_IGN);
	
	//启动线程
	ac_start();

	//执行不到的地方
	pthread_mutex_destroy(&gv_list_lock);
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

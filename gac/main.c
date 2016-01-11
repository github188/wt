/*****************************************************
 *
 * AC管理服务器
 *
 *****************************************************/

#include "gac_header.h"

//配置文件内读出的配置参数
char	cgv_sql_name[32];
char	cgv_sql_user[32];
char	cgv_sql_pass[32];
int		cgv_gac_port;
int		cgv_gac_interval;

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
	if( get_ini(buf, "gac_port", temp) ){
		xyprintf(0, "Get config from ini file is failed -- %s %d", __FILE__, __LINE__);
		return -1;
	}
	cgv_gac_port = atoi( temp );
	if( get_ini(buf, "gac_interval", temp) ){
		xyprintf(0, "Get config from ini file is failed -- %s %d", __FILE__, __LINE__);
		return -1;
	}
	cgv_gac_interval = atoi( temp );
	destroy_ini( fd );
	
	//打印配置文件中读取的配置
	xyprintf(0, "Get config success!!\n\tsql_name = %s\n\tsql_user = %s\n\tsql_pass = %s\n\tac_port = %d\n\tac_interval = %d",
			cgv_sql_name, cgv_sql_user, cgv_sql_pass, cgv_gac_port, cgv_gac_interval);

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
			****                     The GAC is running!                         ****\n\
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

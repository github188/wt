/*****************************************************
 *
 * 主函数 & 启程各线程
 *
 *****************************************************/
#include "sauth_header.h"

//配置文件内读出的配置参数
char			cgv_sql_name[32];								// 数据库名
char			cgv_sql_user[32];								// 数据库登陆用户名
char			cgv_sql_pass[32];								// 数据库登陆密码
unsigned int	cgv_authenticate_port;							// 认证端口,开放给设备的

// 引导服务器链表
LIST_HEAD(			gv_guide_head);								// 引导服务器链表头
pthread_mutex_t		gv_guide_lock;								// 引导服务器链表互斥锁

unsigned int		cgv_platform_port;							// 平台端口,开放给平台的

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
	char temp[32];
	if( get_ini(buf, "authenticate_port", temp) ){
		return -1;
	}
	cgv_authenticate_port = atoi( temp );
	if( get_ini(buf, "auth_plat_port", temp) ){
		return -1;
	}
	cgv_platform_port = atoi( temp );
	destroy_ini( fd );

	//打印配置文件中读取的配置
	xyprintf(0, "** O(∩ _∩ )O ~~ Get config of config file is success!");
	xyprintf(0, "sql_name = %s\n\
			sql_user = %s\n\
			sql_pass = %s",
			cgv_sql_name, cgv_sql_user, cgv_sql_pass);
	xyprintf(0, "authenticate_port = %u\n\
			platform_port = %u\n",
			cgv_authenticate_port, cgv_platform_port);

	if(cgv_authenticate_port <= 1000){
		xyprintf(0,"ERROR:%s %d -- Authenticate_port from the config file is error!", __FILE__, __LINE__);
		return -1;
	}

	return get_guide_list();
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
			****                 The SAuthenticate is running!                   ****\n\
			****                          Pid is %d                            ****\n\
			****                             %s                              ****\n\
			****                                                                 ****\n\
			*************************************************************************\n\
			*************************************************************************\n",
			getpid(), PROFRAM_VERSION);

	//获取配置
	if( get_config() ){
		xyprintf(0, "DATA_ERROR:%s %d -- Get config of datebase is failed!", __FILE__, __LINE__);
		sleep(120);
		exit(-1);
	}

	//信号屏蔽
	signal(SIGPIPE, SIG_IGN);

	/*************************路由器连接监听线程******************************************/
	pthread_t router_conn_pt;
	if( pthread_create(&router_conn_pt, NULL, router_conn_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/*************************平台通信线程************************************************/
	pthread_t platform_conn_thread_pt;
	if( pthread_create(&platform_conn_thread_pt, NULL, platform_conn_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/*************************************************************************************/
	
	while(1){
		sleep(1000);
	}
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

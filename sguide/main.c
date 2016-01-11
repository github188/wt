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
int	cgv_guide_port;						// 引导服务器开放给设备的端口
int	cgv_authenticate_port;				// 认证服务器开放给设备的端口

unsigned int		gv_guide_router_count = 0;		// 引导过的设备计数器

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
	//获取 引导服务器开放给设备的端口
	if( get_ini(buf, "guide_port", temp) ){
		return -1;
	}
	cgv_guide_port = atoi(temp);
	//获取 认证服务器开放给设备的端口
	if( get_ini(buf, "authenticate_port", temp) ){
		return -1;
	}
	cgv_authenticate_port = atoi(temp);
	//关闭config文件
	destroy_ini( fd );

	//打印配置信息
	xyprintf(0, "** O(∩ _∩ )O ~~ Get config success!!\n\tsql_name = %s\n\tsql_user = %s\n\tsql_pass = %s\n\tguide_port = %d\n\tauthenticate_port = %d",
			cgv_sql_name, cgv_sql_user, cgv_sql_pass, cgv_guide_port, cgv_authenticate_port);

	if(cgv_guide_port <= 1000 || cgv_authenticate_port <= 1000){
		xyprintf(0, "ERROR:%s -- %d -- Port is error!!!", __FILE__, __LINE__);
		return -1;
	}

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
			****                      The Sguide is running!                     ****\n\
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

	/****************设备监听线程******************************************************/
	while(1){
		guide_conn_thread();
	}
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

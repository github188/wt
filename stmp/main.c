/*****************************************************
 *
 * STMP服务器
 *
 *****************************************************/

#include "stmp_header.h"

//配置文件内读出的配置参数
char	cgv_sql_name[32];
char	cgv_sql_user[32];
char	cgv_sql_pass[32];

int make_email()
{
	// 网页地址
	char address[] = "www.wshwifi.com";
	char path[] = "/email/email.htm";

	// 获取网页内容
	char *buf = malloc(10 * 1024);
	get_http(address, 80, path, buf, 10240);

	// 找出body部分的开始和结束位置
	char *body_begin, *body_end, *temp;
	body_begin = strstr( buf, "<body");
	if(body_begin == NULL){
		xyprintf(0, "%s %d", __FILE__, __LINE__);
		return 0;
	}
	body_end = strstr( body_begin, "</body>");
	if(body_end == NULL){
		xyprintf(0, "%s %d", __FILE__, __LINE__);
		return 0;
	}
	body_end[7] = 0;

	// 邮件内容存储变量
	char *body = malloc(body_end - body_begin + 8 + 1024);
	memset(body, 0, body_end - body_begin + 8 + 1024);
	char *from_addr = "1955650121@qq.com";
	char *subject = "万商维盟免费公共安全WiFi";
	int count = 0;

	// 数据库连接资源
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		free(handle);
		return -1;
	}

	// 查询qq号
	char qq[20];
	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_CHAR,	qq,		20,		&handle->sql_err);
	sprintf(handle->sql_str, "SELECT top 1000 qq FROM mx_maclist_qq WHERE loginnum = -3");
	//sprintf(handle->sql_str, "select qq from mx_maclist_qq where createdate > '2015-03-25 00:00:00' and createdate < '2015-03-25 23:59:59' group by qq order by qq desc");
	if ( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
		wt_sql_destroy(handle);
		free(handle);
		return -1;
	}

	handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	while(handle->sql_ret != SQL_NO_DATA){
		// 循环发送
		memset(body, 0, body_end - body_begin + 8 + 1024);
		sprintf(body, "%s%s%s%s%s%s%s%s%s%s%s%s%s", "From: \" 万商维盟安全WIFI \"<", from_addr, ">\r\nTo:  ",
			"\"", qq, "\"<", qq,
			"@qq.com>\r\nSubject:",
			subject,
			"\r\nMIME-Version:1.0\r\nContent-type: text/html;charset=UTF-8;boundary=\"---=_NextPart_000_0050_01C\"\r\n\r\n",
			body_begin,
			"\r\n",
			"\r\n.\r\n");
		//printf("\n\n\n%s\n\n\n", body);
		xyprintf(0, "---> ---> ---> ---> ---> ---> send email to %s, count is %d", qq, ++count);

		send_email("smtp.qq.com", body, "1955650121@qq.com", "mx123456789", qq);
		sleep(3);
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条。
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标//清空上次执行的数据 有数据会返回0 没有数据100 出错-1 这并不是一个好做法 应该还有其他的解决方法

		char *to_qq = "254072579";
		
		memset(body, 0, body_end - body_begin + 8 + 1024);
		sprintf(body, "%s%s%s%s%s%s%s%s%s%s%s%s%s", "From: \" 万商维盟安全WIFI \"<", from_addr, ">\r\nTo:  ",
			"\"", to_qq, "\"<", to_qq,
			"@qq.com>\r\nSubject:",
			subject,
			"\r\nMIME-Version:1.0\r\nContent-type: text/html;charset=UTF-8;boundary=\"---=_NextPart_000_0050_01C\"\r\n\r\n",
			body_begin,
			"\r\n",
			"\r\n.\r\n");
		//printf("\n\n\n%s\n\n\n", body);
		
		xyprintf(0, "---> ---> ---> ---> ---> ---> send email to %s, count is %d", to_qq, ++count);

		send_email("smtp.qq.com", body, "1955650121@qq.com", "mx123456789", to_qq);
		//sleep(60);


		to_qq = "24203561";
		
		memset(body, 0, body_end - body_begin + 8 + 1024);
		sprintf(body, "%s%s%s%s%s%s%s%s%s%s%s%s%s", "From: \" 万商维盟安全WIFI \"<", from_addr, ">\r\nTo:  ",
			"\"", to_qq, "\"<", to_qq,
			"@qq.com>\r\nSubject:",
			subject,
			"\r\nMIME-Version:1.0\r\nContent-type: text/html;charset=UTF-8;boundary=\"---=_NextPart_000_0050_01C\"\r\n\r\n",
			body_begin,
			"\r\n",
			"\r\n.\r\n");
		//printf("\n\n\n%s\n\n\n", body);
		
		xyprintf(0, "---> ---> ---> ---> ---> ---> send email to %s, count is %d", to_qq, ++count);

		send_email("smtp.qq.com", body, "1955650121@qq.com", "mx123456789", to_qq);

		to_qq = "941733897";
		
		memset(body, 0, body_end - body_begin + 8 + 1024);
		sprintf(body, "%s%s%s%s%s%s%s%s%s%s%s%s%s", "From: \" 万商维盟安全WIFI \"<", from_addr, ">\r\nTo:  ",
			"\"", to_qq, "\"<", to_qq,
			"@qq.com>\r\nSubject:",
			subject,
			"\r\nMIME-Version:1.0\r\nContent-type: text/html;charset=UTF-8;boundary=\"---=_NextPart_000_0050_01C\"\r\n\r\n",
			body_begin,
			"\r\n",
			"\r\n.\r\n");
		//printf("\n\n\n%s\n\n\n", body);
		
		xyprintf(0, "---> ---> ---> ---> ---> ---> send email to %s, count is %d", to_qq, ++count);

		send_email("smtp.qq.com", body, "1955650121@qq.com", "mx123456789", to_qq);

	free(body);
/*
	sprintf(body, "%s%s%s%s%s%s%s%s%s%s%s%s%s", "From: \" xy \"<", from_addr, ">\r\nTo:  ",
		"\"", "xy", "\"<", to_qq,
		"@qq.com>\r\nSubject:  =?UTF-8?",
		"B?5rWL6K+V6YKu5Lu2",
		"?=\r\nMIME-Version:1.0\r\nContent-type: text/html;charset=UTF-8;boundary=\"---=_NextPart_000_0050_01C\"\r\n\r\n",
		body_begin,
		"\r\n",
		"\r\n.\r\n");
*/
	xyprintf(0, "********** send over! ***************");
	return 0;
}

/** 
 *@brief  线程启动函数
 *@return nothing
 */
int stmp_start()
{
/*************************thread pool run**********************************/
//	pool_init(4, 4, 4, cgv_sql_name, cgv_sql_user, cgv_sql_pass);
//	sleep(1);
/*************************platform_thread*************************************/
//	pthread_t platform_thread_pt;
//	if( pthread_create(&platform_thread_pt, NULL, platform_thread, NULL) != 0 ){
//		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
//	}
/**************************************************************************/

	make_email();
/*	
	while(1){
		sleep(1000);
	}
	*/
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
	destroy_ini( fd );
	
	//打印配置文件中读取的配置
	xyprintf(0, "Get config success!!\n\tsql_name = %s\n\tsql_user = %s\n\tsql_pass = %s",
			cgv_sql_name, cgv_sql_user, cgv_sql_pass);


/*	
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
*/
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
			****                      The STMP is running!                       ****\n\
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

	//信号屏蔽
	signal(SIGPIPE, SIG_IGN);
	
	//启动线程
	stmp_start();

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
/*
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
	*/
	run();
}

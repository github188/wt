/*****************************************************
 *
 * 主函数 & 启程各线程
 *
 *****************************************************/
#include "header.h"

//配置文件内读出的配置参数
char			cgv_sql_name[32];								// 数据库名
char			cgv_sql_user[32];								// 数据库登陆用户名
char			cgv_sql_pass[32];								// 数据库登陆密码

int test_process()
{
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	wt_sql_handle *ihandle = malloc(sizeof(wt_sql_handle));
	memset(ihandle, 0, sizeof(wt_sql_handle));
	
	if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		goto ERR;
	}
	
	if( wt_sql_init(ihandle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
		xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
		goto ERR;
	}


	int count = 0;

	char mac[16], lasttime[32], mobile[16], lastdotime[32], lastdoingtime[32], createdate[32], fristtime[32];
	unsigned int lastrid, renzhengtype, isrenzheng, isonline, zonglinetime, manyou, companyid, id;
	//读取设备升级地址
	SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_ULONG,	&lastrid,		20,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2,	SQL_C_ULONG,	&renzhengtype,	20,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3,	SQL_C_ULONG,	&isrenzheng,	20,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 4,	SQL_C_ULONG,	&isonline,		20,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 5,	SQL_C_ULONG,	&zonglinetime,	20,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 6,	SQL_C_ULONG,	&manyou,		20,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 7,	SQL_C_ULONG,	&companyid,		20,				&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 8,	SQL_C_CHAR,		mac,			16,	&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 9,	SQL_C_CHAR,		lasttime,		32,	&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 10,	SQL_C_CHAR,		mobile,			16,	&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 11,	SQL_C_CHAR,		lastdotime,		32,	&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 12,	SQL_C_CHAR,		lastdoingtime,	32,	&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 13,	SQL_C_CHAR,		createdate,		32,	&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 14,	SQL_C_CHAR,		fristtime,		32,	&handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 15,	SQL_C_ULONG,	&id,			20,				&handle->sql_err);


	unsigned int macid;
	SQLBindCol(ihandle->sqlstr_handle, 1,	SQL_C_ULONG,	&macid,			20,				&ihandle->sql_err);


		sprintf(handle->sql_str, "SELECT TOP 1 lastrid, renzhengtype, isrenzheng, isonline, zongtime, iscanmanyou, companyid, \
			mac, lasttime, mobile, lastdotime, lastsetdoingtime, createdate, firsttime, id from mx_shang_maclist WHERE \
			macid not in (select id from mx_maclist) and isrenzheng=1");
		if (wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, handle->sql_str);
			goto SQL_ERR;
		}

		handle->sql_ret = SQLFetch(handle->sqlstr_handle);
		
		while(handle->sql_ret != SQL_NO_DATA){
			count ++;
			xyprintf(0, "count is %d, id is %u", count, id);

			sprintf(ihandle->sql_str, "INSERT INTO mx_maclist(mac, lasttime, mobile, lastsetdoingtime, createdate, \
				lastrid, firsttime, firstrid, renzhengtype, isrenzheng, isonline, zonlinetime, manyou, companyid) \
				VALUES('%s', '%s', '%s', '%s', '%s', \
					  %u, '%s', %u, %u, %u, %u, %u, %u, %u)",
					mac, lasttime, mobile, lastdoingtime, createdate,
					lastrid, fristtime, lastrid, renzhengtype, isrenzheng, isonline, zonglinetime, manyou, companyid);
			if ( wt_sql_exec(ihandle) ){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, ihandle->sql_str);
				goto SQL_ERR;
			}

			sprintf(ihandle->sql_str, "SELECT SCOPE_IDENTITY()");
			if( wt_sql_exec(ihandle) ){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, ihandle->sql_str);
				goto SQL_ERR;
			}
			ihandle->sql_ret = SQLFetch(ihandle->sqlstr_handle);//获取查询到的结果
			SQLFreeStmt(ihandle->sqlstr_handle, SQL_CLOSE);  //释放游标
			
			sprintf(ihandle->sql_str, "UPDATE mx_shang_maclist set macid = %u WHERE id = %u", macid, id);
			if( wt_sql_exec(ihandle) ){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, ihandle->sql_str);
				goto SQL_ERR;
			}
		
			
			handle->sql_ret = SQLFetch(handle->sqlstr_handle);
		}
	
	xyprintf(0, "return!");
	return 0;

SQL_ERR:
ERR:
	xyprintf(0, "return of -1!");
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
	destroy_ini( fd );

	//打印配置文件中读取的配置
	xyprintf(0, "** O(∩ _∩ )O ~~ Get config of config file is success!");
	xyprintf(0, "sql_name = %s\n\
			sql_user = %s\n\
			sql_pass = %s",
			cgv_sql_name, cgv_sql_user, cgv_sql_pass);

	return test_process();
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
			****                      The TEST running!                          ****\n\
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

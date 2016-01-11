/*****************************************************
 *
 * 对数据库的定时任务
 *
 *****************************************************/
#include "guide_header.h"

#define STATISCITS_DEBUG 1

/** 
 *@brief  maclist shang_maclist 无效数据清楚
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* INVALID_DATA_CLEAN_THREAD(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Invalid data clean thread is running!!!");

	unsigned int sleep_time = 0;
	time_t now;
	struct tm *p;

	while(1){
		time(&now);
		p = localtime(&now);
		if(p->tm_hour < 6){
			sleep_time = ( 6 - p->tm_hour - 1 ) * 60 * 60 + ( 60 - p->tm_min ) * 60;
		}
		else {
			sleep_time = ( 24 + 6 - p->tm_hour - 1) * 60 * 60 + ( 60 - p->tm_min ) * 60;
		}
	
		xyprintf(0, "** (～﹃～)~zZ ~~ Invalid data clean thread will sleep %u s!!!", sleep_time );
		sleep( sleep_time );
		xyprintf(0, "INVALID_DATA_CLEAN:** O(∩ _∩ )O ~~ Invalid data clean thread is get up!!!");
		sleep_time = 24 * 60 * 60;

		// 数据库访问资源
		wt_sql_handle *iud_handle = malloc(sizeof(wt_sql_handle));		//INSERT UPDATE DELETE用`
		memset(iud_handle, 0, sizeof(wt_sql_handle));
	
		while( wt_sql_init(iud_handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){		// 数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
			sleep( 30 );
		}
	
		pthread_mutex_lock(&gv_task_lock);//⊙﹏⊙
		
		//处理时间测试 -- 开始时间
		struct timeval start, end;
		gettimeofday( &start, NULL );

		// 删除shangmac 中 未认证的用户
#ifdef ADVERTISING
		sprintf(iud_handle->sql_str, "DELETE FROM mx_shanghu_maclist WHERE isrenzheng = 0");
#else
		sprintf(iud_handle->sql_str, "DELETE FROM mx_shang_maclist WHERE isrenzheng = 0");
#endif
		if( wt_sql_exec(iud_handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, iud_handle->sql_str);
			goto STR_ERR;
		}
		gettimeofday( &end, NULL );
		xyprintf(0, "INVALID_DATA_CLEAN:☻ ☺ ☻ ☺  mx_shang_maclist table clean is over, used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);

		// 删除maclist中 未认证的用户
		sprintf(iud_handle->sql_str, "DELETE FROM mx_maclist WHERE isrenzheng = 0");
		if( wt_sql_exec(iud_handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, iud_handle->sql_str);
			goto STR_ERR;
		}
		gettimeofday( &end, NULL );
		xyprintf(0, "INVALID_DATA_CLEAN:☻ ☺ ☻ ☺  mx_maclist table clean is over, used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);

		// 删除流量表中 大于24小时的数据
		sprintf(iud_handle->sql_str, "DELETE FROM mx_shebei_datasize_log WHERE DATEDIFF(hh, createdate, GETDATE()) > 24");
		if( wt_sql_exec(iud_handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, iud_handle->sql_str);
			goto STR_ERR;
		}
		gettimeofday( &end, NULL );
		xyprintf(0, "INVALID_DATA_CLEAN:☻ ☺ ☻ ☺  mx_shebei_datasize_log table clean is over, used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
//当一轮循环执行完成的时候,销毁数据库连接资源在下次执行的时候重新创建,如果执行出现错误时,提前结束
STR_ERR:
		pthread_mutex_unlock(&gv_task_lock);//⊙﹏⊙
		
		//处理时间 -- 结束时间
		gettimeofday( &end, NULL );
		xyprintf(0, "INVALID_DATA_CLEAN:☻ ☺ ☻ ☺ ");
		xyprintf(0, "INVALID_DATA_CLEAN:☻ ☺ ☻ ☺  Invalid data clean bao ping'an, used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
		xyprintf(0, "INVALID_DATA_CLEAN:☻ ☺ ☻ ☺ ");
		
		wt_sql_destroy(iud_handle);
		free(iud_handle);
	}//while(1);

ERR:
	xyprintf(0, "INVALID_DATA_CLEAN_ERROR:✟ ✟ ✟ ✟ -- %s %d:Invalid data clean pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  定时更新数据库 m_shang_maclist iscanmanyou
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* TFREE_THREAD(void *fd)
{
#define TFREE_DEBUG		1

	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Tfreemanyou thread is running!!!");

	while(1){
		xyprintf(0, "** (～﹃～)~zZ ~~ Tfree thread will sleep %u s!!!", sgv_tfree_interval );
		sleep( sgv_tfree_interval );
		xyprintf(0, "TFREE:** O(∩ _∩ )O ~~ Tfree thread is get up!!!");
	
		// 数据库访问资源
		wt_sql_handle *iud_handle = malloc(sizeof(wt_sql_handle));		//INSERT UPDATE DELETE用`
		memset(iud_handle, 0, sizeof(wt_sql_handle));
	
		while( wt_sql_init(iud_handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){		// 数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
			sleep( 30 );
		}
		
		pthread_mutex_lock(&gv_task_lock);//⊙﹏⊙
	
		//处理时间测试 -- 开始时间
		struct timeval start, end;
		gettimeofday( &start, NULL );
		
		//更新iscanmanyou标志
#ifdef ADVERTISING
		sprintf(iud_handle->sql_str, "UPDATE mx_shanghu_maclist SET iscanmanyou = 0 WHERE id IN ( SELECT id FROM mx_shanghu_maclist \
			WHERE iscanmanyou = 1 AND isonline = 0 AND DATEDIFF(n, outmanyou, GETDATE()) >= 0)");
#else
		sprintf(iud_handle->sql_str, "UPDATE mx_shang_maclist SET iscanmanyou = 0 WHERE id IN ( SELECT id FROM mx_shang_maclist \
			WHERE iscanmanyou = 1 AND isonline = 0 AND DATEDIFF(n, outmanyou, GETDATE()) >= 0)");
#endif
		if(wt_sql_exec(iud_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- iud_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
			goto STR_ERR;
		}
#if TFREE_DEBUG	
		gettimeofday( &end, NULL );
		xyprintf(0, "TFREE:☻ ☺ ☻ ☺  tfree_fun UPDATE mx_shang_maclist is over, used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
#endif		

#ifdef ADVERTISING
#else
		// 更新会员表信息
		sprintf(iud_handle->sql_str, "UPDATE mx_shanghu_hycard SET isonline=0 WHERE isonline=1 AND id NOT IN \
				(SELECT hycardid FROM mx_shang_maclist WHERE isonline=1 AND renzhengtype=3 AND hycardid>0)");
		if(wt_sql_exec(iud_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- iud_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
			goto STR_ERR;
		}
#endif

#if TFREE_DEBUG	
		gettimeofday( &end, NULL );
		xyprintf(0, "TFREE:☻ ☺ ☻ ☺  tfree_fun UPDATE mx_shanghu_hycard is over, used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
#endif		

		// 更新AP online信息
		sprintf(iud_handle->sql_str, "UPDATE mx_ac_ap SET isonline = 0 WHERE isonline=1 AND DATEDIFF(s, lastdotime, GETDATE()) > %u",
				sgv_ac_interval * 2 );
		if(wt_sql_exec(iud_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- iud_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
			goto STR_ERR;
		}

#if TFREE_DEBUG	
		gettimeofday( &end, NULL );
		xyprintf(0, "TFREE:☻ ☺ ☻ ☺  tfree_fun UPDATE mx_ac_ap is over, used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
#endif		
		
//当一轮循环执行完成的时候,销毁数据库连接资源在下次执行的时候重新创建,如果执行出现错误时,提前结束
STR_ERR:
		pthread_mutex_unlock(&gv_task_lock);//⊙﹏⊙
		
		//处理时间 -- 结束时间
		gettimeofday( &end, NULL );
		xyprintf(0, "TFREE:☻ ☺ ☻ ☺ ");
		xyprintf(0, "TFREE:☻ ☺ ☻ ☺  tfree_fun 's bao ping'an, , used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
		xyprintf(0, "TFREE:☻ ☺ ☻ ☺ ");

		wt_sql_destroy(iud_handle);
		free(iud_handle);
	}//while(1);

ERR:
	xyprintf(0, "TFREE_ERROR:✟ ✟ ✟ ✟ -- %s %d:Tfree pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  loglast 向log表的迁移
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* LOG_LAST_TO_LOG_THREAD(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Loglast to log thread is running!!!");

	while(1){
		xyprintf(0, "** (～﹃～)~zZ ~~ Loglast to log thread will sleep %u s!!!", sgv_tfree_interval );
		sleep( sgv_tfree_interval );
		xyprintf(0, "TFREE:** O(∩ _∩ )O ~~ Loglast to log thread is get up!!!");
	
		// 数据库访问资源
		wt_sql_handle *s_handle = malloc(sizeof(wt_sql_handle));		//SELECT用
		memset(s_handle, 0, sizeof(wt_sql_handle));
		wt_sql_handle *iud_handle = malloc(sizeof(wt_sql_handle));		//INSERT UPDATE DELETE用`
		memset(iud_handle, 0, sizeof(wt_sql_handle));
	
		
		while( wt_sql_init(s_handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			// 数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
			sleep( 30 );
		}
		
		while( wt_sql_init(iud_handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){		// 数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
			sleep( 30 );
		}
		
		//处理时间测试 -- 开始时间
		struct timeval start, end;
		gettimeofday( &start, NULL );
		
		// 用户上网记录表 参数
		int id, macid, shanghuid, rid, bw_up, bw_down, zongtime, mactype;
		char lastdotime[32], lastsetdoingtime[32], createdate[32];
/*
	//
	//将mx_mac_log_last islast=0 的记录移动到mx_mac_log
	//
		SQLBindCol(s_handle->sqlstr_handle, 1,	SQL_C_ULONG,	&id,				32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 2,	SQL_C_ULONG,	&macid,				32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 3,	SQL_C_ULONG,	&shanghuid,			32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 4,	SQL_C_ULONG,	&rid,				32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 5,	SQL_C_ULONG,	&bw_up,				32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 6,	SQL_C_ULONG,	&bw_down,			32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 7,	SQL_C_ULONG,	&zongtime,			32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 8,	SQL_C_ULONG,	&mactype,			32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 9,	SQL_C_CHAR,		&lastdotime,		32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 10,	SQL_C_CHAR,		&lastsetdoingtime,	32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 11,	SQL_C_CHAR,		&createdate,		32, &s_handle->sql_err);
		
		//查出来
		sprintf(s_handle->sql_str, "SELECT id, macid, shanghuid, rid, bw_up, bw_down, zongtime, mactype, lastdotime, lastsetdoingtime, \
				createdate FROM mx_mac_log_last	WHERE islast = 0");
		if(wt_sql_exec(s_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s_handle->sql_str);
			goto STR_ERR;
		}

		//获取第一条记录
		s_handle->sql_ret = SQLFetch(s_handle->sqlstr_handle);

		while( s_handle->sql_ret != SQL_NO_DATA){		//如果有数据就一直执行下去
			//使用另一个handle将记录添加到mx_mac_log表
			sprintf(iud_handle->sql_str, "INSERT INTO mx_mac_log (macid, shanghuid, rid, bw_up, bw_down, \
				zongtime, mactype, lastdotime, lastsetdoingtime, createdate, islast) \
				VALUES (%d, %d, %d, %d, %d, %d, %d, '%s', '%s', '%s', 0)",
				macid, shanghuid, rid, bw_up, bw_down, zongtime, mactype,
				lastdotime, lastsetdoingtime, createdate);
			if(wt_sql_exec(iud_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- iud_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
				goto STR_ERR;
			}

			//删除mx_mac_log_last表中的记录
			sprintf(iud_handle->sql_str, "DELETE FROM mx_mac_log_last WHERE id = %d", id );
			if(wt_sql_exec(iud_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- iud_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
				goto STR_ERR;
			}
			
			//打印一下
			//xyprintf(0, "TFREE:☻ ☺ ☻ ☺  Successful transfer of a mac record!!!");
			//xyprintf(0, "id = %d, macid = %d, shanghuid = %d, rid = %d, zongtime = %d, mactype = %d, lastdotime = %s, lastsetdoingtime = %s, createdate = %s",
			//		id, macid, shanghuid, rid, zongtime, mactype, lastdotime, lastsetdoingtime, createdate);

			//获取下一条记录
			s_handle->sql_ret = SQLFetch(s_handle->sqlstr_handle);	
		}// end while
		
		gettimeofday( &end, NULL );
		xyprintf(0, "LOGLASTTOLOG:☻ ☺ ☻ ☺  mac_log_last to mac_log is over, used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
*/
	//
	//将mx_shenbei_log_last islast=0 的记录移动到mx_shenbei_log
	//
		SQLBindCol(s_handle->sqlstr_handle, 1,	SQL_C_ULONG,	&id,			32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 2,	SQL_C_ULONG,	&shanghuid,		32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 3,	SQL_C_ULONG,	&rid,			32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 4,	SQL_C_ULONG,	&bw_up,			32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 5,	SQL_C_ULONG,	&bw_down,		32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 6,	SQL_C_ULONG,	&zongtime,		32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 7,	SQL_C_CHAR,		&lastdotime,	32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 8,	SQL_C_CHAR,		&createdate,	32, &s_handle->sql_err);
	
		//查出临时表中 islast=0 的记录
#ifdef ADVERTISING
		sprintf(s_handle->sql_str, "SELECT id, shanghuid, rid, bw_up, bw_down, zongtime, lastdotime, createdate \
				FROM mx_shebei_log_last WHERE islast = 0");
#else
		sprintf(s_handle->sql_str, "SELECT id, shanghuid, rid, bw_up, bw_down, zongtime, lastdotime, createdate \
				FROM mx_shenbei_log_last WHERE islast = 0");
#endif
		if(wt_sql_exec(s_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s_handle->sql_str);
			goto STR_ERR;
		}
		
		//获取第一条记录
		s_handle->sql_ret = SQLFetch(s_handle->sqlstr_handle);

		while( s_handle->sql_ret != SQL_NO_DATA){
			//添加记录到mx_shenbei_log
#ifdef ADVERTISING
			sprintf(iud_handle->sql_str, "INSERT INTO mx_shebei_log (shanghuid, rid, bw_up, bw_down, \
				zongtime, lastdotime, createdate) \
				VALUES (%d, %d, %d, %d, %d, '%s', '%s')",
				shanghuid, rid, bw_up, bw_down, zongtime, lastdotime, createdate);
#else
			sprintf(iud_handle->sql_str, "INSERT INTO mx_shenbei_log (shanghuid, rid, bw_up, bw_down, \
				zongtime, lastdotime, createdate, islast) \
				VALUES (%d, %d, %d, %d, %d, '%s', '%s', 0)",
				shanghuid, rid, bw_up, bw_down, zongtime, lastdotime, createdate);
#endif
			if(wt_sql_exec(iud_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- iud_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
				goto STR_ERR;
			}

			//删除mx_shenbei_log_last的记录
#ifdef ADVERTISING
			sprintf(iud_handle->sql_str, "DELETE FROM mx_shebei_log_last WHERE id = %d", id );
#else
			sprintf(iud_handle->sql_str, "DELETE FROM mx_shenbei_log_last WHERE id = %d", id );
#endif
			if(wt_sql_exec(iud_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- iud_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
				goto STR_ERR;
			}

			//xyprintf(0, "TFREE:☻ ☺ ☻ ☺  Successful transfer of a shenbei record!!!");
			//xyprintf(0, "id = %d, shanghuid = %d, rid = %d, zongtime = %d, lastdotime = %s, createdate = %s",
			//		id, shanghuid, rid, zongtime, lastdotime, createdate);
			
			//获取下一条记录
			s_handle->sql_ret = SQLFetch(s_handle->sqlstr_handle);	
		}

//当一轮循环执行完成的时候,销毁数据库连接资源在下次执行的时候重新创建,如果执行出现错误时,提前结束
STR_ERR:
		//处理时间 -- 结束时间
		gettimeofday( &end, NULL );
		xyprintf(0, "LOGLASTTOLOG:☻ ☺ ☻ ☺ ");
		xyprintf(0, "LOGLASTTOLOG:☻ ☺ ☻ ☺  Loglast to log's bao ping'an, , used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
		xyprintf(0, "LOGLASTTOLOG:☻ ☺ ☻ ☺ ");

		wt_sql_destroy(s_handle);
		free(s_handle);
		wt_sql_destroy(iud_handle);
		free(iud_handle);
	}//while(1);

ERR:
	xyprintf(0, "LOGLASTTOLOG:✟ ✟ ✟ ✟ -- %s %d:Loglast to log pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}


#define ONCE_METASTASIS_LOG_NUM		10000
#define RESERVE_DAYS				15

/** 
 *@brief  log迁移
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* LOG_METASTASIS_THREAD(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Logs to old thread is running!!!");

	unsigned int sleep_time = 0;
	time_t now;
	struct tm *p;

	while(1){
		time(&now);
		p = localtime(&now);
		if(p->tm_hour < 4){
			sleep_time = ( 4 - p->tm_hour - 1 ) * 60 * 60 + ( 60 - p->tm_min ) * 60;
		}
		else {
			sleep_time = ( 24 + 4 - p->tm_hour - 1) * 60 * 60 + ( 60 - p->tm_min ) * 60;
		}
		
		xyprintf(0, "** (～﹃～)~zZ ~~ Logs to old thread will sleep %u s!!!", sleep_time);
		sleep( sleep_time );
		xyprintf(0, "METASTASIS:** O(∩ _∩ )O ~~ Logs to old thread is get up!!!");
		sleep_time = 24 * 60 * 60;
	
		// 数据库访问资源
		wt_sql_handle *s_handle = malloc(sizeof(wt_sql_handle));		//SELECT用
		memset(s_handle, 0, sizeof(wt_sql_handle));
		wt_sql_handle *iud_handle = malloc(sizeof(wt_sql_handle));		//INSERT UPDATE DELETE用`
		memset(iud_handle, 0, sizeof(wt_sql_handle));
		
		//按照id排序 一次查询出id最小的10000条 这个变量用来记录最大的id值
		unsigned int log_count = 0;
		
		while( wt_sql_init(s_handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			// 数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
			sleep( 30 );
		}
		
		while( wt_sql_init(iud_handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){		// 数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
			sleep( 30 );
		}
		
		pthread_mutex_lock(&gv_task_lock);//⊙﹏⊙
	
		//处理时间测试 -- 开始时间
		struct timeval start, end;
		gettimeofday( &start, NULL );
		
		// 用户上网记录表 参数
		int id, macid, shanghuid, rid, bw_up, bw_down, zongtime, mactype;
		int bw_up_last, bw_down_last;
		char lastdotime[32], lastsetdoingtime[32], createdate[32];
		
	//
	//将mx_shenbei_log_last islast=0 的记录移动到mx_shenbei_log
	//
		SQLBindCol(s_handle->sqlstr_handle, 1,	SQL_C_ULONG,	&id,			32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 2,	SQL_C_ULONG,	&shanghuid,		32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 3,	SQL_C_ULONG,	&rid,			32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 4,	SQL_C_ULONG,	&bw_up,			32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 5,	SQL_C_ULONG,	&bw_down,		32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 6,	SQL_C_ULONG,	&zongtime,		32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 7,	SQL_C_CHAR,		&lastdotime,	32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 8,	SQL_C_CHAR,		&createdate,	32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 9,	SQL_C_ULONG,	&bw_up_last,	32, &s_handle->sql_err);
		SQLBindCol(s_handle->sqlstr_handle, 10,	SQL_C_ULONG,	&bw_down_last,	32, &s_handle->sql_err);
		
		while(1){
			log_count = 0;
			//查出临时表中 islast=0 的记录
#ifdef ADVERTISING
			sprintf(s_handle->sql_str, "SELECT TOP %d id, shanghuid, rid, bw_up, bw_down, zongtime, lastdotime, createdate \
					FROM mx_shebei_log WHERE DATEDIFF(d, createdate, getdate()) > %d order by id asc",
					ONCE_METASTASIS_LOG_NUM, RESERVE_DAYS);
#else
			sprintf(s_handle->sql_str, "SELECT TOP %d id, shanghuid, rid, bw_up, bw_down, zongtime, lastdotime, createdate, bw_up_last, bw_down_last \
					FROM mx_shenbei_log WHERE DATEDIFF(d, createdate, getdate()) > %d order by id asc",
					ONCE_METASTASIS_LOG_NUM, RESERVE_DAYS);
#endif
			if(wt_sql_exec(s_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s_handle->sql_str);
				goto STR_ERR;
			}
		
			//获取第一条记录
			s_handle->sql_ret = SQLFetch(s_handle->sqlstr_handle);

			if(s_handle->sql_ret == SQL_NO_DATA){
				break;
			}

			while( s_handle->sql_ret != SQL_NO_DATA){
				
				log_count++;
				
				//添加记录到mx_shenbei_log
#ifdef ADVERTISING
				sprintf(iud_handle->sql_str, "INSERT INTO mx_shenbei_logold (shanghuid, rid, bw_up, bw_down, \
					zongtime, lastdotime, createdate, islast) \
					VALUES (%d, %d, %d, %d, %d, '%s', '%s', 0)",
					shanghuid, rid, bw_up, bw_down, zongtime, lastdotime, createdate);
#else
				sprintf(iud_handle->sql_str, "INSERT INTO mx_shenbei_logold (shanghuid, rid, bw_up, bw_down, \
					bw_up_last, bw_down_last, zongtime, lastdotime, createdate, islast) \
					VALUES (%d, %d, %d, %d, %d, %d, %d, '%s', '%s', 0)",
					shanghuid, rid, bw_up, bw_down, bw_up_last, bw_down_last, zongtime, lastdotime, createdate);
#endif
				if(wt_sql_exec(iud_handle)){
					xyprintf(0, "SQL_ERROR: %s %d -- iud_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
					goto STR_ERR;
				}

				//xyprintf(0, "METASTASIS:☻ ☺ ☻ ☺  Successful transfer of a shenbei record!!!");
				//xyprintf(0, "id = %d, shanghuid = %d, rid = %d, zongtime = %d, lastdotime = %s, createdate = %s",
				//		id, shanghuid, rid, zongtime, lastdotime, createdate);
			
				//获取下一条记录
				s_handle->sql_ret = SQLFetch(s_handle->sqlstr_handle);	
			}

			xyprintf(0, "METASTASIS:Metastasis %u shebei log!", log_count);
			//删除mx_shenbei_log_last的记录
#ifdef ADVERTISING
			sprintf(iud_handle->sql_str, "DELETE FROM mx_shebei_log WHERE id in \
					(SELECT TOP %d id FROM mx_shebei_log WHERE DATEDIFF(d, createdate, getdate()) > %d order by id asc)",
					ONCE_METASTASIS_LOG_NUM, RESERVE_DAYS);
#else
			sprintf(iud_handle->sql_str, "DELETE FROM mx_shenbei_log WHERE id in \
					(SELECT TOP %d id FROM mx_shenbei_log WHERE DATEDIFF(d, createdate, getdate()) > %d order by id asc)",
					ONCE_METASTASIS_LOG_NUM, RESERVE_DAYS);
#endif
			if(wt_sql_exec(iud_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- iud_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
				goto STR_ERR;
			}
			xyprintf(0, "METASTASIS:Delete %u shebei log!", log_count);
			sleep(30);
		}
		
//当一轮循环执行完成的时候,销毁数据库连接资源在下次执行的时候重新创建,如果执行出现错误时,提前结束
STR_ERR:
		pthread_mutex_unlock(&gv_task_lock);//⊙﹏⊙
		
		//处理时间 -- 结束时间
		gettimeofday( &end, NULL );
		xyprintf(0, "METASTASIS:☻ ☺ ☻ ☺ ☻ ☺ ");
		xyprintf(0, "METASTASIS:☻ ☺ ☻ ☺  Today's Metastasis log is over, used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
		xyprintf(0, "METASTASIS:☻ ☺ ☻ ☺ ☻ ☺ ");

		wt_sql_destroy(s_handle);
		free(s_handle);
		wt_sql_destroy(iud_handle);
		free(iud_handle);
	}//while(1);

ERR:
	xyprintf(0, "METASTASIS_ERROR:✟ ✟ ✟ ✟ -- %s %d:Logs to old thread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

// 商户log数据统计
int log_tongji(wt_sql_handle *s1_handle, wt_sql_handle *s2_handle, wt_sql_handle *iud_handle, char *s_yesterday)
{
	unsigned int id = 0;
	SQLBindCol(s1_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&id,		32, &s1_handle->sql_err);
	sprintf(s1_handle->sql_str, "SELECT id FROM mx_shanghu");
	if(wt_sql_exec(s1_handle)){
		xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s1_handle->sql_str);
		return -1;	
	}

	//获取第一条记录
	s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);
	while( s1_handle->sql_ret != SQL_NO_DATA ){		//如果有数据就一直执行下去
		
		// 查看log是否存在
		unsigned int temp = 0, count = 0;
		SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&temp,	32, &s2_handle->sql_err);
		sprintf(s2_handle->sql_str, "SELECT top 1 count(id) FROM mx_shang_maclist WHERE shanghuid = %u and \
				createdate>='%s 00:00:00' and createdate<='%s 23:59:59' and isrenzheng = 1", id, s_yesterday, s_yesterday);
		if(wt_sql_exec(s2_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
			return -1;
		}
			
		s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
		count = temp;
		SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标

		sprintf(s2_handle->sql_str, "SELECT TOP 1 id FROM mx_shang_otherinfo WHERE shanghuid = %u", id);

		if( wt_sql_exec(s2_handle) ){
			xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, s2_handle->sql_str);
			return -1;
		}
		s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);//获取查询到的结果
		SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
		if( s2_handle->sql_ret != SQL_NO_DATA ){
			sprintf(iud_handle->sql_str, "UPDATE mx_shang_otherinfo set remacnum = remacnum + %u WHERE shanghuid = %u", count, id);
			if( wt_sql_exec(iud_handle) ){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, iud_handle->sql_str);
				return -1;
			}
		}
		else {
			sprintf(iud_handle->sql_str, "INSERT INTO mx_shang_otherinfo(shanghuid, remacnum) VALUES(%u, %u)", id, count);
			if( wt_sql_exec(iud_handle) ){
				xyprintf(0, "SQL_ERROR:%s %s %d -- sql string is -- %s", __func__, __FILE__, __LINE__, iud_handle->sql_str);
				return -1;
			}
		}

#if STATISCITS_DEBUG
			xyprintf(0, "mac count is over, shanghuid is %u", id);
#endif
		s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);
	}
	return 0;
}

// 商户log数据统计
int log_statiscits(wt_sql_handle *s1_handle, wt_sql_handle *s2_handle, wt_sql_handle *iud_handle, char *s_yesterday, char *s_delete_log_day)
{
	unsigned int id = 0;
	SQLBindCol(s1_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&id,		32, &s1_handle->sql_err);
#ifdef ADVERTISING
	sprintf(s1_handle->sql_str, "SELECT id FROM mx_shanghu WHERE isok = 1");
#else
	sprintf(s1_handle->sql_str, "SELECT id FROM mx_shanghu WHERE isok = 1 and \
			startdate<='%s 00:00:00' and enddate>='%s 23:59:59'", s_yesterday, s_yesterday);
#endif
	if(wt_sql_exec(s1_handle)){
		xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s1_handle->sql_str);
		return -1;	
	}

	//获取第一条记录
	s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);
	while( s1_handle->sql_ret != SQL_NO_DATA ){		//如果有数据就一直执行下去
		
		// 查看log是否存在
		unsigned int temp = 0;
		SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&temp,	32, &s2_handle->sql_err);
		sprintf(s2_handle->sql_str, "SELECT top 1 id FROM mx_shanghu_maclog_day WHERE shanghuid=%u and logdate='%s'", id, s_yesterday);
		if(wt_sql_exec(s2_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
			return -1;
		}
			
		s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
			
		if(s2_handle->sql_ret == SQL_NO_DATA){
#if STATISCITS_DEBUG
			xyprintf(0, "maclog_day no date, shanghuid is %u", id);
#endif
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			//如果log不存在 则统计数据 生成log
			// mac_num
			unsigned int macid = 0;
			SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&macid,	32, &s2_handle->sql_err);
			sprintf(s2_handle->sql_str, "SELECT macid FROM mx_view_maclog_last3 WHERE shanghuid = %u and createdate>='%s 00:00:00' and \
					createdate<='%s 23:59:59' group by macid", id, s_yesterday, s_yesterday);
			if(wt_sql_exec(s2_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
				return -1;
			}
					
			s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
				
			int mac_num = 0;
					
			while( s2_handle->sql_ret != SQL_NO_DATA){
				mac_num++;
				s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);		//获取下一条记录
			}

			unsigned int mac_num_new = 0;
			SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&mac_num_new,	32, &s2_handle->sql_err);
#ifdef ADVERTISING
			sprintf(s2_handle->sql_str, "SELECT count(id) as num FROM mx_shanghu_maclist WHERE shanghuid = %u and \
					createdate>='%s 00:00:00' and createdate<='%s 23:59:59' and isrenzheng = '1'", id, s_yesterday, s_yesterday);
#else
			sprintf(s2_handle->sql_str, "SELECT count(id) as num FROM mx_shang_maclist WHERE shanghuid = %u and \
					createdate>='%s 00:00:00' and createdate<='%s 23:59:59' and isrenzheng = '1'", id, s_yesterday, s_yesterday);
#endif
			if(wt_sql_exec(s2_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
				return -1;
			}
					
			s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
				
			int mac_num_old = mac_num - mac_num_new;
			if(mac_num_old < 0){
				mac_num_old = 0;
				mac_num = mac_num_new;
			}
				
			sprintf(iud_handle->sql_str, "INSERT into mx_shanghu_maclog_day(shanghuid,logdate,macnum,macnumold,macnumnew) \
					values(%u, '%s', '%u', '%d', '%u')",
				    id, s_yesterday, mac_num, mac_num_old, mac_num_new);
			if(wt_sql_exec(iud_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
				return -1;
			}
				
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
				
		}
		else {
#if STATISCITS_DEBUG
			xyprintf(0, "maclog_day has date, shanghuid is %u", id);
#endif
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
		}
				
		//获取下一条记录
		s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);	
	}// end while( s1_handle->sql_ret != SQL_NO_DATA )

	// 删除昨天之前的mac log
	sprintf(s2_handle->sql_str, "DELETE FROM mx_mac_log_last WHERE id IN( SELECT id FROM mx_mac_log_last \
		WHERE createdate<='%s 23:59:59' AND islast = 0)", s_yesterday);
	if(wt_sql_exec(s2_handle)){
		xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
		return -1;
	}
	
	// 删除昨天之前的mac log
	sprintf(s2_handle->sql_str, "delete from mx_shang_maclist where lastdotime <= '%s'", s_delete_log_day);
	if(wt_sql_exec(s2_handle)){
		xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
		return -1;
	}
	sprintf(s2_handle->sql_str, "delete from mx_maclist where id not in (select macid from mx_shang_maclist)");
	if(wt_sql_exec(s2_handle)){
		xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
		return -1;
	}
	sprintf(s2_handle->sql_str, "delete from mx_maclist_qq where macid not in (select id from mx_maclist)");
	if(wt_sql_exec(s2_handle)){
		xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
		return -1;
	}

	return 0;
}

// yesterday展示和点击次数统计
int show_yesterday_statiscits(wt_sql_handle *s1_handle, wt_sql_handle *s2_handle, wt_sql_handle *iud_handle, char* s_yesterday)
{
	unsigned int id = 0;
	SQLBindCol(s1_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&id,		32, &s1_handle->sql_err);
	sprintf(s1_handle->sql_str, "SELECT id FROM mx_shanghu WHERE shanghutype='2' and isok='1' and \
			startdate<='%s 00:00:00' and enddate>='%s 23:59:59'", s_yesterday, s_yesterday);
	if(wt_sql_exec(s1_handle)){
		xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s1_handle->sql_str);
		return -1;
	}

	//获取第一条记录
	s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);
	while( s1_handle->sql_ret != SQL_NO_DATA ){		//如果有数据就一直执行下去
		
		// 查看log是否存在
		unsigned int temp = 0;
		SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&temp,	32, &s2_handle->sql_err);
		sprintf(s2_handle->sql_str, "SELECT top 1 id FROM mx_ad_shanghu_daylog WHERE shanghuid=%u and logdate='%s'", id, s_yesterday);
		if(wt_sql_exec(s2_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
			return -1;
		}
			
		s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
		
		if(s2_handle->sql_ret == SQL_NO_DATA){
#if STATISCITS_DEBUG
			xyprintf(0, "shanghu_daylog no date, shanghuid is %u", id);
#endif
			//如果log不存在 则统计数据 生成log
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			// mac_num
			unsigned int isclick = 0, isclick_count = 0;
			SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&isclick,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 2,	SQL_C_ULONG,		&isclick_count,	32, &s2_handle->sql_err);
			sprintf(s2_handle->sql_str, "SELECT isclick,count(isclick) as num FROM mx_view_ad_show_detail1 WHERE shanghuid = %u and\
					createdate>='%s 00:00:00' and createdate<='%s 23:59:59' group by isclick", id, s_yesterday, s_yesterday);
			if(wt_sql_exec(s2_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
				return -1;
			}
				
			int show_ok_num = 0, show_1 = 0;
			s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
			while(s2_handle->sql_ret != SQL_NO_DATA){
				if(isclick == 0){
					show_ok_num = isclick_count;
				}
				if(isclick == 1){
					show_1 = isclick_count;
				}
				s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
			}

			sprintf(s2_handle->sql_str, "SELECT isclick,count(isclick) as num FROM mx_view_ad_payfor_detail1 WHERE shanghuid = %u and \
					createdate>='%s 00:00:00' and createdate<='%s 23:59:59' group by isclick", id, s_yesterday, s_yesterday);
			if(wt_sql_exec(s2_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
				return -1;
			}
				
			int click_0 = 0, click_ok_num = 0;
			s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
			while(s2_handle->sql_ret != SQL_NO_DATA){
				if(isclick == 0){
					click_0 = isclick_count;
				}
				if(isclick == 1){
					click_ok_num = isclick_count;
				}
				s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
			}

			int show_num  = show_ok_num + show_1;
			int click_num = click_ok_num + click_0;
				
			sprintf(iud_handle->sql_str, "INSERT into mx_ad_shanghu_daylog(shanghuid,logdate,zongnum,adshownum,adclicknum,adshowoknum,adclickoknum) \
					values(%u, '%s', '%d', '%d', '%d', '%d', '%d')",
				    id, s_yesterday, show_num + click_num, show_num, click_num, show_ok_num, click_ok_num);
			if(wt_sql_exec(iud_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
				return -1;
			}
				
		}
		else {
#if STATISCITS_DEBUG
			xyprintf(0, "shanghu_daylog has date, shanghuid is %u", id);
#endif
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
		}
				
		//获取下一条记录
		s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);	
	}// end while( s1_handle->sql_ret != SQL_NO_DATA )
	
	return 0;
}

// 月展示和点击次数统计
int show_month_statiscits(wt_sql_handle *s1_handle, wt_sql_handle *s2_handle, wt_sql_handle *iud_handle, char *s_yesterday, char* s_month_first_day)
{
	unsigned int id = 0;
	SQLBindCol(s1_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&id,		32, &s1_handle->sql_err);
	sprintf(s1_handle->sql_str, "SELECT id FROM mx_shanghu WHERE shanghutype='2' and isok='1' \
			and startdate<='%s 00:00:00' and enddate>='%s 23:59:59'", s_yesterday, s_yesterday);
	if(wt_sql_exec(s1_handle)){
		xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s1_handle->sql_str);
		return -1;
	}

	//获取第一条记录
	s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);
	while( s1_handle->sql_ret != SQL_NO_DATA ){		//如果有数据就一直执行下去

		unsigned int logid = 0;
		char modidate[16];
		SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&logid,		16, &s2_handle->sql_err);
		SQLBindCol(s2_handle->sqlstr_handle, 2,	SQL_C_CHAR,			&modidate,	16, &s2_handle->sql_err);
		sprintf(s2_handle->sql_str, "SELECT top 1 id, modidate FROM mx_ad_shanghu_monthlog WHERE shanghuid='%u' and logdate='%s'", id, s_month_first_day);
		if(wt_sql_exec(s2_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
			return -1;
		}
			
		s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
	
		if(s2_handle->sql_ret == SQL_NO_DATA){
#if STATISCITS_DEBUG
			xyprintf(0, "shanghu_monthlog no date, shanghuid is %u", id);
#endif
			//如果记录不存在 则统计数据 生成记录
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			unsigned int zongnum = 0, shownum = 0, clicknum = 0, showoknum = 0, clickoknum = 0;
			SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&zongnum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 2,	SQL_C_ULONG,		&shownum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 3,	SQL_C_ULONG,		&clicknum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 4,	SQL_C_ULONG,		&showoknum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 5,	SQL_C_ULONG,		&clickoknum,	32, &s2_handle->sql_err);
			
			sprintf(s2_handle->sql_str, "SELECT SUM(zongnum) as zongnum, SUM(adshownum) as adshownum, SUM(adclicknum) as adclicknum, \
					SUM(adshowoknum) as adshowoknum, SUM(adclickoknum) as adclickoknum FROM mx_ad_shanghu_daylog \
					WHERE shanghuid=%u and logdate>='%s' and logdate<='%s'", id, s_month_first_day, s_yesterday);
			if(wt_sql_exec(s2_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
				return -1;
			}
			
			s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);

			sprintf(iud_handle->sql_str, "INSERT into mx_ad_shanghu_monthlog(shanghuid,logdate,zongnum,adshownum,adclicknum,adshowoknum,adclickoknum,modidate) \
					values('%u','%s','%u','%u','%u','%u','%u','%s')",
				    id, s_month_first_day, zongnum, shownum, clicknum, showoknum, clickoknum, s_yesterday);
			if(wt_sql_exec(iud_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
				return -1;
			}
			
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
		
		}
		else if(strcmp(modidate, s_yesterday)){
#if STATISCITS_DEBUG
			xyprintf(0, "shanghu_monthlog has date, shanghuid is %u", id);
#endif
			// 如果记录存在 并且最后更新日期不是今天,则UPDATE
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
				
			unsigned int zongnum = 0, shownum = 0, clicknum = 0, showoknum = 0, clickoknum = 0;
			SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&zongnum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 2,	SQL_C_ULONG,		&shownum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 3,	SQL_C_ULONG,		&clicknum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 4,	SQL_C_ULONG,		&showoknum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 5,	SQL_C_ULONG,		&clickoknum,	32, &s2_handle->sql_err);
			
			sprintf(s2_handle->sql_str, "SELECT SUM(zongnum) as zongnum, SUM(adshownum) as adshownum, SUM(adclicknum) as adclicknum,\
					SUM(adshowoknum) as adshowoknum, SUM(adclickoknum) as adclickoknum FROM mx_ad_shanghu_daylog \
					WHERE shanghuid='%u' and logdate<='%s' and logdate>'%s'", id, s_yesterday, modidate);
			if(wt_sql_exec(s2_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
				return -1;
			}
				
			s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
	
			sprintf(iud_handle->sql_str, "UPDATE mx_ad_shanghu_monthlog set zongnum = zongnum + %u,adshownum = adshownum + %u,adclicknum = adclicknum + %u,\
					adshowoknum = adshowoknum + %u,adclickoknum = adclickoknum + %u, modidate='%s' WHERE id='%u'",
				    zongnum, shownum, clicknum, showoknum, clickoknum, s_yesterday, logid);
			if(wt_sql_exec(iud_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
				return -1;
			}
				
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			
		}

		//获取下一条记录
		s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);	
	}// end while( s1_handle->sql_ret != SQL_NO_DATA )
	
	return 0;
}

// plan yesterday展示和点击次数统计
int plan_yesterday_statiscits(wt_sql_handle *s1_handle, wt_sql_handle *s2_handle, wt_sql_handle *iud_handle, char* s_yesterday)
{
	unsigned int id = 0;	
	SQLBindCol(s1_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&id,		32, &s1_handle->sql_err);
	sprintf(s1_handle->sql_str, "SELECT id FROM mx_ad_plan WHERE state = 1 and \
			startdate<='%s 00:00:00' and enddate>='%s 23:59:59'", s_yesterday, s_yesterday);
	if(wt_sql_exec(s1_handle)){
		xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s1_handle->sql_str);
		return -1;
	}

	//获取第一条记录
	s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);
	while( s1_handle->sql_ret != SQL_NO_DATA ){		//如果有数据就一直执行下去
		
		// 查看log是否存在
		unsigned int temp = 0;
		SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&temp,	32, &s2_handle->sql_err);
		sprintf(s2_handle->sql_str, "SELECT top 1 id FROM mx_ad_plan_daylog WHERE planid = %u and logdate='%s'", id, s_yesterday);
		if(wt_sql_exec(s2_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
			return -1;
		}
			
		s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
			
		if(s2_handle->sql_ret == SQL_NO_DATA){
#if STATISCITS_DEBUG
			xyprintf(0, "plan_daylog no date, planid is %u", id);
#endif
			//如果log不存在 则统计数据 生成log
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			// mac_num
			unsigned int isclick = 0, isclick_count = 0;
			SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&isclick,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 2,	SQL_C_ULONG,		&isclick_count,	32, &s2_handle->sql_err);
			sprintf(s2_handle->sql_str, "SELECT isclick,count(isclick) as num FROM mx_view_ad_show_detail1 WHERE planid = %u and\
					createdate>='%s 00:00:00' and createdate<='%s 23:59:59' group by isclick", id, s_yesterday, s_yesterday);
			if(wt_sql_exec(s2_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
				return -1;
			}
				
			int show_ok_num = 0, show_1 = 0;
			s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
			while(s2_handle->sql_ret != SQL_NO_DATA){
				if(isclick == 0){
					show_ok_num = isclick_count;
				}
				if(isclick == 1){
					show_1 = isclick_count;
				}
				s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
			}

			sprintf(s2_handle->sql_str, "SELECT isclick,count(isclick) as num FROM mx_view_ad_payfor_detail1 WHERE planid = %u and \
					createdate>='%s 00:00:00' and createdate<='%s 23:59:59' group by isclick", id, s_yesterday, s_yesterday);
			if(wt_sql_exec(s2_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
				return -1;
			}
				
			int click_0 = 0, click_ok_num = 0;
			s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
			while(s2_handle->sql_ret != SQL_NO_DATA){
				if(isclick == 0){
					click_0 = isclick_count;
				}
				if(isclick == 1){
					click_ok_num = isclick_count;
				}
				s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
			}

			int show_num  = show_ok_num + show_1;
			int click_num = click_ok_num + click_0;
					
			sprintf(iud_handle->sql_str, "INSERT into mx_ad_plan_daylog(planid,logdate,zongnum,adshownum,adclicknum,adshowoknum,adclickoknum) \
					values(%u, '%s', '%d', '%d', '%d', '%d', '%d')",
				    id, s_yesterday, show_num + click_num, show_num, click_num, show_ok_num, click_ok_num);
			if(wt_sql_exec(iud_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
				return -1;
			}
				
		}
		else {
#if STATISCITS_DEBUG
			xyprintf(0, "plan_daylog has date, planid is %u", id);
#endif
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
		}
				
		//获取下一条记录
		s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);	
	}// end while( s1_handle->sql_ret != SQL_NO_DATA )
	
	return 0;
}

// plan 月展示和点击次数统计
int plan_month_statiscits(wt_sql_handle *s1_handle, wt_sql_handle *s2_handle, wt_sql_handle *iud_handle, char* s_yesterday, char* s_month_first_day)
{
	unsigned int id = 0;
	SQLBindCol(s1_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&id,		32, &s1_handle->sql_err);
	sprintf(s1_handle->sql_str, "SELECT id FROM mx_ad_plan WHERE state = 1 \
			and startdate<='%s 00:00:00' and enddate>='%s 23:59:59'", s_yesterday, s_yesterday);
	if(wt_sql_exec(s1_handle)){
		xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s1_handle->sql_str);
		return -1;
	}

	//获取第一条记录
	s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);
	while( s1_handle->sql_ret != SQL_NO_DATA ){		//如果有数据就一直执行下去

		unsigned int logid = 0;
		char modidate[16];
		SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&logid,		16, &s2_handle->sql_err);
		SQLBindCol(s2_handle->sqlstr_handle, 2,	SQL_C_CHAR,			&modidate,	16, &s2_handle->sql_err);
		sprintf(s2_handle->sql_str, "SELECT top 1 id, modidate FROM mx_ad_plan_monthlog WHERE planid='%u' and logdate='%s'", id, s_month_first_day);
		if(wt_sql_exec(s2_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
			return -1;
		}
			
		s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
	
		if(s2_handle->sql_ret == SQL_NO_DATA){
#if STATISCITS_DEBUG
			xyprintf(0, "plan_monthlog no date, planid is %u", id);
#endif
			//如果记录不存在 则统计数据 生成记录
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			unsigned int zongnum = 0, shownum = 0, clicknum = 0, showoknum = 0, clickoknum = 0;
			SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&zongnum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 2,	SQL_C_ULONG,		&shownum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 3,	SQL_C_ULONG,		&clicknum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 4,	SQL_C_ULONG,		&showoknum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 5,	SQL_C_ULONG,		&clickoknum,	32, &s2_handle->sql_err);
			
			sprintf(s2_handle->sql_str, "SELECT SUM(zongnum) as zongnum, SUM(adshownum) as adshownum, SUM(adclicknum) as adclicknum, \
					SUM(adshowoknum) as adshowoknum, SUM(adclickoknum) as adclickoknum FROM mx_ad_plan_daylog \
					WHERE planid=%u and logdate>='%s' and logdate<='%s'", id, s_month_first_day, s_yesterday);
			if(wt_sql_exec(s2_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
				return -1;
			}
				
			s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
	
			sprintf(iud_handle->sql_str, "INSERT into mx_ad_plan_monthlog(planid,logdate,zongnum,adshownum,adclicknum,adshowoknum,adclickoknum,modidate) \
					values('%u','%s','%u','%u','%u','%u','%u','%s')",
				    id, s_month_first_day, zongnum, shownum, clicknum, showoknum, clickoknum, s_yesterday);
			if(wt_sql_exec(iud_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
				return -1;
			}
				
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			
		}
		else if(strcmp(modidate, s_yesterday)){
#if STATISCITS_DEBUG
			xyprintf(0, "plan_monthlog has date, planid is %u", id);
#endif
			// 如果记录存在 并且最后更新日期不是今天,则UPDATE
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			
			unsigned int zongnum = 0, shownum = 0, clicknum = 0, showoknum = 0, clickoknum = 0;
			SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&zongnum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 2,	SQL_C_ULONG,		&shownum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 3,	SQL_C_ULONG,		&clicknum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 4,	SQL_C_ULONG,		&showoknum,		32, &s2_handle->sql_err);
			SQLBindCol(s2_handle->sqlstr_handle, 5,	SQL_C_ULONG,		&clickoknum,	32, &s2_handle->sql_err);
			
			sprintf(s2_handle->sql_str, "SELECT SUM(zongnum) as zongnum, SUM(adshownum) as adshownum, SUM(adclicknum) as adclicknum,\
					SUM(adshowoknum) as adshowoknum, SUM(adclickoknum) as adclickoknum FROM mx_ad_plan_daylog \
					WHERE planid='%u' and logdate<='%s' and logdate>'%s'", id, s_yesterday, modidate);
			if(wt_sql_exec(s2_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
				return -1;
			}
				
			s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
	
			sprintf(iud_handle->sql_str, "UPDATE mx_ad_plan_monthlog set zongnum = zongnum + %u,adshownum = adshownum + %u,adclicknum = adclicknum + %u,\
					adshowoknum = adshowoknum + %u,adclickoknum = adclickoknum + %u, modidate='%s' WHERE id='%u'",
				    zongnum, shownum, clicknum, showoknum, clickoknum, s_yesterday, logid);
			if(wt_sql_exec(iud_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
				return -1;
			}
				
			SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			
		}

		//获取下一条记录
		s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);	
	}// end while( s1_handle->sql_ret != SQL_NO_DATA )

	return 0;
}

// 周统计 仅在周一执行
int week_statiscits(wt_sql_handle *s1_handle, wt_sql_handle *s2_handle, wt_sql_handle *iud_handle, char* s_yesterday)
{
#define MONDAY 1
	unsigned int id = 0;
	time_t now;
	struct tm *now_tm;
	time(&now);
	now_tm = localtime(&now);
	xyprintf(0, "today is %d", now_tm->tm_wday);
	if(now_tm->tm_wday == MONDAY){
		// 组成今天和周第一天的的字符串
		time_t week_frist_day = now - 7 * 24 * 60 * 60;
		struct tm *week_frist_tm = localtime(&week_frist_day);
		char s_week_first_day[16];
		snprintf(s_week_first_day, 16, "%4d-%02d-%02d", week_frist_tm->tm_year + 1900, week_frist_tm->tm_mon + 1, week_frist_tm->tm_mday);
		xyprintf(0, "s_week_first_day is %s", s_week_first_day);


		// 周展示和点击次数统计
		SQLBindCol(s1_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&id,		32, &s1_handle->sql_err);
		sprintf(s1_handle->sql_str, "SELECT id FROM mx_shanghu WHERE shanghutype='2' and \
				isok='1' and startdate<='%s 00:00:00' and enddate>='%s 23:59:59'",
				s_yesterday, s_week_first_day);
		if(wt_sql_exec(s1_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s1_handle->sql_str);
			return -1;
		}

		//获取第一条记录
		s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);
		while( s1_handle->sql_ret != SQL_NO_DATA ){		//如果有数据就一直执行下去
	
			unsigned int temp = 0;
			SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,			&temp,	16, &s2_handle->sql_err);
			sprintf(s2_handle->sql_str, "SELECT top 1 id FROM mx_ad_shanghu_weekdaylog WHERE shanghuid='%u' and logdate='%s'",
					id, s_week_first_day);
			if(wt_sql_exec(s2_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
				return -1;
			}
			
			s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
		
			if(s2_handle->sql_ret == SQL_NO_DATA){
#if STATISCITS_DEBUG
				xyprintf(0, "shanghu_weeklog no date, shanghuid is %u", id);
#endif
				//如果记录不存在 则统计数据 生成记录
				SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
				unsigned int zongnum = 0, shownum = 0, clicknum = 0, showoknum = 0, clickoknum = 0;
				SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&zongnum,		32, &s2_handle->sql_err);
				SQLBindCol(s2_handle->sqlstr_handle, 2,	SQL_C_ULONG,		&shownum,		32, &s2_handle->sql_err);
				SQLBindCol(s2_handle->sqlstr_handle, 3,	SQL_C_ULONG,		&clicknum,		32, &s2_handle->sql_err);
				SQLBindCol(s2_handle->sqlstr_handle, 4,	SQL_C_ULONG,		&showoknum,		32, &s2_handle->sql_err);
				SQLBindCol(s2_handle->sqlstr_handle, 5,	SQL_C_ULONG,		&clickoknum,	32, &s2_handle->sql_err);
				
				sprintf(s2_handle->sql_str, "SELECT SUM(zongnum) as zongnum, SUM(adshownum) as adshownum, SUM(adclicknum) as adclicknum, \
						SUM(adshowoknum) as adshowoknum, SUM(adclickoknum) as adclickoknum FROM mx_ad_shanghu_daylog \
						WHERE shanghuid='%u' and logdate>='%s' and logdate<='%s'", id, s_week_first_day, s_yesterday);
				if(wt_sql_exec(s2_handle)){
					xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
					return -1;
				}
				
				s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
	
				sprintf(iud_handle->sql_str, "INSERT into mx_ad_shanghu_weekdaylog(shanghuid,logdate,zongnum,adshownum,adclicknum,adshowoknum,adclickoknum,orderindex) \
						values('%u','%s','%u','%u','%u','%u','%u','0')",
					    id, s_week_first_day, zongnum, shownum, clicknum, showoknum, clickoknum);
				if(wt_sql_exec(iud_handle)){
					xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
					return -1;
				}
				
				SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			
			}
			else{
#if STATISCITS_DEBUG
				xyprintf(0, "shanghu_weeklog has date, shanghuid is %u", id);
#endif
				// 如果记录存在 并且最后更新日期不是今天,则UPDATE
				SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			}

			//获取下一条记录
			s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);	
		}// end while( s1_handle->sql_ret != SQL_NO_DATA )
		
		
		
		// plan 周点击统计	
		SQLBindCol(s1_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&id,		32, &s1_handle->sql_err);
		sprintf(s1_handle->sql_str, "SELECT id FROM mx_ad_plan WHERE state = 1 and startdate<='%s 00:00:00' and enddate>='%s 23:59:59'",
				s_yesterday, s_week_first_day);
		if(wt_sql_exec(s1_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s1_handle->sql_str);
			return -1;
		}

		//获取第一条记录
		s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);
		while( s1_handle->sql_ret != SQL_NO_DATA ){		//如果有数据就一直执行下去

			unsigned int temp = 0;
			SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,			&temp,	16, &s2_handle->sql_err);
			sprintf(s2_handle->sql_str, "SELECT top 1 id FROM mx_ad_plan_weekdaylog WHERE planid='%u' and logdate='%s'",
					id, s_week_first_day);
			if(wt_sql_exec(s2_handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
				return -1;
			}
			
			s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
		
			if(s2_handle->sql_ret == SQL_NO_DATA){
#if STATISCITS_DEBUG
				xyprintf(0, "plan_weeklog no date, planid is %u", id);
#endif
				//如果记录不存在 则统计数据 生成记录
				SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
				unsigned int zongnum = 0, shownum = 0, clicknum = 0, showoknum = 0, clickoknum = 0;
				SQLBindCol(s2_handle->sqlstr_handle, 1,	SQL_C_ULONG,		&zongnum,		32, &s2_handle->sql_err);
				SQLBindCol(s2_handle->sqlstr_handle, 2,	SQL_C_ULONG,		&shownum,		32, &s2_handle->sql_err);
				SQLBindCol(s2_handle->sqlstr_handle, 3,	SQL_C_ULONG,		&clicknum,		32, &s2_handle->sql_err);
				SQLBindCol(s2_handle->sqlstr_handle, 4,	SQL_C_ULONG,		&showoknum,		32, &s2_handle->sql_err);
				SQLBindCol(s2_handle->sqlstr_handle, 5,	SQL_C_ULONG,		&clickoknum,	32, &s2_handle->sql_err);
				
				sprintf(s2_handle->sql_str, "SELECT SUM(zongnum) as zongnum, SUM(adshownum) as adshownum, SUM(adclicknum) as adclicknum, \
						SUM(adshowoknum) as adshowoknum, SUM(adclickoknum) as adclickoknum FROM mx_ad_plan_daylog \
						WHERE planid='%u' and logdate>='%s' and logdate<='%s'", id, s_week_first_day, s_yesterday);
				if(wt_sql_exec(s2_handle)){
					xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, s2_handle->sql_str);
					return -1;
				}
				
				s2_handle->sql_ret = SQLFetch(s2_handle->sqlstr_handle);
	
				sprintf(iud_handle->sql_str, "INSERT into mx_ad_plan_weekdaylog(planid,logdate,zongnum,adshownum,adclicknum,adshowoknum,adclickoknum,orderindex) \
						values('%u','%s','%u','%u','%u','%u','%u','0')",
					    id, s_week_first_day, zongnum, shownum, clicknum, showoknum, clickoknum);
				if(wt_sql_exec(iud_handle)){
					xyprintf(0, "SQL_ERROR: %s %d -- s_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
					return -1;
				}
				
				SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			
			}
			else{
#if STATISCITS_DEBUG
				xyprintf(0, "plan_weeklog has date, planid is %u", id);
#endif
				// 如果记录存在 并且最后更新日期不是今天,则UPDATE
				SQLFreeStmt(s2_handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			}
			//获取下一条记录
			s1_handle->sql_ret = SQLFetch(s1_handle->sqlstr_handle);	
		}// end while( s1_handle->sql_ret != SQL_NO_DATA )
		
		
	}// end if(now_tm->tm_wday == 1)

	return 0;
}

/** 
 *@brief  数据统计线程
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* STATISTICS_THREAD(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Statistics thread is running!!!");

	//错误标志,如果数据库操作发生错误,置为1,再重新开始统计
	int error_flag;

	//睡眠时间计算 凌晨2点运行
	unsigned int sleep_time = 0;
	time_t now;
	struct tm *now_tm;

	while(1){
		time(&now);
		now_tm = localtime(&now);
		if(now_tm->tm_hour < 2){
			sleep_time = ( 2 - now_tm->tm_hour - 1 ) * 60 * 60 + ( 60 - now_tm->tm_min ) * 60;
		}
		else {
			sleep_time = ( 24 + 2 - now_tm->tm_hour - 1) * 60 * 60 + ( 60 - now_tm->tm_min ) * 60;
		}
		//sleep_time = 10;
		
		xyprintf(0, "** (～﹃～)~zZ ~~ Statistics thread will sleep %u s!!!", sleep_time);
		sleep( sleep_time );
		xyprintf(0, "STATISTICS:** O(∩ _∩ )O ~~ Statistics thread is get up!!!");
		error_flag = 0;

		// 数据库访问资源
		wt_sql_handle *s1_handle = malloc(sizeof(wt_sql_handle));		//SELECT用
		memset(s1_handle, 0, sizeof(wt_sql_handle));
		wt_sql_handle *s2_handle = malloc(sizeof(wt_sql_handle));		//SELECT用
		memset(s2_handle, 0, sizeof(wt_sql_handle));
		wt_sql_handle *iud_handle = malloc(sizeof(wt_sql_handle));		//INSERT UPDATE DELETE用`
		memset(iud_handle, 0, sizeof(wt_sql_handle));
		
		while( wt_sql_init(s1_handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			// 数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
			sleep( 30 );
		}
		
		while( wt_sql_init(s2_handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			// 数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
			sleep( 30 );
		}
		
		while( wt_sql_init(iud_handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){		// 数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
			sleep( 30 );
		}

		// 组成昨天日期的字符串
		time(&now);
		time_t yesterday_now = now - 24 * 60 * 60;
		struct tm *yesterday_tm;
		yesterday_tm = localtime(&yesterday_now);
		char s_yesterday[16];
		snprintf(s_yesterday, 16, "%4d-%02d-%02d", yesterday_tm->tm_year + 1900, yesterday_tm->tm_mon + 1, yesterday_tm->tm_mday);
#if STATISCITS_DEBUG
		xyprintf(0, "s_yesterday is %s", s_yesterday);
#endif
		char s_month_first_day[16];
		snprintf(s_month_first_day, 16, "%4d-%02d-%02d", yesterday_tm->tm_year + 1900, yesterday_tm->tm_mon + 1, 1);
#if STATISCITS_DEBUG
		xyprintf(0, "s_month_first_day is %s", s_month_first_day);
#endif
		
		time_t delete_log_time = now - 30 * 24 * 60 * 60;
		struct tm *delete_log_tm;
		delete_log_tm = localtime(&delete_log_time);
		char s_delete_log_day[16];
		snprintf(s_delete_log_day, 16, "%4d-%02d-%02d", delete_log_tm->tm_year + 1900, delete_log_tm->tm_mon + 1, delete_log_tm->tm_mday);
#if STATISCITS_DEBUG
		xyprintf(0, "s_delete_log_day is %s", s_delete_log_day);
#endif

		pthread_mutex_lock(&gv_task_lock);//⊙﹏⊙
		
		//处理时间测试 -- 开始时间
		struct timeval start, end;
		gettimeofday( &start, NULL );

		if(log_tongji(s1_handle, s2_handle, iud_handle, s_yesterday) ){
			error_flag = 1;
			goto STR_ERR;
		}
		
		if(log_statiscits(s1_handle, s2_handle, iud_handle, s_yesterday, s_delete_log_day) ){
			error_flag = 1;
			goto STR_ERR;
		}
#ifdef ADVERTISING
#else
		// yesterday展示和点击次数统计
		if( show_yesterday_statiscits(s1_handle, s2_handle, iud_handle, s_yesterday) ){
			error_flag = 1;
			goto STR_ERR;
		}

		// 月展示和点击次数统计
		if( show_month_statiscits(s1_handle, s2_handle, iud_handle, s_yesterday, s_month_first_day) ){
			error_flag = 1;
			goto STR_ERR;
		}

		// plan yesterday展示和点击次数统计
		if( plan_yesterday_statiscits(s1_handle, s2_handle, iud_handle, s_yesterday) ){
			error_flag = 1;
			goto STR_ERR;
		}
		
		// plan 月展示和点击次数统计
		if( plan_month_statiscits(s1_handle, s2_handle, iud_handle, s_yesterday, s_month_first_day) ){
			error_flag = 1;
			goto STR_ERR;
		}

		// 周统计 仅在周一执行
		if( week_statiscits(s1_handle, s2_handle, iud_handle, s_yesterday) ){
			error_flag = 1;
			goto STR_ERR;
		}
#endif

//当一轮循环执行完成的时候,销毁数据库连接资源在下次执行的时候重新创建,如果执行出现错误时,提前结束
STR_ERR:
		pthread_mutex_unlock(&gv_task_lock);//⊙﹏⊙

		//处理时间 -- 结束时间
		gettimeofday( &end, NULL );
		
		wt_sql_destroy(s1_handle);
		free(s1_handle);
		wt_sql_destroy(s2_handle);
		free(s2_handle);
		wt_sql_destroy(iud_handle);
		free(iud_handle);
		if(error_flag){
			xyprintf(0, "STATISTICS_ERROR:Has a error, Used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
			sleep_time = 60;
		}
		else {
			xyprintf(0, "STATISTICS:SUCCESS, Used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
		}
	}//while(1);

ERR:
	xyprintf(0, "STATISTICS_ERROR:✟ ✟ ✟ ✟ -- %s %d:Statistics thread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  上报日报表
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* DAILY_REPORT_THREAD(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Daily report thread is running!!!");

	unsigned int sleep_time = 0;
	time_t now;
	struct tm *p;

	while(1){
		time(&now);
		p = localtime(&now);
		sleep_time = ( 24 - p->tm_hour - 1 ) * 60 * 60 + ( 60 - p->tm_min ) * 60;
		//sleep_time = 0;
		sleep_time += 30;

		xyprintf(0, "** (～﹃～)~zZ ~~ Daily report thread will sleep %u s!!!", sleep_time );
		sleep( sleep_time );
		xyprintf(0, "DAILY_REPORT:** O(∩ _∩ )O ~~ Daily report thread is get up!!!");
		pthread_mutex_lock(&gv_task_lock);//⊙﹏⊙
		
		//处理时间测试 -- 开始时间
		struct timeval start, end;
		gettimeofday( &start, NULL );

		while( SEND_DAILY_REPORT() ){
			sleep(300);
		}

STR_ERR:
		pthread_mutex_unlock(&gv_task_lock);//⊙﹏⊙
		
		//处理时间 -- 结束时间
		gettimeofday( &end, NULL );
		xyprintf(0, "DAILY_REPORT:☻ ☺ ☻ ☺ ");
		xyprintf(0, "DAILY_REPORT:☻ ☺ ☻ ☺  Daily report bao ping'an, used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
		xyprintf(0, "DAILY_REPORT:☻ ☺ ☻ ☺ ");
		
	}//while(1);

ERR:
	xyprintf(0, "DAILY_REPORT_ERROR:✟ ✟ ✟ ✟ -- %s %d:Daily report pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

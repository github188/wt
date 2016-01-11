/*****************************************************
 *
 * 探针服务器
 *
 *****************************************************/
#include "sql.h"
#include "list.h"

/********************** probe with router ******************************/
//探针上报信息头
typedef struct wifi_monitor_data_head
{
	unsigned short       msgHeader;		// 报文头，用于识别是否为定位报文, 网络序, 约定(0x2233)
	unsigned short       pad;			// 保留对齐
	unsigned char        version;		// 版本，目前用1
	unsigned char        flag;			// 保留，目前不用
	unsigned short       len;			// 总数据长度, 网络序
	unsigned short       time;			// 间隔时间，（ms）暂时不用, 网络序
	unsigned char        ap_mac[6];		// ap的mac地址
	unsigned int         Timestamp;		// 时间戳 ，暂时不用, 网络序
	unsigned int         num;			// 客户端数据量, 网络序
	unsigned char        data[0];		// 客户端数据, num个struct wifi_client_st
}wifi_monitor_data_head;

//探针上报信息体
typedef struct wifi_monitor_client_st
{
	char		   rssi;			// 信号
	unsigned char  flag;			// 目前没有用
	unsigned char  mac[6];			// 客户的mac
	unsigned char  assoc_mac[6];	// 终端关联上的AP的mac，如非关联，即填0
	unsigned char  channel;			// 信道
	unsigned char  type;			// 终端类型，是MU还是AP或其他Tag
	unsigned int   age;				// 该报文是多久前（毫秒）探测到的, 网终序
}wifi_monitor_client_st;

//配置文件内读出的配置参数
char	cgv_sql_name[32];
char	cgv_sql_user[32];
char	cgv_sql_pass[32];
int		cgv_monitor_port;

//在线程间传送报文的结构体
struct message{
	char *buf;			// 存储地址
	int	  len;			// 长度
};

/** 
 *@brief  处理无线探针上报数据函数
 *@param  arg		类型 void*				接收到的数据
 *@param  handle    类型 wt_sql_handle*		数据库操作资源集合
 *@return 数据库出错返回WT_SQL_ERR 其他返回0
 */
void* process_process(void* msg, wt_sql_handle *handle)
{
	char* buf = ((struct message*)msg)->buf;
	int len = ((struct message*)msg)->len;
	free(msg);
	
	//判断长度是否正确 -- 长度比报文头还要短
	if(len < sizeof(wifi_monitor_data_head)){
		xyprintf(0, "DATA_ERROR : %s %d", __FILE__, __LINE__);
		goto DATA_ERROR;
	}

	//判断长度是否正确 -- 报文的长度不是完整的
	if( (len - sizeof(wifi_monitor_data_head)) % sizeof(wifi_monitor_client_st) != 0 ){
		xyprintf(0, "DATA_ERROR : %s %d", __FILE__, __LINE__);
		goto DATA_ERROR;
	}

	//转换字节序
	wifi_monitor_data_head *head = (wifi_monitor_data_head*)buf;
	big_little16(&head->msgHeader);
	big_little16(&head->len);
	big_little16(&head->time);
	//big_little32(&head->Timestamp);
	big_little32(&head->num);

	//判断具体数据 -- 第一个字节 和 版本号
	if(head->msgHeader != 0x2233 || head->version != 1){
		xyprintf(0, "DATA_ERROR : %s %d", __FILE__, __LINE__);
		goto DATA_ERROR;
	}
	//包内的长度是否和实际长度相同
	if( head->len != len ){
		xyprintf(0, "DATA_ERROR : %s %d", __FILE__, __LINE__);
		goto DATA_ERROR;
	}
	//包的长度是否正确
	if( head->num * sizeof(wifi_monitor_client_st) + sizeof(wifi_monitor_data_head) != len ){
		xyprintf(0, "DATA_ERROR : %s %d", __FILE__, __LINE__);
		goto DATA_ERROR;
	}

	//查询探针ap的信息，根据探针ap的mac apid，代理id，商户id
	__u32 apid, dailiid, shanghuid;
	SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG,	&apid,		20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 2, SQL_C_ULONG,	&dailiid,	20, &handle->sql_err);
	SQLBindCol(handle->sqlstr_handle, 3, SQL_C_ULONG,	&shanghuid,	20, &handle->sql_err);
	//查询ap的信息
	sprintf(handle->sql_str, "SELECT TOP 1 id, dailiid, shanghuid FROM mx_tz_ap WHERE apmac = '%02x%02x%02x%02x%02x%02x'",
			head->ap_mac[0], head->ap_mac[1], head->ap_mac[2], head->ap_mac[3], head->ap_mac[4], head->ap_mac[5] );
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
		goto SQL_ERR;
	}
	handle->sql_ret = SQLFetch(handle->sqlstr_handle);
	if(handle->sql_ret == SQL_NO_DATA){	//如果没有探针ap的数据，则认为上传的数据错误
		xyprintf(0, "DATA_ERROR : %s %d -- ap_mac is %02x%02x%02x%02x%02x%02x", __FILE__, __LINE__,
				head->ap_mac[0], head->ap_mac[1], head->ap_mac[2], head->ap_mac[3], head->ap_mac[4], head->ap_mac[5]);
		goto DATA_ERROR;
	}
	SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

	//更新在线状态和最后活动时间
	sprintf(handle->sql_str, "UPDATE mx_tz_ap SET isonline = 1, lasttime = getdate() WHERE id = %u", apid);
	if( wt_sql_exec(handle) ){
		xyprintf(0, "SQL_ERR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
		goto SQL_ERR;
	}
	
	//打印下
	//	xyprintf(0, "%02x%02x%02x%02x%02x%02x : <-- num is %u <--", head->ap_mac[0], head->ap_mac[1],
	//		head->ap_mac[2], head->ap_mac[3], head->ap_mac[4], head->ap_mac[5], head->num);

	//临时变量 指向探针ap探测到的移动设备信息
	wifi_monitor_client_st *client = (wifi_monitor_client_st*)(buf + sizeof(wifi_monitor_data_head));

	//依次处理这些设备信息
	int i;
	__u32 tzid, isonline, logid;
	for(i = 0; i < head->num; i++){
		//打印下
		//xyprintf(0, "%02d : %02x%02x%02x%02x%02x%02x",
		//		i + 1, client[i].mac[0], client[i].mac[1], client[i].mac[2], client[i].mac[3], client[i].mac[4], client[i].mac[5]);
		
		//绑定变量和查询语句
		SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG,	&tzid,		20, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 2, SQL_C_ULONG,	&isonline,	20, &handle->sql_err);
		//查询shanghuid和tzmac对应的记录是否存在，以及在线状态
		sprintf(handle->sql_str, "SELECT TOP 1 id, isonline FROM mx_tz_mac WHERE shanghuid = %u AND tzmac = '%02x%02x%02x%02x%02x%02x'",
				shanghuid, client[i].mac[0], client[i].mac[1], client[i].mac[2], client[i].mac[3], client[i].mac[4], client[i].mac[5]);
		if( wt_sql_exec(handle) ){
			xyprintf(0, "SQL_ERR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
			goto SQL_ERR;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
		
		if(handle->sql_ret == SQL_NO_DATA){		//不存在的话插入新数据
			//添加 tz mac
			sprintf(handle->sql_str, "INSERT INTO mx_tz_mac (dailiid, shanghuid, tzmac, lognum, isonline, lasttime, createdate) VALUES \
					( %u, %u, '%02x%02x%02x%02x%02x%02x', 1, 1, getdate() , getdate() )" , dailiid, shanghuid,
					client[i].mac[0], client[i].mac[1], client[i].mac[2],
					client[i].mac[3], client[i].mac[4], client[i].mac[5]);
			if( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
				goto SQL_ERR;
			}
			
			//获取id
			sprintf(handle->sql_str, "SELECT SCOPE_IDENTITY()");
			if( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
				goto SQL_ERR;
			}
			handle->sql_ret		= SQLFetch(handle->sqlstr_handle);
			SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
			
			//添加log
			sprintf(handle->sql_str, "INSERT INTO mx_tz_mac_log_last (dailiid, shanghuid, tzapid, macid, zongtime, zongnum, \
				lasttime, createdate, islast) VALUES ( %u, %u, %u, %u, 0, 1, getdate(), getdate(), 1)",
				dailiid, shanghuid, apid, tzid);
			if( wt_sql_exec(handle) ){
				xyprintf(0, "SQL_ERR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
				goto SQL_ERR;
			}
		}
		else {		//数据存在的话 更新
			if(isonline){
				//如果在线就只更新mac的最后活动时间
				sprintf(handle->sql_str, "UPDATE mx_tz_mac SET lasttime = getdate() WHERE id = %u", tzid );
				if( wt_sql_exec(handle) ){
					xyprintf(0, "SQL_ERR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
					goto SQL_ERR;
				}
				//查找是否有对应这个设备和mac的log 因为一个商户可能会有两台设备
				SQLBindCol(handle->sqlstr_handle, 1, SQL_C_ULONG,	&logid,		20, &handle->sql_err);
				sprintf(handle->sql_str, "SELECT id from mx_tz_mac_log_last WHERE macid = %u AND tzapid = %u AND islast = 1", tzid, apid);
				if( wt_sql_exec(handle) ){
					xyprintf(0, "SQL_ERR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
					goto SQL_ERR;
				}
				handle->sql_ret = SQLFetch(handle->sqlstr_handle);
				SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
				
				if(handle->sql_ret == SQL_NO_DATA){		//不存在的话添加log
					sprintf(handle->sql_str, "INSERT INTO mx_tz_mac_log_last (dailiid, shanghuid, tzapid, macid, zongtime, zongnum, \
						lasttime, createdate, islast) VALUES ( %u, %u, %u, %u, 0, 1, getdate(), getdate(), 1)",
						dailiid, shanghuid, apid, tzid);
					if( wt_sql_exec(handle) ){
						xyprintf(0, "SQL_ERR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
						goto SQL_ERR;
					}
				}
				else {		//存在的话 修改log
					sprintf(handle->sql_str, "UPDATE mx_tz_mac_log_last SET zongtime = DATEDIFF(n, createdate, getdate()), \
							zongnum = zongnum + 1, lasttime = getdate() WHERE id = %u", logid);
					if( wt_sql_exec(handle) ){
						xyprintf(0, "SQL_ERR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
						goto SQL_ERR;
					}
				}
			}
			else {
				//不在线，更新登陆次数、在线状态、最后活动时间
				sprintf(handle->sql_str, "UPDATE mx_tz_mac SET lognum = lognum + 1, isonline = 1, lasttime = getdate() WHERE id = %u", tzid );
				if( wt_sql_exec(handle) ){
					xyprintf(0, "SQL_ERR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
					goto SQL_ERR;
				}
				
				//添加log
				sprintf(handle->sql_str, "INSERT INTO mx_tz_mac_log_last (dailiid, shanghuid, tzapid, macid, zongtime, zongnum, \
					lasttime, createdate, islast) VALUES ( %u, %u, %u, %u, 0, 1, getdate(), getdate(), 1)",
					dailiid, shanghuid, apid, tzid);
				if( wt_sql_exec(handle) ){
					xyprintf(0, "SQL_ERR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
					goto SQL_ERR;
				}
			}
		}
	}
	free(buf);
	return (void*)0;
SQL_ERR:
	free(buf);
	return (void*)WT_SQL_ERROR;
DATA_ERROR:
	free(buf);
	return (void*)0;
}

/** 
 *@brief  无线探针上报信息接收线程
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* monitor_data_thread(void* fd)
{
	pthread_detach( pthread_self() );
	xyprintf(0, "** O(∩ _∩ )O ~~ Monitor data recv & process thread is running!!!");
	
	int sockfd;
	int ret;
	struct sockaddr_in addr;
	char* buf;
	struct message* msg;
	
	while(1){
		//socket初始化
		if (( sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
			xyprintf(errno, "SOCK_ERROR:%s %d -- socket()", __FILE__, __LINE__);
			sleep(10);
			continue;
		}

		//变量准备
		memset(&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family			= AF_INET;
		addr.sin_port			= htons( cgv_monitor_port );
		addr.sin_addr.s_addr	= htonl( INADDR_ANY );

		//绑定端口
		if( bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1 ){
			xyprintf(errno, "SOCK_ERROR:%d %s %d -- connect()", sockfd, __FILE__, __LINE__);
			sleep(3);
			goto SOCK_ERR;
		}
		
		xyprintf(0, "** O(∩ _∩ )O ~~ Socket Ready!!! UDP port is %d!", cgv_monitor_port);

		//循环接收上报信息
		while(1){
			buf = malloc(1500);		//设备每次上传的数据不会超过1500字节
			memset(buf, 0, 1500);
			int ret;
			ret =recvfrom(sockfd, buf, 1500, 0, NULL, NULL);
			if(ret == -1){
				xyprintf(errno, "SOCK_ERROR:%s %d", __FILE__, __LINE__);
				free(buf);
				goto SOCK_ERR;
			}

			//每次接收都申请1500的报文空间 和一个struct message的空间存放报文指针，这些malloc的内存都在报文处理函数中被释放
			msg = malloc( sizeof(struct message) );
			msg->buf = buf;
			msg->len = ret;
			pool_add_worker(process_process, msg);
		}

SOCK_ERR:
		close( sockfd );		//出现错误关闭sockfd 并重新连接
	} //end while(1)
ERR:
	free(buf);
	xyprintf(0, "✟ ✟ ✟ ✟ -- %s %d:Status report pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit( NULL );
}

/** 
 *@brief  定时更新数据库 isonline islast
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* tfree_thread(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Tfreemanyou thread is running!!!");
	
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));
	memset(handle, 0, sizeof(wt_sql_handle));
	
	while(1){
		xyprintf(0, "** (～﹃～)~zZ ~~ Tfree thread will sleep %u s!!!", 5 * 60 );
		sleep( 5 * 60 );
		xyprintf(0, "TFREE:** O(∩ _∩ )O ~~ Tfree thread is get up!!!");
		
		if( wt_sql_init(handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){			//数据库初始化
			xyprintf(0, "SQL_ERROR: %s %d -- datebase connect error!", __FILE__, __LINE__);
			sleep( 5 );
			continue;
		}

		//读取tfree数值
		__u32 tfreetime;
		SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_ULONG,	&tfreetime,				32, &handle->sql_err);
		sprintf(handle->sql_str, "SELECT TOP 1 tzfree FROM mx_tz_config");
		if(wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
			goto STR_ERR;
		}
		handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条	
		if( handle->sql_ret == SQL_NO_DATA){
			break;
		}
		SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标
		
		//更新ap表isonline标志
		sprintf(handle->sql_str, "UPDATE mx_tz_ap SET isonline = 0 WHERE isonline = 1 AND DATEDIFF(n, lasttime, getdate()) > %u", tfreetime);
		if(wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
			goto STR_ERR;
		}
			
		//更新tzmac表isonline标志
		sprintf(handle->sql_str, "UPDATE mx_tz_mac SET isonline = 0 WHERE isonline = 1 AND DATEDIFF(n, lasttime, getdate()) > %u", tfreetime);
		if(wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
			goto STR_ERR;
		}
			
		//更新log_last表islast标志
		sprintf(handle->sql_str, "UPDATE mx_tz_mac_log_last SET islast = 0 WHERE DATEDIFF(n, lasttime, getdate()) > %u", tfreetime);
		if(wt_sql_exec(handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
			goto STR_ERR;
		}
			
		__u32 id, dailiid, shanghuid, tzapid, macid, zongtime, zongnum;
		char lasttime[32] = {0};
		char createdate[32] = {0};

		SQLBindCol(handle->sqlstr_handle, 1,	SQL_C_ULONG,	&id,				32, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 2,	SQL_C_ULONG,	&dailiid,			32, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 3,	SQL_C_ULONG,	&shanghuid,			32, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 4,	SQL_C_ULONG,	&tzapid,			32, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 5,	SQL_C_ULONG,	&macid,				32, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 6,	SQL_C_ULONG,	&zongtime,			32, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 7,	SQL_C_ULONG,	&zongnum,			32, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 8,	SQL_C_CHAR,		&lasttime,			32, &handle->sql_err);
		SQLBindCol(handle->sqlstr_handle, 9,	SQL_C_CHAR,		&createdate,		32, &handle->sql_err);
			
		while(1){
			//将mx_tz_mac_log_last islast=0 的记录移动到mx_tz_mac_log
			sprintf(handle->sql_str, "SELECT TOP 1 id, dailiid, shanghuid, tzapid, macid, zongtime, zongnum, lasttime, createdate \
					FROM mx_tz_mac_log_last	WHERE islast = 0");
			
			if(wt_sql_exec(handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
				goto STR_ERR;
			}
			handle->sql_ret = SQLFetch(handle->sqlstr_handle);//获取查询到的结果 可以用来获取第一条 也可以用来获取下一条	
			if( handle->sql_ret == SQL_NO_DATA){
				break;
			}
			SQLFreeStmt(handle->sqlstr_handle, SQL_CLOSE);  //释放游标

			//添加记录到mx_tz_mac_log
			sprintf(handle->sql_str, "INSERT INTO mx_tz_mac_log (dailiid, shanghuid, tzapid, macid, zongtime, zongnum, lasttime, createdate) \
				VALUES (%u, %u, %u, %u, %u, %u, '%s', '%s')",
				dailiid, shanghuid, tzapid, macid, zongtime, zongnum, lasttime, createdate);
			
			if(wt_sql_exec(handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
				goto STR_ERR;
			}
			//删除mx_tz_mac_log_last的记录
			sprintf(handle->sql_str, "DELETE FROM mx_tz_mac_log_last WHERE id = %d", id );
			if(wt_sql_exec(handle)){
				xyprintf(0, "SQL_ERROR: %s %d -- handle->sql_str: %s", __FILE__, __LINE__, handle->sql_str);
				goto STR_ERR;
			}
				
			xyprintf(0, "TFREE:☻ ☺ ☻ ☺  Successful transfer of a mac record!!!");
			//xyprintf(0, "id = %d, macid = %d, shanghuid = %d, rid = %d, zongtime = %d, mactype = %d, lastdotime = %s, lastsetdoingtime = %s, createdate = %s",
			//		id, macid, shanghuid, rid, zongtime, mactype, lastdotime, lastsetdoingtime, createdate);
		}// end while(1)
STR_ERR:
		wt_sql_destroy(handle);
	}//while(1);

ERR:
	wt_sql_destroy(handle);
	free(handle);
	xyprintf(0, "TFREE:✟ ✟ ✟ ✟ -- %s %d:Tfree pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
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
	if( get_ini(buf, "probe_sql_name", cgv_sql_name) ){
		return -1;
	}
	if( get_ini(buf, "sql_user", cgv_sql_user) ){
		return -1;
	}
	if( get_ini(buf, "sql_pass", cgv_sql_pass) ){
		return -1;
	}
	char temp[16];
	if( get_ini(buf, "monitor_port", temp) ){
		return -1;
	}
	cgv_monitor_port = atoi( temp );
	destroy_ini( fd );

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
			****                     The Probe is running!                       ****\n\
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

	//打印配置文件中读取的配置
	xyprintf(0, "Get config success!!\n\tsql_name = %s\n\tsql_user = %s\n\tsql_pass = %s\n\tmonitor_port = %d",
			cgv_sql_name, cgv_sql_user, cgv_sql_pass, cgv_monitor_port);

	//信号屏蔽
	signal(SIGPIPE, SIG_IGN);
	
	//启动线程
	/*************************thread pool run***********************************/
	pool_init(1, 1, 1, cgv_sql_name, cgv_sql_user, cgv_sql_pass);
	sleep(1);
	/*************************monitor_data_thread*******************************/
	pthread_t monitor_data_pt;
	if( pthread_create(&monitor_data_pt, NULL, monitor_data_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/*************************tfree_thread*************************************/
	pthread_t tfree_pt;
	if( pthread_create(&tfree_pt, NULL, tfree_thread, NULL) != 0 ){
		xyprintf(errno, "PTHREAD_ERROR: %s %d -- pthread_create()", __FILE__, __LINE__);
	}
	/**************************************************************************/
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

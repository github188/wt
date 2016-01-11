/*****************************************************
 *
 * log记录写入
 *
 *****************************************************/
#include "header.h"

#if WRITE_LOG
int				logs_fd		= 0;	// 文件描述符
pthread_mutex_t logs_lock;			// log写入全局互斥锁
char			log_name[32];		// log文件前缀名
int				log_day;			// 当前log文件对应的日期-天
#endif

/**
 *@brief logs文件打开/创建
 *@return success 0 failed -1
 */
int inline logs_create()
{
	int strlen,ret;
	// 组成log文件名 格式是"前缀名_年_月_日.log"
	time_t tm = time(0);
	struct tm *mytime = localtime(&tm);
	char buf[256] = { 0 };
	sprintf(buf,"%s_%d-%02d-%02d.log", log_name,
			mytime->tm_year + 1900, mytime->tm_mon + 1, mytime->tm_mday);
	// 记录当前打开log文件的日期
	log_day = mytime->tm_mday;

	// 打开log文件
	logs_fd = open( buf , O_RDWR | O_CREAT | O_APPEND ,0666);
	if(logs_fd <= 0){
		printf("\n\n\n\n\t\t\tWARNING:OPEN LOG FILE ERROR -- %s!!!\n\n\n\n\n", strerror(errno) );
		return -1;
	}
	return 0;
}

/** 
 *@brief  log初始化函数
 *@param  prefix	类型 char*	log文件前缀
 *@return success 0 failed -1
 */
int logs_init(char* prefix)
{
#if WRITE_LOG
	pthread_mutex_init(&logs_lock,0);	// 初始化全局log写入互斥锁
	strcpy(log_name, prefix);			// 拷贝log前缀名
	return logs_create();				// 调用log文件打开/创建函数
#else
	return 0;
#endif
}

/** 
 *@brief  log反初始化函数 不会被调用到的一个函数
 *@return 成功0 失败-1
 */
void logs_destroy()
{
#if WRITE_LOG
	pthread_mutex_destroy(&logs_lock);
	if(logs_fd){
		close(logs_fd);
		logs_fd = 0;
	}
#endif
}

static char logs_temp[2048];		// log内容临时存放
static char logs_buf[2048];			// log内容
static int logs_strlen,logs_ret;	//
static time_t logs_tm;				// log时间
static struct tm *logs_mytime;		// log时间

/** 
 *@brief  错误log写入函数 
 *@return 无意义
 */
int xyprintf(int err_no, char* format, ...)
{
	//加互斥锁
	pthread_mutex_lock(&logs_lock);
	//将传入的参数组合成字符串
	va_list ap;
	va_start(ap, format);
	vsprintf(logs_temp, format, ap);
	va_end(ap);
	
	//获得当前时间 并组装log字符串(时间 设备ip 是否需要错误描述)
	logs_tm = time(0);
	logs_mytime = localtime(&logs_tm);
	if(err_no){
		logs_strlen = sprintf(logs_buf,"%d-%02d-%02d %02d:%02d:%02d -- %s\n\tERROR MESSAGE: %s\n",
					logs_mytime->tm_year + 1900, logs_mytime->tm_mon + 1, logs_mytime->tm_mday,
					logs_mytime->tm_hour, logs_mytime->tm_min, logs_mytime->tm_sec, logs_temp, strerror(err_no));
	}
	else {
		logs_strlen = sprintf(logs_buf,"%d-%02d-%02d %02d:%02d:%02d -- %s\n",
					logs_mytime->tm_year + 1900, logs_mytime->tm_mon + 1, logs_mytime->tm_mday,
					logs_mytime->tm_hour, logs_mytime->tm_min, logs_mytime->tm_sec, logs_temp);
	}

	printf("%s",logs_buf);	//在屏幕上打印log
	
#if WRITE_LOG
	// 判断日期是否改变
	if( log_day != logs_mytime->tm_mday ){
		// 关闭原来的log文件
		if(logs_fd){
			close(logs_fd);
			logs_fd = 0;
		}
		// 新建log文件, 如果创建失败 则丢掉这个log
		if( logs_create() ){
			pthread_mutex_unlock(&logs_lock);
			return -1;
		}
		// 更换当前log日期
		log_day = logs_mytime->tm_mday;
	}

	// 写入log
	logs_ret = write(logs_fd, logs_buf, logs_strlen);
	if( logs_ret < logs_strlen ){
		printf("%d-%d-%d %d:%d:%d:", logs_mytime->tm_year + 1900, logs_mytime->tm_mon + 1, logs_mytime->tm_mday,
					logs_mytime->tm_hour, logs_mytime->tm_min, logs_mytime->tm_sec);
		printf("\n\n\n\n\t\tWARNING:WRITE LOG FAILED -- %s !!!!\n\n\n\n", strerror(errno) );
		pthread_mutex_unlock(&logs_lock);
		return -1;
	}
	
#endif
	pthread_mutex_unlock(&logs_lock);
	return 0;
}

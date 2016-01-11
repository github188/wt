/*****************************************************
 *
 * 线程池操作函数
 *
 *****************************************************/
#include "header.h"
#include "sql.h"

/*
*线程池里所有运行和等待的任务都是一个CThread_worker
*由于所有任务都在链表里，所以是一个链表结构
*/
typedef struct worker
{
    void			*(*process) (void* arg, wt_sql_handle *hendle);	// 回调函数，任务运行时会调用此函数，注意也可声明成其它形式
    void			*arg;						// 回调函数的参数
    struct worker	*next;
} CThread_worker;

/*线程池结构*/
typedef struct
{
    pthread_mutex_t	queue_lock;				// 线程互斥锁
    pthread_cond_t	queue_ready;			// 条件变量
	CThread_worker	*queue_head;			// 链表结构，线程池中所有等待任务
	int				shutdown;				// 是否销毁线程池
	//pthread_t		*threadid;				// 线程id
	int				thread_num;				// 当前运行的线程池数量
	int				thread_min_sn;			// 线程池最小线程编号
	int				thread_adjust_num;		// 线程调整数量
	int				thread_max_num;			// 线程最大数量
	int				thread_min_num;			// 线程最小数量
	int				thread_sleep_num;		// 在睡觉的线程数量
	unsigned int	cur_queue_size;			// 等待队列中的任务数目
	int				thread_real_num;		// 实际线程数量
	char			sql_name[32];			// 数据库名
	char			sql_user[32];			// 数据库登陆用户名
	char			sql_pass[32];			// 数据库登陆密码
} CThread_pool;

CThread_pool *pool = NULL;			//线程池对象？

/** 
 *@brief  线程执行函数 线程执行的主体函数 这里有这一个线程的一生 它生在这里 在这里执行任务 和 睡觉 吃饭 ,,,, 死亡...
 *@param  arg		类型 void*	线程函数必须，无意义
 *@return 无意义
 */
void * thread_routine (void *arg)
{
	int thread_sn = (long)arg;	// 线程编号

	// 打印一下 告诉程序 我执行正常
	xyprintf(0, "THREAD_ADJUST:** O(∩ _∩ )O ~~ Starting thread success, sn is %04d, id is %lu!", thread_sn, pthread_self());

	// 数据库连接资源空间申请
	wt_sql_handle *handle = malloc(sizeof(wt_sql_handle));

	// 第一层while循环 如果sql出现错误 会被循环执行
	while(1){
		memset(handle, 0, sizeof(wt_sql_handle));
		//连接数据库
		if( wt_sql_init(handle, pool->sql_name, pool->sql_user, pool->sql_pass) ){			//数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error! thread id is %lu!", __func__, __FILE__, __LINE__, pthread_self());
			sleep(5);
			continue;
		}
		
		// 第二层while循环,在sql出现错误的时候会跳出
		while(1){
			// 先申请拿锁
			pthread_mutex_lock (&(pool->queue_lock));
			
			// 如果等待队列为0，则处于阻塞状态; 注意pthread_cond_wait是一个原子操作，等待前会解锁，唤醒后会加锁
			while(pool->cur_queue_size == 0){

				// 判断线程池是否要销毁了
				if (pool->shutdown){
					pool->thread_real_num--;
					pthread_mutex_unlock (&(pool->queue_lock));
					goto END;
				}

				// 判断线程池数量是否要调整 根据当前线程数和最小sn号计算
				// 如线程数量应该为16 最小线程sn号为128
				// 那么编号 小于128 和 大于等于128+16 的线程都会退出
				if( thread_sn >= pool->thread_adjust_num + pool->thread_min_sn || thread_sn < pool->thread_min_sn){
					pool->thread_real_num--;
					pthread_mutex_unlock (&(pool->queue_lock));
					goto END;
				}
				
				pool->thread_sleep_num++;										// 线程睡觉
				pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock));	// 睡觉等待
				pool->thread_sleep_num--;										// 线程睡醒
			}

			// 如果当前任务队列没有任务了 则返回等待
			if( pool->cur_queue_size == 0 || pool->queue_head == NULL){
				continue;
			}

			// 等待队列长度减去1，并取出链表中的头元素
			pool->cur_queue_size--;
			CThread_worker *worker = pool->queue_head;
			pool->queue_head = worker->next;
		
			pthread_mutex_unlock (&(pool->queue_lock));			// 释放互斥锁

			// 调用回调函数，执行任务
			void* res = (*(worker->process)) (worker->arg, handle);
			// 任务执行完成 释放任务资源
			free (worker);
			worker = NULL;
			// 判断任务是否处理正常 -- sql连接是否运行正常
			if((long)res == WT_SQL_ERROR){
				break;
			}
		}
		// 因为sql连接异常 而跳出第二层循环 所以先销毁sql资源,然后重新初始化
		wt_sql_destroy(handle);
	}
END:	//线程池销毁操作
	xyprintf(0, "THREAD_ADJUST:Thread of sn is %04d, id is %ld will exit!", thread_sn, pthread_self() );
	wt_sql_destroy( handle );			// 销毁sql资源
	free( handle );						// 回收sql资源空间
	pthread_exit( NULL );				// 线程退出
}

/** 
 *@brief  线程池控制线程
 *@param  arg		类型 void*	线程函数必须，无意义
 *@return 无
 */
void* pool_adjust (void* arg)
{
	// 打印一下 告诉程序 我执行正常
	xyprintf(0, "** O(∩ _∩ )O ~~ Thread pool adjust thread start, max_num is %u, min_num is %u, min_sn is %u!!!",
			pool->thread_max_num, pool->thread_min_num, pool->thread_min_sn);

	int adjust_count = 0;	// 刻度值
	int interval_time = 30;	// 测量间隔时间

	while(1){
		sleep(interval_time);
		
		pthread_mutex_lock(&(pool->queue_lock));	//加锁
		
		// 如果睡眠的线程超过总线程的一半 刻度值减一
		if(pool->thread_sleep_num > pool->thread_num / 2){
			adjust_count--;
			xyprintf(0, "THREAD_ADJUST:Too many threads sleep! so adjust_count to %d, thread_sleep_num = %d, thread_num = %d, real_num = %d",
					adjust_count, pool->thread_sleep_num, pool->thread_num, pool->thread_real_num);
			
			interval_time = 60;
		}
		else if(pool->cur_queue_size > pool->thread_num){	// 如果队列中的任务 超过线程的数量 刻度值加一
			//刻度值归0
			if(adjust_count < 0){
				adjust_count = 0;
			}

			//队列任务数超过当前线程数的两倍
			if(pool->cur_queue_size > pool->thread_num * 2){
				adjust_count = 2;
				xyprintf(0, "THREAD_ADJUST:Too too many tasks waiting! so adjust_count to %d, thread_sleep_num = %d, thread_num = %d, real_num = %d",
						adjust_count, pool->thread_sleep_num, pool->thread_num, pool->thread_real_num);
			}
			else{
				adjust_count++;
				xyprintf(0, "THREAD_ADJUST:Too many tasks waiting! so adjust_count to %d, thread_sleep_num = %d, thread_num = %d, real_num = %d",
						adjust_count, pool->thread_sleep_num, pool->thread_num, pool->thread_real_num);
			}
			
			interval_time = 10;
		}
		else {
			interval_time = 30;
		}

		// 如果刻度值低于-10 需要关闭线程
		if(adjust_count <= -10){
			adjust_count = 0;

			if(pool->thread_adjust_num > pool->thread_min_num){
				// 如果要调整到的线程大于最大线程数量
				// 进行减少调整 调整到当前线程的1/2
				pool->thread_adjust_num /= 2;
			
				// 如果调整后的线程数量 小于 最小线程数 则...
				if(pool->thread_adjust_num < pool->thread_min_num){
					pool->thread_adjust_num = pool->thread_min_num;
				}
				
				xyprintf(0, "THREAD_ADJUST:Adjust the number of threads -- DEL! %d --> %d", pool->thread_num, pool->thread_adjust_num);
			
				pool->thread_num = pool->thread_adjust_num;

				// 重新申请 存放线程id的空间
				//pthread_t* temp = malloc( pool->thread_adjust_num * sizeof(pthread_t) );
				//memcpy(temp, pool->threadid, pool->thread_adjust_num * sizeof(pthread_t) );
				//free(pool->threadid);
				//pool->threadid = temp;
			}
			else {
				//xyprintf(0, "THREAD_ADJUST:Want adjust the number of threads to %d, but min_num is %u, so...", pool->thread_num, pool->thread_min_num);
				// 线程换血操作 怀疑数据库连接保持时间过长 会出现问题?
				xyprintf(0, "THREAD_ADJUST:Shake thread!!!");
				if(pool->thread_min_sn != pool->thread_max_num * 10){
					pool->thread_min_sn = pool->thread_max_num * 10;
					// 新线程创建 老线程会自动退出
					int i = pool->thread_min_sn;
					pthread_t threadid;
					for(; i < pool->thread_num + pool->thread_min_sn; i++){
						pthread_create(&threadid, NULL, thread_routine, (void*)((long)(i)) );
						pool->thread_real_num++;
				    }
				}
				else {
					pool->thread_min_sn = 1;
					// 新线程创建 老线程会自动退出
					int i = pool->thread_min_sn;
					pthread_t threadid;
					for(; i < pool->thread_num + pool->thread_min_sn; i++){
						pthread_create(&threadid, NULL, thread_routine, (void*)((long)(i)) );
						pool->thread_real_num++;
				    }
				}
			}
		}
		else if(adjust_count >= 2){		// 如果刻度值大于2 再开当前线程二分子一 
			adjust_count = 0;
		
			if(pool->thread_adjust_num < pool->thread_max_num){

				pool->thread_adjust_num = pool->thread_num + pool->thread_num / 2;
			
				if(pool->thread_adjust_num > pool->thread_max_num ){
					pool->thread_adjust_num = pool->thread_max_num;
				}

				xyprintf(0, "THREAD_ADJUST:Adjust the number of threads -- ADD! %d --> %d", pool->thread_num, pool->thread_adjust_num);
			
				// 重新申请 存放线程id的空间
				//pthread_t* temp = malloc( pool->thread_adjust_num * sizeof(pthread_t) );
				//memcpy(temp, pool->threadid, pool->thread_num * sizeof(pthread_t) );
				//free(pool->threadid);
				//pool->threadid = temp;

				// 创建新线程
				pthread_t threadid;
				for(; pool->thread_num < pool->thread_adjust_num; pool->thread_num++){
					pthread_create(&threadid, NULL, thread_routine, (void*)(long)(pool->thread_num + pool->thread_min_sn) );
					pool->thread_real_num++;
				}
			}
			else {
				xyprintf(0, "THREAD_ADJUST:Want adjust the number of threads to %d, but max_num is %u, so...", pool->thread_num, pool->thread_max_num);
			}
		}
		pthread_mutex_unlock (&(pool->queue_lock));
		pthread_cond_broadcast (&(pool->queue_ready));		// 唤醒所有等待线程
	}
	// 到不了这里
	xyprintf(0, "THREAD_ADJUST:Why? Why there? -- %s %d", __FILE__, __LINE__);
	pthread_exit( NULL );				// 线程退出
}

/** 
 *@brief  线程池初始化
 *@param  thread_num	类型 unsigned int	线程池大小
 *@param  max			类型 unsigned int	线程最大数量 不自动调节 则将最大 最小数 和初始数一样
 *@param  min			类型 unsigned int	线程最小数量
 *@param  sql_name		类型 char*
 *@param  sql_user		类型 char*
 *@param  sql_pass		类型 char*
 *@return 无
 */
void pool_init (unsigned int thread_num, unsigned int max_num, unsigned int min_num, char* sql_name, char* sql_user, char* sql_pass)
{
	// 申请线程池对象空间
	pool = (CThread_pool *) malloc( sizeof(CThread_pool) );
	memset( pool, 0, sizeof(CThread_pool) );

	// 变量初始化
    pthread_mutex_init( &(pool->queue_lock), NULL);		// 初始化互斥锁
    pthread_cond_init( &(pool->queue_ready), NULL);		// 初始化条件变量
    pool->queue_head		= NULL;						// 任务队列头置空
    pool->shutdown			= 0;						// 线程销毁标志
	pool->thread_num		= thread_num;				// 线程池线程数量
    pool->thread_min_sn		= 1;
    pool->cur_queue_size	= 0;						// 任务队列任务数
	pool->thread_adjust_num	= thread_num;				// 线程调整数量
	pool->thread_max_num	= max_num;					// 线程最大数量
	pool->thread_min_num	= min_num;					// 线程最小数量
	pool->thread_sleep_num	= 0;						// 在睡觉的线程数量
	//pool->threadid = (pthread_t *) malloc( thread_num * sizeof(pthread_t) );	// 线程id存放空间申请
	strcpy(pool->sql_name, sql_name);
	strcpy(pool->sql_user, sql_user);
	strcpy(pool->sql_pass, sql_pass);


	// 线程创建
	int i = pool->thread_min_sn;
	pthread_t threadid;
	for(; i < thread_num + pool->thread_min_sn; i++){
		pthread_create(&threadid, NULL, thread_routine, (void*)((long)(i)) );
		pool->thread_real_num++;
    }
	
	// 如果最大线程大于当前数,并且最小线程小于当前线程数量,创建线程池调节线程
	if(max_num > thread_num || min_num < thread_num){
		pthread_t adjust;
		pthread_create (&adjust, NULL, pool_adjust, NULL);
	}
}

/** 
 *@brief  向线程池中添加任务
 *@param  process	类型 void*(*)(void*,wt_sql_handle*)		任务执行函数
 *@param  arg		类型 void*								传入任务执行函数的参数
 *@return 无意义
 */
int pool_add_worker (void *(*process) (void *arg, wt_sql_handle *handle), void *arg)
{
    // 构造一个新任务
	CThread_worker *newworker = (CThread_worker *) malloc (sizeof (CThread_worker));
	newworker->process = process;
	newworker->arg = arg;
	newworker->next = NULL;			// 别忘置空

	pthread_mutex_lock(&(pool->queue_lock));	// 加锁
	 
	// 将任务加入到等待队列中
	CThread_worker *member = pool->queue_head;
	if (member != NULL){
		while (member->next != NULL){
			member = member->next;
		}
		member->next = newworker;				// 存放到队列尾
    }
	else {
		pool->queue_head = newworker;
	}

	pool->cur_queue_size++;						// 任务数量

	// 在任务数量大于10的时候 打印报警语句
	if(pool->cur_queue_size >= 20){
		if(pool->cur_queue_size % 100 == 0){
			xyprintf(0, "THREAD_ADJUST:WARNING!!! cur_queue_size is %u, thread_num is %u, real_num = %u, thread_sleep_num is %u",
					pool->cur_queue_size, pool->thread_num, pool->thread_real_num, pool->thread_sleep_num);
		}
		// 在任务数量大于6666的时候 重启程序
		if(pool->cur_queue_size > 6666){
			xyprintf(0, "THREAD_ADJUST:WARNING!!! cur_queue_size is %u, kill myself exit(-1)!", pool->cur_queue_size);
			exit(-1);
		}
	}
	pthread_mutex_unlock (&(pool->queue_lock));	//释放锁

	//好了，等待队列中有任务了，唤醒一个等待线程；注意如果所有线程都在忙碌，这句没有任何作用
	pthread_cond_signal (&(pool->queue_ready));
	return 0;
}

/** 
 *@brief  销毁线程池，等待队列中的任务不会再被执行，但是正在运行的线程会一直把任务运行完后再退出
 *@return success 0 failed -1
 */
int pool_destroy ()
{
	// 防止两次调用
	if (pool->shutdown){
		return -1;
	}
	pool->shutdown = 1;

	pthread_cond_broadcast (&(pool->queue_ready));		// 唤醒所有等待线程，线程池要销毁了

	// 阻塞等待线程退出，否则就成僵尸了
    //int i;
    //for (i = 0; i < pool->thread_num; i++){
    //	pthread_join (pool->threadid[i], NULL);
	//}
	//free (pool->threadid);

    // 销毁等待队列
    CThread_worker *head = NULL;
    while (pool->queue_head != NULL){
        head = pool->queue_head;
		pool->queue_head = pool->queue_head->next;
        free (head);
    }

    // 条件变量和互斥量也别忘了销毁
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_ready));
    
    free (pool);
    // 销毁后指针置空是个好习惯
    pool=NULL;
    return 0;
}

/** 
 *@brief  获取线程池堆积报文数量
 */
unsigned int inline get_cur_queue_size()
{
	return pool->cur_queue_size;
}

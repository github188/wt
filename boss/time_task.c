/*****************************************************
 *
 * 定时任务
 *
 *****************************************************/
#include "boss_header.h"

#define GET_SHANGHU_SHEBEI_TYPE_TIME	24	// +10m

/** 
 *@brief  获取商户和设备类型线程
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* get_ss_thread(void *fd)
{
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Get shanghu and shebei type thread is running!!!");

	unsigned int sleep_time;
	time_t now;
	struct tm *p;

	while(1){
		time(&now);
		p = localtime(&now);
		if(p->tm_hour < GET_SHANGHU_SHEBEI_TYPE_TIME){
			sleep_time = ( GET_SHANGHU_SHEBEI_TYPE_TIME - p->tm_hour - 1 ) * 60 * 60 + ( 60 - p->tm_min ) * 60;
		}
		else {
			sleep_time = ( 24 + GET_SHANGHU_SHEBEI_TYPE_TIME - p->tm_hour - 1) * 60 * 60 + ( 60 - p->tm_min ) * 60;
		}

		sleep_time += 10 * 60;

		xyprintf(0, "GET_SS:** (～﹃～)~zZ ~~ Get shanghu and shebei type thread will sleep %u s!!!", sleep_time );
		sleep( sleep_time );
		xyprintf(0, "GET_SS:** O(∩ _∩ )O ~~ Get shanghu and shebei type thread is get up!!!");

		//处理时间测试 -- 开始时间
		struct timeval start, end;
		gettimeofday( &start, NULL );
	
		struct list_head* pos;
		guide_node *node;
		int wait_count;				// 等待次数
		int carry_out_flag;			// 完成标志

		while(1){
			// 完成标志赋初值 1 完成态
			carry_out_flag = 1;
			
			pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
			
			// 进入循环查找 没有发送get shanghu 命令的user	
			for( pos = gv_guide_list_head.next; pos != &gv_guide_list_head; pos = pos->next ){
				node = (guide_node*)pos;
				// 判断是否已经发送
				if( !node->get_shanghu_flag ){
					// 修改完成标志到 未完成态
					carry_out_flag = 0;
					// 发送get命令
					if( send_get_msg(bg_get_shanghu, node->sockfd) ){
						// 错误 删除这个user 在链表内
						node->stat = SOCK_STAT_DEL;
						carry_out_flag = 1;
					} else {
						// 成功 修改发送标志 并修改定时任务完成标志为未完成态 然后跳出循环
						node->get_shanghu_flag = 1;
						gv_time_ed_flag = 0;
						break;
					}
				}
			}
			
			pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙

			// 判断完成标志 如果都完成了 跳出循环 没有完成 等待刚发送的指令 执行结束
			if( carry_out_flag ){
				break;
			}
			else {
				// 等待次数 置0
				wait_count = 0;

				// 循环等待
				while(1){

					sleep(5);			// 睡眠5秒
					
					wait_count++;		
					if(wait_count == 12 * 10){	// 12 * 5 没有完成,则停止等待
						break;
					}

					// 进入互斥区 判断 任务是否完成
					pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
					if(gv_time_ed_flag){
						pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
						break;
					}
					pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
				}
			}
		}// get shanghu is over

		gettimeofday( &end, NULL );
		xyprintf(0, "GET_SS:☻ ☺ ☻ ☺ ");
		xyprintf(0, "GET_SS:☻ ☺ ☻ ☺  Get shebei type bao ping'an, used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
		xyprintf(0, "GET_SS:☻ ☺ ☻ ☺ ");

		while(1){
			// 完成标志赋初值 1 完成态
			carry_out_flag = 1;
			
			pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
			
			// 进入循环查找 没有发送get shebei type 命令的user	
			for( pos = gv_guide_list_head.next; pos != &gv_guide_list_head; pos = pos->next ){
				node = (guide_node*)pos;
				// 判断是否已经发送
				if( !node->get_shebei_type_flag ){
					// 修改完成标志到 未完成态
					carry_out_flag = 0;
					// 发送get命令
					if( send_get_msg(bg_get_shebei_type, node->sockfd) ){
						// 错误 删除这个user 在链表内
						node->stat = SOCK_STAT_DEL;
						carry_out_flag = 1;
					} else {
						// 成功 修改发送标志 并修改定时任务完成标志为未完成态 然后跳出循环
						node->get_shebei_type_flag = 1;
						gv_time_ed_flag = 0;
						break;
					}
				}
			}
			
			pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙

			// 判断完成标志 如果都完成了 跳出循环 没有完成 等待刚发送的指令 执行结束
			if( carry_out_flag ){
				break;
			}
			else {
				// 等待次数 置0
				wait_count = 0;

				// 循环等待
				while(1){

					sleep(5);			// 睡眠5秒
					
					wait_count++;		
					if(wait_count == 12){	// 12 * 5 没有完成,则停止等待
						break;
					}

					// 进入互斥区 判断 任务是否完成
					pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
					if(gv_time_ed_flag){
						pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
						break;
					}
					pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
				}
			}
		}// get shebei type is over

		//处理时间 -- 结束时间
		gettimeofday( &end, NULL );
		xyprintf(0, "GET_SS:☻ ☺ ☻ ☺ ");
		xyprintf(0, "GET_SS:☻ ☺ ☻ ☺  Get shanghu and shebei type bao ping'an, used %d s %d us!!!", end.tv_sec - start.tv_sec, end.tv_usec -start.tv_usec);
		xyprintf(0, "GET_SS:☻ ☺ ☻ ☺ ");

		pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
		for( pos = gv_guide_list_head.next; pos != &gv_guide_list_head; pos = pos->next ){
			((guide_node*)pos)->get_shebei_type_flag = 0;
			((guide_node*)pos)->get_shanghu_flag	 = 0;
		}
		pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙

	}//while(1);

	// distance
ERR:
	xyprintf(0, "GET_SS_ERROR:✟ ✟ ✟ ✟ -- %s %d:Get shanghu and shebei type pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

/** 
 *@brief  商户超时下线操作
 *@param  fd		类型 void*	线程启动参数,未使用
 *@return nothing
 */
void* tfree_thread(void *fd)
{
#define TFREE_DEBUG		1
	pthread_detach(pthread_self());
	xyprintf(0, "** O(∩ _∩ )O ~~ Tfreemanyou thread is running!!!");

	while(1){
		xyprintf(0, "TFREE:** (～﹃～)~zZ ~~ Tfree thread will sleep %u s!!!", sgv_tupdate );
		sleep( sgv_tupdate );
		xyprintf(0, "TFREE:** O(∩ _∩ )O ~~ Tfree thread is get up!!!");
	
		// 数据库访问资源
		wt_sql_handle *iud_handle = malloc(sizeof(wt_sql_handle));
		memset(iud_handle, 0, sizeof(wt_sql_handle));
	
		while( wt_sql_init(iud_handle, cgv_sql_name, cgv_sql_user, cgv_sql_pass) ){		// 数据库初始化
			xyprintf(0, "SQL_INIT_ERROR:%s %s %d -- Datebase connect error!", __func__, __FILE__, __LINE__);
			sleep( 30 );
		}
		
		//更新isonline标志
		sprintf(iud_handle->sql_str, "UPDATE mx_s_user SET isonline = 0 WHERE isonline = 1 AND DATEDIFF(s, lastdate, GETDATE()) >= %u", sgv_tupdate * 2);
		if(wt_sql_exec(iud_handle)){
			xyprintf(0, "SQL_ERROR: %s %d -- iud_handle->sql_str: %s", __FILE__, __LINE__, iud_handle->sql_str);
			goto STR_ERR;
		}
		
//当一轮循环执行完成的时候,销毁数据库连接资源在下次执行的时候重新创建,如果执行出现错误时,提前结束
STR_ERR:
		//处理时间 -- 结束时间
		xyprintf(0, "TFREE:☻ ☺ ☻ ☺  tfree_fun 's bao ping'an!!!");

		wt_sql_destroy(iud_handle);
		free(iud_handle);
	}//while(1);

ERR:
	xyprintf(0, "TFREE_ERROR:✟ ✟ ✟ ✟ -- %s %d:Tfree pthread is unnatural deaths!!!", __FILE__, __LINE__);
	pthread_exit(NULL);
}

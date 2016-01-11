/*****************************************************
 *
 * 设备类型管理
 *
 *****************************************************/

#include "guide_header.h"

/** 
 *@brief  设备类型链表数据添加
 *@return nothing
 */
void RTS_ADD(char* name, unsigned int num)
{
	rt_node *node = malloc( sizeof(rt_node) );
	memcpy(node->name, name, sizeof(node->name));
	node->num = num;

	pthread_mutex_lock(&gv_boss_flag_lock);
	list_add(&(node->node), &gv_boss_rt_head);
	gv_rt_num++;
	xyprintf(0, "RTS_ADD:name -- %s, num -- %u, rt->num = %u", node->name, node->num, gv_rt_num);
	pthread_mutex_unlock(&gv_boss_flag_lock);
}

/** 
 *@brief  设备类型链表数据清除
 *@return nothing
 */
void RTS_CLEAN()
{
	struct list_head* pos;
	rt_node *node;

	pthread_mutex_lock(&gv_boss_flag_lock);
	
	pos = gv_boss_rt_head.next;
	while( pos != &gv_boss_rt_head ){
		list_del( pos );
		gv_rt_num--;
		
		node = (rt_node*)pos;
		xyprintf(0, "RTS_DEL:name -- %s, num -- %u, rt->num = %u", node->name, node->num, gv_rt_num);
		if(node){
			free( node );
		}

		pos = gv_boss_rt_head.next;
	}
	
	pthread_mutex_unlock(&gv_boss_flag_lock);

}

/** 
 *@brief  设备类型链表数据清除
 *@return can use 0, can't use -1
 */
int RTS_NUM_PROCESS(char* name, unsigned int curr_num)
{
	struct list_head* pos;
	rt_node *node;
	int ret = -1;
	
	pthread_mutex_lock(&gv_boss_flag_lock);
	for( pos = gv_boss_rt_head.next; pos != &gv_boss_rt_head; pos = pos->next ){
		node = (rt_node*)pos;
		if( !strcmp( node->name, name ) ){	// 设备类型相同
			if(curr_num < node->num){		// 数量判断
				ret = 0;
			}
			break;
		}	
	}
	pthread_mutex_unlock(&gv_boss_flag_lock);

	return ret;
}

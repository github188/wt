/*****************************************************
 *
 * 引导链表管理
 *
 *****************************************************/
#include "boss_header.h"

/** 
 *@brief  将对应认证服务器连接置错,在下个epoll循环中删除
 *@param  sockfd	类型 int	网络连接套接字
 *@return nothing
 */
void inline guide_list_err(int sockfd){
	struct list_head* pos;
	pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
	for( pos = gv_guide_list_head.next; pos != &gv_guide_list_head; pos = pos->next ){
		if( sockfd == ((guide_node*)pos)->sockfd){
			((guide_node*)pos)->stat = SOCK_STAT_DEL;//置为-1 备下次添加时 删除
			break;
		}	
	}
	pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
}

/** 
 *@brief  根据sockfd获取设备信息
 *@return 找到 0 没有找到 -1
 */
int inline guide_sockfd_call_all(int sockfd, guide_node* node){
	struct list_head* pos;
	int find_flag = -1;
	//查找发送报文的设备 并将设备信息复制出来
	pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
	for( pos = gv_guide_list_head.next; pos != &gv_guide_list_head; pos = pos->next ){
		if(((guide_node*)pos)->sockfd == sockfd){
			memcpy(node, pos, sizeof(guide_node));
			find_flag = 0;
			break;
		}
	}
	pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
	return find_flag;
}

/** 
 *@brief  根据代理id获取sockfd
 *@return success sockfd, failed -1
 */
int inline guide_id_call_sockfd(unsigned int userid){
	struct list_head* pos;
	//查找发送报文的设备 并将设备信息复制出来
	pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
	for( pos = gv_guide_list_head.next; pos != &gv_guide_list_head; pos = pos->next ){
		if(((guide_node*)pos)->userid == userid){
			pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
			return ((guide_node*)pos)->sockfd;
		}
	}
	pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
	return -1;
}

/** 
 *@brief  根据sockfd获取id
 *@return success sockfd, failed -1
 */
unsigned int inline guide_sockfd_call_id(int sockfd){
	struct list_head* pos;
	//查找发送报文的设备 并将设备信息复制出来
	pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
	for( pos = gv_guide_list_head.next; pos != &gv_guide_list_head; pos = pos->next ){
		if(((guide_node*)pos)->sockfd == sockfd){
			pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
			return ((guide_node*)pos)->userid;
		}
	}
	pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
	return -1;
}

/** 
 *@brief  根据sockfd修改id
 *@return success sockfd, failed -1
 */
void inline guide_sockfd_change_id(int sockfd, unsigned int userid)
{
	struct list_head* pos;
	//查找发送报文的设备 并将设备信息复制出来
	pthread_mutex_lock(&gv_guide_list_lock);//⊙﹏⊙
	for( pos = gv_guide_list_head.next; pos != &gv_guide_list_head; pos = pos->next ){
		if(((guide_node*)pos)->sockfd == sockfd){
			((guide_node*)pos)->userid = userid;
			break;
		}
	}
	pthread_mutex_unlock(&gv_guide_list_lock);//⊙﹏⊙
}

/************************************************************************************************************
*	cs518 - assignment01 
*	thread scheduling 
*
*	thread_unit_lib.h 	
************************************************************************************************************/

#ifndef THREAD_UNIT_LIST_T
#define THREAD_UNIT_LIST_T


#include "my_pthread_t.h"


thread_unit thread_unit_init();


void thread_list_init(thread_unit_list* thread_ptr);

void thread_list_enqueue(thread_unit_list* thread_list, thread_unit* unit);

void thread_list_dequeue(thread_unit_list* thread_list);

thread_unit thread_list_peek(thread_unit_list* thread_list);

thread_unit thread_list_isempty(thread_unit_list* thread_list);

void test();


#endif
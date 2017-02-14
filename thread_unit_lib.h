/************************************************************************************************************
*	cs518 - assignment01 
*	thread scheduling 
*
*	thread_unit_lib.h 	
************************************************************************************************************/

#ifndef THREAD_UNIT_LIST_T
#define THREAD_UNIT_LIST_T


#include "my_pthread_t.h"


/* Thread Unit Library */
thread_unit* thread_unit_init(my_pthread_t* pthread);


/* Thread Unit List Library  */
thread_unit_list* thread_list_init();
void thread_list_enqueue(thread_unit_list* list, thread_unit* unit);
thread_unit* thread_list_dequeue(thread_unit_list* list);
thread_unit* thread_list_peek(thread_unit_list* list);
int thread_list_isempty(thread_unit_list* list);


/* Print statements for DEBUGGING*/
const char* _stringify_state(state s);
void _print_thread_unit(thread_unit* unit);
void _print_thread_list(thread_unit_list* list);

#endif
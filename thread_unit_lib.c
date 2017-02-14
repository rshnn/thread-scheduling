/************************************************************************************************************
*	cs518 - assignment01 
*	thread scheduling 
*
*	thread_unit_lib.c 	
************************************************************************************************************/

#include "thread_unit_lib.h"



/* Intializes a thread_unit structure. */
thread_unit* thread_unit_init(my_pthread_t* pthread){

	thread_unit* tu = (thread_unit *)malloc(sizeof(thread_unit)); 

	tu->thread 			= pthread;
	tu->ucontext 		= NULL;
	tu->state 			= EMBRYO;
	tu->time_slice 		= TIME_QUANTUM;
	tu->run_count 		= 0;
	tu->waiting_on_me 	= NULL;
	tu->next 			= NULL;

	return tu;
}

/* Initialize an empty thread_unit_list */
thread_unit_list* thread_list_init(){

	thread_unit_list* thread_list = (thread_unit_list*)malloc(sizeof(thread_unit_list));

	thread_list->head 	= NULL;
	thread_list->tail 	= NULL;
	thread_list->size 	= 0;

	return thread_list;
}



/* New thread_units added to the end of the queue */
void thread_list_enqueue(thread_unit_list* list, thread_unit* unit){

	// Enqueue on an empty list
	if(list->size == 0){
		list->head = unit;
		list->tail = unit;

	// Default: Add at end and redirect tail
	}else{
		list->tail->next	= unit;
		list->tail 			= unit;
	}

	list->size++;
}



thread_unit* thread_list_dequeue(thread_unit_list* list){

	/* Empty list -- Return NULL */
	if(thread_list_isempty(list)){
		return NULL;
	}

	thread_unit* deq_unit;

	/* One item list */
	if(list->size == 1){

		deq_unit 	= list->head; 
		list->head 	= NULL;
		list->tail 	= NULL; 

	/* Default: return unit at head.  Move head. */
	}else{

		deq_unit 		= list->head;
		list->head 		= list->head->next;
	
	}

	list->size--;
	return deq_unit;

}


/* Peek at head if list is not empty */
thread_unit* thread_list_peek(thread_unit_list* list){

	if(thread_list_isempty(list)){
		return NULL;
	}else{
		return list->head;
	}

}


/* Returns if list is emtpy (0 false || 1 true) */
int thread_list_isempty(thread_unit_list* list){

	if(list->size == 0){
		return 1;
	}else{
		return 0;
	}
}


const char* _stringify_state(state s){

	switch(s)
	{
		case EMBRYO: 		return "EMBRYO";
		case READY: 		return "READY";
		case RUNNING: 		return "RUNNING";
		case WAITING: 		return "WAITING";
		case TERMINATED: 	return "TERMINATED";
		case ZOMBIE: 		return "ZOMBIE";
		default: 			return "UNKNOWN STATE";
	}

}


/* DEBUGGING ONLY: Print out thread unit */
void _print_thread_unit(thread_unit* unit){
	printf("TheadID: %i \t State: %s \t Run count: %i\n", unit->thread->threadID, _stringify_state(unit->state), unit->run_count);
}


/* DEBUGGING ONLY: Print out thread list */
void _print_thread_list(thread_unit_list* list){

	thread_unit* temp;

	if(!thread_list_isempty(list)){
		temp = list->head;

		while(temp->next != NULL){
			_print_thread_unit(temp);

			temp = temp->next;
		}

	}else{
		printf("Empty thread unit list. \n");
	}

}
/************************************************************************************************************
*	cs518 - assignment01 
*	thread scheduling 
*
*	thread_unit_lib.c 	
************************************************************************************************************/

#include "thread_unit_lib.h"


/************************************************************************************************************
*
*    THREAD_UNIT LIBRARY
*
************************************************************************************************************/


/* Initializes a thread_unit structure. Expects a well-formed my_pthread data structure */
thread_unit* thread_unit_init(my_pthread_t* pthread){

	thread_unit* tu;

	if ((tu = (thread_unit *)myallocate(sizeof(thread_unit), __FILE__, __LINE__, 0)) == NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	}

	if ((tu->ucontext = (ucontext_t*) malloc(sizeof(ucontext_t))) == NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	}

	tu->thread 			= pthread;
	tu->state 			= EMBRYO;
	tu->time_slice 		= TIME_QUANTUM;
	tu->run_count 		= 0;
	tu->next 			= NULL;
	tu->wait_next 		= NULL;

	tu->waiting_on_me 	= thread_list_init();

	return tu;
}





/************************************************************************************************************
*
*    THREAD_UNIT_LIST FUNCTIONS (LINKED LIST LIBRARY)
*
************************************************************************************************************/



/* Initialize an empty thread_unit_list */
thread_unit_list* thread_list_init(){

	thread_unit_list* thread_list;

	if ((thread_list = (thread_unit_list*)malloc(sizeof(thread_unit_list))) == NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	}

	thread_list->head 	= NULL;
	thread_list->tail 	= NULL;
	thread_list->iter 	= NULL;
	thread_list->size 	= 0;

	return thread_list;
}



/* New thread_units added to the end of the queue */
void thread_list_enqueue(thread_unit_list* list, thread_unit* unit){

	/* Clear node->next */
	unit->next = NULL;

	// Enqueue on an empty list
	if(thread_list_isempty(list)){
		list->head 			= unit;
		list->tail 			= unit;
		list->iter 			= unit;

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
		list->iter 		= list->head->next;
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

	if(list->size == 0 || list->head == NULL){
		return 1;
	}else{
		return 0;
	}
}




/************************************************************************************************************
*
*    THREAD_UNIT_LIST FUNCTIONS FOR WAITING QUEUE (LINKED LIST LIBRARY)
*
************************************************************************************************************/



/* New thread_units added to the end of the queue */
void thread_list_enqueue_wait(thread_unit_list* list, thread_unit* unit){

	/* Clear node->next */
	unit->wait_next = NULL;

	// Enqueue on an empty list
	if(thread_list_isempty(list)){
		list->head 			= unit;
		list->tail 			= unit;
		list->iter 			= unit;

	// Default: Add at end and redirect tail
	}else{
		list->tail->wait_next	= unit;
		list->tail 				= unit;
	}

	list->size++;
}



thread_unit* thread_list_dequeue_wait(thread_unit_list* list){

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
		list->head 		= list->head->wait_next;
		list->iter 		= list->head->wait_next;
	}

	list->size--;
	return deq_unit;

}




/************************************************************************************************************
*
*    DEBUGGING FUNCTIONS
*
************************************************************************************************************/

/* DEBUGGING ONLY:  Turns enum state into strings for printing*/
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

	if(SUPRESS_PRINTS){
		return;
	}

	printf(ANSI_COLOR_CYAN "TheadID: %ld\t State: %s\t\tRun count: %i\t\tPriority: %i\n" ANSI_COLOR_RESET,
	 unit->thread->threadID, _stringify_state(unit->state), unit->run_count, unit->priority);
}


/* DEBUGGING ONLY: Print out thread list */
void _print_thread_list(thread_unit_list* list){

	if(SUPRESS_PRINTS){
		return;
	}

	thread_unit* temp;

	if(!thread_list_isempty(list)){
		temp = list->head;

		while(temp != NULL){
			_print_thread_unit(temp);

			temp = temp->next;
		}

	}else if(list->head == NULL){
		printf(ANSI_COLOR_CYAN "List head is NULL. \n" ANSI_COLOR_RESET);

	}else{
		printf(ANSI_COLOR_CYAN "Empty thread unit list. \n" ANSI_COLOR_RESET);
	}

}




/* DEBUGGING ONLY: Print out thread wait list */
void _print_thread_list_wait(thread_unit_list* list){


	if(SUPRESS_PRINTS){
		return;
	}

	thread_unit* temp;

	if(!thread_list_isempty(list)){
		temp = list->head;

		while(temp != NULL){
			_print_thread_unit(temp);

			temp = temp->wait_next;
		}

	}else if(list->head == NULL){
		printf(ANSI_COLOR_CYAN "List head is NULL. \n" ANSI_COLOR_RESET);

	}else{
		printf(ANSI_COLOR_CYAN "Empty thread unit list. \n" ANSI_COLOR_RESET);
	}

}
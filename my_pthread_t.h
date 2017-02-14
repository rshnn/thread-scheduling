/************************************************************************************************************
*	cs518 - assignment01 
*	thread scheduling 
*
*	my_pthread_t.h 	
************************************************************************************************************/


#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H 

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <ucontext.h>


#define PAGE_SIZE 			4096					// Stack size defaults to page size
#define PRIORITY_LEVELS 	5						// Number of priority levels
#define TIME_QUANTUM 		50000 					// 50 ms = 50000 us  
#define MAINTENENCE_TIME 	10*TIME_QUANTUM			// Experimental value 


/************************************************************************************************************
*
*    PTHREAD DATA STRUCTURES
*
************************************************************************************************************/

/*
	MY_PTHREAD_T
*/
typedef struct my_pthread_t {

    long int 			threadID;
	void *              return_val;
	int                 priority;

}my_pthread_t;



typedef struct my_pthread_attr_t {
}my_pthread_attr_t;


/*
	MY_PTHREAD_MUTEX_T
*/
typedef struct my_pthread_mutex_t { 
    
    int                         initialized;    // for mutex_init() 
    unsigned int                id;
	int 					    lock;
	struct my_pthread_t* 	    owning_thread;	// Do we instead need a pointer to thread_unit?  Or both?
	struct thread_unit_list_*   waiting_queue;

}my_pthread_mutex_t;



typedef struct my_pthread_mutexattr_t {
}my_pthread_mutexattr_t;



/************************************************************************************************************
*
*    SCHEDULER DATA STRUCTURES
*
************************************************************************************************************/

/*
	STATES 
*/
typedef enum state_ {

	EMBRYO,         // In process of being initialized  
	READY,          // Ready to be scheduled 
	RUNNING,        // Currently executing 
// 	BLOCKED,        // A thread that is blocked waiting for a lock 
	WAITING,        // A thread that is waiting indefinitely for another thread to perform an action (yield+blocked)
	TERMINATED,     // Exited.  Terminated 
	ZOMBIE          // Identified as zombie process 

}state;


 
/*
	THREAD_UNIT
		Contains meta-data about each pthread for scheduling 
		Doubles as a node structure for thread_unit_list 
*/
typedef struct thread_unit_ {
    
    my_pthread_t*       		thread;
    ucontext_t*         		ucontext;
    state               		state;          // NOTE: do we need to keep info on why its waiting here?
	int                 		time_slice;
	int                 		run_count;
    struct thread_unit_list_*   waiting_on_me;  // Linked list of thread_units currently waiting on this thread
    
    struct thread_unit_* 		next;
}thread_unit;



/*
	THREAD_UNIT_LIST
		A linked list of thread_units 
*/
typedef struct thread_unit_list_ {

	struct thread_unit_* 		head;
	struct thread_unit_* 		tail;
	int 						size;

}thread_unit_list;




/* 
	MUTEX_NODE
		Contains a my_pthread_mutex & pointer to next
		Used by thread_scheduler to store a linked list of mutexes
*/
typedef struct mutex_node_ {

	my_pthread_mutex_t* 	mutex;
	my_pthread_mutex_t* 	next;

}mutex_node;




/*
	SCHEDULER
		Main data structure of the scheduler thread.
		Contains priority array
			Array[PRIORITY_LEVELS]
			Each index contains a linked list of thread_units
		Contains mutex_list
			A linked list of mutex_nodes
		Contains two queues of thread_units:  running and waiting 
		Stores ucontexts of main thread and scheduler thread
*/
typedef struct scheduler_t {
    
    int 						initialized;
    struct thread_unit_list_* 	priority_array[PRIORITY_LEVELS];
    struct mutex_node_* 		mutex_list;
    struct thread_unit_list_*	running;
    struct thread_unit_list_*	waiting;
    ucontext_t 					scheduler_ucontext;	// Turn this into a thread_unit?
    ucontext_t					main_ucontext;		// Turn this into a thread_unit?

}scheduler_t;




/************************************************************************************************************
*
*    FUNCTION LIBRARY 
*
************************************************************************************************************/


/* my_pthread core library */
int my_pthread_create( my_pthread_t * thread, my_pthread_attr_t * attr, void *(*function)(void*), void * arg);
void my_pthread_yield();
void pthread_exit(void *value_ptr);
int my_pthread_join(my_pthread_t thread, void **value_ptr);



/* my_pthread mutex library */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const my_pthread_mutexattr_t *mutexattr);
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);



/* scheduler library */
void scheduler_init();
void scheduler_sig_handler();


// /* global vars go in .c file */
// ucontext_t      main_context;
// ucontext_t      scheduler_context;

#endif

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
#include <time.h>

/* Set this to 1 to hide helper print statements */
#define SUPRESS_PRINTS 		1

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define PAGE_SIZE 			4096					// Stack size defaults to page size
#define PRIORITY_LEVELS 	100						// Number of priority levels
#define TIME_QUANTUM 		50000 					// 50 ms = 50000 us  
#define RUNNING_TIME 		30						// Experimental value 



// /* For testing with msort */

// #define mypthread_create	my_pthread_create
// #define mypthread_exit		my_pthread_exit
// #define mypthread_yield		my_pthread_yield
// #define mypthread_join		my_pthread_join

// #define mypthread_mutex_init	my_pthread_mutex_init
// #define mypthread_mutex_lock	my_pthread_mutex_lock
// #define mypthread_mutex_trylock	my_pthread_mutex_trylock
// #define mypthread_mutex_unlock	my_pthread_mutex_unlock
// #define mypthread_mutex_destroy	my_pthread_mutex_destroy

// #define mypthread_t		my_pthread_t
// #define mypthread_attr_t	my_pthread_attr_t

// #define mypthread_mutex_t	my_pthread_mutex_t
// #define mypthread_mutex_attr_t	my_pthread_mutex_attr_t


/************************************************************************************************************
*
*    PTHREAD DATA STRUCTURES
*
************************************************************************************************************/

/*
	MY_PTHREAD_T
*/
typedef struct my_pthread_t {

    long int 				threadID;
	void *              	return_val;
	int                 	priority;
	struct thread_unit_* 	thread_unit;

}my_pthread_t;



typedef struct my_pthread_attr_t {
}my_pthread_attr_t;


/*
	MY_PTHREAD_MUTEX_T
*/
typedef struct my_pthread_mutex_t { 
    
    int                         initialized;    // for mutex_init() 
    int                			id;
	int 					    lock;
	long int 					owner;
	struct my_pthread_t* 	    owning_thread;	// Do we instead need a pointer to thread_unit?  Or both?
	struct thread_unit_*   		waiting_queue;  

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
	// BLOCKED,        // A thread that is blocked waiting for a lock 
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
	int 						priority;
    long int					joinedID;
    char						stack[PAGE_SIZE];

    struct thread_unit_* 		next;

    struct thread_unit_list_* 	waiting_on_me;


	struct thread_unit_*		wait_next;
	// struct thread_unit_*		wait_prev;
	struct thread_unit_* 		mutex_next;

}thread_unit;



/*
	THREAD_UNIT_LIST
		A linked list of thread_units 
*/
typedef struct thread_unit_list_ {

	struct thread_unit_* 		head;
	struct thread_unit_* 		tail;
	struct thread_unit_* 		iter;
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
    long int 					threadID_count;
    struct thread_unit_list_* 	priority_array[PRIORITY_LEVELS];
    struct mutex_node_* 		mutex_list;
    struct thread_unit_* 		currently_running;
    struct thread_unit_list_*	running;
    struct thread_unit_list_*	waiting;

}scheduler_t;




/************************************************************************************************************
*
*    FUNCTION LIBRARY 
*
************************************************************************************************************/


/* my_pthread core library */
int my_pthread_create( my_pthread_t * thread, my_pthread_attr_t * attr, void *(*function)(void*), void * arg);
void my_pthread_yield();
void my_pthread_exit(void *value_ptr);
int my_pthread_join(my_pthread_t thread, void **value_ptr);



/* my_pthread mutex library */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const my_pthread_mutexattr_t *mutexattr);
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);
int my_pthread_mutex_trylock(my_pthread_mutex_t* mutex);
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

void resetTheTimer();


/* scheduler library */
void scheduler_init();
void scheduler_sig_handler();
void priority_level_sort();


#endif

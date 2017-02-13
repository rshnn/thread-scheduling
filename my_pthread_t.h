/*
*	cs518 - assignment01 
*	thread scheduling 
*
*	my_pthread_t.h 	
*/


#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H 


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <ucontext.h>


#define PAGE_SIZE 4096;
#define PRIORITY_NUM
#define TIME_QUANTUM 50000                      // 50 ms 
#define MAINTENENCE_TIME 10*TIME_QUANTUM




/*
	THREAD_STATES 
*/
typedef enum __state__ {

	EMBRYO,         // In process of being initialized  
	READY,          // Ready to be scheduled 
	RUNNING,        // Currently executing 
// 	BLOCKED,        // A thread that is blocked waiting for a lock 
	WAITING,        // A thread that is waiting indefinietly for another thread to perform an action (yield?)
	TERMINATED,     // Exited.  Terminated 
	ZOMBIE          // Defined as zombie process 

}state;
 


/*
	MY_PTHREAD_T
*/
typedef struct my_pthread_t {

    unsigned int        threadID;
	void *              return_val;
	int                 priority;

}my_pthread_t;



typedef struct my_pthread_attr_t {
	// empty? lol
}my_pthread_attr_t;


/*
	MY_PTHREAD_MUTEX_T
		lock 				binary lock (0: unlocked; 1: locked)
		owning_thread 		pointer to the mutex's owning thread 	 
*/
typedef struct my_pthread_mutex_t { 
    
    int                         initialized;    // for mutex_init() 
    unsigned int                id;
	int 					    lock;
	struct my_pthread_t* 	    owning_thread;
	struct scheduler_unit_t*    waiting_queue;

}my_pthread_mutex_t;



typedef struct my_pthread_mutexattr_t {
	// lol also empty 
}my_pthread_mutexattr_t;



/**
* 
*
*    SCHEDULER DATA STRUCTS
*
* 
*/

typedef struct thread_god{
    
    struct scheduler_unit_t * 	thread_unit;        //array of size NUM_PRIORITIES
    ucontext_t          		context;
    
}thread_god;



typedef struct scheduler_unit_t{
    
    my_pthread_t*       		thread;
    ucontext_t          		context;
    state               		state;          // NOTE: do we need to keep info on why its waiting here?
	int                 		time_slice;
	int                 		run_count;
    struct scheduler_unit_t*    waiting_on_me;  // rename later
    
}scheduler_unit_t;



typedef struct mutex_god{
    
    my_pthread_mutex_t*     head;
    
}mutex_god;





/*
    COMMENTS FROM GCHAT
    
    
    move things out of my_pthread_t (into scheduler?)
        protect data structure from user 
    5 levels
    
    DATASTRUCTURE FOR SCHEDULER 

    NOTE:  have some sort of cheeck at the end of pthread_create to make sure all the data structs were initialized properly 

        
    
    Saad: I will mainly write cause mic being weird but
    If we have a doubly linked list as our structure. Would we run one list of threads? And then go to the next list of threads? (one list is equivalent to a specific priority)
    But any new thread is highest priority, so by default, it should be added to the first linked list, no? (in our case, we would have 5 linked lists cause 5 priority levels)
    Here is one way: after each thread runs (1 thread is like 1 index/node in the linked list), we would check the whole structure. So, if we have covered all of priority 0, then we go
    into priority 1. Now, if running one thread then a new thread comes in. New thread comes in priority 0. That means after the current thread (which is in priority 1 is done) we would 
    check if anything in priority 0 and then run that

    From what I understand: maintenance cycle is to update the extremely low priority threads to be upggraded to a higher priority
    I think I understand
    
    What we could do is keep track of how many time units it has run. This way we can also scale priorities up based on time units (when running maintenance)
    It would be easier if we give our own definition of fair: fairness = time units given? fairness = selection based on level?
    
    Which is why it should be based is amount run. E.g. if in lowest priority list there are 2 threads. One (T1) has run 50 units and another (T2) has run 200 units. 
    During maintenance cycle, T1 should be upgraded all the way to possibly priority 1 or 2. While T2 should be upgraded to priority 3 or so.
    
    Same here - would prefer table cause easy indexing

    Yeah, there should be a standard equation or something like that (so we can also justify that they are all adjusted based on some rule)

        new_time_slice = LVL_0_TIME_SLICE * priority_level * 1.5
        
        
    Array of linked list

*/







/**********************************************************************/

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
void scheduler_sig_handler(int signum);


// /* global vars go in .c file */
// ucontext_t      main_context;
// ucontext_t      scheduler_context;

#endif

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


/*
	THREAD_STATES 
		READY		Ready to be scheduled for the first time
		RUNNING		Currently being executed 
		YIELDING	Waiting to be rescheduled 
		COMPLETE 	Completed execution or terminated 
*/
typedef enum state {

	READY, 
	RUNNING, 
	YIELD,
	COMPLETE

}state;


/*
	MY_PTHREAD_T
		context 		current execution context of the thread 
		next_thread		pointer to next thread scheduled to execute
		state 			current enum state of thread
*/
typedef struct my_pthread_t {

	ucontext_t 				context;
	struct my_pthread_t*	next_thread;
	state					thread_state;

}my_pthread_t;



typedef struct my_pthread_attr_t {
	// ?????  
}my_pthread_attr_t;


/*
	MY_PTHREAD_MUTEX_T
		lock 				binary lock (0: unlocked; 1: locked)
		owning_thread 		pointer to the mutex's owning thread 	 
*/
typedef struct my_pthread_mutex_t {
	
	int 					lock;
	struct my_pthread_t* 	owning_thread;

}my_pthread_mutex_t;



typedef struct my_pthread_mutexattr_t {
	// ???????????
}my_pthread_mutexattr_t;

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


#endif

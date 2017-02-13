/************************************************************************************************************
*	cs518 - assignment01 
*	thread scheduling 
*
*	my_pthread_t.c 	
************************************************************************************************************/

#include "my_pthread_t.h"

static scheduler* scheduler;



/* my_pthread core library */
int my_pthread_create( my_pthread_t * thread, my_pthread_attr_t * attr, void *(*function)(void*), void * arg){}
void my_pthread_yield(){}
void pthread_exit(void *value_ptr){}
int my_pthread_join(my_pthread_t thread, void **value_ptr){}

/* my_pthread mutex library */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const my_pthread_mutexattr_t *mutexattr){}
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex){}
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex){}
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex){}


/************************************************************************************************************
*
*    SCHEDULER LIBRARY 
*
************************************************************************************************************/

void scheduler_init(){

	/* Initialize timer signal handler */


};

void scheduler_sig_handler(int signum){

	print("%i\n", signum);
};







int main(){

	printf("test print.\n");

}
/************************************************************************************************************
*	cs518 - assignment01 
*	thread scheduling 
*
*	my_pthread_t.c 	
************************************************************************************************************/

#include "my_pthread_t.h"
#include "thread_unit_lib.h" 	

static scheduler_t* scheduler;



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



    /**********************************************************************************
		Initialize timer signal handler  
    **********************************************************************************/

	struct sigaction signal_action;
	sigemptyset(&signal_action.sa_mask);

	if(sigaction(SIGSEGV, &signal_action, NULL) == -1){
		printf("Failure to initialize signal handler.\n");
		exit(EXIT_FAILURE);
	}




    /**********************************************************************************
		Initialize scheduler structures  
    **********************************************************************************/


	/* 
		Things that need to be initialized 
			scheduler_t scheduler
				priority_array
				mutex_list
				running
				waiting
				scheduler_ucontext
				main_ucontext

	*/


    /**********************************************************************************
		....
    **********************************************************************************/












	/* Direct sig-alarms to scheduler_sig_handler */
	signal(SIGALRM, &scheduler_sig_handler);
	scheduler_sig_handler();

};

void scheduler_sig_handler(){

	struct itimerval timer;


	/* Pause Timer */
    timer.it_value.tv_sec 		= 0;	// Time remaining until next expiration (sec)
    timer.it_value.tv_usec 		= 0;	// "" (microseconds)
    timer.it_interval.tv_sec 	= 0;	// Interval for periodic timer (sec)
    timer.it_interval.tv_usec 	= 0;	// "" (microseconds)
    setitimer(ITIMER_REAL, &timer, NULL);


    /**********************************************************************************
		scheduler activities start
    **********************************************************************************/

    printf("Hi im doing scheduling things every %i microseconds. \n", TIME_QUANTUM);
	

	/**********************************************************************************
		\end scheduling activities. 
	**********************************************************************************/


	/* Set Timer */
    timer.it_value.tv_sec 		= 0;			// Time remaining until next expiration (sec)
    timer.it_value.tv_usec 		= TIME_QUANTUM;	// "" (50 ms)
    timer.it_interval.tv_sec 	= 0;			// Interval for periodic timer (sec)
    timer.it_interval.tv_usec 	= 0;			// "" (microseconds)
    setitimer(ITIMER_REAL, &timer, NULL);


    return;
};
	






int main(){

	// scheduler_init();

	// while(1){
	// 	//printf("I'm spinning \n");
	// 	wait();
	// }



}
/************************************************************************************************************
*	cs518 - assignment01 
*	thread scheduling 
*
*	my_pthread_t.c 	
************************************************************************************************************/

#include "my_pthread_t.h"
#include "thread_unit_lib.h" 	

static scheduler_t* scheduler;
static ucontext_t main_context;
static ucontext_t scheduler_context;

/************************************************************************************************************
*
*    MY_PTHREAD CORE LIBRARY 
*
************************************************************************************************************/

int my_pthread_create( my_pthread_t * thread, my_pthread_attr_t * attr, void *(*function)(void*), void * arg){}
void my_pthread_yield(){}
void pthread_exit(void *value_ptr){}
int my_pthread_join(my_pthread_t thread, void **value_ptr){}


/************************************************************************************************************
*
*    MY_PTHREAD_MUTEX LIBRARY 
*
************************************************************************************************************/

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
		Initialize scheduler structures  
    **********************************************************************************/

	int i;

	scheduler = (scheduler_t*)malloc(sizeof(scheduler_t));

	/* Initialize each priority queue (thread_unit_list) */
	for(i = 0; i<= PRIORITY_LEVELS; i++){
		scheduler->priority_array[i] = thread_list_init();	// thread_list_init() does mallocing
	}

	/* Initialize the currently running and waiting queues */
	scheduler->running = thread_list_init();
	scheduler->waiting = thread_list_init();
	

	scheduler->initialized = 1;



    /**********************************************************************************
		Initialize timer signal handler  
    **********************************************************************************/

	/* The below is useful for redirecting sigactions before setting a signal handler */
	// struct sigaction signal_action;
	// sigemptyset(&signal_action.sa_mask);

	// if(sigaction(SIGSEGV, &signal_action, NULL) == -1){
	// 	printf("Failure to initialize signal handler.\n");
	// 	exit(EXIT_FAILURE);
	// }


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
		\end scheduling activities
	**********************************************************************************/


	/* Set Timer */
    timer.it_value.tv_sec 		= 2;			// Time remaining until next expiration (sec)
    timer.it_value.tv_usec 		= TIME_QUANTUM;	// "" (50 ms)
    timer.it_interval.tv_sec 	= 0;			// Interval for periodic timer (sec)
    timer.it_interval.tv_usec 	= 0;			// "" (microseconds)
    setitimer(ITIMER_REAL, &timer, NULL);


    return;
};
	





/************************************************************************************************************
*
*    DEBUGGING / TESTING
*
************************************************************************************************************/

/* 
	Tests the thread_unit library.  
	Stresses the thread_unit_list linked list structure  
*/
void ___debugging_thread_unit_lib(){
	

	// rshnn Tue 14 Feb 2017 12:20:09 PM EST

	printf("Debugging thread_unit_lib...\n\n");

	int i = 0;
	my_pthread_t* 		pthread_arr[10];
	thread_unit* 		unit_arr[10];
	thread_unit_list* 	list =  thread_list_init();

	while(i <= 10){

		pthread_arr[i] = (my_pthread_t*)malloc(sizeof(my_pthread_t));
		pthread_arr[i]->threadID = i;		
		unit_arr[i] = thread_unit_init(pthread_arr[i]);
		// _print_thread_unit(unit_arr[i]);

		thread_list_enqueue(list, unit_arr[i]);

		i++;
	}

	printf("\nShould show ID's 0 to 10\n");
	_print_thread_list(list);


	/* Pops off head.  Threads 0 to 3 removed */
	thread_list_dequeue(list);
	thread_list_dequeue(list);
	thread_list_dequeue(list);
	thread_list_dequeue(list);

	/* Peek head.  Should not alter list. */
	printf("\nPeeking.  Should show thread 4:\n");
	_print_thread_unit(thread_list_peek(list));


	/* Add new thread to queue (ID = 99) */
	my_pthread_t* temp = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	temp->threadID = 99;
	thread_unit* tempunit = thread_unit_init(temp);
	thread_list_enqueue(list, tempunit);


	/* List should have the following IDs:  4, 5, 6, 7, 8, 9, 10, 99 */
	printf("\nShould show the following IDs: 4, 5, 6, 7, 8, 9, 10, 99\n");
	_print_thread_list(list);


	/* Dequeue all thread_units off the list */
	for(i=0; i<=8; i++){
		thread_list_dequeue(list);
	}
	printf("\nShould show empty list: \n");
	_print_thread_list(list);



	/* Add  ID 99 and 5 back to queue */
	thread_list_enqueue(list, tempunit);
	thread_list_enqueue(list, unit_arr[5]);
	printf("\nShould show only thread 99: \n");
	_print_thread_list(list);
}





int main(){

	getcontext(&main_context);



	scheduler_init();

	// while(1){
	// 	//printf("I'm spinning \n");
	// 	wait();
	// }

	// ___debugging_thread_unit_lib();


}

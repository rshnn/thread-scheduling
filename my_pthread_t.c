/************************************************************************************************************
*	cs518 - assignment01 
*	thread scheduling 
*
*	my_pthread_t.c 	
************************************************************************************************************/

#include "my_pthread_t.h"
#include "thread_unit_lib.h" 	

static scheduler_t* scheduler;
static ucontext_t* 	main_ucontext;
static ucontext_t* 	scheduler_ucontext;

/************************************************************************************************************
*
*    MY_PTHREAD CORE LIBRARY 
*
************************************************************************************************************/

int my_pthread_create( my_pthread_t * thread, my_pthread_attr_t * attr, void *(*function)(void*), void * arg){

	/**********************************************************************************
		PTHREAD_T & THREAD_UNIT SETUP   
    **********************************************************************************/

	/* 
		has my_pthread_t has already been malloced (????)  Assuming it has been malloced already. 
			Assume an empty pthread_t that needs to be populated with defaults enters  
	*/

	thread->threadID = scheduler->threadID_count;
	scheduler->threadID_count++;

	thread->return_val = NULL;

	/* Assign to highest priority (new threads enter highest priority queue) */
	thread->priority = 0;


	/* Init thread_unit for this pthread */
	thread_unit* new_unit 				= thread_unit_init(thread);			//thread_unit_init() mallocs
	new_unit->waiting_on_me 			= thread_list_init();


    /**********************************************************************************
		UCONTEXT SETUP   
    **********************************************************************************/

	/* copy (fork) the current ucontext */
	if(getcontext(new_unit->ucontext) == -1){
		printf("Error obtaining ucontext of thread");
		exit(-1);
	} 

	/* Set up ucontext stack */
	if((new_unit->ucontext->uc_stack.ss_sp = malloc(PAGE_SIZE))==NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	}
	new_unit->ucontext->uc_stack.ss_size 	= PAGE_SIZE;
		/* wtf is happening here^?  I found this online somewhere.  Is it working?*/


	/* Set uc_link to point to addr of scheduler's ucontext */
	new_unit->ucontext->uc_link 			= scheduler_ucontext;

	/* Assign func* to ucontext */
	makecontext(new_unit->ucontext, (void*)function, 1, arg); 		
	// Should we write a separate scheduler_run_thread call?
    


    /**********************************************************************************
		SCHEDULE THAT SHIT    
    **********************************************************************************/
	// into highest priority bc heuristics say so 

	thread_list_enqueue(scheduler->priority_array[0], new_unit);
	

	/*
		By the end of my_pthread_create():
			The incoming pthread ptr will be populated with defaults
			That pthread will be put into a thread_unit_t
			That thread_unit_t's ucontext will be populated 
			That thread_unit will be enqueued into the priority 0 thread list. 
	*/
}


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

	/* Attempt to malloc space for scheduler */
	if ((scheduler = (scheduler_t*)malloc(sizeof(scheduler_t))) == NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	}

	scheduler->threadID_count 		= 2;	// Reserve 0 and 1 for scheduler and main threads (just in case)
	scheduler->currently_running 	= NULL;

	/* Initialize each priority queue (thread_unit_list) */
	for(i = 0; i<= PRIORITY_LEVELS; i++){
		scheduler->priority_array[i] = thread_list_init();	// thread_list_init() mallocs
	}

	/* Initialize the currently running and waiting queues */
	scheduler->running = thread_list_init();
	scheduler->waiting = thread_list_init();
	


	/* TODO: build ucontext of the scheduler (func* to scheduler_sig_handler) */
    /**********************************************************************************
		UCONTEXT SETUP   
    **********************************************************************************/

	/* Attempt to malloc space for scheduler_ucontext */
	if ((scheduler_ucontext = (ucontext_t*)malloc(sizeof(ucontext_t))) == NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	}

	/* copy (fork) the current ucontext */
	if(getcontext(scheduler_ucontext) == -1){
		printf("Error obtaining ucontext of scheduler");
		exit(-1);
	} 

	/* Set up ucontext stack */
	if((scheduler_ucontext->uc_stack.ss_sp = malloc(PAGE_SIZE))==NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	}
	scheduler_ucontext->uc_stack.ss_size 	= PAGE_SIZE;
	scheduler_ucontext->uc_stack.ss_flags 	= 0;
		/* wtf is happening here^?  I found this online somewhere.  Is it working?*/


	/* Set uc_link to point to addr of scheduler_ucontext */
	scheduler_ucontext->uc_link 			= main_ucontext;

	/* Assign func* to ucontext */
	makecontext(scheduler_ucontext, (void*)scheduler_sig_handler, 1, NULL); 	// Should we write a separate scheduler_run_thread call?
    
    /**********************************************************************************/





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

}

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
    timer.it_value.tv_usec 		= TIME_QUANTUM;	// "" (50 ms).  This will be changed to thread_unit->time_slice
    timer.it_interval.tv_sec 	= 0;			// Interval for periodic timer (sec)
    timer.it_interval.tv_usec 	= 0;			// "" (microseconds)
    setitimer(ITIMER_REAL, &timer, NULL);


    return;
}


void scheduler_runThread(thread_unit* thread){

	if(scheduler->currently_running != NULL){
		scheduler->currently_running->state = READY;
	}

	
	scheduler->currently_running 		= thread;
	scheduler->currently_running->state = RUNNING;

	/* Will execute function that thread points to. */ 
	setcontext(thread->ucontext);

	/* Context will switch back to scheduler either when thread completes or at a SIGARLM */
}
	


/************************************************************************************************************
*
*    DEBUGGING / TESTING
*
************************************************************************************************************/

/* 
	Tests the thread_unit library.  
	Stresses the thread_unit_list linked list structure  
*/
void _debugging_thread_unit_lib(){
	

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


void _debugging_pthread_init(){

}

void f1(int x){
	printf("In f1!  %i\n", x);
	return;
}


int main(){


	scheduler_init();


	my_pthread_t* testing = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	my_pthread_attr_t* nonesense; 

	int y = my_pthread_create(testing, nonesense, (void*)f1, (void*) 2);


	_print_thread_list(scheduler->priority_array[0]);

	scheduler_runThread(scheduler->priority_array[0]->head);


	// while(1){
	// 	//printf("I'm spinning \n");
	// 	wait();
	// }

	// _debugging_thread_unit_lib();


}

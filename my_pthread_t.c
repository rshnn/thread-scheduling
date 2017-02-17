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
static int 			SYS_MODE;
// static int 			first_schedule_done;


/************************************************************************************************************
*
*    SCHEDULER LIBRARY 
*
************************************************************************************************************/

void scheduler_init(){


	// first_schedule_done = 0;

	
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


void scheduler_runThread(thread_unit* thread){

	if(thread == NULL){
		printf(ANSI_COLOR_RED "Attempted to schedule a NULL thread\n"ANSI_COLOR_RESET);
		return;
	}

	if(scheduler->currently_running != NULL){
		scheduler->currently_running->state = READY;
	}

	
	scheduler->currently_running 		= thread;
	printf("STATE CHANGE TO RUNNING %ld\n", scheduler->currently_running->thread->threadID);
	scheduler->currently_running->state = RUNNING;

	/* Will execute function that thread points to. */ 
	swapcontext(scheduler_ucontext, thread->ucontext);

	/* Context will switch back to scheduler either when thread completes or at a SIGARLM */
}

void scheduler_sig_handler(){


	if(SYS_MODE == 1){
		return;
	}

    /**********************************************************************************
		scheduler activities start
    **********************************************************************************/
    
    if(scheduler->currently_running != NULL){
    	printf(ANSI_COLOR_MAGENTA "\nScheduler Signal Handler:\n\tThread %ld just ran for %i microseconds.\n" 
    			ANSI_COLOR_RESET, 
    			scheduler->currently_running->thread->threadID, 
    			scheduler->currently_running->time_slice);
		
    }else{
    	printf(ANSI_COLOR_MAGENTA "\nScheduler Signal Handler:\n\tNo threads have run yet.\n" ANSI_COLOR_RESET);

    }
    

    // if(scheduler->priority_array[0]->size > 0){
    // 	if(scheduler->currently_running != NULL){
    // 		scheduler_runThread(scheduler->currently_running->next);
    // 	}else
	   //  	scheduler_runThread(scheduler->priority_array[0]->head);
    // }


	/**********************************************************************************
		\end scheduling activities
	**********************************************************************************/

    my_pthread_yield();

}


void maintenance_cycle(){

	printf(ANSI_COLOR_YELLOW "\nMaitenance Cycle\n" ANSI_COLOR_RESET);

	/**********************************************************************************
		Priority Adjustments
	**********************************************************************************/

	// if(thread in running queue hasnt finished): demote priority
	scheduler->running->iter = scheduler->running->head;
	if(scheduler->running->iter->state == READY){

	}



	/**********************************************************************************
		Populate running queue 
	**********************************************************************************/

	/* Collect MAIT_CYCLE number processes to run and put them into running queue */
	int i; 
	scheduler->running->head = NULL;
	scheduler->running->tail = NULL;
	scheduler->running->iter = NULL;


	for(i=0; i<MAITENANCE_CYCLE; i++){
		// Run until you add a thread to running queue 

		// Check all levels of priority_array
		// Loop back if empty 

		while(scheduler->priority_array[0]->iter != NULL){

			thread_list_enqueue(scheduler->running, thread_list_dequeue(scheduler->priority_array[0]));

			scheduler->priority_array[0]->iter = scheduler->priority_array[0]->iter->next;
		}
		
		//scheduler->priority_array[0]->iter = scheduler->priority_array[0]->head;


	}



	/* Test prints */
	printf(ANSI_COLOR_YELLOW "The current running queue:\n" ANSI_COLOR_RESET);
	_print_thread_list(scheduler->running);


	printf(ANSI_COLOR_YELLOW "The current priority[0] queue:\n" ANSI_COLOR_RESET);
	_print_thread_list(scheduler->priority_array[0]);


}



/************************************************************************************************************
*
*    MY_PTHREAD CORE LIBRARY 
*
************************************************************************************************************/


int my_pthread_create( my_pthread_t * thread, my_pthread_attr_t * attr, void *(*function)(void*), void * arg){

	/**********************************************************************************
		PTHREAD_T & THREAD_UNIT SETUP   
    **********************************************************************************/

	/* Assuming empty pthread pointer passed in. */
	if ((thread = (my_pthread_t*)malloc(sizeof(my_pthread_t))) == NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	}

	thread->threadID = scheduler->threadID_count;
	scheduler->threadID_count++; 	// Uses some global counter for threadID.  Change method later 

	thread->return_val = NULL;

	/* Assign to highest priority (new threads enter highest priority queue) */
	thread->priority = 0;


	/* Init thread_unit for this pthread */
	thread_unit* new_unit 				= thread_unit_init(thread);			//thread_unit_init() mallocs
	new_unit->waiting_on_me 			= thread_list_init();				//thread_list_init() mallocs


    /**********************************************************************************
		UCONTEXT SETUP   
    **********************************************************************************/

	/* copy (fork) the current ucontext */
	if(getcontext(new_unit->ucontext) < 0){
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
    // errno for < 0 (be sure to cleanup if failure)


    /**********************************************************************************
		SCHEDULE THAT SHIT    
    **********************************************************************************/
	// into highest priority bc heuristics say so 
	//new_unit->state = READY;
	thread_list_enqueue(scheduler->priority_array[0], new_unit);


	

	/*
		By the end of my_pthread_create():
			The incoming pthread ptr will be populated with defaults
			That pthread will be put into a thread_unit_t
			That thread_unit_t's ucontext will be populated 
			That thread_unit will be enqueued into the priority 0 thread list.
		End by calling yield().
	*/

	return 1;
}



void my_pthread_yield(){

	SYS_MODE = 1;

	struct itimerval timer;
	thread_unit* thread_up_next = NULL;

	/* Run next thread in the running queue.  If the running queue is empty, run mait_cycle. */


	if(scheduler->running->iter == NULL){
		// End of running queue.  Run mait cycle.
		maintenance_cycle(); 
	}

	if(!thread_list_isempty(scheduler->running)){

		_print_thread_list(scheduler->running);

		thread_up_next	= scheduler->running->iter;
		scheduler->running->iter 	= scheduler->running->iter->next;
		scheduler->currently_running = thread_up_next;
		printf(ANSI_COLOR_MAGENTA "Switching to Thread %ld...\n"ANSI_COLOR_RESET, thread_up_next->thread->threadID);
	}


	/* Set timer */
    timer.it_value.tv_sec 		= 2;			// Time remaining until next expiration (sec)
    timer.it_value.tv_usec 		= TIME_QUANTUM;	// "" (50 ms).  This will be changed to thread_unit->time_slice
    // timer.it_value.tv_usec 		= scheduler->currently_running->time_slice;	// "" (50 ms)
    timer.it_interval.tv_sec 	= 0;			// Interval for periodic timer (sec)
    timer.it_interval.tv_usec 	= 0;			// "" (microseconds)
    setitimer(ITIMER_REAL, &timer, NULL);
	SYS_MODE = 0;

    /* Run next thread in line */
	scheduler_runThread(thread_up_next);


}

void my_pthread_exit(void *value_ptr){

	SYS_MODE = 1;

	if(scheduler->currently_running->state == TERMINATED){
		printf("This thread, TID %ld, has already been terminated.\n", scheduler->currently_running->thread->threadID);
	}

	scheduler->currently_running->state = TERMINATED;
	scheduler->currently_running->thread->return_val = value_ptr;

	/* Free malloced memory and cleanup */
	free(scheduler->currently_running->thread);
	free(scheduler->currently_running->ucontext->uc_stack.ss_sp);
	free(scheduler->currently_running->ucontext);
	//	free(scheduler->currently_running);  // Keep the thread_unit around for scheduler book keeping 

	my_pthread_yield();

}


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
*    DEBUGGING / TESTING
*
************************************************************************************************************/

/*
	Function for testing
*/
void f1(int x){

	while(1){
		sleep(1);
		printf("Executing f1:\tArg is %i\n", x);
	
	}

	my_pthread_exit(NULL);
}




void _debugging_pthread_yield(){


	printf(ANSI_COLOR_RED "\n\nRunning pthread_yield() debug test...\n\n" ANSI_COLOR_RESET);
	
	int NUM_PTHREADS = 5;


	/* Get main's ucontext */
	main_ucontext = (ucontext_t*)malloc(sizeof(ucontext_t));
	main_ucontext->uc_stack.ss_sp = malloc(PAGE_SIZE);
	main_ucontext->uc_stack.ss_size = PAGE_SIZE;
	getcontext(main_ucontext);
	makecontext(main_ucontext, (void*)_debugging_pthread_yield, 1, NULL);

	/* init scheduler...calls sig handler */
	scheduler_init();

	my_pthread_t* pthread_array[NUM_PTHREADS];
	my_pthread_attr_t* 	useless_attr;
	int i;

	for(i=0; i<NUM_PTHREADS;i++){

		pthread_array[i] = (my_pthread_t*)malloc(sizeof(my_pthread_t));

		if(my_pthread_create(pthread_array[i], useless_attr, (void*)f1, (void*) i)){
			printf(ANSI_COLOR_GREEN "Successfully created pthread and enqueued.\n" ANSI_COLOR_RESET);
		}
	} 

	printf("\nPrinting thread list.  Should include pthreads 2 to 12.\n");
	_print_thread_list(scheduler->priority_array[0]);



		

	while(1);

	printf(ANSI_COLOR_RED "End pthread_create() debug test." ANSI_COLOR_RESET);

}



int main(){

	// _debugging_thread_unit_lib();
	// _debugging_pthread_create();

	_debugging_pthread_yield();

}
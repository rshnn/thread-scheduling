/************************************************************************************************************
*	cs518 - assignment01 
*	thread scheduling 
*
*	my_pthread_t.c 	
************************************************************************************************************/

#include "my_pthread_t.h"
#include "thread_unit_lib.h" 	

static scheduler_t* scheduler;

int 				SYS_MODE 				= 0;
int 				scheduler_initialized 	= 0;
int 				first_run_complete		= 0;
int					mutex_count = 0;
int 				test_counter = 0;

my_pthread_mutex_t mutex;

thread_unit*		maintenance_thread_unit;
thread_unit* 		main_thread_unit;



/************************************************************************************************************
*
*    SCHEDULER LIBRARY 
*
************************************************************************************************************/

void priority_level_sort(){
	int i;
	int count = 0;
	thread_unit* iter = NULL;
	thread_unit* current = NULL;
	thread_unit* temp;
	
	_print_thread_list(scheduler->priority_array[0]);
	// if(thread_list_isempty(scheduler->priority_array[0])){
	// 	return;
	// }

	for(i = 0; i < PRIORITY_LEVELS;i++){
		printf("Sorting priority %d \n",i);
		current = scheduler->priority_array[i]->head; //
		if(current == NULL){
			// printf("Priority %d is empty\n",i);
			continue;
		}
		iter 	= current->next;
		current->next = NULL;
		while(iter != NULL){
			current = scheduler->priority_array[i]->head;
			// _print_thread_unit(current);
			// _print_thread_unit(iter);
			// printf("\n\n");
			if(iter->run_count < scheduler->priority_array[i]->head->run_count){
				temp = iter->next;
				iter->next = scheduler->priority_array[i]->head;
				scheduler->priority_array[i]->head = iter;
				iter = temp;	
			}else {

				temp = iter->next;
				while(current->next != NULL && current->next->run_count < iter->run_count){
					current = current->next;
				}
				iter->next = current->next;
				current->next = iter;
				iter = temp;
			}
		}
		current = scheduler->priority_array[i]->head;
		while(current != NULL){
			//printf("TID %ld: run_count: %d \n",current->thread->threadID,current->run_count);
			if(current->next == NULL){
				scheduler->priority_array[i]->tail = current;
			}
			current = current->next;
		}
		scheduler->priority_array[i]->iter = scheduler->priority_array[i]->head->next; 
	}	
}


void scheduler_runThread(thread_unit* thread, thread_unit* prev){



	if(thread == NULL){
		printf(ANSI_COLOR_RED "Attempted to schedule a NULL thread\n"ANSI_COLOR_RESET);
		return;
	}

	// if(scheduler->currently_running->state == RUNNING){
	// 	scheduler->currently_running->state = READY;
	// }

	
	scheduler->currently_running 		= thread;
	// scheduler->currently_running->state = RUNNING;

	/* Will execute function that thread points to. */ 
	

	if(prev == NULL && first_run_complete == 0){
		// Intialize case
		first_run_complete = 1; 
		if ((swapcontext(main_thread_unit->ucontext, thread->ucontext)) < 0)
			printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	}
	else if(prev == NULL && first_run_complete == 1){
		if ((swapcontext(maintenance_thread_unit->ucontext, thread->ucontext)) < 0)
			printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);

	}
	else{
		if ((swapcontext(prev->ucontext, thread->ucontext)) < 0)
			printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	}

	/* Context will switch back to scheduler either when thread completes or at a SIGARLM */
}


void scheduler_sig_handler(){


	if(SYS_MODE == 1){
		printf("TIMER: In the middle of shit.\n");
		return;
	}

    
    if(scheduler->currently_running != NULL){
    	printf(ANSI_COLOR_GREEN "\nSIGALRM\tThread %ld just ran for %i microseconds. \n\tInterrupting Thread %ld.\n" 
    			ANSI_COLOR_RESET, 
    			scheduler->currently_running->thread->threadID, 
    			scheduler->currently_running->time_slice,
    			scheduler->currently_running->thread->threadID);
		
    }else{
    	printf(ANSI_COLOR_GREEN "\nSIGALRM\tNo threads have run yet.\n" ANSI_COLOR_RESET);

    }
    

    my_pthread_yield();
}


void maintenance_cycle(){

	int i;
	thread_unit* temp;

	
	printf(ANSI_COLOR_YELLOW "\n------------------------------------------------" ANSI_COLOR_RESET);
	printf(ANSI_COLOR_YELLOW "\nMaintenance Cycle\n" ANSI_COLOR_RESET);

	printf(ANSI_COLOR_YELLOW "The old running queue:\n"ANSI_COLOR_RESET);
	_print_thread_list(scheduler->running);

	printf("\n");
	for(i=0; i<PRIORITY_LEVELS; i++){
		printf(ANSI_COLOR_YELLOW "The old queue with threads of priority %d:\n" ANSI_COLOR_RESET, i);
		_print_thread_list(scheduler->priority_array[i]);
	}
	printf("\n");


	/**********************************************************************************
		Priority Adjustments	 TODO:  all of this 
	**********************************************************************************/


	/* Adjust priorities of all threads by increasing priority by 1 */
	for(i=1; i<PRIORITY_LEVELS; i++){

		while(!thread_list_isempty(scheduler->priority_array[i])){

			if((temp = thread_list_dequeue(scheduler->priority_array[i])) != NULL){
				int new_priority = temp->priority;
				new_priority--;
				temp->priority = new_priority;
				thread_list_enqueue(scheduler->priority_array[new_priority], temp);
			}

		}
	}


	priority_level_sort();

	printf("Done sorting.\n");
	/**********************************************************************************
		ADD RUNNING BACK IN 	 
	**********************************************************************************/
	

	while(!thread_list_isempty(scheduler->running)){

		if((temp = thread_list_dequeue(scheduler->running)) != NULL){

			printf("after dequeue\n");

			if(temp->state != TERMINATED){
				/* lower a thread's priority before putting it back into multi-priority	queue */
			
				printf("after terminated\n");

				if(temp->priority > (PRIORITY_LEVELS/3) && 
					(!thread_list_isempty(temp->waiting_on_me) || temp->mutex_next != NULL)){
					/* Case to handle priority inversion */
					printf(ANSI_COLOR_MAGENTA "This thread (TID %ld) is valued by others."  
											"Send to Priorit %i.\n"
							ANSI_COLOR_RESET, temp->thread->threadID, (PRIORITY_LEVELS/3));
					temp->priority = (PRIORITY_LEVELS/3);
			
				}else{
					int new_priority = temp->priority;
					new_priority++;
					if(new_priority >= PRIORITY_LEVELS){
						new_priority = PRIORITY_LEVELS-1;
					}
					temp->priority = new_priority;
				}

				
				thread_list_enqueue(scheduler->priority_array[temp->priority], temp);
			}else{
				printf("in else\n");
				_print_thread_unit(temp);
				
				// free(temp->ucontext->uc_stack.ss_sp);
				printf("success 1\n");
				// free(temp->ucontext);
				printf("success 2\n");
				// free(temp);  // Free the thread_unit in maint_cycle after ucontext 
				printf("success 3\n");
				continue;
			}

		
		}
	}


	/**********************************************************************************
		Populate running queue
	**********************************************************************************/

	/* Collect MAINT_CYCLE number processes to run and put them into running queue */
	// int i; 
	scheduler->running->head = NULL;
	scheduler->running->tail = NULL;
	scheduler->running->iter = NULL;

	
	int j;
	int num_quanta = 0;
	int scheduled_time = 0;
	/* Picks (PRIORITY_LEVELS - scheduler priority level) number of threads from each scheduler priority level */
	for(i=0; i<PRIORITY_LEVELS; i++){
		num_quanta = i + 1;

		for(j=0; j<PRIORITY_LEVELS-i; j++){
			thread_unit* temp;
			/* if adding thread increases beyond RUNNING_TIME, break */
			if((scheduled_time+num_quanta) > RUNNING_TIME){
				break;
			}

			if((temp = thread_list_dequeue(scheduler->priority_array[i])) != NULL){
				if(temp->state == READY){
					scheduled_time += num_quanta;
					temp->time_slice = TIME_QUANTUM * num_quanta;
					thread_list_enqueue(scheduler->running, temp);
				}else{
					thread_list_enqueue(scheduler->priority_array[i], temp);
				}
			}
		}
	}


	/* Test prints */

	// printf(ANSI_COLOR_YELLOW "The new waiting queue:\n"ANSI_COLOR_RESET);
	// _print_thread_list_wait(scheduler->waiting);

	printf(ANSI_COLOR_YELLOW "The new running queue:\n" ANSI_COLOR_RESET);
	_print_thread_list(scheduler->running);

	for(i=0; i<PRIORITY_LEVELS; i++){
		printf(ANSI_COLOR_YELLOW "The current queue with threads of priority %d:\n" ANSI_COLOR_RESET, i+1);
		_print_thread_list(scheduler->priority_array[i]);
	}


	printf(ANSI_COLOR_YELLOW "------------------------------------------------\n\n" ANSI_COLOR_RESET);
}


void scheduler_init(){

    /**********************************************************************************
		INITIALIZE SCHEDULER STRUCTURES   
    **********************************************************************************/

	int i;
	SYS_MODE = 1;

	/* Malloc space for scheduler */
	if ((scheduler = (scheduler_t*)malloc(sizeof(scheduler_t))) == NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	}

	scheduler->threadID_count 		= 2;	// Reserve 0 and 1 for scheduler and main threads respectively 
	scheduler->currently_running 	= NULL;

	/* Initialize each priority queue (thread_unit_list) */
	for(i = 0; i<= PRIORITY_LEVELS; i++){
		scheduler->priority_array[i] = thread_list_init();	// thread_list_init() mallocs
	}

	/* Initialize the currently running and waiting queues */
	scheduler->running = thread_list_init();
	scheduler->waiting = thread_list_init();

	/**********************************************************************************
		SETUP THREAD_UNITS FOR MAINTENANCE AND MAIN THREADS   
    **********************************************************************************/

	if((main_thread_unit = (thread_unit*)malloc(sizeof(thread_unit))) == NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
		exit(-1);		
	}

	if((maintenance_thread_unit = (thread_unit*)malloc(sizeof(thread_unit))) == NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
		exit(-1);		
	}

	main_thread_unit->state = READY;
	main_thread_unit->time_slice = TIME_QUANTUM;
	main_thread_unit->run_count = 1;
	main_thread_unit->wait_next = NULL;
	main_thread_unit->waiting_on_me = thread_list_init();
	main_thread_unit->next = NULL;

	main_thread_unit->thread = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	main_thread_unit->thread->threadID = 1;
	main_thread_unit->thread->priority = 0;
	main_thread_unit->thread->thread_unit = main_thread_unit;


	/* MAIN UCONTEXT SETUP */

	/* Attempt to malloc space for main_ucontext */
	if ((main_thread_unit->ucontext = (ucontext_t*)malloc(sizeof(ucontext_t))) == NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
		exit(-1);
	}

	/* copy (fork) the current ucontext */
	if(getcontext(main_thread_unit->ucontext) == -1){
		printf("Error obtaining ucontext of scheduler");
		exit(-1);
	} 

	/* Set up ucontext stack */
	// if((main_thread_unit->ucontext->uc_stack.ss_sp = malloc(PAGE_SIZE))==NULL){
	// 	printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	// 	exit(-1);
	// }
	main_thread_unit->ucontext->uc_stack.ss_sp 		= main_thread_unit->stack;
	main_thread_unit->ucontext->uc_stack.ss_size 	= PAGE_SIZE;

	/* Set uc_link to point to addr of main_ucontext */
	main_thread_unit->ucontext->uc_link 			= main_thread_unit->ucontext;

	/* Assign func* to ucontext */
	makecontext(main_thread_unit->ucontext, (void*)scheduler_sig_handler, 1, NULL); 	// Should we write a separate scheduler_run_thread call?
    

	/* MAINTENANCE UCONTEXT SETUP */
	
	/* Attempt to malloc space for maintenance_ucontext */
	if ((maintenance_thread_unit->ucontext = (ucontext_t*)malloc(sizeof(ucontext_t))) == NULL){
		printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
		exit(-1);
	}

	/* copy (fork) the current ucontext */
	if(getcontext(maintenance_thread_unit->ucontext) == -1){
		printf("Error obtaining ucontext of scheduler");
		exit(-1);
	} 

	/* Set up ucontext stack */
	// if((maintenance_thread_unit->ucontext->uc_stack.ss_sp = malloc(PAGE_SIZE))==NULL){
	// 	printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	// 	exit(-1);
	// }

	maintenance_thread_unit->ucontext->uc_stack.ss_sp 	= maintenance_thread_unit->stack;	
	maintenance_thread_unit->ucontext->uc_stack.ss_size 	= PAGE_SIZE;


	/* Set uc_link to point to addr of maintenance_ucontext */
	maintenance_thread_unit->ucontext->uc_link 			= main_thread_unit->ucontext;

	/* Assign func* to ucontext */
	makecontext(maintenance_thread_unit->ucontext, (void*)maintenance_cycle, 1, NULL); 	// Should we write a separate scheduler_run_thread call?
	
	/* </end> UCONTEXT STUFF */     



	/* Enqueue main thread_unit for scheduling */
	thread_list_enqueue(scheduler->priority_array[0], main_thread_unit);
	// FYI.  scheduler_init ends with a call to yield.  Yield will call 
	// 			the maint_cycle.  The maint_cycle will push the main_thread
	// 			to the running queue.


	/* TODO:  One of these makes the other obsolete.  Remove one.*/
	scheduler->initialized = 1;
	scheduler_initialized = 1;


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
	my_pthread_yield();

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

	/* ASSUME PTHREAD ALREADY HAS SPACE ALLOCATED TO IT FROM USER */
	SYS_MODE = 1;


	/* On first call of create(), init the scheduler. */
	if(scheduler_initialized == 0){
		scheduler_init();
	}



	// if ((thread = (my_pthread_t*)malloc(sizeof(my_pthread_t))) == NULL){
	// 	printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	// }

	thread->threadID = scheduler->threadID_count;
	scheduler->threadID_count++; 	// Uses some global counter for threadID.  Change method later 

	thread->return_val = NULL;


	/* Init thread_unit for this pthread */
	thread_unit* new_unit 				= thread_unit_init(thread);			//thread_unit_init() mallocs
	
	/* pthread struct holds a pointer to it's containing thread_unit struct */
	thread->thread_unit 				= new_unit;

    /**********************************************************************************
		UCONTEXT SETUP   
    **********************************************************************************/

	/* copy (fork) the current ucontext */
	if(getcontext(new_unit->ucontext) < 0){
		printf("Error obtaining ucontext of thread");
		exit(-1);
	} 

	/* Set up ucontext stack */
	// if((new_unit->ucontext->uc_stack.ss_sp = malloc(PAGE_SIZE))==NULL){
	// 	printf("Errno value %d:  Message: %s: Line %d\n", errno, strerror(errno), __LINE__);
	// }
	new_unit->ucontext->uc_stack.ss_sp 	= new_unit->stack;
	new_unit->ucontext->uc_stack.ss_size 	= PAGE_SIZE;
		/* wtf is happening here^?  I found this online somewhere.  Is it working?*/


	/* Set uc_link to point to addr of scheduler's ucontext */
	new_unit->ucontext->uc_link 			= maintenance_thread_unit->ucontext;

	/* Assign func* to ucontext */
	makecontext(new_unit->ucontext, (void*)function, 1, arg); 		
	// Should we write a separate scheduler_run_thread call?
    // errno for < 0 (be sure to cleanup if failure)


    /**********************************************************************************
		SCHEDULE THAT SHIT    
    **********************************************************************************/
	// into highest priority bc heuristics say so 

	new_unit->priority 	= 0;
	new_unit->state 	= READY;
	thread_list_enqueue(scheduler->priority_array[0], new_unit);

	

	/*
		By the end of my_pthread_create():
			The incoming pthread ptr will be populated with defaults
			That pthread will be put into a thread_unit_t
			That thread_unit_t's ucontext will be populated 
			That thread_unit will be enqueued into the priority 0 thread list.
		End by calling yield().
	*/

	SYS_MODE = 0;
	
	// my_pthread_yield();
	

	return 1;
}


void my_pthread_yield(){
	/* 
		Run next thread in the running queue.  If the running queue is empty, run maint_cycle. 
		
		When this function is called, scheduler->running->iter is already pointing to the 
		next thread to be run.  scheduler->currently_running is pointing to the thread that just
		finished execution.
	*/

	SYS_MODE = 1;

	struct itimerval 	timer;
	thread_unit* 		thread_up_next = NULL;
	thread_unit* 		prev = scheduler->currently_running;
	int 				runs = 0;

	/* Currently_running is the one that just finished running */
	if(!thread_list_isempty(scheduler->running) && scheduler->currently_running != NULL){
		if((scheduler->currently_running->state != TERMINATED) &&
			(scheduler->currently_running->state != WAITING)){
			scheduler->currently_running->state = READY;
		}
		runs = scheduler->currently_running->time_slice / TIME_QUANTUM;
		scheduler->currently_running->run_count += runs;
	}


	if(scheduler->running->iter == NULL){
		// End of running queue.  Run maint cycle.
		maintenance_cycle(); 
	}

	if(!thread_list_isempty(scheduler->running)){

		thread_up_next						= scheduler->running->iter;
		scheduler->running->iter 			= scheduler->running->iter->next;
		scheduler->currently_running 		= thread_up_next;
		thread_up_next->state 				= RUNNING;
    	timer.it_value.tv_usec 				= scheduler->currently_running->time_slice;	// "" (50 ms)

		printf(ANSI_COLOR_GREEN "\tSwitching to Thread %ld...\n"ANSI_COLOR_RESET, thread_up_next->thread->threadID);
	}else{
	    timer.it_value.tv_usec 		= TIME_QUANTUM;	// wait 50ms if running queue is empty. 

	}


	/* Set timer */
    timer.it_value.tv_sec 		= 1;			// Time remaining until next expiration (sec)
    timer.it_interval.tv_sec 	= 0;			// Interval for periodic timer (sec)
    timer.it_interval.tv_usec 	= 0;			// "" (microseconds)
    setitimer(ITIMER_REAL, &timer, NULL);


	SYS_MODE = 0;

    /* Run next thread in line */
	scheduler_runThread(thread_up_next, prev);
}


void my_pthread_exit(void *value_ptr){


	SYS_MODE = 1;

	printf(ANSI_COLOR_GREEN"\tThe thread (TID: %ld) is exiting.\n" ANSI_COLOR_RESET, 
		scheduler->currently_running->thread->threadID);

	if(scheduler->currently_running->state == TERMINATED){
		printf("This thread, TID %ld, has already been terminated.\n", scheduler->currently_running->thread->threadID);
	}


	// if(scheduler->currently_running->waiting_on_me != NULL){
	// 	printf("\tThese are the guys waiting on thread %ld:\n", 
	// 		scheduler->currently_running->thread->threadID);
	// 	_print_thread_list_wait(scheduler->currently_running->waiting_on_me);
	// }


	thread_unit* temp;
	while(!thread_list_isempty(scheduler->currently_running->waiting_on_me)){

		if((temp = thread_list_dequeue_wait(scheduler->currently_running->waiting_on_me)) != NULL){
			// _print_thread_unit(temp);
			// printf("\tThread %ld is now ready.\n", temp->thread->threadID);
			temp->state = READY;
			temp->thread->return_val = value_ptr;

		}	
	}


	scheduler->currently_running->state 			= TERMINATED;
	// scheduler->currently_running->thread->threadID 	= -1;


	my_pthread_yield();
}


int my_pthread_join(my_pthread_t thread, void **value_ptr){

	/* 
		TODO: Test 
	*/
	if(thread.thread_unit == NULL){
		printf("Thread to join does not exist\n");
		return -1;
	}
	if(scheduler->currently_running->thread->threadID == thread.threadID){
		printf("Trying to join itself. curr_running->thread->threadID: %ld and thread.threadID: %ld\n",
		 scheduler->currently_running->thread->threadID, thread.threadID);
		return -1;
	}
	if(scheduler->currently_running->state != RUNNING){
		printf("Not possible to join because I am not ready\n");
		return -1;
	}

	printf("\tI am joining Thread %ld\n", thread.threadID);

	scheduler->currently_running->joinedID = thread.threadID;
	scheduler->currently_running->state = WAITING;
	//my_pthread_yield();
	// scheduler->currently_running->thread->return_val = value_ptr;

	thread_list_enqueue_wait(thread.thread_unit->waiting_on_me, scheduler->currently_running);

	// thread_list_enqueue_wait(scheduler->waiting, scheduler->currently_running);
	// thread_list_enqueue(scheduler->priority_array[scheduler->currently_running->thread->priority], scheduler->currently_running);

	/*******************************************************/

	// /* 
	// 	TEMP FOR TESTING
	// 		Adding curr_running to the waiting queue.  This job will be done by the scheduler.
	// 		Move this stuff there when ready.
	// 		NOTE:  using wait_next. NOT the enqueue functions. 
	// */

	// thread_unit* unit 	= scheduler->currently_running;
	// //unit->wait_next 	= NULL;

	// // Enqueue on an empty list
	// if(thread_list_isempty(scheduler->waiting)){
	// 	scheduler->waiting->head 			= unit;
	// 	scheduler->waiting->tail 			= unit;
	// 	scheduler->waiting->iter 			= unit;

	// // Default: Add at end and redirect tail
	// }else{
	// 	scheduler->waiting->tail->wait_next		= unit;
	// 	scheduler->waiting->tail 				= unit;
	// }

	// scheduler->waiting->size++;
	// /*******************************************************/	




	my_pthread_yield();
	if(value_ptr != NULL){
		*value_ptr = scheduler->currently_running->thread->return_val;
	}
	return 0;
}


/************************************************************************************************************
*
*    MY_PTHREAD_MUTEX LIBRARY 
*
************************************************************************************************************/

int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const my_pthread_mutexattr_t *mutexattr){
	SYS_MODE = 1;
	
	if(scheduler_initialized == 0){
		scheduler_init();
	}

	if(mutex->initialized == 1){
		printf("Mutex is already initialized\n");
		return -1;
	}
	mutex->initialized = 1;
	mutex->id = mutex_count;
	mutex_count++;
	mutex->waiting_queue = NULL;
	resetTheTimer();
	SYS_MODE = 0;
	return 0;
}
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex){
	//idea here is to pass ownership of the lock to next in queue instead of checking if the lock is available
	//only do we actually set the lock to 1 once we call lock for an empty queue
	thread_unit* temp; 
	if(mutex->initialized == 0){
		printf("Trying to lock an uninitialized mutex\n");
		return -1;
	}
	if(mutex->lock == 1 && mutex->owner == scheduler->currently_running->thread->threadID){
		printf("You own the lock, why are you trying to lock it?\n");
		return -1;
	}
	if(mutex->lock == 1){
		// Lock is currently in use & wait_queue is empty 
		SYS_MODE = 1;
		temp = mutex->waiting_queue;
		printf("lock is in use, adding myself to the mutex waiting queue for lock %d\n",mutex->lock); 
		if(temp == NULL){
			temp = scheduler->currently_running;
			mutex->waiting_queue = temp;

			//mutex->waiting_queue = scheduler->currently_running;
			printf("TID %ld has been added to the waiting queue.\n",scheduler->currently_running->thread->threadID);
			scheduler->currently_running->state = WAITING;
			_print_thread_unit(scheduler->currently_running);
			my_pthread_yield();

			printf("I have been revived.\n");
			_print_thread_unit(scheduler->currently_running);
			//when thread resumes, it should be getting the lock
			mutex->lock = 1;
			mutex->owner = scheduler->currently_running->thread->threadID;
			return 0;
		}

		while(temp->mutex_next != NULL){ 
		//add thread to the end of the queue
		//use a thread list here?  but then we need to create functions using mutex_next instead of next
			temp = temp->mutex_next;
		}
		temp->mutex_next = scheduler->currently_running;
		scheduler->currently_running->state = WAITING;
		my_pthread_yield();
		printf("\tI got the lock. TID %ld\n", scheduler->currently_running->thread->threadID);
		mutex->lock = 1;
		mutex->owner = scheduler->currently_running->thread->threadID;
		return 0;
	}

	/* Lock says its available, but its waiting_queue is not empty  */
	if(mutex->lock == 0 && mutex->waiting_queue != NULL){
		printf("wait your turn\n");
		SYS_MODE = 1;
		temp = mutex->waiting_queue;
		while(temp->mutex_next != NULL){
			temp = temp->mutex_next;
		}
		temp->mutex_next = scheduler->currently_running;
		scheduler->currently_running->state = WAITING;
		my_pthread_yield();
		mutex->lock = 1;
		mutex->owner = scheduler->currently_running->thread->threadID;
		return 0;
	}

	/* First person to claim lock */
	if(mutex->lock == 0 && mutex->waiting_queue == NULL){
		SYS_MODE = 1;
		mutex->lock = 1;
		mutex->owner = scheduler->currently_running->thread->threadID;
		resetTheTimer();
		return 0;
	}
	printf("panic\n");
	return -1;
}
/* Resets timer to 4 TQ */
void resetTheTimer(){


	int 				TIMER_MULTIPLE 	= 3;
	struct itimerval 	timer;
	thread_unit* 		thread_up_next = NULL;
	thread_unit* 		prev = scheduler->currently_running;

	printf("Resetting timer.\n");
	/* Set timer */
    timer.it_value.tv_sec 		= 0;			// Time remaining until next expiration (sec)
    timer.it_value.tv_usec 		= TIMER_MULTIPLE*TIME_QUANTUM;	// wait 50ms if running queue is empty. 
    timer.it_interval.tv_sec 	= 0;			// Interval for periodic timer (sec)
    timer.it_interval.tv_usec 	= 0;			// "" (microseconds)
    setitimer(ITIMER_REAL, &timer, NULL);


	SYS_MODE = 0;
	return;
}

int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex){
	if(mutex->initialized == 0){
		printf("Trying to unlock an uninitialized mutex\n");
		return -1;
	}
	if(mutex->lock == 0){
		printf("nothing to unlock\n");
		return -1;
	}
	if(mutex->lock == 1 && mutex->owner != scheduler->currently_running->thread->threadID){
		printf("not the owner so not unlocking\n");
		return -1;
	}
	if(mutex->lock == 1 && mutex->owner == scheduler->currently_running->thread->threadID){
		
		SYS_MODE = 1;
		if(mutex->waiting_queue == NULL){
			mutex->lock = 0;
			mutex->owner = -1; // because thread 0 is main
			my_pthread_yield();
			//resetTheTimer();
			return 0;
		}else{
			mutex->waiting_queue->state = READY;
			mutex->waiting_queue = mutex->waiting_queue->mutex_next;
			mutex->owner = -1;
			//leave it locked so nobody sneaks in for a steal
			my_pthread_yield();
			//resetTheTimer();
			return 0;
		}
	}
	printf("panic\n");
	return -1;
}
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex){

	SYS_MODE = 1;
	if(mutex->initialized == 0){
		printf("Trying to destroy an uninitialized mutex\n");
		my_pthread_yield();
		return -1;
	}
	// if(mutex->lock == 1 && mutex->waiting_queue!= NULL){
	// 	printf("Cant destory while others are waiting for the lock\n");
	// 	//we should force release or we might hit a deadlock
	// 	my_pthread_mutex_unlock(&mutex);
	// 	return 0;
	// }
	if(mutex->lock == 1 && mutex->owner == scheduler->currently_running->thread->threadID && mutex->waiting_queue == NULL){
		SYS_MODE =1;
		printf("you are the owner of the lock and can freely destroy it");
		mutex->initialized = 0;
		mutex->owner = -1;
		mutex->lock = 0;
		resetTheTimer();
		//no memory was allocated so its on the user to free
		return 0;
	}
	if(mutex->lock == 0 && mutex->owner == -1){
		SYS_MODE =1;
		printf("no one waiting for the lock so its safe to destroy\n");
		mutex->initialized = 0;
		//no memory was allocated so its on the user to free
		resetTheTimer();
		return 0;
	}

	//panic?
	printf("panic!\n");
	return 0;
}


int my_pthread_mutex_trylock(my_pthread_mutex_t* mutex){

	if(mutex->lock == 1){
		return -1;
	}else{
		my_pthread_mutex_lock(mutex);
		return 0;
	}

}
	

/************************************************************************************************************
*
*    DEBUGGING / TESTING
*
************************************************************************************************************/

/*
	Function for testing
		Loop: sleep for 0.5s, and then print.
*/
void f1(int x){

	while(1){
		// usleep(500000);
		// printf("\tExecuting f1:\tArg is %i\n", x);
	
	}

	my_pthread_exit(NULL);
}

void f2(int x){


	//printf("\tExecuting f2:\tArg is %i.\n", x);

	sleep(10);
	
	// while(1){
	// 	usleep(100000);
	// 	printf("\tExecuting f2:\tArg is %i.\n", x);
	// }
	printf("\tf2 completes execution.\n");

	char* a;
	a = (char*) malloc(sizeof(char));

    strcpy(a,"hello world");
    my_pthread_exit((void*)a);

}

void f3(my_pthread_t* thread){

	printf("\tExecuting f3\tJoins thread %ld\n", thread->threadID);

	char* receive;

	my_pthread_join(*thread, (void**)&receive);
	

	printf("\tI got this: %s\n", (char*) receive);
	my_pthread_exit(NULL);

}



void m1(my_pthread_t* thread){

	my_pthread_mutex_lock(&mutex);

	printf("TID %ld got the lock", thread->threadID);	

	test_counter+= thread->threadID;

	printf("\tI changed the counter to: %i\n", test_counter);

	sleep(5);
	printf("Im awake.\n");

	my_pthread_mutex_unlock(&mutex);
	printf("\tTID %ld is releasing the lock.", thread->threadID);

	my_pthread_exit(NULL);
}



void _debugging_pthread_join(){

	printf(ANSI_COLOR_RED "\n\nRunning pthread_join() debug test...\n\n" ANSI_COLOR_RESET);
	
	int NUM_PTHREADS = 5;


	// my_pthread_t pthread_array[NUM_PTHREADS];
	my_pthread_t* pthread_array = (my_pthread_t*)malloc(NUM_PTHREADS * sizeof(my_pthread_t));
	my_pthread_attr_t* useless_attr;

	int i;

	for(i=0; i<NUM_PTHREADS;i++){

		/* TID 2: Give last pthread functionptr to f2 (f2 terminates after a few seconds) */
		if(i == 0){
			if(my_pthread_create(&pthread_array[i], useless_attr, (void*)f2, (void*) i)){
				printf(ANSI_COLOR_GREEN "Successfully created f2 pthread and enqueued. TID %ld\n" 
					ANSI_COLOR_RESET, pthread_array[i].threadID);
			}
			continue;
		}
		
		/* TID 3:  f3 (joins TID2) */
		if(i == 1){
			if(my_pthread_create(&pthread_array[i], useless_attr, (void*)f3, (void*) &pthread_array[0])){
				printf(ANSI_COLOR_GREEN "Successfully created f3 pthread and enqueued. TID %ld\n" 
					ANSI_COLOR_RESET, pthread_array[i].threadID);
			}
			continue;
		}

		/* TID 4: f3 (joins TID2) */
		if(i == 2){
			if(my_pthread_create(&pthread_array[i], useless_attr, (void*)f3, (void*) &pthread_array[0])){
				printf(ANSI_COLOR_GREEN "Successfully created f3 pthread and enqueued. TID %ld\n" 
					ANSI_COLOR_RESET, pthread_array[i].threadID);
			}
			continue;
		}




		if(my_pthread_create(&pthread_array[i], useless_attr, (void*)f1, (void*) i)){
			printf(ANSI_COLOR_GREEN "Successfully created f1 pthread and enqueued. TID %ld\n" 
					ANSI_COLOR_RESET, pthread_array[i].threadID);
		}
	} 



	printf("\nPrinting priority array 0 (Inside main).  Should include pthreads 2 to 6.\n");
	_print_thread_list(scheduler->priority_array[0]);


	/* Main joins on thread2 */
	my_pthread_join(pthread_array[0], NULL);

	while(1){
		usleep(500000);
		printf("\tExecuting main!\n");
	}
}



void _debugging_pthread_mutex(){

		printf(ANSI_COLOR_RED "\n\nRunning pthread_join() debug test...\n\n" ANSI_COLOR_RESET);
	
	int NUM_PTHREADS = 5;


	// my_pthread_t pthread_array[NUM_PTHREADS];
	my_pthread_t* pthread_array = (my_pthread_t*)malloc(NUM_PTHREADS * sizeof(my_pthread_t));
	my_pthread_attr_t* useless_attr;
	my_pthread_mutexattr_t* useless_mattr;

	my_pthread_mutex_init(&mutex, useless_mattr);

	int i;

	for(i=0; i<NUM_PTHREADS;i++){

		/* TID 2: grab */
		if(my_pthread_create(&pthread_array[i], useless_attr, (void*)m1, (void*) &pthread_array[i])){
			printf(ANSI_COLOR_GREEN "Successfully created f2 pthread and enqueued. TID %ld\n" 
				ANSI_COLOR_RESET, pthread_array[i].threadID);
		}

		

	} 

	printf("\nPrinting priority array 0 (Inside main).  Should include pthreads 2 to 6.\n");
	_print_thread_list(scheduler->priority_array[0]);


	/* Main joins on thread2 */
	//my_pthread_join(pthread_array[0], NULL);

	while(1){
		usleep(500000);
		printf("\tExecuting main!\n");
	}


}

int main(){

	 
		// View the debugging.c file to view the old debugging functions.
		// NOTE:  They might not all work.  Stuff might have changed since then 
	

	// _debugging_thread_unit_lib();
	// _debugging_pthread_create();
	// _debugging_pthread_yield();

	_debugging_pthread_mutex();
}


#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include "my_pthread_t.h"

int 				test_counter1 = 0;
int 				test_counter2 = 0;

pthread_mutex_t test_mutex1;
pthread_mutex_t test_mutex2;


void m1(pthread_t* thread){

	pthread_mutex_lock(&test_mutex1);

	// printf("\tm1: I (TID %ld) got the lock", thread->threadID);	

	// test_counter+= thread->threadID;
	test_counter1++;
	printf("\tm1: I changed the counter1 to:\t\t"ANSI_COLOR_RED" %i\n"ANSI_COLOR_RESET, test_counter1);

	pthread_yield();
	pthread_mutex_unlock(&test_mutex1);
	pthread_exit(NULL);
}

void m2(pthread_t* thread){

	pthread_mutex_lock(&test_mutex2);

	// printf("\tm2: I (TID %ld) got the lock", thread->threadID);	

	// test_counter+= thread->threadID;
	test_counter2++;
	printf("\tm2: I changed the counter2 to:\t\t"ANSI_COLOR_RED" %i\n"ANSI_COLOR_RESET, test_counter2);

	pthread_yield();
	pthread_mutex_unlock(&test_mutex2);
	pthread_exit(NULL);
}




void test_function(int num){

  	struct timeval start, end;
  	gettimeofday(&start, NULL);


	printf(ANSI_COLOR_RED "\n\nRunning pthread_join() debug test...\n\n" ANSI_COLOR_RESET);
	
	int NUM_PTHREADS = num;


	// my_pthread_t pthread_array[NUM_PTHREADS];
	pthread_t* pthread_array = (pthread_t*)malloc(NUM_PTHREADS * sizeof(pthread_t));
	pthread_attr_t* useless_attr = (pthread_attr_t*)malloc(sizeof(pthread_attr_t));
	pthread_mutexattr_t* useless_mattr = (pthread_mutexattr_t*)malloc(sizeof(pthread_mutexattr_t));

	if(pthread_attr_init(useless_attr)<0){
		printf("1");
	}

	if(pthread_mutexattr_init(useless_mattr)<0){
		printf("2");
	}

	if(pthread_mutex_init(&test_mutex1, useless_mattr)<0){
				printf("3");

	}
	
	if(pthread_mutex_init(&test_mutex2, useless_mattr)<0){
		printf("4");

	}

	int i;

	for(i=0; i<NUM_PTHREADS;i++){

		// For evens
		if(i%2 == 0){
			/* TID all: lock mutex 1.  increment global counter.  unlock.  exit */
			if(pthread_create(&pthread_array[i], useless_attr, (void*)m1, (void*) &pthread_array[i])<0){
				// printf(ANSI_COLOR_GREEN "Successfully created m1 pthread and enqueued. TID %ld\n" 
					// ANSI_COLOR_RESET, pthread_array[i].threadID);
				printf("error\n");
				exit(-1);
			}
		}else{
		// For odds
			/* TID all: lock mutex 2.  increment global counter.  unlock.  exit */
			if(pthread_create(&pthread_array[i], useless_attr, (void*)m2, (void*) &pthread_array[i])<0){
				// printf(ANSI_COLOR_GREEN "Successfully created m2 pthread and enqueued. TID %ld\n" 
					// ANSI_COLOR_RESET, pthread_array[i].threadID);
			}
			printf("error\n");
			exit(-1);
		}


		
	} 



	/* Main joins all pthreads */
	for(i=0; i<NUM_PTHREADS;i++){
		pthread_join(pthread_array[i], NULL);
	}	
	// my_pthread_join(pthread_array[NUM_PTHREADS-1], NULL);
	



  	gettimeofday(&end, NULL);

  	long int total_time = (end.tv_sec*1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);


	
	if((test_counter1 + test_counter2) == num){
		printf(ANSI_COLOR_GREEN"\nSuccessful run with %i threads.\n"ANSI_COLOR_RESET, num);
	}else{
		printf(ANSI_COLOR_RED"\nFailure. Counters are 1: %i and 2: %i but expected %i\n"
			ANSI_COLOR_RESET, test_counter1, test_counter2, num);
	}

	printf("\npriority_levels:\t%i\nrunning_time:\t\t%i\n", PRIORITY_LEVELS,  RUNNING_TIME);
	printf("Total run time is: %ld microseconds.\n", total_time);
	printf(ANSI_COLOR_GREEN"Safely ending.\n"ANSI_COLOR_RESET); 

}

int main(int argc, char **argv){

	 
	// View the debugging.c file to view the old debugging functions.
	// NOTE:  They might not all work.  Stuff might have changed since then 


	// _debugging_thread_unit_lib();
	// _debugging_pthread_create();
	// _debugging_pthread_yield();
	// _debugging_pthread_exit();
	// _debugging_pthread_mutex(how_many_threads_ya_want);

	if(argc != 2){
		printf(ANSI_COLOR_RED"Usage: ./my_pthread_t [NUM_THREADS]\n"ANSI_COLOR_RESET);
		exit(-1);
	}

	int how_many_threads_ya_want = atoi(argv[1]);
	printf("%i\n", how_many_threads_ya_want);

	test_function(how_many_threads_ya_want);
}


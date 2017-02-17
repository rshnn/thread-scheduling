# thread-scheduling

cs-518 osdesign  



## dev-notes


### rshnn Fri 17 Feb 2017 01:58:38 PM EST

Heuristic for scheduling: 
+ At mait_cycle, add 10 threads to the running queue.
    * Dequeue from priority_array levels and enqueue into running
* They will run for their designated time_slice. 
* Once all 10 are complete, run mait_cycle 
    - dequeue one thread_unit from the running queue (NOTE do not alter running queue between mait_cycle runs)
    + Assess the new priority of that thread unit
        * Whats its state?  Terminated?  Waiting?  Ready?
        * How many times has it run already?
        * Etc.
    * Dequeue until running queue is empty
    * Add 10 threads to the now empty running queue from the priority array levels, repeat 

### rshnn Tue 14 Feb 2017 08:18:33 AM EST

Can use the following to redirect the signal handler   
This might be cool to use in `scheduler_init()` if we want to go to another handler in the begining to do some work before scheduler_sig_handler is defaultly used.  

```
signal_action.sa_flags            = SA_SIGINFO; 
signal_action.sa_sigaction        = another_signal_handler;
```


Idunno, sounded cool.  Might not be useful.  

### rshnn Mon 13 Feb 2017 09:16:47 AM EST

Meeting Notes | Sunday 2016.02.12

+ Saad had issues with his microphone.  Look into other outlets for future calls 
+ Pair programmed with kodia (or something)
    * View into single file 
+ Data structure discussion
    * Priority levels will be represented as as array 
        - Each index of the array will represent a priority. (Array[0] = highest)
        - Each index of the array will contain a linked list of thread_units (linked list structure)
            + Thread_units will contain a bunch of info about a thread
                * its ucontext, its state, pointer to pthread_t, time slice, run_count, etc. 
    * The scheduler will manage things using this structure 
+ NOTE:  Have some sort of check before you run pthread_create() to make sure the scheduler is initialized correctly 
+ Consider separating the scheduler from my_pthread library 

**data structures**

* pthread library 
    - `my_pthread_t`
        + dh624: We should keep the user away from important info about this structure.  Do not put ucontext or state in here. 
        + Agreed.  All that logistic stuff was moved into data structures gears toward the scheduler 
        + Contains: 
            * `unsigned int     threadID`
            * `void *           return_val`
            * `int              priority`


    - `my_pthread_mutex`
        + contains:
            * `int initialized`
            * `unsigned int id`
            * `int lock`
                - state of lock
            * `my_pthread_took * owning_thread`
                - Will we instead need a pointer to the thread_unit?
                - Or both?
            * `thread_unit_list* waiting_queue`

* scheduler data structures 

    - `state`
        + enumeration 
            * EMBRYO
            * READY
            * RUNNING
            * WAITING
                - Includes when a thread is yielding/joined and when blocked by a lock
            * TERMINATED
            * ZOMBIE
        + States gathered from an old project dh624 had
                    
    - `thread_unit` _(was named scheduler_unit_t)_
        + Contains: 
            * `my_pthread_t*    thread`
            * `ucontext_t       context`
            * `state            state`
            * `int              time_slice`
            * `int              run_count`
            * `struct           thread_unit* waiting_on_me`
                - linked list of thread_units waiting on this thread to finish
            * `thread_unit*     next`
                - Makes it a node structure for thread_unit_list
    
    - `thread_unit_list`
        + list of thread units
        + Contains:
            * `thread_unit* head`
            * `thread_unit* tail`
            * `int size`

    - `mutex_node`
        + Why not just turn my_ptread_mutex_t into a node?
            * Trying to keep those structures away from users
            * Same reason as why we have a separate thread_unit struct to keep my_pthread_t free of data we dont want the user to see 
        + Contains:  
            * `my_pthread_mutex_t*  mutex`
            * `my_pthread_mutex_t   next`

    - `scheduler`  _(was named thread_god)_
        + Main struct that keeps track of the priority array and mutexes
        + Contains: 
            * `thread_unit_list priority_array[PRIORITY_LEVELS]`
                - An array, length PRIORITY_LEVELS, whose indexes each contain a list of thread_units
                - Each index will represent its own priority level
                - Each thread_unit in the list located at that particular index will be considered at that priority level
                - Ex.  All thread_units in `priority_array[2].thread_unit_list ` will be of priority level 2.
            * `mutex_node* mutex_list`
                + List of mutexes
                + Do we need separate lists for locked and unlocked mutexes?
            - `thread_unit_list running`
            - `thread_unit_list waiting`
                + Do we really need the running&waiting queues?
                + The assignment specifies that we do
                + But the role can be accomplished using states
            - `ucontext_t scheduler_ucontext`
            - `ucontext_t main_ucontext`


### rshnn Fri 10 Feb 2017 03:11:50 PM EST

Boiler plated the header and .c file.  Still need to flesh out the structs we're using.  

So far, only made three structs: 

+ `struct state`
    + Something that holds the state of a thread 
    + Might need to add more states later on?
    
+ `struct my_pthread_t` 
    * For sure need to add more to this struct

+ `struct my_thread_attr_t`
    + No idea what to put in this thing yet
    
- `struct my_pthread_mutex_t`
    + I just made it a simple binary lock for now.

+ `struct my_pthread_mutex_attr_t`
    * ????????

I'll try to find some time later to flesh out a basic scheduler struct



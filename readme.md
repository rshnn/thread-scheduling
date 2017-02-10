# thread-scheduling

cs-518 osdesign  



#### dev-notes

>rrp78 Fri 10 Feb 2017 03:11:50 PM EST

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

# gcc (Ubuntu 4.8.5-4ubuntu2) 4.8.5 (rshnn)
CC = gcc

# -m32 	32-bit mode 
# -O0	No optimization option  
# -g 	Enable gdb debugging 
CFLAGS 		= -m32 -O0
DEBUGGER 	= -g

SOURCE 		= my_pthread_t.h my_pthread_t.c thread_unit_lib.h thread_unit_lib.c memory-manager.c memory-manager.h
TARGET 		= my_pthread_t


# Toggle gdb debugger 
ifeq ($(DEBUG), TRUE)
CFLAGS += $(DEBUGGER)
endif

make: 
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

debug: 
	make DEBUG=TRUE

clean: 
	rm -f $(TARGET) swagmaster.swp 

rebuild:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

pthread:
	gcc -pthread -o pthread_test pthread_test.c 

pthread_debug:
	gcc -m32 -O0 -pthread -g -o pthread_test pthread_test.c 
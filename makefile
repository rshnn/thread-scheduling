# gcc (Ubuntu 4.8.5-4ubuntu2) 4.8.5 (rshnn)
CC = gcc

# -m32 	32-bit mode 
# -O0	No optimization option  
# -g 	Enable gdb debugging 
CFLAGS 		= -m32 -O0
DEBUGGER 	= -g

SOURCE 		= my_pthread_t.h my_pthread_t.c thread_unit_lib.h thread_unit_lib.c
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
	rm -f $(TARGET)

rebuild:
	rm -f $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

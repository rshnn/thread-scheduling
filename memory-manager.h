/****************************************************************************
*
* memory-manager.h
*
****************************************************************************/

#ifndef MEMORY_MANAGER_H

#define MEMORY_MANAGER_H

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <ucontext.h> 
#include <errno.h>
#include <sys/types.h>
#include <string.h>

#define SHOW_PRINTS  		1

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// #define malloc(x) mymalloc(x, __FILE__, __LINE__);
// #define free(x) myfree(x, __FILE__, __LINE__);

/************************************************************************************************************
*
*    MEMORY-MANAGER DATA STRUCTURES
*
************************************************************************************************************/

//for memEntry
// +------1-------+-------1------+------1------+----6----+------23-----------+
// |     valid    |     isfree   |   right_dep | GARBAGE |  request_size     |
// |              |              |             |         |                   |
// +--------------+--------------+-------------+---------+-------------------+
//
// 23 request_size  : max request size is 8388608 (8MB) 


typedef struct PTEntry_{

    unsigned int    used:1;                 // Is the page currently used 
    unsigned int    resident:1;             // Is the page resident in memory 
    unsigned int    left_dependent:1;       // Do we need to load the next page
    unsigned int    right_dependent:1; 
    unsigned int    dirty:1;                // Indicates if the page has been written to (i.e needs to be written back to memory when evicted)
    // unsigned int    UNUSED:4;
    unsigned int    largest_available:12;   // Size of largest fragment available inside the page
    unsigned int    mem_page_number:15;     // Index of page in memory (if it is loaded) (2048 pages for default page size)
    unsigned int    swap_page_number: 12;    // Index of page in swap where it is resident 

}PTEntry;


/* 
    A set of 128 PTEntries. Initially only 1 is allocated per thread.  
    Up to 32 can be made for a thread.  (128*32) = 4096 total possible pages   
*/
typedef struct PTBlock_{
    struct PTEntry_ ptentries[128];         // The block of PTEntries 
    unsigned int    TID:8;                  // TID of owning thread
    unsigned int    blockID;                // ID number of block (0 indexed)
}PTBlock;




typedef struct SuperPTArray_{
    unsigned int    array[32];              // Max PTBlocks for a thread is 32  TODO can convert this to one int
    unsigned int    saturated[32];          // Is the PTBlock at index i full?  TODO """ 
    unsigned int    TID:8;                  // TID of thread that owns this SuperPTA
}SuperPTArray;


typedef struct ThrInfo_{
    unsigned int    TID;                    // TID of thread
    unsigned int    num_blocks;             // Number of PTBlocks it owns
    unsigned int    num_pages;              // Number of PTEntries it uses
    struct SuperPTArray_* SPTArray;         // Pointer to thread's SuperPTArray
    struct PTBlock_**   blocks;             // Array of blocks pointers
}ThrInfo;

/*
    A record of who's page is currently sitting in memory.
    There will be a structure (book_keeper) that is an array of
    [TOTAL_PAGES_IN_MEMORY] of these.  One MemBook per page. 

    Example:  {TID: 10, t_p_n: 2} ==> Thread10's page 2 is resident here
*/
typedef struct MemBook_{
    unsigned int    isfree:1;               // Is this page in mem free
    int             TID:8;                  // TID of thread who's page is resident (max:256)
    int             thread_page_number:12;  // Page# of the owning thread that is resident  (max:4096)
    PTEntry*        entry;
}MemBook;



typedef struct SwapUnit_{
    unsigned int    valid:1;
    unsigned int    TID:8;
    PTEntry*        ptentry;
}SwapUnit;



// typedef struct PageZero_{

//     unsigned int    available_chunks;
//     unsigned int    next_chunk;

// }PageZero_


/************************************************************************************************************
*
*    BIT FIELD EXTRACTORS 
*
************************************************************************************************************/

/* PTEntry is 32 bits */
#define makePTEntry(used, resident, left_dependent, right_dependent, \
        dirty, largest_available, mem_page_number, swap_page_number) (struct PTEntry_){ \
        used, resident, left_dependent, right_dependent, dirty, \
        largest_available, mem_page_number, swap_page_number}



//for memEntry
// +------1-------+-------1------+------1------+----6----+------23-----------+
// |     valid    |     isfree   |   right_dep | GARBAGE |  request_size     |
// |              |              |             |         |                   |
// +--------------+--------------+-------------+---------+-------------------+
//
// 23 request_size  : max request size is 8388608 (8MB) 

#define getValidBitME(header) ((header & 0x80000000)>>31)
#define getIsFreeBitME(header) ((header & 0x40000000)>>30)
#define getRightDepBitME(header) ((header & 0x20000000)>>29)
#define getRequestSizeME(header) (header & 0x007FFFFF)


//for page table entry 
#define getUsedBitPT(entry) ((entry & 0x80000000)>>31)
#define getResidentBitPT(entry) ((entry & 0x40000000)>>30)
#define getLeftDependentBitPT(entry) ((entry & 0x20000000)>>29)
#define getRightDependentBitPT(entry) ((entry & 0x10000000)>>28)
#define getDirtyBitPT(entry) ((entry & 0x08000000)>>27)
#define getUnusedBitPT(entry) ((entry & 0x07000000)>>24) //unused
#define getLargestAvailable_BitPT(entry) ((entry & 0x00FFF000)>>12)
#define getPageNumberPT(entry) ((entry & 0x00000FFF))

/************************************************************************************************************
*
*    MEMORY-MANAGER FUNCTION LIBRARY
*
************************************************************************************************************/

void* scheduler_malloc(int size,int condition);
void* myallocate(int size, char* FILE, int LINE, int tid);
void* mydellocate(void* ptr);

#endif

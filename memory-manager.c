/****************************************************************************
*
* memory-manager.c
*
****************************************************************************/
#include "memory-manager.h"


int** 			memory;				// main memory.  An array of char ptrs
FILE* 			swap_file;			// swap file. 
MemBook* 		book_keeper;		// An array of MemBooks.  Keeps records of memory
SuperPTArray*	SPTA_library;		// Array of SPA's. One for each thread (index = TID)
ThrInfo** 		thread_list;		// Array of ThrInfo ptrs for all threads 
SwapUnit* 		swap_bank; 			// Array of SwapUnit structs.  Book keeping for swap file

int 	MEMORY_SIZE 	= 2<<20;	// 8MB  (8388608 bytes)
int 	SWAP_SIZE		= 2<<23; 	// 16MB (16777216 bytes)
int 	PAGE_SIZE 		= 0;		// Dynamically populated in init(). ~4096 bytes
int 	PAGES_IN_MEMORY = 0;		// Dynamically populated in init(). 2048 for PS=4096
int 	PAGES_IN_SWAP 	= 0;		// Dynamically populated in init(). 4096 for PS=4096
int 	STACK_SIZE 		= 128; 		// Size of a thread's stack TODO: CHANGE THIS IN MYPTHREAD
int 	MAX_THREADS 	= 0;		// Dynamically populated in init(). 8 for PS=4096
int 	VALID_PAGES_MEM	= 0;

int 	initialized 	= 0;		// Boolean to check if mem-manger is init'ed
int 	swap_count 		= 0;		// Number of pages in swap file occupied 




/****************************************************************************
****************************************************************************
*							DEBUGGING FUNCTIONS
****************************************************************************
****************************************************************************/

/**
*	Private Debugging Function
* 		Prints a ThrInfo struct
*/
void _printThrInfo(ThrInfo* thread){
	if(!SHOW_PRINTS)
		return;

	printf(ANSI_COLOR_MAGENTA "ThreadID: %i\tNumBlocks:%i\tNumPages:%i\n" ANSI_COLOR_RESET, \
		thread->TID, thread->num_blocks, thread->num_pages);
}


/**
*	Private Debugging Function
* 		Prints a memEntry header.
*		Input:  32-bit header as integer. 	
*			header{	
*					1:	valid,
*					1:	isfree,
*					1:	right_dependent
*					6:	unused
*					23:	request size
*			}
*
*/
void _printMemEntry(int header){
	if(!SHOW_PRINTS)
		return;
    printf(ANSI_COLOR_MAGENTA"ME_header: \t0x%x\t[v: %i | f: %i | rdep: %i | req_size: %i]\n"\
    	ANSI_COLOR_RESET, header, getValidBitME(header), getIsFreeBitME(header), \
    	getRightDepBitME(header), getRequestSizeME(header));

}




/**
*	Private Debugging Function
* 		Prints a page table entry.
*		Input:  32-bit entry as integer. 	
*			entry{	
*					1:	used,
*					1:	resident,
*					1:	left_dep,
*					1:	right_dep,
*					1:	dirty,
*					3:	unused
*					12:	largest_avail,
*					12: page_number
*			}
*
*/
void _printPageTableEntry(PTEntry* entry){
	if(!SHOW_PRINTS)
		return;
	printf(ANSI_COLOR_MAGENTA"PTEntry: \t[u:%i |r:%i |ld:%i |rd:%i |d:%i |la:%i |pn: %i |  spn: %i]\n" ANSI_COLOR_RESET, \
		entry->used, entry->resident, entry->left_dependent, \
		entry->right_dependent, entry->dirty, entry->largest_available, \
		entry->mem_page_number, entry->swap_page_number);

}







/***************************************************************************
****************************************************************************
*							HELPER FUNCTIONS
****************************************************************************
****************************************************************************/

/**
*	Helper Function
*		Builds an integer that represents a 32-bit memEntry	
* 		Usage:  int entry = initMemEntry(1,0,0,35010);
*/
int initMemEntry(int valid, int isfree, int left_dep, int request_size){
	int entry = 0;

	if (valid)
		entry += 0x80000000;
	if (isfree)
		entry += 0x40000000;
	if (left_dep)
		entry += 0x20000000;

	entry += request_size;

	return entry;
}



/**
*	Helper Function
*		Builds a new empty ThrInfo struct.  
* 			Saves a pointer to respective index in thread_list
* 		Input:  TID of owning thread
*		Returns 0 on success
*/
int buildThrInfo(int tid){

	int i;

	ThrInfo* temp 		= (ThrInfo*)malloc(sizeof(ThrInfo));
	temp->TID 			= tid;
	temp->num_blocks 	= 0;
	temp->num_pages 	= 0;
	temp->SPTArray 		= &(SPTA_library[tid]);			// Cannot make more than MAX_THREADS.  Will break here

	/* Array of ptblock pointers.  They point to nothing initially */
	temp->blocks 		= (PTBlock**)malloc(32*sizeof(PTBlock*));
	for(i=0; i<32; i++){
		temp->blocks[i] = NULL;						// Problem? 
	}

	/* Write to global thread_list structure */
	thread_list[tid] 	= temp;

	return 0;
}

/**
*	Helper Function
*		Initializes all the structures needed to manage memory:
*			memory, book_keeper, SPTA_library, swap_file
*/
void initMemoryManager(){

	if(initialized == 1)
		return;


	int i,j;
	PAGE_SIZE 		= sysconf(_SC_PAGE_SIZE);
	PAGES_IN_MEMORY = (MEMORY_SIZE / PAGE_SIZE);
	PAGES_IN_SWAP 	= (SWAP_SIZE / PAGE_SIZE);
	MAX_THREADS 	= (PAGE_SIZE/ (sizeof(ucontext_t)+4));			// 4 for mementries
	VALID_PAGES_MEM = PAGES_IN_MEMORY - 2; 			// Last 2 reserved for scheduler stuff 

	if(SHOW_PRINTS){	
		printf(ANSI_COLOR_CYAN"SYSTEM INFO:\n\tPAGE_SIZE:\t\t\t%i\n"ANSI_COLOR_RESET,\
			PAGE_SIZE);
		printf(ANSI_COLOR_CYAN"\tMEMORY_SIZE:\t\t\t%d\n"ANSI_COLOR_RESET, MEMORY_SIZE);
		printf(ANSI_COLOR_CYAN"\tPAGES_IN_MEMORY:\t\t%d\n"ANSI_COLOR_RESET, PAGES_IN_MEMORY);
		printf(ANSI_COLOR_CYAN"\tVALID_IN_MEMORY:\t\t%d\n"ANSI_COLOR_RESET, VALID_PAGES_MEM);
		printf(ANSI_COLOR_CYAN"\tSWAP_SIZE:\t\t\t%d\n"ANSI_COLOR_RESET, SWAP_SIZE);
		printf(ANSI_COLOR_CYAN"\tPAGES_IN_SWAP:\t\t\t%d\n"ANSI_COLOR_RESET, PAGES_IN_SWAP);
		printf(ANSI_COLOR_CYAN"\tsizeof(ucontext+stack):\t\t%d\n"ANSI_COLOR_RESET, \
			STACK_SIZE+sizeof(ucontext_t));
		printf(ANSI_COLOR_CYAN"\tMAX_THREADS:\t\t\t%d\n"ANSI_COLOR_RESET, MAX_THREADS);
		printf(ANSI_COLOR_CYAN"\t------------------------------------\n"ANSI_COLOR_RESET);
	}

	/****************************************************************************
	*			INIT MEMORY
	*
	****************************************************************************/	
	
	/* memory is an array of char ptrs. Each char* will point to a page */
	memory = (int **) malloc( PAGES_IN_MEMORY * sizeof(int*));

	/* Populating memory array.  Obtain pointers using memalign. */
	for(i=0; i<PAGES_IN_MEMORY; i++){
		// The memalign function allocates a block of size bytes whose address
		// is a multiple of boundary. The boundary must be a power of two! 
		// The function memalign works by allocating a somewhat larger block, 
		// and then returning an address within the block that is on the specified 
		// boundary.
		memory[i] = (int*) memalign(PAGE_SIZE, PAGE_SIZE);
		// memcpy(memory[i], &temp, sizeof(int));		
	}



	/****************************************************************************
	*			INIT BOOK_KEEPER
	*
	****************************************************************************/	
	/* Each page in memory gets a MemBook to track who is currently resident */
	book_keeper = (MemBook*) malloc(VALID_PAGES_MEM * sizeof(MemBook));
	for(i=0; i<PAGES_IN_MEMORY; i++){
		book_keeper[i].isfree 				= 1;
		book_keeper[i].TID 					= -2;	// Made it -2 in case -1 causes problems		
		book_keeper[i].thread_page_number 	= -2;	// since we used -1 for a completed thread
		book_keeper[i].entry 				= NULL;  // pointer to PTE of the resident page 
	}


	/****************************************************************************
	*			INIT SPTA_library
	*
	****************************************************************************/	
	/* Each thread gets a SuperPTArray */
	SPTA_library = (SuperPTArray*) malloc( MAX_THREADS * sizeof(SuperPTArray));

	/* Init all values to 0. */
	for(i=0; i<MAX_THREADS; i++){
		for(j=0; j<32; j++){
			SPTA_library[i].array[j] 		= 0;
			SPTA_library[i].saturated[j] 	= 0;

		}
		SPTA_library[i].TID = i;
	}


	/****************************************************************************
	*			INIT THREAD_LIST
	*
	****************************************************************************/	
	/* One ThrInfo struct ptr per thread.  The structs is NOT allocated here. */
	thread_list = (ThrInfo**)malloc(MAX_THREADS * sizeof(ThrInfo*));
	/* Init all the pointers to NULL.  They will be populated by makeThrInfo() */
	for(i=0; i<MAX_THREADS; i++){
		// thread_list[i] = NULL;
		buildThrInfo(i);
	}


	/****************************************************************************
	*			INIT SWAP_SPACE
	*
	****************************************************************************/	


	swap_file = fopen("swagmaster.swp", "w+");
	// fseek(swap_file, 16777217, SEEK_SET);
	
	ftruncate(fileno(swap_file), 16777216);


	close(fileno(swap_file));





	// lseek(fileno(swap_file), 16777216, SEEK_SET);
	// rewind(swap_file);



	/****************************************************************************
	*			INIT SWAP_BANK
	*
	****************************************************************************/	
	swap_bank = (SwapUnit*)malloc(PAGES_IN_SWAP* sizeof(SwapUnit));
	for(i = 0; i < PAGES_IN_SWAP ; i++){
		swap_bank[i].valid = 1;
		swap_bank[i].TID = -2;
		swap_bank[i].ptentry = NULL;
	}

	initialized = 1;
}





/**
*	Helper Function
*		Adds a new empty PTBlock to ThrInfo's block linked list.
* 		Input:  ThrInfo of owning thread
* 		Returns 0 on success 
*/
int addPTBlock(int tid){

	ThrInfo* thread = thread_list[tid];

	if(thread->num_blocks == 32){
		return -1;
	}

	int i;
	PTBlock* block 	= (PTBlock*)malloc(sizeof(PTBlock));
	block->TID 		= thread->TID;

	block->blockID 	= thread->num_blocks;

	/* Init all ptentries */
	for(i=0; i<128; i++){
		block->ptentries[i] = makePTEntry(0,0,0,0,0,PAGE_SIZE-4, 2048, 0);
	}

	thread->blocks[block->blockID] = block;	 		// BlockID is 0 indexed  If breaks here.  Null pointer issues?
	thread->num_blocks++; 							// Not 0 indexed 


	thread->SPTArray->array[block->blockID] 	= 1;
	thread->SPTArray->saturated[block->blockID] = 0;

	return 0;
}


PTBlock* nextAvailableBlock(int tid, int startwith){

	int i;
	int block_index = -1;
	ThrInfo* thread = thread_list[tid];

	SuperPTArray* sptarray = thread->SPTArray;

	for(i=startwith; i<32; i++){
		if(sptarray->array[i] == 1 && sptarray->saturated[i] == 0){
			// This block is initialized and unsaturated
			block_index = i;
			break;
		}
	}
	if(block_index == -1){
		// Need to add a new block 
		int success = addPTBlock(tid);
		if(success == -1){
			return NULL;
		}

		block_index = thread->num_blocks-1; 		// Grabs index of newest 
		// addPTBlock() will update the sptarray
	}
	return thread->blocks[block_index];

}



	// printf(ANSI_COLOR_RED"NumPages: %i"ANSI_COLOR_RESET, numPages);


/*  */
void* multiplePageRequest(int size, char* FILE, int LINE, int tid){

	int i;
	int first_allocate = 0;
	PTEntry* myPTE;

	int minusFirstPage = size-(PAGE_SIZE-4);
	int numPages = (minusFirstPage)/(PAGE_SIZE);
	double doub_numPages = ((double)minusFirstPage)/((double)PAGE_SIZE); // How many besides the first page?
	/*Look at reminder*/
	if((doub_numPages-((double)numPages)) > 0)
		numPages++;
	numPages++; 	//Add one for the first page

	MemBook array[numPages];


	/*(1) Get thr info*/
	ThrInfo* thread = thread_list[tid];
	if(thread->num_blocks == 0)
		first_allocate = 1;



	if((swap_count+numPages > (PAGE_SIZE-1)) || (numPages > (PAGES_IN_MEMORY-2))){
		printf(ANSI_COLOR_RED"No more room for you, dude.\n" ANSI_COLOR_RESET);
		return NULL;
	}

	int z;
	for(z=0; z<numPages; z++){
		/*For each numPage, populate array*/



		PTBlock* myblock 			= NULL; 		// Block with free space in it
			// Case 1: First malloc by this thread.  Generate new PTBlock.  Update SupPTA
		if(first_allocate){
			// printf("Case 1: first!\n");
			addPTBlock(tid);
			first_allocate = 0;
			myblock = thread->blocks[0];
		}else{
			// Case 2: Regular case. 
			// printf("Case 2: regular.  Numblocks:%i\n", thread->num_blocks);
			myblock = nextAvailableBlock(tid, 0);
			if(myblock == NULL){
				printf(ANSI_COLOR_RED"Cannot allocate anymore blocks to %i.\n"ANSI_COLOR_RESET, tid);
				return NULL;
			}		

		}
		// Ive got the block with available space now (myblock)		

		

		int mypagenumber = -1;
		while(mypagenumber == -1){


			for(i=0; i<128; i++){
				PTEntry temp = myblock->ptentries[i];

				int available_size = temp.largest_available;
				
				// printf("asize:%i\n",available_size);
				// Is this page a brand new page?  Empty?
				if(available_size == (PAGE_SIZE-4)){
					mypagenumber = i;
					// printf("enough size available here.: %i\n",mypagenumber);
					break;
				}
			}
			
			if(mypagenumber == -1){
				// myblock doesn't have a page with enough space
				myblock = nextAvailableBlock(tid, myblock->blockID);
				if(myblock == NULL){
					printf(ANSI_COLOR_RED"No more blocks available for this thread.\n" ANSI_COLOR_RESET);
					return NULL;//no more blocks available break from loop condition
				}
			}
		}

		// printf("This is the page im using:\t%i\n", mypagenumber);

		PTEntry* myPTE = &(thread->blocks[myblock->blockID]->ptentries[mypagenumber]);
		array[z].entry 				= myPTE;
		array[z].thread_page_number = mypagenumber + (myblock->blockID)*128;


		/*TODO:  Change this so that the last block still has available space*/
		myPTE->largest_available = 0;

		// printf(ANSI_COLOR_CYAN"\tarray[%i] is %i\n"ANSI_COLOR_RESET, z, array[z].thread_page_number);
		

	// end of for(numPages)
	// Now my array of MemPages should be full  
	}

	int j;
	int first_spot= -1; 
	for(j=0; j<numPages; j++){
		/* 
			Grab MemPage from array=myEntry.
				myEntry->swap_page_number
		*/
		myPTE = array[j].entry;
		int myrealpagenumber = array[j].thread_page_number;



		// printf("%i, mrpn:%i\n",j, myrealpagenumber);

		if(myPTE->mem_page_number == 2048){ 
		//no swap entry made yet, find an open swap struct entry to secure its spot in the swap file
			int available_index = -1;
			



			/* Find a spot in swap*/
			for(i = 0; i < PAGES_IN_SWAP; i++){
				if(swap_bank[i].valid == 0){
					continue;
				}
				available_index = i;
				swap_bank[i].valid = 0;
				swap_bank[i].TID = tid;
				swap_bank[i].ptentry = myPTE;
				// Write to PTE 
				myPTE->swap_page_number = available_index;
				swap_count++;
				myPTE->used = 1;
				myPTE->resident = 1; //going to be resident real soon
				break;
			}
			

			if(first_spot == -1){
				//case 1:  first page

				/* find continguous numPages number of pages from end */
				int foundspot = 0;

				/*  Find the first empty spot satisfying conditions from end */
				for( i = VALID_PAGES_MEM-1-numPages; i >= 0; i--){

					if(book_keeper[i].isfree == 1){ //found a spot
						book_keeper[i].isfree = 0;
						book_keeper[i].TID = tid;
						book_keeper[i].thread_page_number = myrealpagenumber;
						book_keeper[i].entry = myPTE;
						foundspot = 1;
						
						available_index = i;
						myPTE->mem_page_number = i;
						break;
					}
				}


				/* Could not find first free spot */
				if(foundspot == 0){//kick someone out of memory

					i = VALID_PAGES_MEM-1-numPages;
					PTEntry* guy = book_keeper[i].entry;
					int guys_swap_spot = guy->swap_page_number;
					guy->resident = 0;

					book_keeper[i].TID = tid;
					book_keeper[i].thread_page_number = myrealpagenumber;
					book_keeper[i].entry = myPTE;

					/*update my pte->mempage*/
					myPTE->mem_page_number = i;
					available_index = i;

					/*write guy back*/
					swap_file = fopen("swagmaster.swp", "r+");
					lseek(fileno(swap_file), (guys_swap_spot*PAGE_SIZE), SEEK_SET);
					write(fileno(swap_file), memory[i], PAGE_SIZE);
					close(fileno(swap_file));
					
				}
				
				first_spot = i;


			}else{
				// case 2:  not first 

				first_spot++;
				int myspot = first_spot;
				//now to find a spot in memory


				if(book_keeper[myspot].isfree == 1){ //found a spot
					book_keeper[myspot].isfree = 0;
					book_keeper[myspot].TID = tid;
					book_keeper[myspot].thread_page_number = myrealpagenumber;
					book_keeper[myspot].entry = myPTE;
					
					available_index = myspot;
					myPTE->mem_page_number = myspot;
				}else{
					//kick em out, write him back to his swap spot.

					PTEntry* guy = book_keeper[myspot].entry;
					int guys_swap_spot = guy->swap_page_number;
					guy->resident = 0;

					book_keeper[myspot].TID = tid;
					book_keeper[myspot].thread_page_number = myrealpagenumber;
					book_keeper[myspot].entry = myPTE;

					/*update my pte->mempage*/
					myPTE->mem_page_number = myspot;
					available_index = myspot;

					/*write guy back*/
					swap_file = fopen("swagmaster.swp", "r+");
					lseek(fileno(swap_file), (guys_swap_spot*PAGE_SIZE), SEEK_SET);
					write(fileno(swap_file), memory[myspot], PAGE_SIZE);
					close(fileno(swap_file));	


				}


			}//end of else.  I should have available_index.  Safely removed whoever was there/accounted for case1


			/*write my page into memory[i]*/
			swap_file = fopen("swagmaster.swp", "r");
			lseek(fileno(swap_file), ((myPTE->swap_page_number)*PAGE_SIZE), SEEK_SET);
			read(fileno(swap_file), memory[available_index], PAGE_SIZE);
			close(fileno(swap_file));


			


			myPTE->largest_available = PAGE_SIZE-4-size-4;

			if(j==0){
				// start_of_request = (void*)(start_of_my_page+4);
				array[j].entry->right_dependent = 1;
				first_spot = myPTE->mem_page_number;
			
			}else if(j==numPages-1){
				array[j].entry->left_dependent = 1;
			
			}else{
				array[j].entry->left_dependent = 1;
				array[j].entry->right_dependent = 1;
			}
			myPTE->largest_available = 0;
			_printPageTableEntry(myPTE);


		}else{

			printf(ANSI_COLOR_RED"Panic!\n"ANSI_COLOR_RESET);
		}

		/* Give them a spot in swapfile */



	}

	/*Write mem entry in to first spot.  Write blank mem entry at the end of request*/


	int end_spot = first_spot;
	first_spot = first_spot - numPages;


	/* memEntry for this request */
	int temp = initMemEntry(1, 0, 0, size);
	memcpy(memory[first_spot], &temp, sizeof(int));		



	/*memEntry to append to end of this request*/
	int next_ME = initMemEntry(1, 1, 0, 0);
	memcpy(((memory[end_spot])+0x4+size), &next_ME, sizeof(int));
	

	int* start_of_my_request = memory[first_spot]+4;

	printf(ANSI_COLOR_GREEN"Large requst return:\tPN:%i, BN:%i, start:%i, end%i\n"ANSI_COLOR_RESET, \
		myPTE->mem_page_number, (array[0].thread_page_number)/128, first_spot, end_spot);


	return (void*) start_of_my_request;

}







/****************************************************************************
****************************************************************************
*							LIBRARY FUNCTIONS
****************************************************************************
****************************************************************************/

void* scheduler_malloc(int size){
	void* da_pointer;
	static int iteration = 0;
	int temp_entry;
	int* currME;
	int isfree;
	int seg_size;
	int offset = PAGE_SIZE*2;
	if(initialized == 0)
		initMemoryManager();

	if(size == 16)
		printf("The value of is free %i on thread_unit iteration %i\n", \
			book_keeper[PAGES_IN_MEMORY-10].isfree,iteration);

	if(size == 348)
		printf("The value of is free %i on a ucontext iteration %i\n", \
			book_keeper[PAGES_IN_MEMORY-10].isfree,iteration);


	currME = memory[PAGES_IN_MEMORY-2];
	if(book_keeper[PAGES_IN_MEMORY-2].isfree == 1){//set it up (just in case)
		book_keeper[PAGES_IN_MEMORY-2].isfree = 0;
		int temp = initMemEntry(1, 0, 0, size);
		memcpy(memory[PAGES_IN_MEMORY-2], &temp, sizeof(int));
		int next_ME = initMemEntry(1, 1, 0, PAGE_SIZE-4-size);
		memcpy(((memory[PAGES_IN_MEMORY-2])+0x4+size), &next_ME, sizeof(int));
		memcpy((memory[PAGES_IN_MEMORY-2]+0x4+size), &next_ME, sizeof(int));
		return currME+0x4;
	}else{ //set up is done just look for a free mementry and chop us as needed
		while(offset >= -4){//change to offset?
			seg_size = getRequestSizeME(*currME);
			isfree = getIsFreeBitME(*currME);
			if(isfree && seg_size >= size){
				//found it
				if(seg_size-size < 4){
					// Not adding a new memEntry.  You get the block, guy.
					temp_entry = initMemEntry(1, 0, 0, seg_size);
					memcpy(currME, &temp_entry, sizeof(int));	
					return da_pointer = currME+4;
				}else{
					// chopchop
					temp_entry = initMemEntry(1, 0, 0, size);
					memcpy(currME, &temp_entry, sizeof(int));	
					temp_entry = initMemEntry(1, 1, 0, seg_size-size-4);
					//isfree = getIsFreeBitME(temp_entry); //this was used for testing right?
					// printf("isfree: %i\n", isfree);	
					// _printMemEntry(temp_entry);
					// printf("Lookatme!\t"); _printMemEntry(temp_entry);
					memcpy(currME+4+size, &temp_entry, sizeof(int));		
					da_pointer = currME+4;
					return da_pointer;
				}
			}
			currME = currME+seg_size+0x4;
			offset = offset - seg_size -4;
		} 
		// must have found no space, hit max threads?
		printf(ANSI_COLOR_RED"Panic. Out of reserve memory for scheduler.\n"ANSI_COLOR_RESET);
		return NULL;
	}
	return NULL;	
	
}

void* myallocate(int size, char* FILE, int LINE, int tid){



	if(size > PAGE_SIZE-4){
		return multiplePageRequest(size, FILE, LINE, tid);
	}



	int i;
	int first_allocate = 0;
	
	if(initialized == 0)
		initMemoryManager();

	/*********************************************************************
	(0) Fetch ThrInfo for this thread from thread_list

	(0.5) If request > PAGE_SIZE, do stuff with giving back multiple pages
		Follow similar pattern to below, but check for enough contiguous pages

	(1) Locate PTBlock with available space
		Check SuperPTA linearly for a 1.
			Check saturated at that index to see if its full.
			If not, save that index=A.

	(2) Locate PTEntry with enough space 
		Go to Ath block (stored as array in ThrInfo)
			Iterate through PTEntries (block->ptentries[0 - 127])
			Check ptentries[i]->largest_available
			If request fits, save that index, i=B
		We need ThrInfo->TID's Page B

	(3)	Get my page into some spot in memory 

			Do i have an assigned spot in memory yet? (mem_pag!= 2048)
				Yes: Am i resident in this assigned spot? (book_keeper)
					Yes: Continue;
					No:  Move that guy into swap_bank 	(check his PTE)	

				No: Look for a free spot in memory. (book_keeper) (mem_page == 2048)
					If all full, look to evict.  	
						Is there enough swap_space?
							Failure return NULL.  Swap full
						Find the first MemBook that isnt mine
							get it out of here!
							My spot.  (save index to PTE->mem_page_number)
	
	(4) Generate memEntry for this request
		Iterate through the page via memEntries.
		Find the one where request fits.  
		Do the memEntry things. 

		Collect Pointer and return to thread
		Location of memEntry+ sizeof(memEntry)

		
	**********************************************************************/


	/********************************************/
	/* (0)  Fetch ThrInfo */ 
	ThrInfo* thread = thread_list[tid];

	if(thread->num_blocks == 0)					
		first_allocate = 1;

	/********************************************/
	/* (1)  Locate PTBlock with available space */

	PTBlock* myblock 			= NULL; 		// Block with free space in it
		// Case 1: First malloc by this thread.  Generate new PTBlock.  Update SupPTA
	if(first_allocate){
		// printf("first!\n");
		addPTBlock(tid);
		myblock = thread->blocks[0];
	}else{
		// Case 2: Regular case. 
		printf("Case 2 regular.  Numblocks:%i\n", thread->num_blocks);
		myblock = nextAvailableBlock(tid, 0);
		if(myblock == NULL){
			printf(ANSI_COLOR_RED"Cannot allocate anymore blocks to %i.\n"ANSI_COLOR_RESET, tid);
			return NULL;
		}		

	}
	// Ive got the block with available space now (myblock)			
	// TODO:  Havent done anything about requests > PAGESIZE yet



	/********************************************/
	/* (2) Locate PTEntry with enough space */

	int mypagenumber = -1;
	while(mypagenumber == -1){ //infinite loop

		for(i=0; i<128; i++){
			PTEntry temp = myblock->ptentries[i];
			int available_size = temp.largest_available;
			
			// Does request fit in here?  (sizeof(int) = sizeof(memEntry))
			if(available_size > size){
				mypagenumber = i;
				break;
			}
		}
		
		if(mypagenumber == -1){
			// myblock doesn't have a page with enough space
			myblock = nextAvailableBlock(tid, myblock->blockID);
			if(myblock == NULL){
				printf(ANSI_COLOR_RED"No more blocks available for this thread.\n" ANSI_COLOR_RESET);
				return NULL;//no more blocks available break from loop condition
			}
		}
	}
	// I've got the pagenumber that this thread can use now (mypagenumber)
	PTEntry* myPTE = &(thread->blocks[myblock->blockID]->ptentries[mypagenumber]);

	// printf("POINTIES: %i, %p\n", thread->TID, *(thread->blocks));

	/********************************************/
	/* (3)	Get my page into some spot in memory  */



	int* start_of_my_page = NULL;

	if(swap_count >= 4095){
		printf(ANSI_COLOR_RED"No more swap space available.\n" ANSI_COLOR_RESET);//cant make anymore pages, swap space is full
		return NULL;
	}

	int myrealpagenumber = mypagenumber+(myblock->blockID)*128;
	if(myPTE->mem_page_number == 2048){ //no swap entry made yet, find an open swap struct entry to secure its spot in the swap file
		int available_index = -1;
		for(i = 0; i < PAGES_IN_SWAP; i++){
			if(swap_bank[i].valid == 0){
				continue;
			}
			available_index = i;
			swap_bank[i].valid = 0;
			swap_bank[i].TID = tid;
			swap_bank[i].ptentry = myPTE;
			// Write to PTE 
			myPTE->swap_page_number = available_index;
			swap_count++;
			myPTE->used = 1;
			myPTE->resident = 1; //going to be resident real soon
			break;
		}
		//now to find a spot in memory
		int foundspot = 0;
		for( i = 0; i < VALID_PAGES_MEM; i++){
			if(book_keeper[i].isfree == 1){ //found a spot
				book_keeper[i].isfree = 0;
				book_keeper[i].TID = tid;
				book_keeper[i].thread_page_number = myrealpagenumber;
				book_keeper[i].entry = myPTE;
				foundspot = 1;
				
				available_index = i;
				myPTE->mem_page_number = i;
				break;
			}
		}
		if(foundspot == 0){//kick someone out of memory
			for(i = 0; i < VALID_PAGES_MEM;i++){
				if(book_keeper[i].TID != tid  && book_keeper[i].TID != 0){//kick out the ith index
					//TODO//
					PTEntry* guy = book_keeper[i].entry;
					int guys_swap_spot = guy->swap_page_number;
					guy->resident = 0;

					book_keeper[i].TID = tid;
					book_keeper[i].thread_page_number = myrealpagenumber;
					book_keeper[i].entry = myPTE;

					/*update my pte->mempage*/
					myPTE->mem_page_number = i;
					available_index = i;

					/*write guy back*/
					swap_file = fopen("swagmaster.swp", "r+");
					lseek(fileno(swap_file), (guys_swap_spot*PAGE_SIZE), SEEK_SET);
					write(fileno(swap_file), memory[i], PAGE_SIZE);
					close(fileno(swap_file));
					
					break;
				}
			}
		}

		/*write my page into memory[i]*/
		swap_file = fopen("swagmaster.swp", "r");
		lseek(fileno(swap_file), ((myPTE->swap_page_number)*PAGE_SIZE), SEEK_SET);
		read(fileno(swap_file), memory[available_index], PAGE_SIZE);
		close(fileno(swap_file));


		/* New page created, do the base case memEntry stuff and return ptr. */
		int temp = initMemEntry(1, 0, 0, size);
		memcpy(memory[available_index], &temp, sizeof(int));		
		int next_ME = initMemEntry(1, 1, 0, PAGE_SIZE-4-size);

		// void* ((memory[available_index])+0x4+size);				// 4 is the size of memEntry

		memcpy(((memory[available_index])+0x4+size), &next_ME, sizeof(int));
		start_of_my_page = memory[available_index];

		printf(ANSI_COLOR_GREEN"The earlier return:\tPN:%i, BN:%i\n"ANSI_COLOR_RESET, \
			myPTE->mem_page_number, myblock->blockID);


		myPTE->largest_available = PAGE_SIZE-4-size-4;
		_printPageTableEntry(myPTE);

		return (void*) (start_of_my_page+4);


	}else{

		int my_mem_spot = myPTE->mem_page_number;

		/* AM i already in this mem spot? */
		if(book_keeper[my_mem_spot].TID != tid){
			/*im not already here*/


			/* I already have a set location in memory */
			PTEntry* entry_to_evict = book_keeper[my_mem_spot].entry;
			entry_to_evict->resident = 0;
						
			/*write guy back*/
			swap_file = fopen("swagmaster.swp", "r+");
			lseek(fileno(swap_file), ((entry_to_evict->swap_page_number)*PAGE_SIZE), SEEK_SET);
			write(fileno(swap_file), memory[my_mem_spot], PAGE_SIZE);
			close(fileno(swap_file));

			/*write my page into memory[mymemspot]*/
			swap_file = fopen("swagmaster.swp", "r");
			lseek(fileno(swap_file), ((myPTE->swap_page_number)*PAGE_SIZE), SEEK_SET);
			read(fileno(swap_file), memory[my_mem_spot], PAGE_SIZE);
			close(fileno(swap_file));

			
		}
		start_of_my_page = memory[my_mem_spot];
	}

	/* At the end of this, start_of_my_page ptr is defined.*/


	/********************************************/
	/* (4) Generate memEntry for this request */
		// Iterate through the page via memEntries.
		// Find the one where request fits.  
		// Do the memEntry things. 
	
	void* da_pointer;
	int found = 0;
	int temp_entry;
	int* currME = start_of_my_page;
	int isfree;
	while(!found){
		int seg_size = getRequestSizeME(*currME);
		int isfree = getIsFreeBitME(*currME);

		if(isfree && seg_size >= size){
			//found it

			if(seg_size-size < 4){
				// Not adding a new memEntry.  You get the block, guy.
				temp_entry = initMemEntry(1, 0, 0, seg_size);
				memcpy(currME, &temp_entry, sizeof(int));	
				da_pointer = currME+4;
						
			}else{

				// chopchop
				temp_entry = initMemEntry(1, 0, 0, size);
				memcpy(currME, &temp_entry, sizeof(int));	


				temp_entry = initMemEntry(1, 1, 0, seg_size-size-4);
				isfree = getIsFreeBitME(temp_entry);

				// printf("isfree: %i\n", isfree);	
				// _printMemEntry(temp_entry);
				// printf("Lookatme!\t"); _printMemEntry(temp_entry);
				memcpy(currME+4+size, &temp_entry, sizeof(int));		

				da_pointer = currME+4;
			}
			
			found = 1;
		}

		currME = currME+seg_size+0x4;
	}



	/*Look through memEntries to update largestAvailable in PTE*/


	currME = start_of_my_page;

	int largest = 0;
	int offset = PAGE_SIZE;
	
	while(offset >= -4){

		int isvalid = getValidBitME(*currME);
		isfree = getIsFreeBitME(*currME);
		int seg_size = getRequestSizeME(*currME);
		// _printMemEntry(*currME);
		// printf("\t\tReqSize is %i at offset: %i\n", seg_size, offset);

		if (!isvalid){
			/*prob the last one*/
			// printf("not valid.\n");
			int d = getRequestSizeME(*currME);
			if(d > largest){
			 	largest = d;
			}
			break;
		}

		if(isfree){
			/*update largest*/
			// printf("Is free!\n");
			 int d = getRequestSizeME(*currME);
			 if(d > largest){
			 	largest = d;
			 }
		}

		currME = currME+0x4+seg_size;

		offset = offset - seg_size - 4;
		// printf("\t\tnewoffset: %i\n", offset);
	}


	printf("I found it its this: %i\n", largest);
	/* Modify myPTE's values when done*/
	myPTE->largest_available = largest;


	printf(ANSI_COLOR_GREEN"The later return: \tPN:%i, BN: %i\n" ANSI_COLOR_RESET, \
		myPTE->mem_page_number, myblock->blockID);







	_printPageTableEntry(myPTE);

	return da_pointer;


}


void* mydellocate(void* ptr){
	//call protect memory to get me an array of PTE's with all pages loaded and unprotected
	int num_pages = 5;
	PTEntry* array[num_pages];
	int* currME;
	int* nextME;
	int* prevME;
	int temp_entry;
	//int isvalid = getValidBitME(*currME);
	int	isfree;
	int prev_seg_size = 0;
	int next_seg_size = 0;
	//get currME for left most PTE
	currME = (array[0]->mem_page_number)*PAGE_SIZE + memory[0]; //address pointing to mementry of leftmost page
	int curr_seg_size = getRequestSizeME(*currME);
	//now given the pointer *ptr i can find the mementry that the pointer falls under
	while((currME+curr_seg_size +4) < (int*)ptr){//jump to next mementry until we find the one corresponding to ptr
		prev_seg_size = curr_seg_size;
		currME = currME + curr_seg_size +4;
		curr_seg_size = getRequestSizeME(*currME);
	}
	//i found the mementry
	temp_entry = initMemEntry(1, 1, 0, curr_seg_size);//set the bit to free
	memcpy(currME, &temp_entry, sizeof(int));
	//i have freed the mementry 
	//now to check the next mementry to free it
	nextME = currME + curr_seg_size +4;
	next_seg_size = getRequestSizeME(*nextME);
	isfree = getIsFreeBitME(*nextME);
	if(isfree){//next mementry is also free so lets merge
		temp_entry = initMemEntry(1, 1, 0, curr_seg_size+4+next_seg_size);
		memcpy(currME, &temp_entry, sizeof(int));
	}
	//now check the left mementry
	if(currME != (array[0]->mem_page_number)*PAGE_SIZE + memory[0]){
		//then there might be free left mementry
		curr_seg_size = getRequestSizeME(*currME);
		prevME = currME-4-curr_seg_size;
		prev_seg_size = getRequestSizeME(*prevME);
		isfree = getIsFreeBitME(*prevME);
		if(isfree){//merge the left with the right one if its free
			temp_entry = initMemEntry(1, 1, 0, curr_seg_size+4+prev_seg_size);
			memcpy(prevME, &temp_entry, sizeof(int));
		}
	}
	currME = (array[0]->mem_page_number)*PAGE_SIZE + memory[0];
	if(getIsFreeBitME(*currME)){
		//point to first PTE and set pte.largestAvailable to PAGE_SIZE-4
		//if pte was right dependent then set the next pages to not valid
	}
	//TODO update largest available in the PTE or just let the myallocate do it for us
	return NULL;
	
	
}




int debug_1_simple_allocates_multiple_thread(){


    printf(ANSI_COLOR_YELLOW"DEBUG TEST: Simple allocs, multiple threads!\n" ANSI_COLOR_RESET);
    int i;
    for(i=0; i<3; i++){

		printf("~~~~~~~~TID=4~~~~~~~~\n");
		int* z = (int*) myallocate(30, " ", 2, 4);
	    *z = 3284;
	}

	printf("~~~~~~~~TID=3~~~~~~~~\n");
 	int* x = (int*) myallocate(4, " ", 2, 3);
 	*x =234;


	printf("~~~~~~~~TID=4~~~~~~~~\n");	
 	int* xy = (int*) myallocate(4, " ", 2, 4);
 	*xy = i*i*i;


	
	printf("~~~~~~~~TID=5~~~~~~~~\n");
 	int* xdy = (int*) myallocate(4, " ", 2, 5);
 	*xdy = i*i*i;

 	return 0;
}


int debug_2_multiple_page_request(){

/*	initMemoryManager();

	printf("~~~~~~~~TID=4~~~~~~~~\n");
	int* a = (int*) myallocate(PAGE_SIZE*3, " ", 3, 4);
	*a = 38;
	printf(ANSI_COLOR_RED"%i\n"ANSI_COLOR_RESET, *a);

	printf("~~~~~~~~TID=3~~~~~~~~\n");
	myallocate(PAGE_SIZE*4, " ", 3, 3);


	printf("~~~~~~~~TID=4~~~~~~~~\n");
	myallocate(PAGE_SIZE*4, " ", 3, 4);

*/
	/* This does not work bc no signal handler for mprotect yet */
	// printf(ANSI_COLOR_RED"%i\n"ANSI_COLOR_RESET, *a);
	int*a;
	int*b;
	int*c;

	// printf("~~~~~~~~TID=3~~~~~~~~\n");
	// //myallocate(PAGE_SIZE*4, " ", 3, 3);
	// printf("~~~~~~~~TID=4~~~~~~~~\n");
	// a = (int*) scheduler_malloc(4,2);
	// *a = 38;
	// printf(ANSI_COLOR_RED"a:%i\n"ANSI_COLOR_RESET, *a);
	// b = (int*) scheduler_malloc(4,2);
	// *b = 5;
	// printf(ANSI_COLOR_RED"a:%i, b:%i\n"ANSI_COLOR_RESET, *a,*b);
	// c = (int*) scheduler_malloc(4,2);
	// *c = 5;
	// printf(ANSI_COLOR_RED"a:%i, b:%i, c:%i\n"ANSI_COLOR_RESET, *a,*b,*c);
	
	return 0;
}
	

/*
int main(){



	//debug_1_simple_allocates_multiple_thread();
 	
 	debug_2_multiple_page_request();


    return 0;

}*/


#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>
#include <terminals.h>
#include <hardware.h>

int **interruptVector[TRAP_VECTOR_SIZE];

void TrapKernelHandler(ExceptionInfo *info);
void TrapClockHandler(ExceptionInfo *info);
void TrapIllegalHandler(ExceptionInfo *info);
void TrapMemoryHandler(ExceptionInfo *info);
void TrapMathHandler(ExceptionInfo *info);
void TrapTTYReceiveHandler(ExceptionInfo *info);
void TrapTTYTransmitHandler(ExceptionInfo *info);
struct pte *region0Pt, *region1Pt;  //page table pointers
int freePages;

/*
    *  This is the primary entry point into the kernel:
    *
    *  ExceptionInfo *info
    *  unsigned int pmem_size
    *  void *orig_brk
    *  char **cmd_args
    */
extern void KernelStart(ExceptionInfo *info, unsigned int pmem_size, void **orig_brk, char **cmd_args)
{
    /* Initialize interrupt vector table */
    interruptVector = (int *) malloc(TRAP_VECTOR_SIZE * sizeof(int*)); // allocate space in physical memory
    
    // assign handler functions to indices
    interruptVector[TRAP_KERNEL] = TrapKernelHandler;
    interruptVector[TRAP_CLOCK] = TrapClockHandler;
    interruptVector[TRAP_ILLEGAL] = TrapIllegalHandler;
    interruptVector[TRAP_MEMORY] = TrapMemoryHandler;
    interruptVector[TRAP_MATH] = TrapMathHandler;
    interruptVector[TRAP_TTY_RECEIVE] = TrapTTYReceiveHandler;
    interruptVector[TRAP_TTY_TRANSMIT] = TrapTTYTransmitHandler;
    
    // fill in rest of empty slots
    for (size_t i = TRAP_DISK; i < TRAP_VECTOR_SIZE; i++)
    {
        interruptVector[i] = NULL;
    }

    // Build a boolean array that keeps track of free pages
    //RW: maybe we could do this after 
    buildFreePages(pmem_size);
    // Build 1 initial page table
    initPT();
    // Initialize terminals

    // Enable virtual memory
    WriteRegister(REG_VM_ENABLE, 1);
    // Create idle process
    // Create init process
    return;
}




void buildFreePages(unsigned int pmem_size) {
    num_pages = DOWN_TO_PAGE(pmem_size) >> PAGESHIFT;
    //keep track of free pages
    freePages = Malloc(total_pages * sizeof(int));

    //TOOD: iterate through and set freePages to indicate free (do a linked list? or just an array)
	TracePrintf("buildFreePages DONE \n"); 

}

void initPT() {
    //initial region 0 & 1 page tables are placed at the top page of region 1
    region0Pt = (struct pte *)(DOWN_TO_PAGE(VMEM_1_LIMIT) - 2 * PAGESIZE); //vmem should be the same as pmem at start
    region1Pt = (struct pte *)(DOWN_TO_PAGE(VMEM_1_LIMIT) - PAGESIZE);

    //setup initial ptes in region 1 page table and region 0 page table
    int page_itr;
    //init region 0 page table
    for (page_itr = 0; page_itr < KERNEL_STACK_PAGES; page_itr++) {  
        int index = PAGE_TABLE_LEN - page_itr - 1;
        region0Pt[index].pfn = index;
        region0Pt[index].uprot = 0;
        region0Pt[index].kprot = PROT_READ | PROT_WRITE;
        region0Pt[index].valid = 1;
    }

    //region 1 setup
	for (page_itr = VMEM_1_BASE >> PAGESHIFT; page_itr < (UP_TO_PAGE(kernel_break) >> PAGESHIFT); page_itr++) {
		region1Pt[page_itr].pfn = PAGE_TABLE_LEN + page_itr;
		region1Pt[page_itr].uprot = 0;
		region1Pt[page_itr].valid = 1;

		if (VMEM_1_BASE + pt_iter1 * PAGESIZE < (UP_TO_PAGE(&_etext) >> PAGESHIFT)) {
			region1Pt[page_itr].kprot = (PROT_READ | PROT_EXEC);
		} else {
			region1Pt[page_itr].kprot = (PROT_READ | PROT_WRITE);
		}
	}
    //Next: init values for Page Tables
    TracePrintf("initPT DONE \n"); 


}

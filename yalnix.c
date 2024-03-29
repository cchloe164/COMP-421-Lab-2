#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>

// #include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/loadinfo.h>

#include "sharedvar.c"
#include "handlers.c"

void *tty_buf; // buffer in virtual memory region 1

void buildFreePages(unsigned int pmem_size);
void RemoveHeadFromExitQueue(struct pcb *current);
void PushExitToExited(struct exitedChild *child, struct pcb *current);
void initPT();
int vm_enabled = false;

/*
    *  This is the primary entry point into the kernel:
    *
    *  ExceptionInfo *info
    *  unsigned int pmem_size
    *  void *orig_brk
    *  char **cmd_args
    */
extern void KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args)
{
    TracePrintf(0, "Starting Kernel...\n"); 
    currKernelBrk = orig_brk;
    
    /* Initialize interrupt vector table */
    TracePrintf(0, "Setting up interrupt vector table...\n"); 
    interruptVector = (void **) malloc(TRAP_VECTOR_SIZE * sizeof(int*)); // allocate space in physical memory

    // assign handler functions to indices
    interruptVector[TRAP_KERNEL] = TrapKernelHandler;
    interruptVector[TRAP_CLOCK] = TrapClockHandler;
    interruptVector[TRAP_ILLEGAL] = TrapIllegalHandler;
    interruptVector[TRAP_MEMORY] = TrapMemoryHandler;
    interruptVector[TRAP_MATH] = TrapMathHandler;
    interruptVector[TRAP_TTY_RECEIVE] = TrapTTYReceiveHandler;
    interruptVector[TRAP_TTY_TRANSMIT] = TrapTTYTransmitHandler;
    
    // fill in rest of empty slots
    int i;
    for (i = TRAP_DISK; i < TRAP_VECTOR_SIZE; i++)
    {
        interruptVector[i] = NULL;
    }
    WriteRegister(REG_VECTOR_BASE, (RCS421RegVal) &interruptVector[0]);
    
    buildFreePages(pmem_size);  // Build a boolean array that keeps track of free pages
    
    initPT();   // Build 1 initial page table

    // Initialize terminals

    // Enable virtual memory
    WriteRegister(REG_VM_ENABLE, 1);
    TracePrintf(0, "Virtual memory enabled...\n");
    vm_enabled = true;

    // Create idle process
    
    TracePrintf(1, "-------TraceTraceTrace5--------");
    struct pcb *pcb1 = malloc(sizeof(struct pcb));
    pcb1->region0 = &region0Pt[0];
    SetProcID(pcb1);
    LoadProgram("idle", cmd_args, info, pcb1->region0);
    curr_proc = pcb1;
    idle_pcb = pcb1;

    struct pcb *pcb2 = malloc(sizeof(struct pcb));
    SetProcID(pcb2);
    BuildRegion0(pcb2);

    pcb1->waiting = false;
    pcb2->waiting = false;
    pcb1->children_head = NULL;
    pcb1->children_tail = NULL;
    pcb2->children_tail = NULL;
    pcb2->children_head = NULL;

    pcb1->exited_children_head = NULL;
    pcb1->exited_children_tail = NULL;
    pcb2->exited_children_head = NULL;
    pcb2->exited_children_tail = NULL;

    int res = ContextSwitch(SwitchNewProc, &pcb1->ctx, (void *)pcb1, (void *)pcb2);
    if (res == 0) {
        TracePrintf(0, "ContextSwitch was successful!\n", res);
    } else {
        TracePrintf(0, "ERROR: ContextSwitch was unsuccessful.\n", res);
    }

    
    LoadProgram(cmd_args[0], cmd_args, info, pcb2->region0);
    TracePrintf(0, "END OF CODE REACHED!!!!\n");

    return;
}


/**
Build a structure to keep track of what page frames in physical memory are free. 
For this purpose, you might be able to use a linked list of physical frames, 
implemented in the frames themselves. Or you can have a separate structure, 
which is probably easier, though slightly less efficient. This list of free page 
frames should be based on the pmem_size argument passed to your KernelStart, but 
be careful not include any memory that is already in use by your kernel.
*/
void buildFreePages(unsigned int pmem_size) {
    //will allocate first before the kernel is allocated, then set the bits to PAGE_USED
	TracePrintf(0, "Building free pages...\n"); 
    //For testing: iterate and print free pages and print used pages
    num_pages = DOWN_TO_PAGE(pmem_size) >> PAGESHIFT;
    num_free_pages = num_pages;
    int page_itr;
    //keep track of free pages
    freePages = malloc(num_pages * sizeof(int));
    for (page_itr = 0; page_itr < num_pages; page_itr++) {
        freePages[page_itr] = PAGE_FREE;
    }
    //TODO: iterate through and set freePages to indicate free (do a linked list? or just an array)
	TracePrintf(0, "Finished building free pages\n"); 

}

/**
* The kernel does, however, control the mapping (i.e., the page table entry) that 
specifies this translation, by building and initializing the contents of the page table. 
This division of labor makes sense because memory references occur very frequently as the 
CPU executes instructions, whereas the virtual-to-physical address mapping (the page table) 
changes relatively infrequently and in accord with the process abstraction and memory protection 
policy implemented by the kernel.
*/
void initPT() { //TODO: add this as input struct pte *region0Pt
	TracePrintf(0, "Building page table...\n"); 
    //initial region 0 & 1 page tables are placed at the top page of region 1
    //setup initial ptes in region 1 page table and region 0 page table
    unsigned long vaddr;
    //init region 0 page table (see pg 22)
    // TracePrintf(0, "REGION 0\n");
    // TracePrintf(0, "base: %p\nlimit: %p\n", KERNEL_STACK_BASE, KERNEL_STACK_LIMIT);
    for (vaddr = KERNEL_STACK_BASE; vaddr < KERNEL_STACK_LIMIT; vaddr += PAGESIZE) {  
        int page = vaddr >> PAGESHIFT; // addr -> page number

        // TracePrintf(0, "page: %d addr: %p\n", page, page_itr);
        region0Pt[page].pfn = page;
        region0Pt[page].uprot = PROT_NONE;
        region0Pt[page].kprot = PROT_READ | PROT_WRITE;
        region0Pt[page].valid = 1;
        freePages[page] = PAGE_USED; //set the page as used in our freePages structure WHY?
        num_free_pages--;

    }
    //region 1 setup
    //iterate starting from VMEM_1_Base until the kernel break to establish PTs
    // TracePrintf(0, "base: %p\nbreak: %p\n", VMEM_1_BASE, currKernelBrk);
    unsigned int brk = (uintptr_t) currKernelBrk;
    for (vaddr = VMEM_1_BASE; vaddr < brk; vaddr += PAGESIZE) {
        int page = (vaddr >> PAGESHIFT) - PAGE_TABLE_LEN;
        TracePrintf(2, "\npage_itr: %p\nindex: %d\nbrk: %p\n", vaddr, page, brk);
        region1Pt[page].pfn = vaddr >> PAGESHIFT;
        region1Pt[page].uprot = PROT_NONE;
        region1Pt[page].valid = 1;

    //     /**
    //     from page 24:
    //     In particular, the page table entries for your kernel text should be set 
    //     to “read” and “execute” protection for kernel mode, and the page table 
    //     entries for your kernel data/bss/heap should be set to “read” and “write” 
    //     protection for kernel mode; the user mode protection for both kinds of kernel 
    //     page table entries should be “none” (no access).
    //     */
        if (VMEM_1_BASE + (page << PAGESHIFT) < (UP_TO_PAGE(&_etext))) { //up till the _etext
            region1Pt[page].kprot = (PROT_READ | PROT_EXEC);
        } else {
            region1Pt[page].kprot = (PROT_READ | PROT_WRITE);
        }

        TracePrintf(2, "Still initializing Page Table \n"); 
        freePages[vaddr >> PAGESHIFT] = PAGE_USED;
    };


    //Next: init values for Page Tables
    //set the REG_PTR0 and REG_PTR1
    RCS421RegVal ptr0;
    RCS421RegVal ptr1;
    ptr0 = (RCS421RegVal) &region0Pt; //TODO: maybe take this as input
    ptr1 = (RCS421RegVal) &region1Pt;
    WriteRegister(REG_PTR0, ptr0);
    WriteRegister(REG_PTR1, ptr1);
    TracePrintf(0, "Finished initializing Page Table \n"); 
}

/*
Before your kernel has enabled virtual memory, the entire physical memory 
of the machine is already available to your kernel. The job of SetKernelBrk 
in this case is very easy. You need only keep track of the location of the 
current break for the kernel. The initial location of the break is passed to 
your kernel’s KernelStart procedure as the orig_brk argument, as described in 
Section 3.3. Calls to SetKernelBrk, before virtual memory has been enabled, 
simply move this break location to the new location specified as addr in the 
SetKernelBrk call; for this project, the kernel break will never be moved down
 to a lower address than where it already is at. You will need to know the current
  location of your kernel’s break in order to correctly initialize page table entries 
  for your kernel’s Region 1 before enabling virtual memory.

  After you have enabled virtual memory, the job of the SetKernelBrk
   function that you will write becomes more complicated. After this point,
    your implementation of SetKernelBrk, when called with an argument of addr,
     must allocate physical memory page frames and map virtual pages as necessary
      to make addr the new kernel break.
*/
extern int SetKernelBrk(void *addr) {
    TracePrintf(0, "SET currKernelBreak from %p to addr %p \n", currKernelBrk, addr); 
    
    if (!vm_enabled) {
        //just need to change kernel break if vm is not enabled. otherwise do other stuff
        currKernelBrk = addr;// (do this in a separate if bc there's a chance the new addr is invalid)
        return 0;
    } else { //after fully enabling VM, need to change it
        int pagesNeeded;
        //find how many pages we need to make valid and used in phsyical memory 
        pagesNeeded = (UP_TO_PAGE((long)addr) >> PAGESHIFT) - (DOWN_TO_PAGE((long)currKernelBrk) >> PAGESHIFT);
        if (num_free_pages < pagesNeeded) {
            //not enough memory
            //do something here (TODO)
            TracePrintf(0, "Not enough memory for setKernelBrk\n");
            return ERROR;
        } else {
            int page;
            int currBrkPg = ((long)currKernelBrk >> PAGESHIFT) - PAGE_TABLE_LEN;
            TracePrintf(2, "-------TraceTraceTrace--------"); 
            for (page = 0; page < pagesNeeded; page++) {
                TracePrintf(2, "-------TraceTraceTrace2--------%d\n", currBrkPg + page);
                int newPage = findFreePage();

                region1Pt[currBrkPg + page].pfn = newPage;
                region1Pt[currBrkPg + page].uprot = PROT_NONE;
                region1Pt[currBrkPg + page].valid = 1;
                region1Pt[currBrkPg + page].kprot = (PROT_READ | PROT_WRITE);
                // freePages[currBrkPg + page] = PAGE_USED; //set the page as used in our freePages structure WHY?
                // num_free_pages--;
            }
            //TODO: flush?
            TracePrintf(2, "-------TraceTraceTrace3--------");
            WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
            
            currKernelBrk = addr;
            TracePrintf(2, "-------TraceTraceTrace4--------");
            return 0;

        }
    }
    
}



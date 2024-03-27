#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>

#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/loadinfo.h>

// #include "load.c"
#include "sharedvar.c"
#include "handlers.c"
#include "contextswitch.c"
// #include "contextswitch2.c"
#include "getpid.c"

void *tty_buf; // buffer in virtual memory region 1
// struct pte region0Pt[PAGE_TABLE_LEN], region1Pt[PAGE_TABLE_LEN]; // page table pointers
// void *currKernelBrk;
int next_proc_id = 0;

void TrapKernelHandler(ExceptionInfo *info);
void TrapClockHandler(ExceptionInfo *info);
void TrapIllegalHandler(ExceptionInfo *info);
void TrapMemoryHandler(ExceptionInfo *info);
void TrapMathHandler(ExceptionInfo *info);
void TrapTTYReceiveHandler(ExceptionInfo *info);
void TrapTTYTransmitHandler(ExceptionInfo *info);
void buildFreePages(unsigned int pmem_size);
void initPT();


int vm_enabled = false;
void freePage(int pfn);
int findFreePage();

int LoadProgram(char *name, char **args, ExceptionInfo *info, struct pte *ptr0);

extern int SetKernelBrk(void *addr);

extern int Fork(void);
extern int Exec(char *filename, char **argvec);
extern void Exit(int status) __attribute__ ((noreturn));;
extern int Wait(int *status_ptr);
// extern int GetPid_(void);
extern int Brk(void *addr);
extern int Delay(int clock_ticks);
extern int TtyRead(int tty_id, void *buf, int len);
extern int TtyWrite(int tty_id, void *buf, int len);
SavedContext *SwitchNewProc(SavedContext *ctxp, void *p1, void *p2);


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

    // Build a boolean array that keeps track of free pages
    //RW: maybe we could do this after 
    buildFreePages(pmem_size);
    // Build 1 initial page table
    // struct pte *region0Pt = malloc(PAGE_TABLE_LEN * sizeof(struct pte));
    // initPT(region0Pt);
    initPT();

    // Initialize terminals

    // Enable virtual memory
    WriteRegister(REG_VM_ENABLE, 1);
    TracePrintf(0, "Virtual memory enabled...");
    vm_enabled = true;
    // Create idle process
    //TODO:  The idle process should have Yalnix process ID 0. Is this done?
    struct pcb pcb1;
    pcb1.region0 = region0Pt;

    struct pcb pcb2;

    LoadProgram("idle", cmd_args, info, &(pcb1.region0));
    //TODO: create pcbs for each process. Write a create pcb function to create pcbs and create pcb for idle program?
    // Create init process; need to call this context switch
    //create pcb for init
    int res = ContextSwitch(SwitchNewProc, &pcb1->ctx, (void *)pcb1, (void *)pcb2);
    TracePrintf(0, "Result from ContextSwitch: %d", res);
    
    LoadProgram(cmd_args[0], cmd_args, info, &pcb2.region0);

    //TODO: read piazza @130

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
//will allocate first before the kernel is allocated, then set the bits to PAGE_USED
void buildFreePages(unsigned int pmem_size) {
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
    // TracePrintf(0, "\nbase: %p\nlimit: %p\n", KERNEL_STACK_BASE, KERNEL_STACK_LIMIT);
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
    // TracePrintf(0, "\nbase: %p\nbreak: %p\n", VMEM_1_BASE, currKernelBrk);
    unsigned int brk = (uintptr_t) currKernelBrk;
    for (vaddr = VMEM_1_BASE; vaddr < brk; vaddr += PAGESIZE) {
        int page = (vaddr >> PAGESHIFT) - 512;
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
        freePages[vaddr >> PAGESHIFT] = PAGE_USED; //page_itr >> PAGESHIFT?
        num_free_pages--;
    };


    //Next: init values for Page Tables
    //set the REG_PTR0 and REG_PTR1
    RCS421RegVal ptr0;
    RCS421RegVal ptr1;
    ptr0 = (RCS421RegVal) &region0Pt; //TODO: maybe take this as input
    ptr1 = (RCS421RegVal) &region1Pt;
    // TracePrintf(0, "PTR0 is here:%d", ptr0);
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
    TracePrintf(0, "SET currKernelBreak %p to addr %p \n", currKernelBrk, addr); 
    
    if (!vm_enabled) {
        //just need to change kernel break if vm is not enabled. otherwise do other stuff
        currKernelBrk = addr;// (do this in a separate if bc there's a chance the new addr is invalid)
        return 0;
    } else { //after fully enabling VM, need to change it
        int pagesNeeded;
        //find how many pages we need to make valid and used in phsyical memory 
        pagesNeeded = (UP_TO_PAGE((long)addr) >> PAGESHIFT) - ((long)currKernelBrk >> PAGESHIFT);
        if (num_free_pages < pagesNeeded) {
            //not enough memory
            //do something here (TODO)
            TracePrintf(0, "Not enough memory for setKernelBrk\n");
            return ERROR;
        } else {
            int page;
            int currBrkPg = (long)currKernelBrk >> PAGESHIFT;
            for (page = 0; page < pagesNeeded; page++) {
                
                int newPage = findFreePage();
                region1Pt[currBrkPg + page].pfn = newPage;
                region1Pt[currBrkPg + page].uprot = PROT_NONE;
                region1Pt[currBrkPg + page].valid = 1;
                region1Pt[currBrkPg + page].kprot = (PROT_READ | PROT_WRITE);
                // freePages[currBrkPg + page] = PAGE_USED; //set the page as used in our freePages structure WHY?
                // num_free_pages--;
            }
            //TODO: flush?

            currKernelBrk = addr;
            return 0;

        }
    }
    
}


// extern int SetKernelBrk(void *addr) {
//     (void)addr;
//     return 0;
// }

/* Kernel Trap Handlers */


extern int Fork(void) {
    TracePrintf(0, "Fork called!\n");
    return 0;
}
extern int Exec(char *filename, char **argvec) {
    (void)filename;
    (void)argvec;
    TracePrintf(0, "Exec called!\n");
    return 0;
}
extern void Exit(int status) {
    (void)status;
    TracePrintf(0, "Exit called!\n");
    while(1){}
}
extern int Wait(int *status_ptr) {
    (void)status_ptr;
    TracePrintf(0, "Wait called!\n");
    return 0;
}

extern int Brk(void *addr) {
    TracePrintf(0, "Brk called!\n");
    (void)addr;
    return 0;
}
extern int Delay(int clock_ticks) {
    (void)clock_ticks;
    TracePrintf(0, "Delay called!\n");
    return 0;
}
extern int TtyRead(int tty_id, void *buf, int len) {
    (void)tty_id;
    (void)buf;
    (void)len;
    TracePrintf(0, "TtyRead called!\n");
    return 0;
}
extern int TtyWrite(int tty_id, void *buf, int len) { 
    (void)tty_id;
    (void)buf;
    (void)len;
    TracePrintf(0, "TtyWrite called!\n");
    return 0;
}
/*
 *  Load a program into the current process's address space.  The
 *  program comes from the Unix file identified by "name", and its
 *  arguments come from the array at "args", which is in standard
 *  argv format.
 *
 *  Returns:
 *      0 on success
 *     -1 on any error for which the current process is still runnable
 *     -2 on any error for which the current process is no longer runnable
 *
 *  This function, after a series of initial checks, deletes the
 *  contents of Region 0, thus making the current process no longer
 *  runnable.  Before this point, it is possible to return ERROR
 *  to an Exec() call that has called LoadProgram, and this function
 *  returns -1 for errors up to this point.  After this point, the
 *  contents of Region 0 no longer exist, so the calling user process
 *  is no longer runnable, and this function returns -2 for errors
 *  in this case.
 */
int LoadProgram(char *name, char **args, ExceptionInfo *info, struct pte *ptr0)//TODO: add arguments for info and Region0 pointer that way we can clean it
{
    int fd;
    int status;
    struct loadinfo li;
    char *cp;
    char *cp2;
    char **cpp;
    char *argbuf;
    int i;
    unsigned long argcount;
    int size;
    int text_npg;
    int data_bss_npg;
    int stack_npg;

    TracePrintf(0, "LoadProgram '%s', args %p\n", name, args);

    if ((fd = open(name, O_RDONLY)) < 0) {
        TracePrintf(0, "LoadProgram: can't open file '%s'\n", name);
        return -1;
    }

    status = LoadInfo(fd, &li);
    TracePrintf(0, "LoadProgram: LoadInfo status %d\n", status);
    switch (status) {
        case LI_SUCCESS:
            break;
        case LI_FORMAT_ERROR:
            TracePrintf(0,
            "LoadProgram: '%s' not in Yalnix format\n", name);
            close(fd);
            return -1;
        case LI_OTHER_ERROR:
            TracePrintf(0, "LoadProgram: '%s' other error\n", name);
            close(fd);
            return -1;
        default:
            TracePrintf(0, "LoadProgram: '%s' unknown error\n", name);
            close(fd);
            return -1;
    }
    TracePrintf(0, "text_size 0x%lx, data_size 0x%lx, bss_size 0x%lx\n",
	li.text_size, li.data_size, li.bss_size);
    TracePrintf(0, "entry 0x%lx\n", li.entry);

    /*
     *  Figure out how many bytes are needed to hold the arguments on
     *  the new stack that we are building.  Also count the number of
     *  arguments, to become the argc that the new "main" gets called with.
     */
    size = 0;
    for (i = 0; args[i] != NULL; i++) {
	    size += strlen(args[i]) + 1;
    }
    argcount = i;
    TracePrintf(0, "LoadProgram: size %d, argcount %d\n", size, argcount);

    /*
     *  Now save the arguments in a separate buffer in Region 1, since
     *  we are about to delete all of Region 0.
     */
    cp = argbuf = (char *)malloc(size);
    for (i = 0; args[i] != NULL; i++) {
        strcpy(cp, args[i]);
        cp += strlen(cp) + 1;
    }
  
    /*
     *  The arguments will get copied starting at "cp" as set below,
     *  and the argv pointers to the arguments (and the argc value)
     *  will get built starting at "cpp" as set below.  The value for
     *  "cpp" is computed by subtracting off space for the number of
     *  arguments plus 4 (for the argc value, a 0 (AT_NULL) to
     *  terminate the auxiliary vector, a NULL pointer terminating
     *  the argv pointers, and a NULL pointer terminating the envp
     *  pointers) times the size of each (sizeof(void *)).  The
     *  value must also be aligned down to a multiple of 8 boundary.
     */
    cp = ((char *)USER_STACK_LIMIT) - size;
    cpp = (char **)((unsigned long)cp & (-1 << 4));	/* align cpp */
    cpp = (char **)((unsigned long)cpp - ((argcount + 4) * sizeof(void *)));

    text_npg = li.text_size >> PAGESHIFT;
    data_bss_npg = UP_TO_PAGE(li.data_size + li.bss_size) >> PAGESHIFT;
    stack_npg = (USER_STACK_LIMIT - DOWN_TO_PAGE(cpp)) >> PAGESHIFT;

    TracePrintf(0, "LoadProgram: text_npg %d, data_bss_npg %d, stack_npg %d\n", text_npg, data_bss_npg, stack_npg);

    /*
     *  Make sure we have enough *virtual* memory to fit everything within
     *  the size of a page table, including leaving at least one page
     *  between the heap and the user stack
     */
    if (MEM_INVALID_PAGES + text_npg + data_bss_npg + 1 + stack_npg + 1 + KERNEL_STACK_PAGES > PAGE_TABLE_LEN) {
        TracePrintf(0, 
        "LoadProgram: program '%s' size too large for VIRTUAL memory\n", 
        name);
        free(argbuf);
        close(fd);
        return -1;
    }

    TracePrintf(0, "made past virtual memory check\n");
    /*
     *  And make sure there will be enough *physical* memory to
     *  load the new program.
     */
    // >>>> The new program will require text_npg pages of text,
    // >>>> data_bss_npg pages of data/bss, and stack_npg pages of
    // >>>> stack.  In checking that there is enough free physical
    // >>>> memory for this, be sure to allow for the physical memory
    // >>>> pages already allocated to this process that will be
    // >>>> freed below before we allocate the needed pages for
    // >>>> the new program being loaded.

    if (data_bss_npg + text_npg + stack_npg > num_free_pages) {
        TracePrintf(0,
            "LoadProgram: program '%s' size too large for PHYSICAL memory\n",
            name);
        free(argbuf);
        close(fd);
        return -1;
    }
    TracePrintf(0, "made past phsyical memory check\n");

    // >>>> Initialize sp for the current process to (void *)cpp.
    // >>>> The value of cpp was initialized above.
    info->sp = (void *)cpp; //take this from info input
    /*
     *  Free all the old physical memory belonging to this process,
     *  but be sure to leave the kernel stack for this process (which
     *  is also in Region 0) alone.
     */
    // >>>> Loop over all PTEs for the current process's Region 0,
    // >>>> except for those corresponding to the kernel stack (between
    // >>>> address KERNEL_STACK_BASE and KERNEL_STACK_LIMIT).  For
    // >>>> any of these PTEs that are valid, free the physical memory
    // >>>> memory page indicated by that PTE's pfn field.  Set all
    // >>>> of these PTEs to be no longer valid.
    
    int currTable;
    for (currTable = 0; currTable < PAGE_TABLE_LEN; currTable++) {
        // TracePrintf(0, "made to this\n");
        if ((currTable < KERNEL_STACK_BASE >> PAGESHIFT) | (currTable > KERNEL_STACK_LIMIT)) {
            // TracePrintf(0, "made to this2\n");
            if (ptr0[currTable].valid == 1) { //not sure if these are supposed to be pointers or the actual object
                // TracePrintf(0, "made to this3\n");
                freePage(ptr0[currTable].pfn); //TODO: may need ptr0[currTable]->pfn here
                ptr0[currTable].valid = 0;

            }
            // TracePrintf(0, "made to this4\n");
            
        }
    }
    TracePrintf(0, "made to looping over PTEs (line 440)\n");
    
    /*
     *  Fill in the page table with the right number of text,
     *  data+bss, and stack pages.  We set all the text pages
     *  here to be read/write, just like the data+bss and
     *  stack pages, so that we can read the text into them
     *  from the file.  We then change them read/execute.
     */

    // >>>> Leave the first MEM_INVALID_PAGES number of PTEs in the
    // >>>> Region 0 page table unused (and thus invalid)

    for (i = 0; i < MEM_INVALID_PAGES; i++) {
        ptr0[i].valid = 0;
        
    }

    /* First, the text pages */
    // >>>> For the next text_npg number of PTEs in the Region 0
    // >>>> page table, initialize each PTE:
    // >>>>     valid = 1
    // >>>>     kprot = PROT_READ | PROT_WRITE
    // >>>>     uprot = PROT_READ | PROT_EXEC
    // >>>>     pfn   = a new page of physical memory

    int textIter;
    for (textIter = MEM_INVALID_PAGES; textIter < MEM_INVALID_PAGES + text_npg; textIter++) {
        ptr0[textIter].valid = 1;
        ptr0[textIter].kprot = (PROT_READ | PROT_WRITE);
        ptr0[textIter].uprot = (PROT_READ | PROT_EXEC);
        ptr0[textIter].pfn = findFreePage();
        
    }
    /* Then the data and bss pages */
    // >>>> For the next data_bss_npg number of PTEs in the Region 0
    // >>>> page table, initialize each PTE:
    // >>>>     valid = 1
    // >>>>     kprot = PROT_READ | PROT_WRITE
    // >>>>     uprot = PROT_READ | PROT_WRITE
    // >>>>     pfn   = a new page of physical memory
    int bssIter;
    for (bssIter = MEM_INVALID_PAGES + text_npg; bssIter < MEM_INVALID_PAGES + text_npg + data_bss_npg; bssIter++) {
        ptr0[bssIter].valid = 1;
        ptr0[bssIter].kprot = (PROT_READ | PROT_WRITE);
        ptr0[bssIter].uprot = (PROT_READ | PROT_WRITE);
        ptr0[bssIter].pfn = findFreePage();
        

    }
    //TODO: keep track of the top pof the stack here

    /* And finally the user stack pages */
    // >>>> For stack_npg number of PTEs in the Region 0 page table
    // >>>> corresponding to the user stack (the last page of the
    // >>>> user stack *ends* at virtual address USER_STACK_LIMIT),
    // >>>> initialize each PTE:
    // >>>>     valid = 1
    // >>>>     kprot = PROT_READ | PROT_WRITE
    // >>>>     uprot = PROT_READ | PROT_WRITE
    // >>>>     pfn   = a new page of physical memory
    int userIter;
    for (userIter = (int)(USER_STACK_LIMIT >> PAGESHIFT) - 1; userIter > (USER_STACK_LIMIT >> PAGESHIFT) - stack_npg - 1; userIter--) {
        ptr0[userIter].valid = 1;
        ptr0[userIter].kprot = (PROT_READ | PROT_WRITE);
        ptr0[userIter].uprot = (PROT_READ | PROT_WRITE);
        ptr0[userIter].pfn = findFreePage();
    }


    

    /*
     *  All pages for the new address space are now in place.  Flush
     *  the TLB to get rid of all the old PTEs from this process, so
     *  we'll be able to do the read() into the new pages below.
     */
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    
    /*
     *  Read the text and data from the file into memory.
     */
    if (read(fd, (void *)MEM_INVALID_SIZE, li.text_size+li.data_size) != (int) (li.text_size+li.data_size)) {
        TracePrintf(0, "LoadProgram: couldn't read for '%s'\n", name);
        free(argbuf);
        close(fd);
        // >>>> Since we are returning -2 here, this should mean to
        // >>>> the rest of the kernel that the current process should
        // >>>> be terminated with an exit status of ERROR reported
        // >>>> to its parent process.
        return -2;
    }
    
    close(fd);			/* we've read it all now */

    /*
     *  Now set the page table entries for the program text to be readable
     *  and executable, but not writable.
     */
    // >>>> For text_npg number of PTEs corresponding to the user text
    // >>>> pages, set each PTE's kprot to PROT_READ | PROT_EXEC.

    for (textIter = MEM_INVALID_PAGES; textIter < MEM_INVALID_PAGES + text_npg; textIter++) {
        ptr0[textIter].kprot = (PROT_READ | PROT_EXEC);
        
    }


    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    
    /*
     *  Zero out the bss
     */
    memset((void *)(MEM_INVALID_SIZE + li.text_size + li.data_size),'\0', li.bss_size);
     
    int z;
    for (z = 0; z < PAGE_TABLE_LEN ; z++) {
        if (ptr0[z].valid == 1) {
            TracePrintf(1, "PageTables tracing: entry %d contains PFN %d. \n", z, ptr0[z].pfn);
        } else {
            TracePrintf(1, "PageTables tracing: entry %d is invalid/empty. \n", z);
        }
    }
    /*
     *  Set the entry point in the ExceptionInfo.
     */
    // >>>> Initialize pc for the current process to (void *)li.entry
    info->pc = (void *)li.entry;
    TracePrintf(0, "made to this1. argcount is %d\n", argcount);
    /*
     *  Now, finally, build the argument list on the new stack.
     */
    *cpp++ = (char *)argcount;		/* the first value at cpp is argc */
    TracePrintf(0, "made to this2.\n");

    cp2 = argbuf;
    
    for (i = 0; i < (int) argcount; i++) {      /* copy each argument and set argv */
        *cpp++ = cp;
        strcpy(cp, cp2);
        cp += strlen(cp) + 1;
        cp2 += strlen(cp2) + 1;
    }
    
    free(argbuf);
    *cpp++ = NULL;	/* the last argv is a NULL pointer */
    *cpp++ = NULL;	/* a NULL pointer for an empty envp */
    *cpp++ = 0;		/* and terminate the auxiliary vector */

   
    /*
     *  Initialize all regs[] registers for the current process to 0,
     *  initialize the PSR for the current process also to 0.  This
     *  value for the PSR will make the process run in user mode,
     *  since this PSR value of 0 does not have the PSR_MODE bit set.
     */
    // >>>> Initialize regs[0] through regs[NUM_REGS-1] for the
    // >>>> current process to 0.
    // >>>> Initialize psr for the current process to 0.
    for (i = 0; i <= NUM_REGS-1; i++) {
        info->regs[i] = 0;
    }
    info->psr = 0;

    // TracePrintf(0, "Creating PCB structure...");
    // struct pcb *proc = malloc(sizeof(struct pcb));
    // proc->process_id = next_proc_id;
    // next_proc_id++;
    // TracePrintf(0, "done\n");

    return 0;
}


/**
frees a page
*/
void freePage(int pfn) {

    freePages[pfn] = PAGE_FREE;
    num_free_pages++;

}

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/loadinfo.h>
#include <fcntl.h>
#include <string.h>

#define	PAGE_FREE	0
#define PAGE_USED   1       

void *tty_buf; // buffer in virtual memory region 1
struct pte region0Pt[PAGE_TABLE_LEN], region1Pt[PAGE_TABLE_LEN]; // page table pointers
int *freePages;
int num_pages;
int num_free_pages;
void *currKernelBrk;

void TrapKernelHandler(ExceptionInfo *info);
void TrapClockHandler(ExceptionInfo *info);
void TrapIllegalHandler(ExceptionInfo *info);
void TrapMemoryHandler(ExceptionInfo *info);
void TrapMathHandler(ExceptionInfo *info);
void TrapTTYReceiveHandler(ExceptionInfo *info);
void TrapTTYTransmitHandler(ExceptionInfo *info);
void buildFreePages(unsigned int pmem_size);
void initPT(void *orig_brk);

void **interruptVector;


void freePage(int pfn);
int findFreePage();

int LoadProgram(char *name, char **args, ExceptionInfo *info, struct pte *ptr0);

extern int SetKernelBrk(void *addr);

extern int Fork(void);
extern int Exec(char *filename, char **argvec);
extern void Exit(int status) __attribute__ ((noreturn));;
extern int Wait(int *status_ptr);
extern int GetPid(void);
extern int Brk(void *addr);
extern int Delay(int clock_ticks);
extern int TtyRead(int tty_id, void *buf, int len);
extern int TtyWrite(int tty_id, void *buf, int len);


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
    initPT(orig_brk);

    // Initialize terminals

    // Enable virtual memory
    WriteRegister(REG_VM_ENABLE, 1);
    TracePrintf(0, "Virtual memory enabled...");
    // Create idle process
    LoadProgram(cmd_args[0], cmd_args, info, (struct pte *)REG_PTR0);
    // Create init process
    


    (void)info;
    (void)cmd_args;


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
void initPT(void *orig_brk) {
	TracePrintf(0, "Building page table...\n"); 
    //TODO: set the tags in the free  pages list to 0
    //initial region 0 & 1 page tables are placed at the top page of region 1
    //TODO: check these indice
    // region0Pt = (struct pte *)(DOWN_TO_PAGE(VMEM_1_LIMIT) - 2 * VMEM_REGION_SIZE); 
    // region1Pt = (struct pte *)(DOWN_TO_PAGE(VMEM_1_LIMIT) - VMEM_REGION_SIZE);

    //setup initial ptes in region 1 page table and region 0 page table
    unsigned long page_itr;
    //init region 0 page table (see pg 22)
    // for (page_itr = PMEM_BASE; page_itr < KERNEL_STACK_PAGES; page_itr++) {
    // TracePrintf(0, "REGION 0\n");
    // TracePrintf(0, "\nbase: %p\nlimit: %p\n", KERNEL_STACK_BASE, KERNEL_STACK_LIMIT);
    for (page_itr = KERNEL_STACK_BASE; page_itr < KERNEL_STACK_LIMIT; page_itr += PAGESIZE) {  
        // int index = PAGE_TABLE_LEN - page_itr - 1;
        int index = page_itr >> PAGESHIFT; // addr -> page number

        // TracePrintf(0, "page: %d addr: %p\n", index, page_itr);
        region0Pt[index].pfn = index;
        region0Pt[index].uprot = PROT_NONE;
        region0Pt[index].kprot = PROT_READ | PROT_WRITE;
        region0Pt[index].valid = 1;
        freePages[index] = PAGE_USED; //set the page as used in our freePages structure WHY?
        num_free_pages--;

    }
    //region 1 setup
    //iterate starting from VMEM_1_Base until the kernel break to establish PTs
    // TracePrintf(0, "\nbase: %p\nbreak: %p\n", VMEM_1_BASE, currKernelBrk);
    unsigned int brk = (uintptr_t) currKernelBrk;
    for (page_itr = VMEM_1_BASE; page_itr < brk; page_itr += PAGESIZE) {
        int index = (page_itr >> PAGESHIFT) - 512;
        TracePrintf(0, "\npage_itr: %p\nindex: %d\nbrk: %p\n", page_itr, index, brk);
        region1Pt[index].pfn = page_itr >> PAGESHIFT;
        region1Pt[index].uprot = PROT_NONE;
        region1Pt[index].valid = 1;

        if (VMEM_1_BASE + (index << PAGESHIFT) < (UP_TO_PAGE(&_etext))) { //up till the _etext
            region1Pt[index].kprot = (PROT_READ | PROT_EXEC);
        } else {
            region1Pt[index].kprot = (PROT_READ | PROT_WRITE);
        }

        TracePrintf(0, "Still initializing Page Table \n"); 
        freePages[page_itr >> PAGESHIFT] = PAGE_USED; //page_itr >> PAGESHIFT?
        num_free_pages--;
    };
    // for (page_itr = VMEM_1_BASE >> PAGESHIFT; page_itr < (UP_TO_PAGE(orig_brk) >> PAGESHIFT); page_itr++) {
    //     TracePrintf(0, "page_itr: %d\n", page_itr);
	// 	region1Pt[page_itr].pfn = PAGE_TABLE_LEN + page_itr;
	// 	region1Pt[page_itr].uprot = PROT_NONE;
	// 	region1Pt[page_itr].valid = 1;
    //     /**
    //     from page 24:
    //     In particular, the page table entries for your kernel text should be set 
    //     to “read” and “execute” protection for kernel mode, and the page table 
    //     entries for your kernel data/bss/heap should be set to “read” and “write” 
    //     protection for kernel mode; the user mode protection for both kinds of kernel 
    //     page table entries should be “none” (no access).
    //     */
	// 	if (VMEM_1_BASE + (page_itr << PAGESHIFT) < (UP_TO_PAGE(&_etext) << PAGESHIFT)) { //up till the _etext
	// 		region1Pt[page_itr].kprot = (PROT_READ | PROT_EXEC);
	// 	} else {
	// 		region1Pt[page_itr].kprot = (PROT_READ | PROT_WRITE);
	// 	}

    //     // freePages[PAGE_TABLE_LEN + page_itr] = PAGE_USED;
	// }

    //Next: init values for Page Tables
    //set the REG_PTR0 and REG_PTR1
    RCS421RegVal ptr0;
    RCS421RegVal ptr1;
    ptr0 = (RCS421RegVal) &region0Pt;
    ptr1 = (RCS421RegVal) &region1Pt;
    WriteRegister(REG_PTR0, ptr0);
    WriteRegister(REG_PTR1, ptr1);
    (void) orig_brk;
    TracePrintf(0, "Finished initializing Page Table \n"); 
}

extern int SetKernelBrk(void *addr) {
    TracePrintf(0, "SET currKernelBreak %p to addr %p \n", currKernelBrk, addr); 
    currKernelBrk = addr;
    //after fully enabling VM, need to change it
    return 0;
}


/* HANDLERS */

/**
 * This type of trap results from a “kernel call” (also called a “system call” or
 * “syscall”) trap instruction executed by the current user processes. Such a trap is used by user pro-
 * cesses to request some type of service from the operating system kernel, such as creating a new
 * process, allocating memory, or performing I/O. All different kernel call requests enter the kernel
 * through this single type of trap.
*/
void TrapKernelHandler(ExceptionInfo *info) {
    /**
     * This type of trap results from a “kernel call” (also called a “system call” or
     * “syscall”) trap instruction executed by the current user processes. Such a trap is used by user pro-
     * cesses to request some type of service from the operating system kernel, such as creating a new
     * process, allocating memory, or performing I/O. All different kernel call requests enter the kernel
     * through this single type of trap.
    */
    TracePrintf(0, "TRAP_KERNEL handler called!\n");
    // if (info->code == YALNIX_FORK) {
    //     info->regs[0] = Fork();
    // }
    // else if (info->code == YALNIX_EXEC)
    // {
    //     info->regs[0] = Exec((char *) info->regs[1], (char **)info->regs[2]);
    // }
    // else if (info->code == YALNIX_EXIT)
    // {
    //     Exit(0);
    // }
    // else if (info->code == YALNIX_WAIT)
    // {
    //     info->regs[0] = Wait((int *) info->regs[1]);
    // }
    // else if (info->code == YALNIX_GETPID)
    // {
    //     info->regs[0] = GetPid();
    // }
    // else if (info->code == YALNIX_BRK)
    // {
    //     info->regs[0] = Brk((void *) info->regs[1]);
    // }
    // else if (info->code == YALNIX_DELAY)
    // {
    //     info->regs[0] = Delay(info->regs[1]);
    // }
    // else if (info->code == YALNIX_TTY_READ)
    // {
    //     info->regs[0] = TtyRead(info->regs[1], (void *) info->regs[2], info->regs[3]);
    // }
    // else if (info->code == YALNIX_TTY_WRITE)
    // {
    //     info->regs[0] = TtyWrite(info->regs[1], (void *) info->regs[2], info->regs[3]);
    // }
    (void)info;
};

/**
 * This type of interrupt results from the machine’s hardware clock, which generates pe-
 * riodic clock interrupts. The period between clock interrupts is exaggerated in the simulated hardware
 * in order to avoid using up too much real CPU time on the shared CLEAR hardware systems.
*/
void TrapClockHandler(ExceptionInfo *info){
    /**
     * Your Yalnix kernel should implement round-robin process scheduling with a time
     * quantum per process of 2 clock ticks. After the current process has been running as the current
     * process continuously for at least 2 clock ticks, if there are other runnable processes on the ready
     * queue, perform a context switch to the next runnable process.
    */
    TracePrintf(0, "TRAP_CLOCK handler called!\n");
    // Delay(2);

    (void) info;
};

/**
 * This type of exception results from the execution of an illegal instruction by the
 * currently executing user process. An illegal instruction can be, for example, an undefined machine
 * language opcode or an illegal addressing mode.
*/
void TrapIllegalHandler(ExceptionInfo *info){
    /**Terminate the currently running Yalnix user process and print a message giving the
    process ID of the process and an explanation of the problem; and continue running other processes.*/
    TracePrintf(0, "TRAP_ILLEGAL handler called!\n");
    // if (info->code == TRAP_ILLEGAL_ILLOPC) {
    //     printf("Process %d: Illegal opcode", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_ILLOPN)
    // {
    //     printf("Process %d: Illegal operand", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_ILLADR)
    // {
    //     printf("Process %d: Illegal addressing mode", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_ILLTRP)
    // {
    //     printf("Process %d: Illegal software trap", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_PRVOPC)
    // {
    //     printf("Process %d: Privileged opcode", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_PRVREG)
    // {
    //     printf("Process %d: Privileged register", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_COPROC)
    // {
    //     printf("Process %d: Coprocessor error", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_BADSTK)
    // {
    //     printf("Process %d: Bad stack", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_KERNELI)
    // {
    //     printf("Process %d: Linux kernel sent SIGILL", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_USERIB)
    // {
    //     printf("Process %d: Received SIGILL or SIGBUS from user", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_ADRALN)
    // {
    //     printf("Process %d: Invalid address alignment", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_ADRERR)
    // {
    //     printf("Process %d: Non-existent physical address", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_OBJERR)
    // {
    //     printf("Process %d: Object-specific HW error", GetPid());
    // }
    // else if (info->code == TRAP_ILLEGAL_KERNELB)
    // {
    //     printf("Process %d: Linux kernel sent SIGBUS", GetPid());
    // } else {
    //     printf("Code not found.");
    // }
    
    Halt();
    (void)info;
};

/**
 * This type of exception results from an attempt to access memory at some virtual
 * address, when the access is disallowed by the hardware. The access may be disallowed because the
 * address is outside the virtual address range of the hardware (outside Region 0 and Region 1), because
 * the address is not mapped in the current page tables, or because the access violates the current page
 * protection specified in the corresponding page table entry.
*/
void TrapMemoryHandler(ExceptionInfo *info){
    /**
     * The kernel must determine if this exception represents an implicit request by the
     * current process to enlarge the amount of memory allocated to the process’s stack, as described in
     * more detail in Section 3.4.4. If so, the kernel enlarges the process’s stack to “cover” the address that
     * was being referenced that caused the exception (the addr field in the ExceptionInfo) and then
     * returns from the exception, allowing the process to continue execution with the larger stack. Other-
     * wise, terminate the currently running Yalnix user process and print a message giving the process ID
     * of the process and an explanation of the problem; and continue running other processes
    */
    // check if virtual addr is in region 0, below the stack, and above the break for the process
    TracePrintf(0, "TRAP_MEMORY handler called!\n");
    // int addri = (uintptr_t) info->addr;
    // if (addri > VMEM_0_BASE || addri < VMEM_0_LIMIT || addri < KERNEL_STACK_BASE) {
    //     SetKernelBrk(info->addr);
    // } 
    // else {
    //     if (info->code == TRAP_MEMORY_MAPERR)
    //     {
    //         printf("Process %d: No mapping at addr", GetPid());
    //     }
    //     else if (info->code == TRAP_MEMORY_ACCERR)
    //     {
    //         printf("Process %d: Protection violation at addr", GetPid());
    //     }
    //     else if (info->code == TRAP_MEMORY_KERNEL)
    //     {
    //         printf("Process %d: Linux kernel sent SIGSEGV at addr", GetPid());
    //     }
    //     else if (info->code == TRAP_MEMORY_USER)
    //     {
    //         printf("Process %d: Received SIGSEGV from user", GetPid());
    //     }
    //     else
    //     {
    //         printf("Code not found.");
    //     }

    //     Halt();
    // }
    (void)info;
};

/**
 * This type of exception results from any arithmetic error from an instruction executed
 * by the current user process, such as division by zero or an arithmetic overflow.
*/
void TrapMathHandler(ExceptionInfo *info){
    /**
     * Terminate the currently running Yalnix user process and print a message giving the
     * process ID of the process and an explanation of the problem; and continue running other processes
    */
    TracePrintf(0, "TRAP_MATH handler called!\n");
    // if (info->code == TRAP_MATH_INTDIV)
    // {
    //     printf("Process %d: Integer divide by zero", GetPid());
    // }
    // else if (info->code == TRAP_MATH_INTOVF)
    // {
    //     printf("Process %d: Integer overflow", GetPid());
    // }
    // else if (info->code == TRAP_MATH_FLTDIV)
    // {
    //     printf("Process %d: Floating divide by zero", GetPid());
    // }
    // else if (info->code == TRAP_MATH_FLTOVF)
    // {
    //     printf("Process %d: Floating overflow", GetPid());
    // }
    // else if (info->code == TRAP_MATH_FLTUND)
    // {
    //     printf("Process %d: Floating underflow", GetPid());
    // }
    // else if (info->code == TRAP_MATH_FLTRES)
    // {
    //     printf("Process %d: Floating inexact result", GetPid());
    // }
    // else if (info->code == TRAP_MATH_FLTINV)
    // {
    //     printf("Process %d: Invalid floating operation", GetPid());
    // }
    // else if (info->code == TRAP_MATH_FLTSUB)
    // {
    //     printf("Process %d: FP subscript out of range", GetPid());
    // }
    // else if (info->code == TRAP_MATH_KERNEL)
    // {
    //     printf("Process %d: Linux kernel sent SIGFPE", GetPid());
    // }
    // else if (info->code == TRAP_MATH_USER)
    // {
    //     printf("Process %d: Received SIGFPE from user", GetPid());
    // }
    // else
    // {
    //     printf("Code not found.");
    // }
   Halt();
   (void)info;
};

/**
 * This type of interrupt is generated by the terminal device controller hardware
 * when a line of input is completed from one of the terminals attached to the system.
*/
void TrapTTYReceiveHandler(ExceptionInfo *info){
    /**
     * This interrupt signifies that a new line of input is available from the terminal
     * indicated by the code field in the ExceptionInfo passed by reference by the hardware to this
     * interrupt handler function. The kernel should read the input from the terminal using a TtyReceive
     * hardware operation and if necessary should buffer the input line for a subsequent TtyRead kernel
     * call by some user process.
    */
    TracePrintf(0, "TRAP_TTYRECEIVE handler called!\n");
    // TtyReceive(info->code, tty_buf, TERMINAL_MAX_LINE);
    (void)info;
};

/**
 * This type of interrupt is generated by the terminal device controller hard-
 * ware when the current buffer of data previously given to the controller on a TtyTransmit instruc-
 * tion has been completely sent to the terminal.
*/
void TrapTTYTransmitHandler(ExceptionInfo *info){
    /**
     * This interrupt signifies that the previous TtyTransmit hardware oper-
     * ation on some terminal has completed. The specific terminal is indicated by the code field in the
     * ExceptionInfo passed by reference by the hardware to this interrupt handler function. The ker-
     * nel should complete the TtyWrite kernel call and unblock the blocked process that started this
     * TtyWrite kernel call that started this output. If other TtyWrite calls are pending for this termi-
     * nal, also start the next one using TtyTransmit.
    */
    TracePrintf(0, "TRAP_TTYTRANSMIT handler called!\n");
    // TtyWrite(info->code, tty_buf, TERMINAL_MAX_LINE);
    (void)info;
};


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
extern int GetPid(void) {
    TracePrintf(0, "GetPid called!\n");
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
int
LoadProgram(char *name, char **args, ExceptionInfo *info, struct pte *ptr0)//TODO: add arguments for info and Region0 pointer that way we can clean it
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
        return (-1);
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
            return (-1);
        case LI_OTHER_ERROR:
            TracePrintf(0, "LoadProgram: '%s' other error\n", name);
            close(fd);
            return (-1);
        default:
            TracePrintf(0, "LoadProgram: '%s' unknown error\n", name);
            close(fd);
            return (-1);
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
        return (-1);
    }

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
        return (-1);
    }

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
        if ((currTable < KERNEL_STACK_BASE) | (currTable > KERNEL_STACK_LIMIT)) {
            
            if (ptr0[currTable].valid == 1) { //not sure if these are supposed to be pointers or the actual object
                freePage(ptr0[currTable].pfn); //TODO: may need ptr0[currTable]->pfn here
                ptr0[currTable].valid = 0;

            }
        }
    }

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
    for (userIter = USER_STACK_LIMIT >> PAGESHIFT; userIter > (USER_STACK_LIMIT >> PAGESHIFT) - stack_npg; userIter--) {
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
        return (-2);
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

    /*
     *  Set the entry point in the ExceptionInfo.
     */
    // >>>> Initialize pc for the current process to (void *)li.entry
    info->pc = (void *)li.entry;

    /*
     *  Now, finally, build the argument list on the new stack.
     */
    *cpp++ = (char *)argcount;		/* the first value at cpp is argc */
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

    return (0);
}

/**
Finds a free page, returns the PFN or -1 if there are no free pages available.
*/
int findFreePage() {
    int page_itr;
    for (page_itr = 0; page_itr < num_pages; page_itr++) {
        if (freePages[page_itr] == PAGE_FREE) {
            freePages[page_itr] = PAGE_USED;
            return page_itr;
        }
    }
    return -1;
} 

/**
frees a page
*/
void freePage(int pfn) {

    freePages[pfn] = PAGE_FREE;

}
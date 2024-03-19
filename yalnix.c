#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>

#define	PAGE_FREE	0
#define PAGE_USED   1       

void *tty_buf; // buffer in virtual memory region 1
struct pte *region0Pt, *region1Pt;  //page table pointers
int *freePages;
int num_pages;

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
    /* Initialize interrupt vector table */
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

    // Build a boolean array that keeps track of free pages
    //RW: maybe we could do this after 
    buildFreePages(pmem_size);
    // Build 1 initial page table
    initPT(orig_brk);

    // Initialize terminals

    // Enable virtual memory
    WriteRegister(REG_VM_ENABLE, 1);
    // Create idle process
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
    //For testing: iterate and print free pages and print used pages
    num_pages = DOWN_TO_PAGE(pmem_size) >> PAGESHIFT;
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
    //TODO: set the tags in the free  pages list to 0
    //initial region 0 & 1 page tables are placed at the top page of region 1
    //TODO: check these indice
    region0Pt = (struct pte *)(DOWN_TO_PAGE(VMEM_1_LIMIT) - 2 * VMEM_REGION_SIZE); //vmem should be the same as pmem at start
    region1Pt = (struct pte *)(DOWN_TO_PAGE(VMEM_1_LIMIT) - VMEM_REGION_SIZE);

    //setup initial ptes in region 1 page table and region 0 page table
    int page_itr;
    //init region 0 page table (see pg 22)
    for (page_itr = PMEM_BASE; page_itr < KERNEL_STACK_PAGES; page_itr++) {  
        int index = PAGE_TABLE_LEN - page_itr - 1;
        region0Pt[index].pfn = index;
        region0Pt[index].uprot = PROT_NONE;
        region0Pt[index].kprot = PROT_READ | PROT_WRITE;
        region0Pt[index].valid = 1;
        freePages[index] = PAGE_USED;
    }

    //region 1 setup
    //iterate starting from VMEM_1_Base until the kernel break to establish PTs
	for (page_itr = VMEM_1_BASE >> PAGESHIFT; page_itr < (UP_TO_PAGE(orig_brk) >> PAGESHIFT); page_itr++) {
		region1Pt[page_itr].pfn = PAGE_TABLE_LEN + page_itr;
		region1Pt[page_itr].uprot = PROT_NONE;
		region1Pt[page_itr].valid = 1;
        /**
        from page 24:
        In particular, the page table entries for your kernel text should be set 
        to “read” and “execute” protection for kernel mode, and the page table 
        entries for your kernel data/bss/heap should be set to “read” and “write” 
        protection for kernel mode; the user mode protection for both kinds of kernel 
        page table entries should be “none” (no access).
        */
		if (VMEM_1_BASE + (page_itr << PAGESHIFT) < (UP_TO_PAGE(&_etext) << PAGESHIFT)) { //up till the _etext
			region1Pt[page_itr].kprot = (PROT_READ | PROT_EXEC);
		} else {
			region1Pt[page_itr].kprot = (PROT_READ | PROT_WRITE);
		}

        freePages[PAGE_TABLE_LEN + page_itr] = PAGE_USED;
	}

    //Next: init values for Page Tables

    //set the REG_PTR0 and REG_PTR1
    RCS421RegVal ptr0;
    RCS421RegVal ptr1;
    ptr0 = (RCS421RegVal) region0Pt;
    ptr1 = (RCS421RegVal) region1Pt;
    WriteRegister(REG_PTR0, ptr0);
    WriteRegister(REG_PTR1, ptr1);
    TracePrintf(0, "Finished initializing Page Table \n"); 
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
    if (info->code == YALNIX_FORK) {
        info->regs[0] = Fork();
    }
    else if (info->code == YALNIX_EXEC)
    {
        info->regs[0] = Exec((char *) info->regs[1], (char **)info->regs[2]);
    }
    else if (info->code == YALNIX_EXIT)
    {
        Exit(0);
    }
    else if (info->code == YALNIX_WAIT)
    {
        info->regs[0] = Wait((int *) info->regs[1]);
    }
    else if (info->code == YALNIX_GETPID)
    {
        info->regs[0] = GetPid();
    }
    else if (info->code == YALNIX_BRK)
    {
        info->regs[0] = Brk((void *) info->regs[1]);
    }
    else if (info->code == YALNIX_DELAY)
    {
        info->regs[0] = Delay(info->regs[1]);
    }
    else if (info->code == YALNIX_TTY_READ)
    {
        info->regs[0] = TtyRead(info->regs[1], (void *) info->regs[2], info->regs[3]);
    }
    else if (info->code == YALNIX_TTY_WRITE)
    {
        info->regs[0] = TtyWrite(info->regs[1], (void *) info->regs[2], info->regs[3]);
    }
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
    Delay(2);

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
    if (info->code == TRAP_ILLEGAL_ILLOPC) {
        printf("Process %d: Illegal opcode", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_ILLOPN)
    {
        printf("Process %d: Illegal operand", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_ILLADR)
    {
        printf("Process %d: Illegal addressing mode", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_ILLTRP)
    {
        printf("Process %d: Illegal software trap", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_PRVOPC)
    {
        printf("Process %d: Privileged opcode", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_PRVREG)
    {
        printf("Process %d: Privileged register", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_COPROC)
    {
        printf("Process %d: Coprocessor error", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_BADSTK)
    {
        printf("Process %d: Bad stack", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_KERNELI)
    {
        printf("Process %d: Linux kernel sent SIGILL", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_USERIB)
    {
        printf("Process %d: Received SIGILL or SIGBUS from user", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_ADRALN)
    {
        printf("Process %d: Invalid address alignment", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_ADRERR)
    {
        printf("Process %d: Non-existent physical address", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_OBJERR)
    {
        printf("Process %d: Object-specific HW error", GetPid());
    }
    else if (info->code == TRAP_ILLEGAL_KERNELB)
    {
        printf("Process %d: Linux kernel sent SIGBUS", GetPid());
    } else {
        printf("Code not found.");
    }
    
    Halt();
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
    int addri = (uintptr_t) info->addr;
    if (addri > VMEM_0_BASE || addri < VMEM_0_LIMIT || addri < KERNEL_STACK_BASE) {
        SetKernelBrk(info->addr);
    } 
    else {
        if (info->code == TRAP_MEMORY_MAPERR)
        {
            printf("Process %d: No mapping at addr", GetPid());
        }
        else if (info->code == TRAP_MEMORY_ACCERR)
        {
            printf("Process %d: Protection violation at addr", GetPid());
        }
        else if (info->code == TRAP_MEMORY_KERNEL)
        {
            printf("Process %d: Linux kernel sent SIGSEGV at addr", GetPid());
        }
        else if (info->code == TRAP_MEMORY_USER)
        {
            printf("Process %d: Received SIGSEGV from user", GetPid());
        }
        else
        {
            printf("Code not found.");
        }

        Halt();
    }
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
    if (info->code == TRAP_MATH_INTDIV)
    {
        printf("Process %d: Integer divide by zero", GetPid());
    }
    else if (info->code == TRAP_MATH_INTOVF)
    {
        printf("Process %d: Integer overflow", GetPid());
    }
    else if (info->code == TRAP_MATH_FLTDIV)
    {
        printf("Process %d: Floating divide by zero", GetPid());
    }
    else if (info->code == TRAP_MATH_FLTOVF)
    {
        printf("Process %d: Floating overflow", GetPid());
    }
    else if (info->code == TRAP_MATH_FLTUND)
    {
        printf("Process %d: Floating underflow", GetPid());
    }
    else if (info->code == TRAP_MATH_FLTRES)
    {
        printf("Process %d: Floating inexact result", GetPid());
    }
    else if (info->code == TRAP_MATH_FLTINV)
    {
        printf("Process %d: Invalid floating operation", GetPid());
    }
    else if (info->code == TRAP_MATH_FLTSUB)
    {
        printf("Process %d: FP subscript out of range", GetPid());
    }
    else if (info->code == TRAP_MATH_KERNEL)
    {
        printf("Process %d: Linux kernel sent SIGFPE", GetPid());
    }
    else if (info->code == TRAP_MATH_USER)
    {
        printf("Process %d: Received SIGFPE from user", GetPid());
    }
    else
    {
        printf("Code not found.");
    }
   Halt();
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
    TtyReceive(info->code, tty_buf, TERMINAL_MAX_LINE);
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
    TtyWrite(info->code, tty_buf, TERMINAL_MAX_LINE);
};
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>
#include <terminals.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>

int **interruptVector[TRAP_VECTOR_SIZE];

void TrapKernelHandler(ExceptionInfo *info);
void TrapClockHandler(ExceptionInfo *info);
void TrapIllegalHandler(ExceptionInfo *info);
void TrapMemoryHandler(ExceptionInfo *info);
void TrapMathHandler(ExceptionInfo *info);
void TrapTTYReceiveHandler(ExceptionInfo *info);
void TrapTTYTransmitHandler(ExceptionInfo *info);

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
    // Build 1 initial page table
    // Initialize terminals
    // Enable virtual memory
    // Create idle process
    // Create init process

    return;
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
    if (info->code == YALNIX_FORK) {
        *info->regs[0] = Fork();
    }
    else if (info->code == YALNIX_EXEC)
    {
        *info->regs[0] = Exec(info->regs[1], info->regs[2]);
    }
    else if (info->code == YALNIX_EXIT)
    {
        Exit(0);
    }
    else if (info->code == YALNIX_WAIT)
    {
        *info->regs[0] = Wait(info->regs[1]);
    }
    else if (info->code == YALNIX_GETPID)
    {
        *info->regs[0] = GetPid();
    }
    else if (info->code == YALNIX_BRK)
    {
        *info->regs[0] = Brk(info->regs[1]);
    }
    else if (info->code == YALNIX_DELAY)
    {
        *info->regs[0] = Delay(info->regs[1]);
    }
    else if (info->code == YALNIX_TTY_READ)
    {
        *info->regs[0] = TtyRead(info->regs[1], info->regs[2], info->regs[3]);
    }
    else if (info->code == YALNIX_TTY_WRITE)
    {
        *info->regs[0] = TtyWrite(info->regs[1], info->regs[2], info->regs[3]);
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
   Delay(2);
};

/**
 * This type of exception results from the execution of an illegal instruction by the
 * currently executing user process. An illegal instruction can be, for example, an undefined machine
 * language opcode or an illegal addressing mode.
*/
void TrapIllegalHandler(ExceptionInfo *info){
    /**Terminate the currently running Yalnix user process and print a message giving the
    process ID of the process and an explanation of the problem; and continue running other processes.*/
    if (info->code == TRAP_ILLEGAL_ILLOPC) {
        printf("Process %d: Illegal opcode", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_ILLOPN)
    {
        printf("Process %d: Illegal operand", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_ILLADR)
    {
        printf("Process %d: Illegal addressing mode", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_ILLTRP)
    {
        printf("Process %d: Illegal software trap", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_PRVOPC)
    {
        printf("Process %d: Privileged opcode", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_PRVREG)
    {
        printf("Process %d: Privileged register", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_COPROC)
    {
        printf("Process %d: Coprocessor error", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_BADSTK)
    {
        printf("Process %d: Bad stack", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_KERNELI)
    {
        printf("Process %d: Linux kernel sent SIGILL", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_USERIB)
    {
        printf("Process %d: Received SIGILL or SIGBUS from user", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_ADRALN)
    {
        printf("Process %d: Invalid address alignment", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_ADRERR)
    {
        printf("Process %d: Non-existent physical address", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_OBJERR)
    {
        printf("Process %d: Object-specific HW error", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_KERNELB)
    {
        printf("Process %d: Linux kernel sent SIGBUS", getpid());
    } else {
        printf("Code not found.")
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
    void *addr = info->addr;
    if (addr > VMEM_0_BASE || addr < VMEM_0_LIMIT || addr < KERNEL_STACK_BASE) {
        SetKernelBrk(addr);
    } 
    else {
        if (info->code == TRAP_MEMORY_MAPPER)
        {
            printf("Process %d: No mapping at addr", getpid());
        }
        else if (info->code == TRAP_MEMORY_ACCERR)
        {
            printf("Process %d: Protection violation at addr", getpid());
        }
        else if (info->code == TRAP_MEMORY_KERNEL)
        {
            printf("Process %d: Linux kernel sent SIGSEGV at addr", getpid());
        }
        else if (info->code == TRAP_MEMORY_USER)
        {
            printf("Process %d: Received SIGSEGV from user", getpid());
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
    if (info->code == TRAP_MATH_INTDIV)
    {
        printf("Process %d: Integer divide by zero", getpid());
    }
    else if (info->code == TRAP_MATH_INTOVF)
    {
        printf("Process %d: Integer overflow", getpid());
    }
    else if (info->code == TRAP_MATH_FLTDIV)
    {
        printf("Process %d: Floating divide by zero", getpid());
    }
    else if (info->code == TRAP_MATH_FLTOVF)
    {
        printf("Process %d: Floating overflow", getpid());
    }
    else if (info->code == TRAP_MATH_FLTUND)
    {
        printf("Process %d: Floating underflow", getpid());
    }
    else if (info->code == TRAP_MATH_FLTRES)
    {
        printf("Process %d: Floating inexact result", getpid());
    }
    else if (info->code == TRAP_MATH_FLTINV)
    {
        printf("Process %d: Invalid floating operation", getpid());
    }
    else if (info->code == TRAP_MATH_FLTSUB)
    {
        printf("Process %d: FP subscript out of range", getpid());
    }
    else if (info->code == TRAP_MATH_KERNEL)
    {
        printf("Process %d: Linux kernel sent SIGFPE", getpid());
    }
    else if (info->code == TRAP_MATH_USER)
    {
        printf("Process %d: Received SIGFPE from user", getpid());
    }
    else
    {
        printf("Code not found.")
    }
   Halt();
};

/**
 * This type of interrupt is generated by the terminal device controller hardware
 * when a line of input is completed from one of the terminals attached to the system.
*/
void TrapTTYReceiveHandler(ExceptionInfo *info);

/**
 * This type of interrupt is generated by the terminal device controller hard-
 * ware when the current buffer of data previously given to the controller on a TtyTransmit instruc-
 * tion has been completely sent to the terminal.
*/
void TrapTTYTransmitHandler(ExceptionInfo *info);

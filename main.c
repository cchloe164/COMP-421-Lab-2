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
void TrapClockHandler(ExceptionInfo *info);

/**
 * This type of exception results from the execution of an illegal instruction by the
 * currently executing user process. An illegal instruction can be, for example, an undefined machine
 * language opcode or an illegal addressing mode.
*/
void TrapIllegalHandler(ExceptionInfo *info){
    /**Terminate the currently running Yalnix user process and print a message giving the
    process ID of the process and an explanation of the problem; and continue running other processes.*/
    if (info->code == TRAP_ILLEGAL_ILLOPC) {
        TracePrintf(0, "Process %d: Illegal opcode", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_ILLOPN)
    {
        TracePrintf(0, "Process %d: Illegal operand", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_ILLADR)
    {
        TracePrintf(0, "Process %d: Illegal addressing mode", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_ILLTRP)
    {
        TracePrintf(0, "Process %d: Illegal software trap", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_PRVOPC)
    {
        TracePrintf(0, "Process %d: Privileged opcode", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_PRVREG)
    {
        TracePrintf(0, "Process %d: Privileged register", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_COPROC)
    {
        TracePrintf(0, "Process %d: Coprocessor error", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_BADSTK)
    {
        TracePrintf(0, "Process %d: Bad stack", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_KERNELI)
    {
        TracePrintf(0, "Process %d: Linux kernel sent SIGILL", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_USERIB)
    {
        TracePrintf(0, "Process %d: Received SIGILL or SIGBUS from user", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_ADRALN)
    {
        TracePrintf(0, "Process %d: Invalid address alignment", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_ADRERR)
    {
        TracePrintf(0, "Process %d: Non-existent physical address", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_OBJERR)
    {
        TracePrintf(0, "Process %d: Object-specific HW error", getpid());
    }
    else if (info->code == TRAP_ILLEGAL_KERNELB)
    {
        TracePrintf(0, "Process %d: Linux kernel sent SIGBUS", getpid());
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
void TrapMemoryHandler(ExceptionInfo *info);

/**
 * This type of exception results from any arithmetic error from an instruction executed
 * by the current user process, such as division by zero or an arithmetic overflow.
*/
void TrapMathHandler(ExceptionInfo *info);

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

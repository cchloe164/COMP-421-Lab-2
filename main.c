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




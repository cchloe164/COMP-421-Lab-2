#include <comp421/hardware.h>

#include "delay.c"
// #include "brk.c"

/* HANDLERS */

/**
 * This type of trap results from a “kernel call” (also called a “system call” or
 * “syscall”) trap instruction executed by the current user processes. Such a trap is used by user pro-
 * cesses to request some type of service from the operating system kernel, such as creating a new
 * process, allocating memory, or performing I/O. All different kernel call requests enter the kernel
 * through this single type of trap.
 */
void TrapKernelHandler(ExceptionInfo *info)
{
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
void TrapClockHandler(ExceptionInfo *info)
{   //Notes from justin's group
    //have a queue: ready queue and delay queue. Delay system call sets in pcb a field for delay. here, iterate through the delay queue and decrement the pcb delay field. 
    //also for delay, when adding from ready queue to delay queue, context switch to a ready process
    //also here in TrapClockHandler, have to do the Tick_ for the current active process (after doing the delay handling)
    
    //Iterate through the delay queue and decrement the clock ticks in the pcb field (could change this to a field in the queue objs)
    //start with a dummy head. iterate through and go until you get back to the dummy head. 
    struct queue_item *curr_proc_item = waiting_queue_head; //current item in the queue. iterate throug them all
    //TODO: add a dummy to the tail of the queue or wait until null
    // int procs_deleted = 0;
    // int i;
    TracePrintf(0, "TRAP_CLOCK handler called!\n");
    while (curr_proc_item != NULL) {
        curr_proc = curr_proc_item->proc;
        
        int ticks_left = curr_proc->delay_ticks;
        curr_proc->delay_ticks = ticks_left - 1; //tick it down
        if (curr_proc->delay_ticks <= 0) { // the process is done waiting; remove it from the delay queue and put it on the ready queue
            RemoveItemFromWaitingQueue(curr_proc_item);
            PushProcToQueue(curr_proc);
            // procs_deleted += 1;//question here: do we update the queue size here or later? 
        }
        curr_proc_item = curr_proc_item->next;
    }
    Tick_();

    (void)info;
};


/**
 * This type of exception results from the execution of an illegal instruction by the
 * currently executing user process. An illegal instruction can be, for example, an undefined machine
 * language opcode or an illegal addressing mode.
 */
void TrapIllegalHandler(ExceptionInfo *info)
{
    /**Terminate the currently running Yalnix user process and print a message giving the
    process ID of the process and an explanation of the problem; and continue running other processes.*/
    TracePrintf(0, "TRAP_ILLEGAL handler called!\n");
    // if (info->code == TRAP_ILLEGAL_ILLOPC) {
    //     printf("Process %d: Illegal opcode", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_ILLOPN)
    // {
    //     printf("Process %d: Illegal operand", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_ILLADR)
    // {
    //     printf("Process %d: Illegal addressing mode", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_ILLTRP)
    // {
    //     printf("Process %d: Illegal software trap", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_PRVOPC)
    // {
    //     printf("Process %d: Privileged opcode", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_PRVREG)
    // {
    //     printf("Process %d: Privileged register", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_COPROC)
    // {
    //     printf("Process %d: Coprocessor error", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_BADSTK)
    // {
    //     printf("Process %d: Bad stack", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_KERNELI)
    // {
    //     printf("Process %d: Linux kernel sent SIGILL", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_USERIB)
    // {
    //     printf("Process %d: Received SIGILL or SIGBUS from user", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_ADRALN)
    // {
    //     printf("Process %d: Invalid address alignment", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_ADRERR)
    // {
    //     printf("Process %d: Non-existent physical address", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_OBJERR)
    // {
    //     printf("Process %d: Object-specific HW error", GetPid_());
    // }
    // else if (info->code == TRAP_ILLEGAL_KERNELB)
    // {
    //     printf("Process %d: Linux kernel sent SIGBUS", GetPid_());
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
void TrapMemoryHandler(ExceptionInfo *info)
{
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
    //         printf("Process %d: No mapping at addr", GetPid_());
    //     }
    //     else if (info->code == TRAP_MEMORY_ACCERR)
    //     {
    //         printf("Process %d: Protection violation at addr", GetPid_());
    //     }
    //     else if (info->code == TRAP_MEMORY_KERNEL)
    //     {
    //         printf("Process %d: Linux kernel sent SIGSEGV at addr", GetPid_());
    //     }
    //     else if (info->code == TRAP_MEMORY_USER)
    //     {
    //         printf("Process %d: Received SIGSEGV from user", GetPid_());
    //     }
    //     else
    //     {
    //         printf("Code not found.");
    //     }

        Halt();
    // }
    (void)info;
};

/**
 * This type of exception results from any arithmetic error from an instruction executed
 * by the current user process, such as division by zero or an arithmetic overflow.
 */
void TrapMathHandler(ExceptionInfo *info)
{
    /**
     * Terminate the currently running Yalnix user process and print a message giving the
     * process ID of the process and an explanation of the problem; and continue running other processes
     */
    TracePrintf(0, "TRAP_MATH handler called!\n");
    // if (info->code == TRAP_MATH_INTDIV)
    // {
    //     printf("Process %d: Integer divide by zero", GetPid_());
    // }
    // else if (info->code == TRAP_MATH_INTOVF)
    // {
    //     printf("Process %d: Integer overflow", GetPid_());
    // }
    // else if (info->code == TRAP_MATH_FLTDIV)
    // {
    //     printf("Process %d: Floating divide by zero", GetPid_());
    // }
    // else if (info->code == TRAP_MATH_FLTOVF)
    // {
    //     printf("Process %d: Floating overflow", GetPid_());
    // }
    // else if (info->code == TRAP_MATH_FLTUND)
    // {
    //     printf("Process %d: Floating underflow", GetPid_());
    // }
    // else if (info->code == TRAP_MATH_FLTRES)
    // {
    //     printf("Process %d: Floating inexact result", GetPid_());
    // }
    // else if (info->code == TRAP_MATH_FLTINV)
    // {
    //     printf("Process %d: Invalid floating operation", GetPid_());
    // }
    // else if (info->code == TRAP_MATH_FLTSUB)
    // {
    //     printf("Process %d: FP subscript out of range", GetPid_());
    // }
    // else if (info->code == TRAP_MATH_KERNEL)
    // {
    //     printf("Process %d: Linux kernel sent SIGFPE", GetPid_());
    // }
    // else if (info->code == TRAP_MATH_USER)
    // {
    //     printf("Process %d: Received SIGFPE from user", GetPid_());
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
void TrapTTYReceiveHandler(ExceptionInfo *info)
{
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
void TrapTTYTransmitHandler(ExceptionInfo *info)
{
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
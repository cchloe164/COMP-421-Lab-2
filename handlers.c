// #include <comp421/hardware.h>

#include "delay.c"
#include "fork.c"
#include "brk.c"
#include "getpid.c"
#include "exec.c"
#include "exit.c"
#include "wait.c"
#include "ttyread.c"
#include "ttywrite.c"

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
    if (info->code == YALNIX_FORK) {
        TracePrintf(0, "TRAP_KERNEL handler found a fork.\n");
        info->regs[0] = ForkFunc();
    }
    else if (info->code == YALNIX_EXEC)
    {
        TracePrintf(0, "TRAP_KERNEL handler found a exec.\n");
        info->regs[0] = ExecFunc((char *) info->regs[1], (char **)info->regs[2], info);
    }
    else if (info->code == YALNIX_EXIT)
    {
        TracePrintf(0, "TRAP_KERNEL handler found a exit.\n");
        ExitFunc(0);
    }
    else if (info->code == YALNIX_WAIT)
    {
        TracePrintf(0, "TRAP_KERNEL handler found a wait.\n");
        info->regs[0] = WaitFunc((int *) info->regs[1]);
    }
    else if (info->code == YALNIX_GETPID)
    {
        TracePrintf(0, "TRAP_KERNEL handler found a getpid.\n");
        TracePrintf(0, "currproc: %d\n", curr_proc->process_id);
        info->regs[0] = GetPid_(curr_proc);
        
    }
    else if (info->code == YALNIX_BRK)
    {
        TracePrintf(0, "TRAP_KERNEL handler found a brk.\n");
        info->regs[0] = BrkFunc((void *) info->regs[1]);
    }
    else if (info->code == YALNIX_DELAY)
    {   TracePrintf(0, "TRAP_KERNEL handler found a delay.\n");
        info->regs[0] = DelayFunc(info->regs[1]);
    }
    else if (info->code == YALNIX_TTY_READ)
    {   //TODO
        info->regs[0] = TtyReadFunc(info->regs[1], (void *) info->regs[2], info->regs[3]);
    }
    else if (info->code == YALNIX_TTY_WRITE)
    {
        info->regs[0] = TtyWriteFunc(info->regs[1], (void *) info->regs[2], info->regs[3]);
    }
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
    struct pcb *current_proc;
    TracePrintf(0, "TRAP_CLOCK handler called!\n");
    while (curr_proc_item != NULL) {
        current_proc = curr_proc_item->proc;
        TracePrintf(1, "TRAP_CLOCK handler decrementing clock for waiting queue item with pid %d!\n", current_proc->process_id);
        
        int ticks_left = current_proc->delay_ticks;
        current_proc->delay_ticks = ticks_left - 1; //tick it down
        if (current_proc->delay_ticks <= 0) { // the process is done waiting; remove it from the delay queue and put it on the ready queue
            TracePrintf(1, "Item with id %d delay has expired! now moving to the ready queue\n", current_proc->process_id);
            RemoveItemFromWaitingQueue(curr_proc_item);
            PushProcToQueue(current_proc);

            TracePrintf(1, "Switching from process %d to process id %d\n", curr_proc->process_id, current_proc->process_id);
            ContextSwitch(SwitchExist, &curr_proc->ctx, (void *)curr_proc, (void *)current_proc);

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
    
    if (info->code == TRAP_ILLEGAL_ILLOPC) {
        TracePrintf(0, "Process %d: Illegal opcode", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_ILLOPN)
    {
        TracePrintf(0, "Process %d: Illegal operand", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_ILLADR)
    {
        TracePrintf(0, "Process %d: Illegal addressing mode", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_ILLTRP)
    {
        TracePrintf(0, "Process %d: Illegal software trap", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_PRVOPC)
    {
        TracePrintf(0, "Process %d: Privileged opcode", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_PRVREG)
    {
        TracePrintf(0, "Process %d: Privileged register", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_COPROC)
    {
        TracePrintf(0, "Process %d: Coprocessor error", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_BADSTK)
    {
        TracePrintf(0, "Process %d: Bad stack", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_KERNELI)
    {
        TracePrintf(0, "Process %d: Linux kernel sent SIGILL", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_USERIB)
    {
        TracePrintf(0, "Process %d: Received SIGILL or SIGBUS from user", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_ADRALN)
    {
        TracePrintf(0, "Process %d: Invalid address alignment", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_ADRERR)
    {
        TracePrintf(0, "Process %d: Non-existent physical address", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_OBJERR)
    {
        TracePrintf(0, "Process %d: Object-specific HW error", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_ILLEGAL_KERNELB)
    {
        TracePrintf(0, "Process %d: Linux kernel sent SIGBUS", GetPid_(curr_proc));
    } else {
        TracePrintf(0, "Code not found.");
    }
    ExitFunc(1);
    (void)info;
};

/**
 * This type of exception results from an attempt to access memory at some virtual
 * address, when the access is disallowed by the hardware. The access may be disallowed because the
 * address is outside the virtual address range of the hardware (outside Region 0 and Region 1), because
 * the address is not mapped in the current page tables, or because the access violates the current page
 * protection specified in the corresponding page table entry.
 */
void TrapMemoryHandler(ExceptionInfo *info) {
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

    int terminate = true;
    TracePrintf(0, "TRAP_MEMORY handler called!\n");
    unsigned long addri = (unsigned long) info->addr;
    unsigned long addr_vpn = DOWN_TO_PAGE((void *)info->addr) >> PAGESHIFT;


    int curr_limit = curr_proc->sp;
    TracePrintf(1, "KerneStack requested moving down the current stack bottom from %d to %d\n", curr_limit, addr_vpn);
    TracePrintf(1, "KerneStack requested moving down the current stack bottom from address %d to %d\n", (long)curr_limit << PAGESHIFT, addri);
    if (info->code == TRAP_MEMORY_MAPERR)
    {
        TracePrintf(0, "Process %d: No mapping at addr", GetPid_(curr_proc));
        //3 checks: is is in the kerenel stack? Is it less than the current process's break? (in the user heap)
        //otherwise, set region0 from cur sp-1 down to new vpn. set new sp to new sp vpn
        if (addri > KERNEL_STACK_BASE) {
            TracePrintf(0, "Process %d: address is in the kernel stack", GetPid_(curr_proc));
        } else if (addr_vpn < curr_proc->brk) {
            TracePrintf(0, "Process %d: address vpn %d is in the user heap at %d", GetPid_(curr_proc), addr_vpn, curr_proc->brk);
        } else {
            
            
            
            int pgs_up = curr_limit - addr_vpn;
            int i;
            TracePrintf(1, "KerneStack moving down the current stack bottom from %d to %d\n", curr_limit, addr_vpn);
            for (i = 0; i < pgs_up; i++) {
                //set up the user page
                TracePrintf(1, "adding page %d\n", addr_vpn + i);
                curr_proc->region0[addr_vpn + i].valid = 1;
                curr_proc->region0[addr_vpn + i].uprot = PROT_READ | PROT_WRITE;
                curr_proc->region0[addr_vpn + i].kprot = PROT_READ | PROT_WRITE;
                curr_proc->region0[addr_vpn + i].pfn = findFreePage();

            }
            // TracePrintf(0, "WER ARE HERE1\n");
            WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
            curr_proc->sp = addr_vpn;
            // info->sp = addr_vpn;
            terminate = false;
            // TracePrintf(0, "WER ARE HERE2\n");

        }
    } else if (info->code == TRAP_MEMORY_ACCERR)
    {
        TracePrintf(0, "Process %d: Protection violation at addr\n", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_MEMORY_KERNEL)
    {
        TracePrintf(0, "Process %d: Linux kernel sent SIGSEGV at addr\n", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_MEMORY_USER)
    {
        TracePrintf(0, "Process %d: Received SIGSEGV from user\n", GetPid_(curr_proc));
    }
    else
    {
        TracePrintf(0, "Code not found.\n");
    }
    if (terminate == 1) {
        TracePrintf(0, "WER ARE HERE2\n");
        ExitFunc(1);
    }
    // }
    (void)info;
}


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
    if (info->code == TRAP_MATH_INTDIV)
    {
        printf("Process %d: Integer divide by zero", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_MATH_INTOVF)
    {
        printf("Process %d: Integer overflow", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_MATH_FLTDIV)
    {
        printf("Process %d: Floating divide by zero", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_MATH_FLTOVF)
    {
        printf("Process %d: Floating overflow", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_MATH_FLTUND)
    {
        printf("Process %d: Floating underflow", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_MATH_FLTRES)
    {
        printf("Process %d: Floating inexact result", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_MATH_FLTINV)
    {
        printf("Process %d: Invalid floating operation", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_MATH_FLTSUB)
    {
        printf("Process %d: FP subscript out of range", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_MATH_KERNEL)
    {
        printf("Process %d: Linux kernel sent SIGFPE", GetPid_(curr_proc));
    }
    else if (info->code == TRAP_MATH_USER)
    {
        printf("Process %d: Received SIGFPE from user", GetPid_(curr_proc));
    }
    else
    {
        printf("Code not found.");
    }
    ExitFunc(1);
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
    TtyReceiveFunc(info->code);
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
    TtyTransmitFunc(info->code);
    (void)info;
};
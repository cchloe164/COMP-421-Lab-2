#include "contextswitch.c"

int ticks_passed = 0;

/**
Delay
*/
int DelayFunc(int clock_ticks)
{
    // edge cases: clockticks 0; return 0, negative, return error

    TracePrintf(0, "Delay called!\n");
    if (clock_ticks == 0)
    {
        return 0;
    }
    if (clock_ticks < 0)
    {
        return ERROR;
    }

    // create a waiting node, add this to the waiting queue (malloc it)
    struct queue_item *waiting_node = malloc(sizeof(struct queue_item));
    waiting_node->proc = curr_proc;
    waiting_node->ticks_left = clock_ticks;
    curr_proc->delay_ticks = clock_ticks;
    // TracePrintf(1, "Currentpid %d\n", curr_proc->procfess_id, proc2->process_id);
    RemoveProcFromReadyQueue(curr_proc);
    // keep pcb as a field, next and current, also a counter to know how much to wait for
    //  add the item to the waiting queue
    PushItemToWaitingQueue(waiting_node);
    /// could have a global clock var that keeps track of time, but i think it might be better to have timers that decerase for each process
    struct pcb *next_pcb = FindReadyPcb();
    ContextSwitch(SwitchExist, &curr_proc->ctx, (void *)curr_proc, (void *)next_pcb); // TODO: Change this to be next_pcb when FindReadyPcb is done
    //-make a new switch function ()

    // new switch function:
    // writeregister regptr0, (rcs421) destpcb ->reg0pt
    // writeregister(flush, flush all)
    // currpcb = destpcb
    // return &destpcb->ctx
    //  (void)clock_ticks;
    return 0;
}

/**
 * Your Yalnix kernel should implement round-robin process scheduling with a time
 * quantum per process of 2 clock ticks. After the current process has been running as the current
 * process continuously for at least 2 clock ticks, if there are other runnable processes on the ready
 * queue, perform a context switch to the next runnable process.
 */
extern void Tick_()
{
    TracePrintf(0, "Clock ticks passed: %d\n", ticks_passed);
    ticks_passed++;

    if (ticks_passed > 2)
    { // check if current process should be switched out
        // TracePrintf(0, "Time limit reached on Process %d!.\n", curr_proc->process_id);
        TracePrintf(0, "Time limit reached on Process!.\n");
        int res = SetNextProc();
        if (res == 1)
        {
            TracePrintf(0, "Clock ticks reset!\n");
            ticks_passed = 0; // reset timer
        }
        else
        {
            TracePrintf(0, "Current process will keep running!\n");
        }
    }
}

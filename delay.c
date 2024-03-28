int ticks_passed = 0;

/**
Delay
*/
extern int Delay(int clock_ticks) {
    //edge cases: 0; return 0, negative, return error
    //create a waiting node, add this to the waiting queue (malloc it)
    //keep pcb as a field, next and current, also a counter to know how much to wait for
    //
    ///they have a global clock var that keeps track of time
    //contextswitch(normalswitchfunc, &currpcb->cs, (void *)currPcb, (void *idlePcb);
    //-make a new switch function ()

    //new switch function
    //writeregister regptr0, (rcs421) destpcb ->reg0pt
    //writeregister(flush, flush all)
    //currpcb = destpcb
    //return &destpcb->ctx
    (void)clock_ticks;
    TracePrintf(0, "Delay called!\n");
    return 0;
}

/**
 * Your Yalnix kernel should implement round-robin process scheduling with a time
 * quantum per process of 2 clock ticks. After the current process has been running as the current
 * process continuously for at least 2 clock ticks, if there are other runnable processes on the ready
 * queue, perform a context switch to the next runnable process.
 */
extern void Tick_() {
    TracePrintf(0, "Clock ticks passed: %d\n", ticks_passed);
    ticks_passed++;

    if (ticks_passed > 2) { // check if current process should be switched out
        // TracePrintf(0, "Time limit reached on Process %d!.\n", curr_proc->process_id);
        TracePrintf(0, "Time limit reached on Process!.\n");
        int res = SetNextProc();
        if (res == 1) {
            TracePrintf(0, "Clock ticks reset!\n");
            ticks_passed = 0; // reset timer
        } else {
            TracePrintf(0, "Current process will keep running!\n");
        }
    }
}




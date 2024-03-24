int ticks_passed = 0;

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
        TracePrintf(0, "Time limit reached on Process %d!.\n", curr_proc->process_id);
        int res = SetNextProc();
        if (res == 1) {
            TracePrintf(0, "Clock ticks reset!\n");
            ticks_passed = 0; // reset timer
        } else {
            TracePrintf(0, "Current process will keep running!\n");
        }
    }
}




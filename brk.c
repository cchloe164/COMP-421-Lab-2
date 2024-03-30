void freePage(int pfn);
int BrkFunc(void *addr){
    TracePrintf(0, "BREAK CALLED!\n");
    unsigned long roundedBrk = UP_TO_PAGE(addr) >> PAGESHIFT;
    //TODO: store the current pcb in main so this can access it.

    //Two cases: move up vs move down

    //move down case: 
    if (roundedBrk < curr_proc->brk && roundedBrk >= MEM_INVALID_PAGES) {//TODO: test if this works?
        TracePrintf(1, "Break moving down the current break position\n");
        int pgs_down = curr_proc->brk - roundedBrk;
        int i;
        for (i = 0; i < pgs_down; i++) {
            //free the page
            freePage(curr_proc->brk + i);
            region0Pt[curr_proc->brk+i].valid = 0;
        }
        curr_proc->brk = roundedBrk;
        return 0;

    } else if (roundedBrk >= curr_proc->brk) { //TOOD:  && roundedBrk < curr_proc->userStack check to make sure it doesn't hit the user stack
        TracePrintf(1, "Break moving up the current break position from %d to %d\n", curr_proc->brk, roundedBrk);
        int pgs_up = roundedBrk - curr_proc->brk;
        int i;
        for (i = 0; i < pgs_up; i++) {
            //set up the user page
            region0Pt[curr_proc->brk + i].valid = 1;
			region0Pt[curr_proc->brk + i].uprot = (PROT_READ | PROT_WRITE);
			region0Pt[curr_proc->brk + i].kprot = (PROT_READ | PROT_WRITE);
			region0Pt[curr_proc->brk + i].pfn = findFreePage();
			
        }
        curr_proc->brk = roundedBrk;
        return 0;
    }
    return ERROR;
}
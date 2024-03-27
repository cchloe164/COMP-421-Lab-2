
int Brk(void *addr){
    TracePrintf(0, "BREAK CALLED!");
    int roundedBrk = UP_TO_PAGE(addr) >> PAGESHIFT;
    //TODO: store the current pcb in main so this can access it.

    //Two cases: move up vs move down

    //move down case: 
    if (roundedBrk < current_pcb->brk && roundedBrk >= MEM_INVALID_PAGES) {
        TracePrintf(0, "Break moving down the current break position\n");
        int pgs_down = current_pcb->brk - roundedBrk;
        int i;
        for (i = 0; i < pgs_down; i++) {
            //free the page
            freePage(current_pcb->brk + i);
            region0Pt[current_pcb->brk+i].valid = 0;
        }
        current_pcb->brk = roundedBrk;
        return 0;

    } else if (roundedBrk >= current_pcb->brk && roundedBrk < current_pcb->userStack) { //check to make sure it doesn't hit the user stack
        int pgs_up = roundedBrk - current_pcb->brk;
        int i;
        for (i = 0; i < pgs_up; i++) {
            //set up the user page
            region0Pt[current_pcb->brk + i].valid = 1;
			region0Pt[current_pcb->brk + i].uprot = (PROT_READ | PROT_WRITE);
			region0Pt[current_pcb->brk + i].kprot = (PROT_READ | PROT_WRITE);
			region0Pt[current_pcb->brk + i].pfn = findFreePage();
			
        }
        current_pcb->brk = roundedBrk;
        return 0;
    }
    return ERROR;
}
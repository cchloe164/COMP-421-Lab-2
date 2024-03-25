#include <comp421/hardware.h>

// context switching from an existing process to a new process
SavedContext *SwitchNewProc(SavedContext *ctxp, void *p1, void *p2) {
    
    struct pcb *proc1 = (struct pcb *) p1;
    struct pcb *proc2 = (struct pcb *) p2;

    // save context of proc 1
    proc1->ctx = ctxp;

    int page;
    for (page = 0; page < KERNEL_STACK_PAGES; page++) {
        int free_page = findFreePage(); // find free page in old proc's region 0
        if (free_page == -1) {
            TracePrintf(0, "ERROR: No more free pages!\n");
            return -1;
        } else {
            TracePrintf(0, "Page %d is free in Region 0!", new_page);
            // copy entry from old proc's kernel stack to new proc's
            long old_addr = proc1->kernel_stack << PAGESHIFT;
            long new_addr = new_page << PAGESHIFT;
            memcpy((void *)old_addr, (void *)new_addr, PAGESIZE);
        }
    }

    // flush all entries in region 0 from TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    // redirect current region 0 pointer to new process
    WriteRegister(REG_PTR0, (RCS421RegVal) proc2->kernel_stack);

    return proc2->ctx;
}

// context switching from an existing process to another existing process
SavedContext *SwitchExist(SavedContext *ctxp, void *p1, void *p2) {
    struct pcb *proc1 = (struct pcb *) p1;
    struct pcb *proc2 = (struct pcb *) p2;

    // save context of proc 1
    proc1->ctx = ctxp;

    // flush all entries in region 0 from TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    // redirect current region 0 pointer to new process
    WriteRegister(REG_PTR0, (RCS421RegVal) proc2->kernel_stack);

    return proc2->ctx;
}

// context switching to no process, exiting kernal
SavedContext *SwitchNoProc(SavedContext *ctxp, void *p1, void *p2) {
    
    free(freePages);
    free(interruptVector);

    (void) p1;
    (void) p2;

    return ctxp;
}
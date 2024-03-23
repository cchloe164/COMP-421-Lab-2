#include <hardware.h>

#include "sharedvar.c"

// context switching from an existing process to a new process
SavedContext *MySwitch(SavedContext *ctxp, void *p1, void *p2) {
    
    struct pcb *proc1 = (struct pcb *) p1;
    struct pcb *proc2 = (struct pcb *) p2;

    int page;
    for (page = 0; page < KERNEL_STACK_PAGES; page++) {
        // find free page in old proc's region 0
        int new_page;
        for (new_page = 0; new_page < num_pages; new_page++)
        {
            if (freePages[new_page] == PAGE_FREE) {
                TracePrintf("Page %d is free in Region 0!", new_page);
                break;
            }
        }

        // copy entry from old proc's kernel stack to new proc's
        memcpy(p1->kernal_stack >> PAGESHIFT ,new_page >> PAGESHIFT, PAGESIZE);
    }

    // flush all entries in region 0 from TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    // redirect current region 0 pointer to new process
    WriteRegister(REG_PTR0, (RCS421RegVal) p2->kernal_stack);

    return proc2->ctx;
}

// context switching from an existing process to another existing process
SavedContext *MySwitch(SavedContext *ctxp, void *p1, void *p2) {
    
    struct pcb *proc1 = (struct pcb *) p1;
    struct pcb *proc2 = (struct pcb *) p2;

    // flush all entries in region 0 from TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    // redirect current region 0 pointer to new process
    WriteRegister(REG_PTR0, (RCS421RegVal)p2->kernal_stack);

    return proc2->ctx;
}

// context switching to no process, exiting kernal
SavedContext *MySwitch(SavedContext *ctxp, void *p1, void *p2) {
    
    free(freePages);
    free(interruptVector);

    return ctxp;
}
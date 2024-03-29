#include <comp421/hardware.h>

// context switching from an existing process to a new process
SavedContext *SwitchNewProc(SavedContext *ctxp, void *p1, void *p2)
{
    struct pcb *proc1 = (struct pcb *)p1;
    struct pcb *proc2 = (struct pcb *)p2;

    TracePrintf(0, "Switching from existing process %d to new process %d\n", proc1->process_id, proc2->process_id);

    // save context of proc 1
    proc2->ctx = *ctxp;

    // borrow from the top of the Region 1 page table
    int temp_vpn = BorrowR1Page();
    struct pte *temp = &region1Pt[temp_vpn]; 
    struct pte temp_copy = *temp;   // store original pte info

    // find a new Region0 virtual pointer
    unsigned long source_addr;
    for (source_addr = KERNEL_STACK_BASE; source_addr < KERNEL_STACK_LIMIT; source_addr += PAGESIZE)
    {
        int page = source_addr >> PAGESHIFT;
        TracePrintf(0, "Copying source page %d to dest page %d...\n", page, temp_vpn);
        
        // set the temp borrowed to have the same pfn as the new region 0 PTE
        temp->uprot = PROT_NONE;
        temp->kprot = PROT_READ | PROT_WRITE;
        temp->valid = 1;
        temp->pfn = (proc2->region0[page]).pfn;

        // copy from the current region 0 to the new region 0 (pseudo region 0)
        unsigned long dest_addr = (temp_vpn + PAGE_TABLE_LEN) << PAGESHIFT;
        TracePrintf(0, "dest addr: %p\tsource addr: %p\n", dest_addr, source_addr);
        TracePrintf(0, "dest pfn: %d\n", temp->pfn);
        memcpy((void *) dest_addr, (void *) source_addr, PAGESIZE);

        TracePrintf(0, "done\n");

        // flush all entries in region 0 from TLB at that specific address
        WriteRegister(REG_TLB_FLUSH, dest_addr);
    }

    region1Pt[temp_vpn] = temp_copy;    // restore borrowed pte from R1
    freePages[temp_vpn + PAGE_TABLE_LEN] = PAGE_FREE;
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

    curr_proc = proc2;
    unsigned long phys_addr = (unsigned long)((PAGE_TABLE_LEN + proc2->free_vpn) << PAGESHIFT);
    
    TracePrintf(0, "Writing REG0_PTR0 to %p...", phys_addr);
    WriteRegister(REG_PTR0, (RCS421RegVal)phys_addr);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    TracePrintf(0, "done\n");

    // return the ctx
    return &proc2->ctx;
}


// get copy of context and create kernel stack
SavedContext *SwitchFork(SavedContext *ctxp, void *p1, void *p2)
{
    struct pcb *parent = (struct pcb *)p1;
    struct pcb *child = (struct pcb *)p2;

    TracePrintf(0, "Context switching from parent process %d to child process %d\n", parent->process_id, child->process_id);

    // save context of proc 1
    child->ctx = *ctxp;

    // borrow from the top of the Region 1 page table
    int temp_vpn = BorrowR1Page();
    struct pte *temp = &region1Pt[temp_vpn];
    struct pte temp_copy = *temp; // store original pte info
    temp->uprot = PROT_NONE;
    temp->kprot = PROT_READ | PROT_WRITE;
    temp->valid = 1;

    // find a new Region0 virtual pointer
    unsigned long addr;
    for (addr = VMEM_0_BASE; addr < VMEM_0_LIMIT; addr += PAGESIZE)
    {
        int page = addr >> PAGESHIFT;
        TracePrintf(1, "Copying source page %d to dest page %d...\n", page, temp_vpn);

        // copy over pte settings
        child->region0[page].valid = parent->region0[page].valid;

        if (child->region0[page].valid == 1)
        {
            TracePrintf(1, "Setting up valid PTE page %d...\n", page);
            child->region0[page].uprot = parent->region0[page].uprot;
            child->region0[page].kprot = parent->region0[page].kprot;
            child->region0[page].pfn = findFreePage(); // allocate physical memory

            // set the temp borrowed to have the same pfn as the new region 0 PTE
            temp->pfn = (child->region0[page]).pfn;

            // copy from the current region 0 to the new region 0 (pseudo region 0)
            unsigned long dest_addr = (temp_vpn + PAGE_TABLE_LEN) << PAGESHIFT;
            TracePrintf(1, "dest addr: %p\tsource addr: %p\n", dest_addr, addr);
            TracePrintf(1, "dest pfn: %d\n", temp->pfn);
            TracePrintf(1, "memcpying content from page %d to page %d using R1 page %d...", page, child->region0[page].pfn, temp_vpn);
            memcpy((void *)dest_addr, (void *)addr, PAGESIZE);
            TracePrintf(1, "done\n");

            // flush all entries in region 0 from TLB at that specific address
            WriteRegister(REG_TLB_FLUSH, dest_addr);
        }
    }
    // TracePrintf(0, "508 valid: %d", parent->region0[508].valid);

    region1Pt[temp_vpn] = temp_copy; // restore borrowed pte from R1
    freePages[temp_vpn + PAGE_TABLE_LEN] = PAGE_FREE;
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

    curr_proc = child;
    unsigned long phys_addr = (unsigned long)((PAGE_TABLE_LEN + child->free_vpn) << PAGESHIFT);

    TracePrintf(0, "Writing REG0_PTR0 to %p...", phys_addr);
    WriteRegister(REG_PTR0, (RCS421RegVal)phys_addr);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    TracePrintf(0, "done\n");
    PushProcToQueue(parent);
    // return the ctx
    return &child->ctx;
}


// context switching from an existing process to another existing process
SavedContext *SwitchExist(SavedContext *ctx, void *p1, void *p2) {

    struct pcb *proc1 = (struct pcb *) p1;
    struct pcb *proc2 = (struct pcb *) p2;

    TracePrintf(0, "Switching to an existing process, from process %d to process %d\n", proc1->process_id, proc2->process_id);
    // save context of proc 1
    proc1->ctx = *ctx;

    // flush all entries in region 0 from TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    // redirect current region 0 pointer to new process
    WriteRegister(REG_PTR0, (RCS421RegVal) proc2->region0);

    curr_proc = p2;

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    return &proc2->ctx;
}




// context switching from an existing process to another existing process
SavedContext *SwitchExit(SavedContext *ctx, void *p1, void *p2) {

    struct pcb *proc1 = (struct pcb *) p1;
    struct pcb *proc2 = (struct pcb *) p2;

    TracePrintf(0, "Switching to an existing process out of an exiting process, from process %d to process %d\n", proc1->process_id, proc2->process_id);
    // save context of proc 1
    // proc1->ctx = *ctx;

    (void)ctx;
    int vpn;
    TracePrintf(0, "We have arrived here2 in SwitchExit!\n");
    for (vpn = VMEM_0_BASE; vpn < VMEM_0_LIMIT; vpn += PAGESIZE) {
        int page = vpn >> PAGESHIFT;
        // TracePrintf(0, "We have arrived here3 in SwitchExit!\n");
        // TracePrintf(0, "We have arrived here3 in SwitchExit! %d\n", proc1->region0[vpn] == NULL);
        struct pte curr_pte = proc1->region0[page];
        // TracePrintf(0, "We have arrived here4 in SwitchExit!\n");

        if (curr_pte.valid == 1) {
            freePage(curr_pte.pfn);
            curr_pte.valid = 0;
        }
    }
    // flush all entries in region 0 from TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    // redirect current region 0 pointer to new process
    WriteRegister(REG_PTR0, (RCS421RegVal) proc2->region0);

    curr_proc = p2;

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
    return &proc2->ctx;
}

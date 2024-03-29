unsigned int next_process_id; // sequentially incremented with each new process created
// cloning no loadprogram nor context switch

//need to copy savedcontext in fork (for the contextswitch) with a memcpy
/***
The Fork kernel call is the only way an existing process can create a new process in Yalnix 
(there is no inherent limit on the maximum number of processes that can exist at once). 
The memory image of the new process (the child) is a copy of that of the process calling Fork 
(the parent). When the Fork call completes, both the parent process and the child process return 
(separately) from the Fork kernel call as if each had been the one to call Fork, since the child 
s a copy of the parent. The only distinction is the fact that the return value in the calling (parent) 
process is the (nonzero) process ID of the new (child) process, while the value returned in the child is 0;
 as noted in Section 7.1, the process ID for each (real) process should be greater than 0 
 (you should use the process ID value of 0 for your idle process). When returning from a successful Fork, 
 you can return first either as the new child process or as the parent process (think about why you might want 
 to do one or the other). If, for any reason, the new process cannot be created, Fork instead returns the value ERROR
  to the calling process.
*/
int ForkFunc(void) {

}

SavedContext *SwitchNewProc(SavedContext *ctxp, void *p1, void *p2)
{
    struct pcb *proc1 = (struct pcb *)p1;
    struct pcb *proc2 = (struct pcb *)p2;

    TracePrintf(0, "Switching from existing process %d to new process %d\n", proc1->process_id, proc2->process_id);

    // save context of proc 1
    proc2->ctx = *ctxp; // WHY?
    // (void) ctxp;

    //had a global variable that filled from the top of VMEM1limit (make sure not to touch the kernelstack pages. he had a buffer)
    int temp_vpn = 0;   // TODO: implement case where no invalid ptes are found
    unsigned long vaddr;
    for (vaddr = VMEM_1_BASE; vaddr < VMEM_1_LIMIT; vaddr += PAGESIZE) {
        int page = (vaddr >> PAGESHIFT) - PAGE_TABLE_LEN;
        TracePrintf(2, "\nvaddr: %p\nvpn: %d\nlimit: %p\n", vaddr, page, VMEM_1_LIMIT);
        if (region1Pt[page].valid == 0) {
            temp_vpn = page;
            break;
        }
    }
    struct pte *temp = &region1Pt[temp_vpn]; // borrow from the top of the Region 1 page table

    // region1Pt[PAGE_TABLE_LEN - 2] = proc2->pcb_reg0_pfn; //reset the pointer
    // find a new Region0 virtual pointer (TODO: find this for the bit stuff)
    // long new_reg0 = 0;
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

        // TracePrintf(0, "Address of old_addr: %d\n", old_addr);
        // TracePrintf(0, "Address of new_addr: %d\n", new_addr);
        // void *memcpy(void *dest, const void * src, size_t n)
        // VMEM_0_LIMIT - (page + 1) should be the top pof
        // finally copy.vc

        // copy from the current region 0 to the new region 0 (pseudo region 0)
        unsigned long dest_addr = (temp_vpn + PAGE_TABLE_LEN) << PAGESHIFT;
        TracePrintf(0, "dest addr: %p\tsource addr: %p\n", dest_addr, source_addr);
        TracePrintf(0, "dest pfn: %d\n", temp->pfn);
        memcpy((void *) dest_addr, (void *) source_addr, PAGESIZE);

        TracePrintf(0, "done\n");

        // flush all entries in region 0 from TLB // at that specific address
        WriteRegister(REG_TLB_FLUSH, dest_addr);
    }

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
    curr_proc = proc2;
    // redirect current region 0 pointer to new process
    // region0Pt = *proc2->kernel_stack;
    // unsigned long newR0paddr = (unsigned long)&proc2->region0[0];
    // unsigned long newR0paddr = (unsigned long)proc2->region0;
    // TracePrintf(0, "r0 vpn: %d\n", newR0paddr >> PAGESHIFT);
    // TracePrintf(0, "r0 offset: %d\n", newR0paddr & PAGEOFFSET);

    // proc2->region0[508].valid=1;
    // proc2->region0[507].valid=1;
    // proc2->region0[509].valid=1;
    // TracePrintf(0, "newr0paddr created\n");
    //unsigned long phys_addr = (unsigned long)((region1Pt[(newR0paddr >> PAGESHIFT)- PAGE_TABLE_LEN].pfn << PAGESHIFT)| (newR0paddr & PAGEOFFSET));
    unsigned long phys_addr = (unsigned long)((PAGE_TABLE_LEN + proc2->free_vpn) << PAGESHIFT);
    // unsigned long phys_addr = proc2->region0;
    
    TracePrintf(0, "Writing REG0_PTR0 to %p...", phys_addr);
    // TracePrintf(0, "index %d created\n", newR0paddr >> PAGESHIFT);
    // TracePrintf(0, "Region 1 entry %d\n", (newR0paddr >> PAGESHIFT)- PAGE_TABLE_LEN);
    // TracePrintf(0, "phys_addr pfn %d created\n", region1Pt[(newR0paddr >> PAGESHIFT)- PAGE_TABLE_LEN].pfn); // this is correct
    WriteRegister(REG_PTR0, (RCS421RegVal)phys_addr);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    TracePrintf(0, "done\n");

    // return the ctx
    return &proc2->ctx;
}
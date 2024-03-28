#include <comp421/hardware.h>

// #include "../COMP-421-Lab-2/sharedvar.c"

// context switching from an existing process to a new process
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
        int page = (vaddr >> PAGESHIFT) - 512;
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
    // redirect current region 0 pointer to new process
    // region0Pt = *proc2->kernel_stack;
    // unsigned long newR0paddr = (unsigned long)&proc2->region0;
    unsigned long newR0paddr = (unsigned long)&proc2->region0[0];
    WriteRegister(REG_PTR0, (RCS421RegVal)newR0paddr);
    TracePrintf(0, "Page Table vpn 468 pfn: %d\tvalid: %d\n", proc2->region0[468].pfn, proc2->region0[468].valid);
    
    // return the ctx
    return &proc2->ctx;
}

// // // context switching from an existing process to another existing process
// // SavedContext *SwitchExist(SavedContext *ctx, void *p1, void *p2) {
// //     struct pcb *proc1 = (struct pcb *) p1;
// //     struct pcb *proc2 = (struct pcb *) p2;

// //     // save context of proc 1
// //     proc1->ctx = ctx;

// //     // flush all entries in region 0 from TLB
// //     WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

// //     // redirect current region 0 pointer to new process
// //     WriteRegister(REG_PTR0, (RCS421RegVal) proc2->kernel_stack);

// //     return proc2->ctx;
// // }

// // // context switching to no process, exiting kernal
// // SavedContext *SwitchNoProc(SavedContext *ctx, void *p1, void *p2) {
    
// //     free(freePages);
// //     free(interruptVector);

// //     (void) p1;
// //     (void) p2;

// //     return ctx;
// // }
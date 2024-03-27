#include <comp421/hardware.h>

// #include "../COMP-421-Lab-2/sharedvar.c"

// context switching from an existing process to a new process
SavedContext *SwitchNewProc(SavedContext *ctxp, void *p1, void *p2)
{
    struct pcb *proc1 = (struct pcb *)p1;
    struct pcb *proc2 = (struct pcb *)p2;

    TracePrintf(0, "Switching from existing process %d to new process %d\n", proc1->process_id, proc2->process_id);

    // save context of proc 1
    proc1->ctx = *ctxp;
    //had a global variable that filled from the top of VMEM1limit (make sure not to touch the kernelstack pages. he had a buffer)
    struct pte *new_reg0 = &region1Pt[(long)currKernelBrk >> PAGESHIFT]; // find a free virtual page to store the region 0 PT
    proc2->region0 = new_reg0;
    // allocate a new page for the second process PCB in

    new_reg0->pfn = findFreePage();
    new_reg0->uprot = PROT_NONE;
    new_reg0->kprot = (PROT_READ | PROT_WRITE);
    new_reg0->valid = 1;
    proc2->reg0_pfn = (long)&new_reg0;
    // int page;
    struct pte tempPT = ((struct pte *)region1Pt)[PAGE_TABLE_LEN - 1]; // borrow from the top of the Region 1 page table

    // region1Pt[PAGE_TABLE_LEN - 2] = proc2->pcb_reg0_pfn; //reset the pointer
    // find a new Region0 virtual pointer (TODO: find this for the bit stuff)
    // long new_reg0 = 0;
    long address;
    for (address = KERNEL_STACK_BASE; address < KERNEL_STACK_LIMIT; address += PAGESIZE)
    {

        int page = address >> PAGESHIFT;
        int free_page = findFreePage(); // find free page in old proc's region 0
        if (free_page == -1)
        {
            TracePrintf(0, "ERROR: No more free pages!\n");
            // return ERROR; //TODO: fix this output
        }
        else
        {
            TracePrintf(0, "Page %d is free in Region 0, copying...\n", free_page);

            // copy entry from old proc's kernel stack to new proc's
            // long old_addr = proc1->kernel_stack << PAGESHIFT; //old code from the beginning. Not sure if this is correct? should point to VMEM_0_LIMIT - page anyways
            // //I think the above address is wrong bc we should be iterating through *each* PTE in the kernel stack, which each has a differen address
            // long new_addr = free_page << PAGESHIFT;
            // first, set up the new region 0 kernel stack to have free pages
            ((struct pte *)new_reg0)[page].pfn = free_page;
            ((struct pte *)new_reg0)[page].uprot = PROT_NONE;
            ((struct pte *)new_reg0)[page].kprot = (PROT_READ | PROT_WRITE);
            ((struct pte *)new_reg0)[page].valid = 1;

            // set the temp borrowed to have the same pfn as the new region 0 PTE
            tempPT.uprot = PROT_NONE;
            tempPT.kprot = PROT_READ | PROT_WRITE;
            tempPT.valid = 1;
            tempPT.pfn = free_page;

            

            TracePrintf(0, "made it here with temp %d andfrom %d\n", (unsigned long)&tempPT, (unsigned long)address);
            // TracePrintf(0, "Address of old_addr: %d\n", old_addr);
            // TracePrintf(0, "Address of new_addr: %d\n", new_addr);
            // copy from the current region 0 to the new region 0 (pseudo region 0)
            // void *memcpy(void *dest, const void * src, size_t n)
            // VMEM_0_LIMIT - (page + 1) should be the top pof
            // finally copy.vc
            memcpy((void *)(unsigned long)&tempPT, (void *)(unsigned long)(address), PAGESIZE);

            TracePrintf(0, "done\n");
            // flush all entries in region 0 from TLB // at that specific address
            WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
        }
    }
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
    // redirect current region 0 pointer to new process
    // region0Pt = *proc2->kernel_stack;
    WriteRegister(REG_PTR0, (RCS421RegVal)proc2->region0);
    // return the ctx
    return &proc2->ctx;
}

// // // context switching from an existing process to another existing process
// // SavedContext *SwitchExist(SavedContext *ctxp, void *p1, void *p2) {
// //     struct pcb *proc1 = (struct pcb *) p1;
// //     struct pcb *proc2 = (struct pcb *) p2;

// //     // save context of proc 1
// //     proc1->ctx = ctxp;

// //     // flush all entries in region 0 from TLB
// //     WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

// //     // redirect current region 0 pointer to new process
// //     WriteRegister(REG_PTR0, (RCS421RegVal) proc2->kernel_stack);

// //     return proc2->ctx;
// // }

// // // context switching to no process, exiting kernal
// // SavedContext *SwitchNoProc(SavedContext *ctxp, void *p1, void *p2) {
    
// //     free(freePages);
// //     free(interruptVector);

// //     (void) p1;
// //     (void) p2;

// //     return ctxp;
// // }
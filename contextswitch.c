// #include <comp421/hardware.h>

// // context switching from an existing process to a new process
// SavedContext *SwitchNewProc(SavedContext *ctxp, void *p1, void *p2) {
//     struct pcb *proc1 = (struct pcb *) p1;
//     struct pcb *proc2 = (struct pcb *) p2;

//     TracePrintf(0, "Switching from existing process %d to new process %d\n", proc1->process_id, proc2->process_id);

//     // save context of proc 1
//     proc1->ctx = *ctxp;

//     // struct pte new_reg0 = findFreePage();
//     int page;
//     //I'm not sure how this works with separate files but I can't access currKernelBrk. instead was using 512 placeholder
//     struct pte tempPT = *(struct pte *)(UP_TO_PAGE(currKernelBrk)); //borrow from the top of the Kernel Brk (currKernelBrk)
//     TracePrintf(0, "made it here\n");
//     // region1Pt[PAGE_TABLE_LEN - 2] = proc2->pcb_reg0_pfn; //reset the pointer
//     //find a new Region0 virtual pointer (TODO: find this for the bit stuff)
//     // long new_reg0 = 0;
//     for (page = 0; page < KERNEL_STACK_PAGES; page++) {
//         int free_page = findFreePage(); // find free page in old proc's region 0
//         if (free_page == -1) {
//             TracePrintf(0, "ERROR: No more free pages!\n");
//             return (SavedContext *) -1;
//         } else {
//             TracePrintf(0, "Page %d is free in Region 0, copying...", free_page);
//             // copy entry from old proc's kernel stack to new proc's
//             long old_addr = proc1->kernel_stack << PAGESHIFT; //old code from the beginning. Not sure if this is correct? should point to VMEM_0_LIMIT - page anyways
//             //I think the above address is wrong bc we should be iterating through *each* PTE in the kernel stack, which each has a differen address
//             long new_addr = free_page << PAGESHIFT;
//             //will need to do this (TODO)
//             tempPT.uprot = PROT_NONE;
//             tempPT.kprot = PROT_READ | PROT_WRITE;
//             tempPT.valid = 1;
//             tempPT.pfn = new_addr;
//             TracePrintf(0, "Address of old_addr: %d\n", old_addr);
//             TracePrintf(0, "Address of new_addr: %d\n", new_addr);
//             TracePrintf(0, "done\n");
//             //copy from the current region 0 to the new region 0 (pseudo region 0)
//             //void *memcpy(void *dest, const void * src, size_t n)
//             //VMEM_0_LIMIT - (page + 1) should be the top pof 
//             memcpy((void *)((long)&tempPT), (void *)((long)(VMEM_0_LIMIT - (page + 1)) << PAGESHIFT), PAGESIZE);
//             // flush all entries in region 0 from TLB // at that specific address
//             WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
//         }
//     }
    

//     // redirect current region 0 pointer to new process
//     // region0Pt = *proc2->kernel_stack;
//     WriteRegister(REG_PTR0, (RCS421RegVal) proc2->kernel_stack);
//     //return the ctx
//     return &proc2->ctx;
// }

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
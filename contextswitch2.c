// #include <comp421/hardware.h>
// //workspace for contextswitch (CC has some changes that are unpushed)

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
//     struct pte tempPT = *((struct pte *) region1Pt)[PAGE_TABLE_LEN - 2]; //borrow from the top of the Region 1 page table
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
//             // ''
//             // // find a new physical page, copy 1 page from p1 kernel stack to that page

// 			// int free_addr = free_page << PAGESIZE;
			

// 			// // int free_vpn = current_pcb->brk_pos;

// 			// // empty virtual addrr at vpn * PAGESIZE, copy this page to empty_pfn
// 			// memcpy((void *) (free_addr), (void *) (VMEM_0_LIMIT - (page + 1) << PAGESIZE), PAGESIZE);

//             // //create the region 0 above the current break position (kernel break or brk?)
//             // //set as used, and the bits
//             // //memcopy to that spot FROM the vpn
//             // //set that PTE to have a pfn at the physical pfn, rest of the bits


// 			// // make free_vpn point to pcb1's physical pt0
// 			// ((struct pte*) pt0)[free_vpn].pfn = pcb1 -> pfn_pt0;
// 			// WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
// 			// // finally modify the kernel stack entry in pcb1's pt0
// 			// ((struct pte*) (free_vpn << PAGESIZE))[PAGE_TABLE_LEN - 1 - page].pfn = free_page;
// 			// ((struct pte*) (free_vpn << PAGESIZE))[PAGE_TABLE_LEN - 1 - page].uprot = PROT_NONE;
// 			// ((struct pte*) (free_vpn << PAGESIZE))[PAGE_TABLE_LEN - 1 - page].kprot = (PROT_READ | PROT_WRITE);
// 			// ((struct pte*) (free_vpn << PAGESIZE))[PAGE_TABLE_LEN - 1 - page].valid = 1;

// 			// WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
//             // ''
//             // copy entry from old proc's kernel stack to new proc's
//             long old_addr = proc1->kernel_stack << PAGESHIFT; //old code from the beginning. Not sure if this is correct? should point to VMEM_0_LIMIT - page anyways
//             //I think the above address is wrong bc we should be iterating through *each* PTE in the kernel stack, which each has a differen address
//             long new_addr = free_page << PAGESHIFT;
//             //will need to do this (TODO)
//             tempPT.uprot = PROT_NONE;
//             tempPT.kprot = PROT_READ | PROT_WRITE;
//             tempPT.valid = 1;
//             tempPT.pfn = free_page;
//             WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
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
//     WriteRegister(REG_PTR0, (RCS421RegVal) proc2->reg0_pfn);
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
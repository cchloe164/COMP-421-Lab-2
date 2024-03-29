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
//-make a new switch function ()

    //new switch function:
    //writeregister regptr0, (rcs421) destpcb ->reg0pt
    //writeregister(flush, flush all)
    //currpcb = destpcb
    //return &destpcb->ctx
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


// // // context switching to no process, exiting kernal
// // SavedContext *SwitchNoProc(SavedContext *ctx, void *p1, void *p2) {
    
// //     free(freePages);
// //     free(interruptVector);

// //     (void) p1;
// //     (void) p2;

// //     return ctx;
// // }

//cropped from other file since I couldn't figure out access things
// #include "contextswitch.c"
int ticks_passed = 0;

/**
Delay
*/
extern int DelayFunc(int clock_ticks) {
    //edge cases: clockticks 0; return 0, negative, return error

    TracePrintf(0, "Delay called!\n");
    if (clock_ticks == 0) {
        return 0;
    }
    if (clock_ticks < 0) {
        return ERROR;
    }
    
    //create a waiting node, add this to the waiting queue (malloc it)
    struct queue_item *waiting_node = malloc(sizeof(struct queue_item));
    waiting_node->proc = curr_proc;
    waiting_node->ticks_left = clock_ticks;
    curr_proc->delay_ticks = clock_ticks;
    // TracePrintf(1, "Currentpid %d\n", curr_proc->procfess_id, proc2->process_id);
    RemoveProcFromReadyQueue(curr_proc);
    //keep pcb as a field, next and current, also a counter to know how much to wait for
    // add the item to the waiting queue
    PushItemToWaitingQueue(waiting_node);
    ///could have a global clock var that keeps track of time, but i think it might be better to have timers that decerase for each process
    struct pcb *next_pcb = FindReadyPcb();
    (void)next_pcb;
    ContextSwitch(SwitchExist, &curr_proc->ctx, (void *)curr_proc, (void *)idle_pcb);//TODO: Change this to be next_pcb when FindReadyPcb is done
    //-make a new switch function ()

    //new switch function:
    //writeregister regptr0, (rcs421) destpcb ->reg0pt
    //writeregister(flush, flush all)
    //currpcb = destpcb
    //return &destpcb->ctx
    // (void)clock_ticks;
    return 0;
}

/**
 * Your Yalnix kernel should implement round-robin process scheduling with a time
 * quantum per process of 2 clock ticks. After the current process has been running as the current
 * process continuously for at least 2 clock ticks, if there are other runnable processes on the ready
 * queue, perform a context switch to the next runnable process.
 */
extern void Tick_() {
    TracePrintf(0, "Clock ticks passed: %d\n", ticks_passed);
    ticks_passed++;

    if (ticks_passed > 2) { // check if current process should be switched out
        // TracePrintf(0, "Time limit reached on Process %d!.\n", curr_proc->process_id);
        TracePrintf(0, "Time limit reached on Process!.\n");
        int res = SetNextProc();
        if (res == 1) {
            TracePrintf(0, "Clock ticks reset!\n");
            ticks_passed = 0; // reset timer
        } else {
            TracePrintf(0, "Current process will keep running!\n");
        }
    }
}




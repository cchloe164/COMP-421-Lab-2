#include <comp421/hardware.h>

/**
 * The Fork kernel call is the only way an existing process can create a new process in Yalnix (there is
 * no inherent limit on the maximum number of processes that can exist at once). The memory image
 * of the new process (the child) is a copy of that of the process calling Fork (the parent). When the
 * Fork call completes, both the parent process and the child process return (separately) from the Fork
 * kernel call as if each had been the one to call Fork, since the child is a copy of the parent. The only
 * distinction is the fact that the return value in the calling (parent) process is the (nonzero) process ID
 * of the new (child) process, while the value returned in the child is 0; as noted in Section 7.1, the
 * process ID for each (real) process should be greater than 0 (you should use the process ID value of 0
 * for your idle process). When returning from a successful Fork, you can return first either as the new
 * child process or as the parent process (think about why you might want to do one or the other). If, for
 * any reason, the new process cannot be created, Fork instead returns the value ERROR to the calling
 * process.
*/
int Fork_(void)
{
    TracePrintf(0, "Fork called!\n");
    struct pcb *parent = curr_proc;
    struct pcb *child = malloc(sizeof(struct pcb));
    SetProcID(child);
    BuildRegion0(child);    // Allocate and set up kernel stack
    int res = ContextSwitch(SwitchFork, &parent->ctx, (void *)parent, (void *)child);
    if (res == 0) {
        TracePrintf(0, "ContextSwitch was successful!\n", res);
    } else {
        TracePrintf(0, "ERROR: ContextSwitch was unsuccessful.\n", res);
    }

    TracePrintf(0, "Copying content from parent to child...\n");
    // borrow a pte from R1
    int temp_vpn = findFreeVirtualPage();
    struct pte *temp = &region1Pt[temp_vpn];
    TracePrintf(0, "BEFORE USING PFN: %d\n", region1Pt[temp_vpn].pfn);
    struct pte temp_copy = *temp; // store original pte info
    // setup protection settings
    temp->uprot = PROT_NONE;
    temp->kprot = PROT_READ | PROT_WRITE;
    temp->valid = 1;

    // iterate over region 0
    unsigned long addr;
    for (addr = VMEM_BASE; addr < VMEM_0_LIMIT; addr += PAGESIZE)
    {
        int page = addr >> PAGESHIFT;
        if (addr >= KERNEL_STACK_BASE && addr < KERNEL_STACK_LIMIT) {
            TracePrintf(1, "Skipped copying kernel stack (p. %d)\n", page);
        } else {
            TracePrintf(1, "Copying page %d...\n", page);

            // copy over pte settings
            child->region0[page].valid = parent->region0[page].valid;

            if (child->region0[page].valid == 1) {
                TracePrintf(1, "Setting up valid PTE...\n");
                child->region0[page].uprot = parent->region0[page].uprot;
                child->region0[page].kprot = parent->region0[page].kprot;
                child->region0[page].pfn = findFreePage(); // allocate physical memory

                temp->pfn = child->region0[page].pfn; // set the temp borrowed to have the same pfn as the new region 0 PTE

                unsigned long dest_addr = (temp_vpn + PAGE_TABLE_LEN) << PAGESHIFT;
                TracePrintf(1, "memcpying content from page %d to page %d using R1 page %d...", page, child->region0[page].pfn, temp_vpn);
                memcpy((void *)dest_addr, (void *)addr, PAGESIZE);
                TracePrintf(1, "done\n");
            }
        }
    }

    TracePrintf(0, "Content copied successfully!\n");

    TracePrintf(0, "USING PFN: %d\n", region1Pt[temp_vpn].pfn);
    region1Pt[temp_vpn] = temp_copy;    // free borrowed pte
    TracePrintf(0, "AFTER USING PFN: %d\n", region1Pt[temp_vpn].pfn);
    freePages[temp_vpn + PAGE_TABLE_LEN] = PAGE_FREE;
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

    ContextSwitch(SwitchExist, &parent->ctx, (void *)parent, (void *)child);
    TracePrintf(0, "What\n");

    // curr_proc = child;
    // unsigned long phys_addr = (unsigned long)((PAGE_TABLE_LEN + child->free_vpn) << PAGESHIFT);

    // TracePrintf(0, "Writing REG0_PTR0 to %p...", phys_addr);
    // WriteRegister(REG_PTR0, (RCS421RegVal)phys_addr);
    // WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    // TracePrintf(0, "done\n");

    return 0;
}
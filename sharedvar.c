#include <comp421/hardware.h>

#define PAGE_FREE 0
#define PAGE_USED 1

int *freePages;
int num_pages;
int num_free_pages;
struct pte region0Pt[PAGE_TABLE_LEN], region1Pt[PAGE_TABLE_LEN]; // page table pointers
void *currKernelBrk;
int next_proc_id = 0;

void **interruptVector;

struct pcb { //TODO: I've added a few fields for some of the other functions but haven't updated the pcb creation code for idle/init yet
    int process_id;
    SavedContext ctx;
    int brk; //stores the break position of the current process (for brk.c)
    int reg0_pfn; //stores the physical pfn of reg 0
    struct pte *region0; //stores pointer to physical address of region 0
    unsigned long free_vpn; // free virtual page number
    int delay_ticks; // the amount of ticks remaining if the process is Delayed

    struct pcb *parent; // this process's parent if it has one
    struct pcb *children_head;  // head child of children chain
    struct pcb *children_tail;  
    struct pcb *next_sibling;
    struct pcb *prev_sibling;

    struct exitedChild *exited_children_head;
    struct exitedChild *exited_children_tail;
    int waiting;
};

// an informal round-robin queue based on clock ticks
struct queue_item {
    struct queue_item *prev; // for waiting queue only; ready queue is only one direction link
    struct pcb *proc;
    int ticks_left;
    struct queue_item *next;
};

// an informal round-robin queue based on clock ticks
struct exitedChild {
    struct exitedChild *prev; // for waiting queue only; ready queue is only one direction link
    struct pcb *parent;
    int status;
    int process_id;
    struct exitedChild *next;
};

struct queue_item *waiting_queue_head;
struct queue_item *waiting_queue_tail;

struct queue_item *queue_head;
struct queue_item *queue_tail;

struct pcb *curr_proc;
struct pcb *idle_pcb;
int queue_size = 0;
int waiting_queue_size = 0;

/**
 * Set process id.
*/
void SetProcID(struct pcb *proc) {
    proc->process_id = next_proc_id;
    next_proc_id++;
}


//creates pcb
struct pcb *create_pcb() {
    struct pcb *new = malloc(sizeof(struct pcb*));
    SetProcID(new);
    new->brk = 0;      
    new->reg0_pfn = 0; 
    new->region0 = NULL;  
    new->free_vpn = 0; 
    new->delay_ticks = 0;       
    new->children_head = NULL; 
    new->children_tail = NULL;
    new->next_sibling = NULL;
    new->prev_sibling = NULL;
    new->exited_children_head = NULL;
    new->exited_children_tail = NULL;
    new->waiting = 0;

    return new;
}



/**
 Exit queue functions

*/

void PushExitToExited(struct exitedChild *child, struct pcb *current) {
    child->next = NULL; // no next process (this is useful for the trapclock handler)

    // push onto queue
    if (current->exited_children_head == NULL) {
        current->exited_children_head = child;
        current->exited_children_tail = current->exited_children_head;
    } else {
        current->exited_children_tail->next = child;
        child->prev = current->exited_children_tail;
        current->exited_children_tail = child;
    }
}

void RemoveHeadFromExitQueue(struct pcb *current) {
    struct exitedChild *nextChild = current->exited_children_head;
    if (current->exited_children_head != NULL) {
        if (current->exited_children_head == current->exited_children_tail) {
            current->exited_children_head = NULL;
            current->exited_children_tail = NULL;
        } //else just pop the head
        current->exited_children_head = nextChild->next;
        current->exited_children_head->prev = NULL;
        
    free(nextChild);
    }
}


/**
 * Push new process to waiting queue.
*/
void PushProcToWaitingQueue(struct pcb *proc) {
    TracePrintf(0, "Pushing proc %d to waiting queue!\n", proc->process_id);
    // wrap process as new queue item
    struct queue_item *new = malloc(sizeof(struct queue_item));
    new->proc = proc;
    new->ticks_left = proc->delay_ticks;
    new->next = NULL; // no next process (this is useful for the trapclock handler)

    // push onto queue
    if (waiting_queue_size == 0) {
        waiting_queue_head = new;
        waiting_queue_tail = queue_head;
    } else {
        waiting_queue_tail->next = new;
        new->prev = queue_tail;
        waiting_queue_tail = new;
    }
    waiting_queue_size++;
}

/**
 * Push new process to waiting queue.
*/
void PushItemToWaitingQueue(struct queue_item *new) {
    TracePrintf(0, "Pushing proc %d to waiting queue!\n", new->proc->process_id);
    // wrap process as new queue item
    new->next = NULL; // no next process (this is useful for the trapclock handler)

    // push onto queue
    if (waiting_queue_size == 0) {
        waiting_queue_head = new;
        waiting_queue_tail = waiting_queue_head;
    } else {
        waiting_queue_tail->next = new;
        new->prev = waiting_queue_tail;
        waiting_queue_tail = new;
    }
    waiting_queue_size++;
}

void RemoveChildFromParent(struct pcb *item, struct pcb *parent_proc) {
    
    // Case 1: If the item is the only item in the queue
    if (parent_proc->children_head == item && parent_proc->children_tail == item) {
        parent_proc->children_head = NULL;
        parent_proc->children_tail = NULL;
    } else {
        // Case 2: If the item is the head of the queue
        if (parent_proc->children_head == item) {
            parent_proc->children_head = item->next_sibling;
            parent_proc->children_head->prev_sibling = NULL;
        } else {
            // Case 3: If the item is the tail of the queue
            if (parent_proc->children_tail == item) {
                parent_proc->children_tail = item->prev_sibling;
                parent_proc->children_tail->next_sibling = NULL;
            } else {
                // Case 4: If the item is somewhere in the middle of the queue
                item->prev_sibling->next_sibling = item->next_sibling;
                item->next_sibling->prev_sibling = item->prev_sibling;
            }
        }
    }
}
/**
 * remove a queue item from the waiting queue.
*/

void RemoveItemFromWaitingQueue(struct queue_item *item) {
    TracePrintf(0, "Removing proc %d to waiting queue!\n", item->proc->process_id);
    waiting_queue_size--;

    // Case 1: If the item is the only item in the queue
    if (waiting_queue_head == item && waiting_queue_tail == item) {
        waiting_queue_head = NULL;
        waiting_queue_tail = NULL;
    } else {
        // Case 2: If the item is the head of the queue
        if (waiting_queue_head == item) {
            waiting_queue_head = item->next;
            waiting_queue_head->prev = NULL;
        } else {
            // Case 3: If the item is the tail of the queue
            if (waiting_queue_tail == item) {
                waiting_queue_tail = item->prev;
                waiting_queue_tail->next = NULL;
            } else {
                // Case 4: If the item is somewhere in the middle of the queue
                item->prev->next = item->next;
                item->next->prev = item->prev;
            }
        }
    }
    free(item);

    // Free memory allocated to the removed item
    // free((void *)item);

}

/**
 * Push new process to ready queue.
*/
void PushProcToQueue(struct pcb *proc) {
    TracePrintf(0, "Pushing process id %d to the ready queue\n", proc->process_id);
    // wrap process as new queue item
    struct queue_item *new = malloc(sizeof(struct queue_item));
    new->proc = proc;
    // new.ticks_left = 2;

    // push onto queue
    if (queue_size == 0) {
        queue_head = new;
        queue_tail = queue_head;
    } else {
        queue_tail->next = new;
        queue_tail = new;
    }
    queue_size++;
}

void RemoveItemFromReadyQueue(struct queue_item *item) {
    TracePrintf(0, "Removing proc %d to waiting queue!\n", item->proc->process_id);
    queue_size--;
    // Case 1: If the item is the only item in the queue
    if (queue_head == item && queue_tail == item) {
        queue_head = NULL;
        queue_tail = NULL;
    } else {
        // Case 2: If the item is the head of the queue
        if (queue_head == item) {
            queue_head = item->next;
            queue_head->prev = NULL;
        } else {
            // Case 3: If the item is the tail of the queue
            if (queue_tail == item) {
                queue_tail = item->prev;
                queue_tail->next = NULL;
            } else {
                // Case 4: If the item is somewhere in the middle of the queue
                item->prev->next = item->next;
                item->next->prev = item->prev;
            }
        }
    }
    free(item);
}
/**
Removes the process from the ready queue, if it exists
*/
void RemoveProcFromReadyQueue(struct pcb *proc) {

    struct queue_item *curr_proc_item = queue_head; //current item in the queue. iterate throug them all
    //TODO: add a dummy to the tail of the queue or wait until null
    // int procs_deleted = 0;
    // int i;
    struct pcb *current_proc;
    TracePrintf(0, "RemoveProcFromReadyQueue for process id %d!\n", proc->process_id);
    while (curr_proc_item != NULL) {
        current_proc = curr_proc_item->proc;
        if (current_proc->process_id == proc->process_id) {
            //found a proc that matches
            TracePrintf(1, "Found a matching process in the ready queue to remove! Removing it now.\n");
            RemoveItemFromReadyQueue(curr_proc_item);
        }
        // if (current_proc->delay_ticks <= 0) { // the process is done waiting; remove it from the delay queue and put it on the ready queue
        //     TracePrintf(1, "Item with id %d delay has expired! now moving to the ready queue\n", current_proc->process_id);
        //     RemoveItemFromWaitingQueue(curr_proc_item);
        //     PushProcToQueue(current_proc);

        //     TracePrintf(1, "Switching from process %d to process id %d\n", curr_proc->process_id, current_proc->process_id);
        //     ContextSwitch(SwitchExist, &curr_proc->ctx, (void *)curr_proc, (void *)current_proc);

        // }
        curr_proc_item = curr_proc_item->next;
    }

}
/**
 * Find a ready proc, or idle if none on the queue
*/
struct pcb * FindReadyPcb() { //TODO: this procedure gets used by delay to find another pcb to switch to
    TracePrintf(0, "Attempting to set new process from delay call...\n");
    if (queue_size == 0) {//TODO: check if this is 1? maybe
        TracePrintf(0, "Queue is empty right now!\n");
        return idle_pcb; 
    } else {
        TracePrintf(0, "Found a ready process to switch to for delay!.\n", queue_head->proc->process_id, curr_proc->process_id);
        return queue_head->proc;//TODO: might be switching to itself? 
    }
}
/**
 * Assign next ready available process to next_proc and return 1 if successful. 
 * Otherwise, return 0 if none are ready and -1 if queue is empty.
*/
int SetNextProc() {
    TracePrintf(0, "Attempting to set new process...\n");
    if (queue_size == 0) {
        TracePrintf(0, "Queue is empty right now!\n");
        return -1; 
    } else {
        struct pcb *curhead = queue_head->proc;
        TracePrintf(0, "Popped Process %d and added Process %d back onto queue.\n", curhead->process_id, curr_proc->process_id);
        PushProcToQueue(curr_proc); // push old process onto queue
        curr_proc = queue_head->proc;   // set new current process
        queue_head = queue_head->next;  // shift queue forward
        queue_size--;
        return 1;
    }
}

/**
Finds a free page, returns the PFN or -1 if there are no free pages available.
*/
int findFreePage()
{
    int page;
    for (page = 0; page < num_pages; page++)
    {
        if (freePages[page] == PAGE_FREE)
        {
            TracePrintf(0, "Found free page %d in Region 0!\n", page);
            freePages[page] = PAGE_USED;
            num_free_pages--;
            return page;
        }
    }
    TracePrintf(0, "ERROR: No free page found in region 0!\n");
    return -1;
}

/**
 * Finds a free virtual page (VPN of Region 1), starting from the top of V1. Returns -1 if no free pages found.
 */
int findFreeVirtualPage()
{
    unsigned long vaddr;
    for (vaddr = VMEM_1_LIMIT - (3 * PAGESIZE); vaddr > (unsigned long)&currKernelBrk; vaddr -= PAGESIZE) //TODO: try removing 3*pagesize
    {
        int page = (vaddr >> PAGESHIFT) - PAGE_TABLE_LEN;
        if (region1Pt[page].valid == 0)
        {
            TracePrintf(0, "Found free page %d in Region 1!\n", page);
            freePages[page + PAGE_TABLE_LEN] = PAGE_USED;
            return page;
        }
    }
    TracePrintf(0, "ERROR: No free page found in region 1!\n");
    return -1;
}

/**
 * Return free page number from Region 1 for borrowing. If none available, return -1.
*/
int BorrowR1Page() {
    unsigned long vaddr;
    for (vaddr = VMEM_1_BASE; vaddr < VMEM_1_LIMIT; vaddr += PAGESIZE)
    {
        int page = (vaddr >> PAGESHIFT) - PAGE_TABLE_LEN;
        if (region1Pt[page].valid == 0)
        {
            TracePrintf(0, "Borrowing page %d from Region 1!\n", page);
            return page;
        }
    }
    TracePrintf(0, "ERROR: No more free pages in Region 1 to borrow!\n");
    return -1;
}

/**
frees a page
*/
void freePage(int pfn)
{
    TracePrintf(0, "Freeing page %d!\n", pfn);
    freePages[pfn] = PAGE_FREE;
    num_free_pages++;
}


/**
 * Allocate and set up Region 0 for given PCB.
 */
void BuildRegion0(struct pcb *proc)
{
    TracePrintf(0, "Building Region 0 for process %d!\n", proc->process_id);

    unsigned long virtualPage = findFreeVirtualPage(); // find a free virtual page. Use this to store the address to the new Region 0.
    region1Pt[virtualPage].valid = 1;
    region1Pt[virtualPage].kprot = PROT_READ | PROT_WRITE;
    region1Pt[virtualPage].uprot = PROT_NONE;
    region1Pt[virtualPage].pfn = PAGE_TABLE_LEN + virtualPage;
    WriteRegister(REG_TLB_FLUSH, region1Pt[virtualPage].pfn << PAGESHIFT);

    proc->region0 = (struct pte *)((PAGE_TABLE_LEN + virtualPage) << PAGESHIFT); // set it equal to the address
    proc->free_vpn = virtualPage;
    TracePrintf(0, "Proc R0 paddr %p\n", &proc->region0);

    TracePrintf(0, "Allocating and setting up kernel stack...\n");
    int vaddr3;
    for (vaddr3 = KERNEL_STACK_BASE; vaddr3 < KERNEL_STACK_LIMIT; vaddr3 += PAGESIZE)
    {
        int vpn = vaddr3 >> PAGESHIFT;
        proc->region0[vpn].pfn = findFreePage();
        proc->region0[vpn].uprot = PROT_NONE;
        proc->region0[vpn].kprot = PROT_READ | PROT_WRITE;
        proc->region0[vpn].valid = 1;
    }
    TracePrintf(0, "Allocation and setup done.\n");
}
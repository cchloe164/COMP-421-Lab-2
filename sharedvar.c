

#include <comp421/hardware.h>

#define PAGE_FREE 0
#define PAGE_USED 1

int *freePages;
int num_pages;
int num_free_pages;
struct pte region0Pt[PAGE_TABLE_LEN], region1Pt[PAGE_TABLE_LEN]; // page table pointers
void *currKernelBrk;

void **interruptVector;

struct pcb { //TODO: I've added a few fields for some of the other functions but haven't updated the pcb creation code for idle/init yet
    int process_id;
    int kernel_stack;  // first page of kernal stack
    int reg0_pfn; //stores the physical pfn of reg 0
    int brk; //stores the break position of the current process (for brk.c)
    struct pte *region0; //stores current region 0 pointer
    SavedContext ctx;
    
    int delay_ticks; // the amount of ticks remaining if the process is Delayed
};

// an informal round-robin queue based on clock ticks
struct queue_item {
    struct queue_item *prev; // for waiting queue only; ready queue is only one direction link
    struct pcb *proc;
    int ticks_left;
    struct queue_item *next;
};

struct queue_item *waiting_queue_head;
struct queue_item *waiting_queue_tail;

struct queue_item *queue_head;
struct queue_item *queue_tail;

struct pcb *curr_proc;
struct pcb *idle_pcb;
int queue_size = 0;
int waiting_queue_size = 0;

//creates pcb
struct pcb create_pcb(int pid, int kernel_stack, int reg0_pfn, int brk, SavedContext context, int delay_ticks) {
    struct pcb new_pcb;
    new_pcb.process_id = pid;
    new_pcb.kernel_stack = kernel_stack;
    new_pcb.reg0_pfn = reg0_pfn;
    new_pcb.brk = brk;
    new_pcb.ctx = context;
    new_pcb.delay_ticks = delay_ticks;
    return new_pcb;
}

/**
 * Push new process to waiting queue.
*/
void PushProcToWaitingQueue(struct pcb *proc) {
    // wrap process as new queue item
    struct queue_item *new = malloc(sizeof(struct queue_item));
    new->proc = proc;
    new->ticks_left = proc->delay_ticks;
    new->next = NULL; // no next process (this is useful for the trapclock handler)

    // push onto queue
    if (waiting_queue_size == 0) {
        queue_head = new;
        queue_tail = queue_head;
    } else {
        queue_tail->next = new;
        new->prev = queue_tail;
        queue_tail = new;
    }
    waiting_queue_size++;
}

/**
 * Push new process to waiting queue.
*/
void PushItemToWaitingQueue(struct queue_item *new) {
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

/**
 * remove a queue item from the waiting queue.
*/

void RemoveItemFromWaitingQueue(struct queue_item *item) {
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

    // Free memory allocated to the removed item
    // free((void *)item);


}

/**
 * Push new process to ready queue.
*/
void PushProcToQueue(struct pcb *proc) {
    // wrap process as new queue item
    struct queue_item new;
    new.proc = proc;
    // new.ticks_left = 2;

    // push onto queue
    if (queue_size == 0) {
        queue_head = &new;
        queue_tail = queue_head;
    } else {
        queue_tail->next = &new;
        queue_tail = &new;
    }
    queue_size++;
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
    } else if (queue_head->ticks_left == 0) {
        TracePrintf(0, "Popped Process %d and added Process %d back onto queue.\n", queue_head->proc->process_id, curr_proc->process_id);
        PushProcToQueue(curr_proc); // push old process onto queue
        curr_proc = queue_head->proc;   // set new current process
        queue_head = queue_head->next;  // shift queue forward
        queue_size--;
        return 1;
    } else {
        TracePrintf(0, "No ready process on queue.\n");
        return 0;
    }
}

/**
Finds a free page, returns the PFN or -1 if there are no free pages available.
*/
int findFreePage()
{
    int page_itr;
    for (page_itr = 0; page_itr < num_pages; page_itr++)
    {
        if (freePages[page_itr] == PAGE_FREE)
        {
            freePages[page_itr] = PAGE_USED;
            num_free_pages--;
            return page_itr;
        }
    }
    return -1;
}
#define PAGE_FREE 0
#define PAGE_USED 1

int *freePages;
int num_pages;
int num_free_pages;

void **interruptVector;

struct pcb {
    int process_id;
    int kernel_stack;  // first page of kernal stack
    SavedContext *ctx;
};

// an informal round-robin queue based on clock ticks
struct queue_item {
    struct pcb *proc;
    int ticks_left;
    struct queue_item *next;
};

struct queue_item *queue_head;
struct queue_item *queue_tail;

struct pcb *curr_proc;
int queue_size = 0;

/**
 * Push new process to queue.
*/
void PushProcToQueue(struct pcb *proc) {
    // wrap process as new queue item
    struct queue_item new;
    new.proc = proc;
    new.ticks_left = 2;

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
#include <comp421/hardware.h>

// #include "contextswitch.c"

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
int ForkFunc(void)
{
    TracePrintf(0, "Fork called!\n");
    struct pcb *parent = curr_proc;
    struct pcb *child = malloc(sizeof(struct pcb));

    // set parent-child relations
    TracePrintf(0, "Set family relationships!\n");
    child->parent = parent;
    child->prev_sibling = parent->children_tail;
    if (parent->children_head == NULL) {
        parent->children_head = child;
    } else {
        parent->children_tail->next_sibling = child;
    }
    parent->children_tail = child;

    SetProcID(child);
    BuildRegion0(child);    // Allocate and set up kernel stack
    int res = ContextSwitch(SwitchFork, &parent->ctx, (void *)parent, (void *)child);
    if (res == 0) {
        TracePrintf(0, "ContextSwitch was successful!\n", res);
    } else {
        TracePrintf(0, "ERROR: ContextSwitch was unsuccessful.\n", res);
    }

    PushProcToWaitingQueue(parent);

    return child->process_id;
}
/**
Exit is the normal means of terminating a process. The current process is terminated,
 and the integer value of status is saved for later collection by the parent process 
 if the parent calls to Wait. However, when a process exits or is terminated by the kernel, 
 if that process has children, each of these children should continue to run normally, but
  it will no longer have a parent and is thus then referred to as an “orphan” process. When 
  such an orphan process later exits, its exit status is not saved or reported to its parent
   process since it no longer has a parent process; unlike in Unix, an orphan process is not “inherited” by any other process.
When a process exits or is terminated by the kernel, all resources used by the calling process 
are freed, except for the saved status information (if the process is not an orphan).
 The Exit kernel call can never return.
*/


void ExitFunc(int status) {

    //go through each pte in region, free page associated with the pfn and set valid bit

    int vpn;
    TracePrintf(0, "Exit Called! on process id %d\n", curr_proc->process_id);
    for (vpn = 0; vpn < PAGE_TABLE_LEN; vpn++) {
        struct pte *curr_pte = &curr_proc->region0[vpn];
        if (curr_pte->valid == 1) {
            freePage(curr_pte->pfn);
            curr_pte->valid = 0;
        }
    }

    //take care of orphan case; assign parent to null for children

    //if parent dies first, then inform children that you're ded
    if (curr_proc->children_head != NULL) {
        struct pcb *child = (struct pcb *) &curr_proc->children_head; //(this should take the first child anyways, even if it is a list)
        while (child != NULL) { //reset parents of orphans
            child->parent = NULL;
            child = child->next_sibling;
        }
    }
    //valid bit 0s for region 0
    
    //go to Region 1 pte with the region 0, free pfn and reset valid bit
    struct pte *reg1pte = &region1Pt[curr_proc->free_vpn];
    reg1pte->valid = 0;
    freePage(reg1pte->pfn);

    //free the current pcb
    
    //in pcb, have a pointer to the parent and a linkedlist of exited child processes and their statuses (int status)
    //uptdate status in the exit list 
    //also a linkedlist of children normally
    
    if (curr_proc->parent != NULL) {
        //gotta update the parent that I am done. remove from parent's children list and add to exit list
        struct pcb *parent_proc = curr_proc->parent;
        if (parent_proc->children_head != NULL) {
            struct pcb *child = curr_proc->children_head; //(this should take the first child anyways, even if it is a list)
            while (child != NULL) { //reset parents of orphans
                if (child->process_id == curr_proc->process_id) {
                    //remove from this
                    RemoveChildFromParent(child, parent_proc);
                }
                child = child->next_sibling;
            }
        }
        //TODO: create this struct for storing exited children and their statuses
        struct exitedChild *exited = malloc(sizeof(struct exitedChild));
        exited->process_id = curr_proc->process_id;
        exited->status = status;
        PushExitToExited(exited, parent_proc);

        if (parent_proc->waiting == true) { //if my parent was waiting, add it to the ready queue
            parent_proc->waiting = false;
            PushProcToQueue(parent_proc);
        }



    }
    
    //TODO: do we need a new switch function?
    //context switch to next ready process, if none, switch to idle
	ContextSwitch(SwitchExist, &(curr_proc->ctx), (void *)curr_proc, FindReadyPcb());
    free((void *)curr_proc);//free the PCB?
    while (1) {}

}


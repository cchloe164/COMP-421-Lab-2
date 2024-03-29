/**
Collect the process ID and exit status returned by a child process of the calling program. 
When a child process Exits, its exit status information is added to a FIFO queue of child processes
 not yet collected by its specific parent. After the Wait call, this child process information is 
 removed from the queue. If the calling process has no remaining child processes (exited or running), 
 ERROR is returned instead as the result of the Wait call and the integer pointed to by status_ptr is
  not modified. Otherwise, if there are no exited child processes waiting for collection by this calling
   process, the calling process is blocked until its next child calls exits or is terminated by the kernel
    (if a process is terminated by the kernel, its exit status should appear to its parent as ERROR). 
    On success, the Wait call returns the process ID of the child process and that child’s exit status
     is copied to the integer pointed to by the status_ptr argument. On any error, this call instead 
     returns ERROR.
*/


extern int WaitFunc(int *status_ptr) {
    //status pointer points to integer of child's exit status
    //check if there are any children
    //if no children ever, return error
    TracePrintf(1, "WaitFunc called for parent process %d\n", curr_proc->process_id);
    if (curr_proc->children_head == NULL) {
        if (curr_proc->exited_children_head == NULL) {
            //no children ever
            return ERROR;
        }
        //if a child is running, then contextswitch to ready process/idle
            //then child exits
            //when child exits, look at parent and see if parent is waiting
            //so have a status field in parent pcb that indicates whether waiting or not
            //if parent waiting, contextswitch to parent
        curr_proc->waiting = true;
        ContextSwitch(SwitchExist, &(curr_proc->ctx), (void *)curr_proc, curr_proc->children_head);
    }
    //this will resume after the contextswitch in waitfunc. 
    //then, there should be a child in the exitlist
    //then write status of that child to the pointer
    struct exitedChild *exited  = curr_proc->exited_children_head;
    status_ptr = &exited->status;
    RemoveHeadFromExitQueue(curr_proc);
    (void)status_ptr;
    return exited->process_id;
    
}
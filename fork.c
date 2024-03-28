unsigned int next_process_id; // sequentially incremented with each new process created
// cloning no loadprogram nor context switch

//need to copy savedcontext in fork (for the contextswitch) with a memcpy
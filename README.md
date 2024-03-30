Chloe Chen cc126
Ryan Wang rzw1


The majority of our project should work. See "known bugs" section below this section for the known bugs. Thanks!: 

*KernelStart should be implemented completely. It switches from idle to init (the given function in the terminal). It can also switch back and forth between processes
*Thus, ContextSwitch is implemented completely. 
*GetPid is implemented
*Delay kernel call is implemented. Blocking processes work and context switching works. 
*Delay works when called several times in the same function
*SetKernelBrk is functional. Brk is also functional. Thus, when either the kernel or an init process needs lots of memory, the kernel can handle it. 
*Fork is implemented, with only one known bug (see below). A parent process can create a child process and will be returned to appropriately
*Exec is implemented. When a process wishes to execute another function, the other function executes appropriately and returns to the base process
*Exit is implemented. When a single process exits, the memory that it uses is freed and the next Ready process (or idle) is run
*Wait is implemented. When a parent wishes to wait for its child, it does so appropriately through calls such as Delay() in the child process. 
*Ready processes switch when they are running for more than 2 clock ticks, if there are other processes on the ready queue

*Most kernel calls related to terminal I/O handling are written. This includes the TrapKernelHandler, TrapClockHandler, TrapIllegalHandler, TrapMemoryHandler, TrapMathHandler, TrapTTYReceiveHandler. TrapTTYTransmitHandler is currently being implemented, and was not complete at the time of submission. 


Known bugs:

There is a small bug when a forked process calls exec (it ends up in idle, rather than returning to the parent). 

TTYTransmit is currently not implemented as of the writing of this README. We also have not tested the I/O robustly for concurrent terminals up to NUM_TERMINALS (4); it is only tested to work with one terminal. We expect it may have issues with more than 1 terminal.  


Thanks!
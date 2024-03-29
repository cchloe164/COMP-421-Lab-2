// loadprogram but no contextswitch
#include "load.c"

//Replace the currently running program in the calling process’s memory with the program stored in the file named by filename
/*
Based on Loadprogram (Page 14 for exec description)
 */
int ExecFunc(char *name, char **args, ExceptionInfo *info) {

    if (LoadProgram(name, args, info, curr_proc->region0) == ERROR) {
        return ERROR;
    }
    return 0;
}

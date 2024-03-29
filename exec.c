// loadprogram but no contextswitch
//Replace the currently running program in the calling process’s memory with the program stored in the file named by filename
/*
Based on Loadprogram (Page 14 for exec description)
 */
int LoadProgram(char *name, char **args, ExceptionInfo *info, struct pte *ptr0);
int ExecFunc(char *name, char **args, ExceptionInfo *info) {

    if (LoadProgram(name, args, info, curr_proc->region0) == ERROR) {
        return ERROR;
    }
    return 0;
}

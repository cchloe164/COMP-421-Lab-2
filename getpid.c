
/**
 * Returns the process ID of the calling process.
 */
extern int GetPid_(struct pcb *info)
{
    TracePrintf(0, "GetPid: returned %d\n", info->process_id);
    return info->process_id;
}
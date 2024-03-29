extern int WaitFunc(int *status_ptr)
{
    (void)status_ptr;
    TracePrintf(0, "Wait called!\n");
    return 0;
}
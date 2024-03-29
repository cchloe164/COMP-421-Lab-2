extern void ExitFunc(int status)
{
    (void)status;
    TracePrintf(0, "Exit called!\n");
    while (1)
    {
    }
}
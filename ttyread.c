extern int TtyReadFunc(int tty_id, void *buf, int len)
{
    (void)tty_id;
    (void)buf;
    (void)len;
    TracePrintf(0, "TtyRead called!\n");
    return 0;
}
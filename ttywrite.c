extern int TtyWriteFunc(int tty_id, void *buf, int len)
{
    (void)tty_id;
    (void)buf;
    (void)len;
    TracePrintf(0, "TtyWrite called!\n");
    return 0;
}

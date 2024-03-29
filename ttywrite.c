int TtyWriteFunc(int tty_id, void *buf, int len)
{
    (void)tty_id;
    (void)buf;
    TracePrintf(0, "TtyWrite called!\n");
    
    return len;
}

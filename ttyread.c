int TtyReceiveFunc(int tty_id)
{
    TracePrintf(0, "TtyRead called!\n");
    char *new_line = malloc(sizeof(char) * TERMINAL_MAX_LINE);          // allocate memory for new line
    int len = TtyReceive(tty_id, (void *)&new_line, TERMINAL_MAX_LINE); // fill line

    struct line *new = malloc(sizeof(struct line));
    new->content = new_line;
    new->len = len;
    new->next_line = NULL;

    struct term tty = terms[tty_id];
    if (tty.read_in == NULL)
    {
        tty.read_in = new;
    }
    else
    {
        struct line *curr = tty.read_in;
        while (curr->next_line != NULL)
        {
            curr = curr->next_line;
        }
        curr->next_line = new;
    }

    TracePrintf(0, "Terminal %d read in length %d\n", tty_id, len);
    return len;
}

int TtyReadFunc(int tty_id, void *buf, int len) {
    (void) buf;
    (void) len;
    if (terms[tty_id].read_queue == NULL) {
        terms[tty_id].read_queue = curr_proc;
    } else {
        struct pcb *ptr = terms[tty_id].read_queue;
        while (ptr->read_q_next != NULL) {
            ptr = ptr->read_q_next;
        }
        ptr->read_q_next = curr_proc;
    }
    Halt();
    return 0;
}
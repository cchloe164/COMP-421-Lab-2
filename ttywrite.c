void addToLineTail(struct line *head, struct line *new) {
    if (head == NULL)
    {
        head = new;
    }
    else
    {
        struct line *curr = head;
        while (curr->next_line != NULL)
        {
            curr = curr->next_line;
        }
        curr->next_line = new;
    }
}

void addToProcTail(struct pcb *head, struct pcb *new) {
    if (head == NULL)
    {
        head = new;
    }
    else
    {
        struct pcb *curr = head;
        while (curr->write_q_next != NULL)
        {
            curr = curr->write_q_next;
        }
        curr->write_q_next = new;
    }
}

int TtyTransmitFunc(int tty_id)
{
    TracePrintf(0, "TtyTransmitFunc called!\n");
    struct term *tty = &terms[tty_id];

    if (tty->write_out != NULL) {
        TracePrintf(0, "1\n");
        TtyTransmit(tty_id, tty->write_out->content, tty->write_out->len);
        //tty->write_out = tty->write_out->next_line;
        TracePrintf(0, "2\n");
    }
    TracePrintf(0, "3\n");

    if (tty->write_queue == NULL) {
        TracePrintf(0, "Was null\n");

        ContextSwitch(SwitchExist, &curr_proc->ctx, (void *)curr_proc, FindReadyPcb());
        return 0;
    }

    ContextSwitch(SwitchExist, &curr_proc->ctx, (void *)curr_proc, tty->write_queue);
    //tty->write_queue = tty->write_queue->write_q_next;
    
    tty->mutex = 1;
    return 0;
}


int TtyWriteFunc(int tty_id, void *buf, int len)
{
    TracePrintf(0, "TtyWrite called!\n");
    struct line *new = malloc(sizeof(struct line));
    new->content = (char *)buf;
    new->len = len;
    new->next_line = NULL;

    struct term *tty = &terms[tty_id];
    addToLineTail(tty->write_out, new);
    addToProcTail(tty->write_queue, curr_proc);

    if (tty->mutex == 1) {
        TracePrintf(0, "transmitted1\n");
        TtyTransmit(tty_id, buf, len);
        //tty->write_out = tty->write_out->next_line;
        TracePrintf(0, "transmitted\n");
        ContextSwitch(SwitchExist, &curr_proc->ctx, (void *)curr_proc, FindReadyPcb());
    } else {
        TracePrintf(0, "did not transmit\n");
        ContextSwitch(SwitchExist, &curr_proc->ctx, (void *)curr_proc, FindReadyPcb());

    }

    return len;
}

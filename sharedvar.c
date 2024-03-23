#define PAGE_FREE 0
#define PAGE_USED 1

int *freePages;
int num_pages;
int num_free_pages;

void **interruptVector;

struct pcb
{
    int process_id;
    SavedContext *ctx;
    int kernel_stack;  // first page of kernal stack
};
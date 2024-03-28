// cloning no loadprogram nor context switch

//need to copy savedcontext in fork (for the contextswitch) with a memcpy

int Fork_(void)
{
    TracePrintf(0, "Fork called!\n");
    return 0;
}
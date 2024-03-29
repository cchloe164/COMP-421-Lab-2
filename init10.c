#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>

int main()
{
    write(0, "init10! Testing Fork!\n", 6);
    int x = Fork();
    TracePrintf(0, "child pid is: %i\n", GetPid());
    TracePrintf(0, "currentForkReturn is: %i\n", x);
    Exit(0);
}
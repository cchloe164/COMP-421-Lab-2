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

int recurse();
int main()
{
    TracePrintf(0, "starting recursing \n");
    recurse();
    TracePrintf(0, "-------bugggingggg \n");
    Exit(0);
}

int recurse() {
    TracePrintf(0, "recursing \n");
    recurse();
    return 0;
}

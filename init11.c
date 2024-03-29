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
    write(0, "init11! Testing Fork with multiple !\n", 6);
    // int status; 
    int x = Fork();
    if (x != 0) { //not the child
        TracePrintf(0, "child pid is: %i\n", GetPid());
        TracePrintf(0, "spawned child: %i\n", x);
        int z = Fork();
        if (z != 0) { //not the child
            TracePrintf(0, "spawned child: %i\n", z);
            int y = Fork();
            if (y != 0) {
                TracePrintf(0, "spawned child: %i\n", y);
            }
        }
    }
    
    Exit(0);
}
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
    write(0, "init13! Testing Fork and wait with exec!\n",10);
    int status; 
    int x = Fork();
    if (x != 0) { //not the child
        TracePrintf(0, "pid is: %i\n", GetPid());
        TracePrintf(0, "spawned child: %i\n", x);
        TracePrintf(0, "waiting for child!: %i\n", x);
        write(0, "we in parent now\n", 15);
        Wait(&status);
        write(0, "Done waiting for child\n", 20);
        TracePrintf(0, "done waiting for child! status: %i\n", status);
        int z = Fork();
        if (z != 0) { //not the child
            TracePrintf(0, "spawned child: %i\n", z);
            int y = Fork();
            
            if (y != 0) {
                TracePrintf(0, "spawned child: %i\n", y);
                
            }
        }
    } else {//delay the child for a bit
        TracePrintf(0, "Delaying in the child! child pid is: %i\n", GetPid());
        write(0, "we in child\n", 10);
        char *args[2] = {"execTestProg", NULL}; 
        Exec(args[0], args);
        write(0, "we done executing in child\n", 10);
        TracePrintf(0, "Done executing in the child! child pid is: %i\n", GetPid());
    }
    
    Wait(&status);
    write(0, "Everything done\n", 15);
    Exit(0);
}
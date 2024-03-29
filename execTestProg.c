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

int
main()
{
    // process id is 1
    write(0, "execTestProg! testing Exec\n", 15);
    TracePrintf(0, "PID of execTestProg: %i\n", GetPid());
    // Brk((void *) 200000);
    Delay(4);
    Delay(2);
    Exit(0);
}

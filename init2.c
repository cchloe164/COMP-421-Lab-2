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
    write(0, "init2!\n", 6);
    TracePrintf(0, "init2 pid is: %i\n", GetPid());

    Exit(0);
}

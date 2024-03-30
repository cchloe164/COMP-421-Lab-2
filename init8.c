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
    char *buf = malloc(sizeof(char)*2);
    buf[0] = 'a';
    buf[1] = '\n';

    while(1){
        TtyWrite(1, buf, 2);
    }

    TtyWrite(1, buf, 2);
    Exit(0);
}
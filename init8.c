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
    char *text = "HELLO!\n";
    TtyWrite(1, (void *)text, 6);
    while(1) {}
    Exit(0);
}
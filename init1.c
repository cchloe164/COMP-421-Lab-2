#include <unistd.h>
#include <comp421/yalnix.h>

int
main()
{
    // process id is 1
    write(2, "init!\n", 6);
    Exit(0);
}

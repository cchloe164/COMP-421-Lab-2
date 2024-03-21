#include <unistd.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int
main()
{
    // process id is 0
    write(2, "idle!\n", 6);
    while(1){
        Pause();
    }
}
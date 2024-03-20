#include <unistd.h>
#include <comp421/yalnix.h>

int
main()
{
    write(2, "idle!\n", 6);
    while(1){
        Pause();
    }
}
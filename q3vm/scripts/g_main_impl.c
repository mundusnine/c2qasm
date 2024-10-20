#include "../../scripts/adder.h"
#include "bg_lib.h"

void main_init(void){
    int res = adder(34,35);
    char temp[64] = {0};
    snprintf(temp,64,"Starting up and adding %d\n",res);
    trap_Printf(temp);
}

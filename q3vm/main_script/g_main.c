#include "../commands.h"
#include "../scripts/bg_lib.h"

void main_init(void);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
int vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4,
           int arg5, int arg6, int arg7, int arg8, int arg9, int arg10,
           int arg11) {
    switch(command){
        case COMM_INIT:
        {
            main_init();
            break;
        }
    }
    return 0;
}


#include "q3vm.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc,char** argv){
    q3vm_init();
    printf("Loaded");
    // int should_end = 0;
    // while(!should_end){
    //     q3vm_pre_update();
    //     sleep(1);
    // }
}
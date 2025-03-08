#include "adder.h"
// float variableC =0.1f;
// int variableB;
// int adder(int a,int b){
//     return a+b;
// }
// float add_f_conv(unsigned int num_a,float num_b){
//     return num_a + num_b;
// }
// float add_f(float num_a,float num_b){
//     return num_a + num_b;
// }
// int multi_op_and_stacksize(int a,int b){
//     int c = a-b;
//     int d = a+c;
//     return d*c;
// }
// int adder_stacksize(int a,int b){
//     int c = a+b;
//     return c;
// }
// int conv_2char(char a,int b){
//     return a+b;
// }

float add_i42f4_test(int num_a,float num_b){
    float num_b_half = num_b/2.0f;
    return num_a + num_b_half;
}
// int adder_abs(int a,int b){
//     // int c = 420;
//     int d = 35;
//     return 34+d;
// }

// int div(int a){
//     int m = 2;
//     return a / m;
// }
// int mod(int a){
//     int m = 2;
//     return a % m;
// }
// int ptr(int* a){
//     int i = 34;
//     return *a + i;
// }

// void ptr_2(int* a){
//     int i = 84;
//     *a = 5 * i;
// }
// void ptr_3(int** a){
//     static int i = 84;
//     static char b = '0';
//     *a = &i;
// }
//@TODO: Reevaluate the generation with chibicc of multiple 
// Global vars that are initiliazed at different positions. The 
// order in which they are created is the inverse to the logical order of execution...
// int variableB = 69;
// int adder2(int c, int d){
//     return c+d;
// }
// char* shower(void){
//     return "I am a big shower not a cleaner";
// }

// Stack
//     |
//     |
//     |
//     |
// 69  |
// __  |

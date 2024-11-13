#define KNOB_IMPLEMENTATION
#include "knob.h"

int test_cstr_match(char* left, char* right){
    int i =0;
    while(left[i] != '\0' && right[i] != '\0'){
        if(left[i] != right[i]){
            return i;
        }
        ++i;
    }
    if(left[i] != '\0' || right[i] != '\0')
        return -1;
    return 1;
}

// 1. Generate C files [X]
// 2. Compile to bytecode [~]
    // with lcc(reevaluate if we should do this)
        // 3. strip unneeded info from lcc compiled files
    // with our compiler [X]
// 4. Validate output is the same [X]
// 5. Validate with inline bytecode [X]
// 6. Add runtime tests
#define TESTED_EXE_FILENAME "./bin/chibicc.com"
#define OUTPUT_FILE "./tests/test_output.asm"
#define EXE_CMDS "-cc1","-cc1-output",OUTPUT_FILE,"-cc1-input"

#define ANSI_RESET "\033[0m"
#define ANSI_RED(a) "\033[31m" a ANSI_RESET
#define ANSI_GREEN(a) "\033[32m" a ANSI_RESET
#define ANSI_YELLOW(a) "\033[33m" a ANSI_RESET
#define ANSI_BLUE(a) "\033[34m" a ANSI_RESET
#define ANSI_MAGENTA(a) "\033[34m" a ANSI_RESET
#define ANSI_CYAN(a) "\033[36m" a ANSI_RESET

typedef struct {
    const char* name;
    const char* file_gen;
    const char* content;
    const char* expected;
    int (*validation_func)(char*,const char*);
} Test;

typedef struct {
    Test* items;
    size_t count;
    size_t capacity;
} Tests;

int gen_file_do(const char* testname,const char* filename,const char* content,const char* expect,int (*func_do)(char*,const char*)){
    
    char* filepath = knob_temp_sprintf("./tests/%s",filename);
    knob_write_entire_file(filepath,(void*)content,strlen(content));
    Knob_Cmd cmd = {0};
    knob_cmd_append(&cmd,TESTED_EXE_FILENAME,EXE_CMDS,filepath);
    if(!knob_cmd_run_sync(cmd)){
        knob_log(KNOB_ERROR,"Failed compilation of test file named: %s",filepath);
        return 0;
    }
    
    int result = func_do(OUTPUT_FILE,expect);
    if(!result){
        knob_log(KNOB_ERROR,ANSI_RED(ANSI_RED("-------------[TEST] ") ANSI_CYAN("%s") ANSI_RED(" FAILED -------------")),testname);
    }
    return result;
}

void tests_run_all(Tests* tests,int stop_on_failure){

    size_t succeeded_count = 0;
    for(int i =0; i < tests->count;++i){
        Test curr = tests->items[i];
        succeeded_count += gen_file_do(curr.name,curr.file_gen,curr.content,curr.expected,curr.validation_func);
    }
    
    if(succeeded_count != tests->count){
        knob_log(KNOB_ERROR,ANSI_RED("-------------NOT ALL TESTS PASSED-------------"));
    }
    else{
        knob_log(KNOB_INFO,ANSI_GREEN("-------------ALL TESTS PASSED-------------"));
    }
}

int validate_output_match(char* output_filename,const char* expect){
    static Knob_String_Builder sb_out = {0};
    static Knob_String_View sv_out = {0};
    static Knob_String_View sv_exp = {0};
    sb_out.count = 0;
    sv_out.count = 0;
    sv_exp.count = 0;
    knob_read_entire_file(OUTPUT_FILE,&sb_out);
    knob_sb_append_null(&sb_out);
    sv_out = knob_sv_from_cstr(sb_out.items);
    sv_exp = knob_sv_from_cstr(expect);

    int matched = 1;
    while(sv_out.count > 0 && sv_exp.count > 0){
        Knob_String_View line_out = knob_sv_chop_by_delim(&sv_out,'\n');
        Knob_String_View line_exp = knob_sv_chop_by_delim(&sv_exp,'\n');
        char temp1[128] = {0};
        char temp2[128] = {0};
        memcpy(temp1,line_out.data,line_out.count);
        memcpy(temp2,line_exp.data,line_exp.count);
        int idx = test_cstr_match(temp1,temp2);
        if(idx != 1){
            char temp_err[128] = {0};
            for(int i =0; i < idx;++i){
                temp_err[i] = '~';
            }
            knob_log(KNOB_ERROR,ANSI_YELLOW("Got:"));
            knob_log(KNOB_ERROR,ANSI_RED("%s"),temp1);
            knob_log(KNOB_ERROR,ANSI_RED("%s") ANSI_YELLOW("^"),temp_err);
            knob_log(KNOB_ERROR,ANSI_YELLOW("Expected:"));
            knob_log(KNOB_ERROR,ANSI_RED("%s"),temp2);
            knob_log(KNOB_ERROR,ANSI_RED("%s") ANSI_YELLOW("^"),temp_err);
            matched = 0;
            break;
        }
    }
    return matched && sv_out.count == sv_exp.count;
}

MAIN(TESTS){
    // Check args for:
        // Do tests only on lcc output
        // Do tests only on inline bytecode
    Tests tests = {0};

    Test add = {
        name: "Add function generation",
        file_gen: "add_test.c",
        content: 
        "int adder(int a,int b){\n"
        "    return a+b;\n"
        "}\n",
        expected: 
        "export adder\n"
        "code\n"
        "proc adder 0 0\n"
        "ADDRFP4 0\n"
        "INDIRI4\n"
        "ADDRFP4 4\n"
        "INDIRI4\n"
        "ADDI4\n"
        "RETI4\n"
        "LABELV $1\n"
        "endproc adder 0 0\n",
        validation_func: validate_output_match
    };
    knob_da_append(&tests,add);
    Test conversion = {
        name: "Add function with type conversion generation",
        file_gen: "conv_add_test.c",
        content: 
        "int conv_2char(char a,int b){\n"
        "    return a+b;\n"
        "}\n",
        expected: 
        "export conv_2char\n"
        "code\n"
        "proc conv_2char 0 0\n"
        "ADDRFP4 0\n"
        "INDIRI1\n"
        "CVII4 1\n"
        "ADDRFP4 4\n"
        "INDIRI4\n"
        "ADDI4\n"
        "RETI4\n"
        "LABELV $1\n"
        "endproc conv_2char 0 0\n",
        validation_func: validate_output_match
    };
    knob_da_append(&tests,conversion);
    Test add_stacksize = {
        name: "Add function with a stacksize(local variable)",
        file_gen: "add_localvar_test.c",
        content: 
        "int adder_stacksize(int a,int b){\n"
            "int c = a+b;\n"
            "return c;\n"
        "}\n",
        expected: 
        "export adder_stacksize\n"
        "code\n"
        "proc adder_stacksize 4 0\n"
        "ADDRLP4 0\n"
        "ADDRFP4 0\n"
        "INDIRI4\n"
        "ADDRFP4 4\n"
        "INDIRI4\n"
        "ADDI4\n"
        "ASGNI4\n"
        "ADDRLP4 0\n"
        "INDIRI4\n"
        "RETI4\n"
        "LABELV $1\n"
        "endproc adder_stacksize 4 0\n",
        validation_func: validate_output_match
    };
    knob_da_append(&tests,add_stacksize);
    Test multiop_stacksize = {
        name: "Multiop function with a stacksize for 2 local variables",
        file_gen: "multiop_localvar_test.c",
        content: 
        "int multi_op_and_stacksize(int a,int b){\n"
            "int c = a-b;\n"
            "int d = a+c;\n"
            "return d*c;\n"
        "}\n",
        expected: 
        "export multi_op_and_stacksize\n"
        "code\n"
        "proc multi_op_and_stacksize 8 0\n"
        //;8:    int c = a-b;
        "ADDRLP4 0\n"
        "ADDRFP4 0\n"
        "INDIRI4\n"
        "ADDRFP4 4\n"
        "INDIRI4\n"
        "SUBI4\n"
        "ASGNI4\n"
        //;9:    int d = a+c;
        "ADDRLP4 4\n"
        "ADDRFP4 0\n"
        "INDIRI4\n"
        "ADDRLP4 0\n"
        "INDIRI4\n"
        "ADDI4\n"
        "ASGNI4\n"
        //;10:    return d*c;
        "ADDRLP4 4\n"
        "INDIRI4\n"
        "ADDRLP4 0\n"
        "INDIRI4\n"
        "MULI4\n"
        "RETI4\n"
        "LABELV $1\n"
        "endproc multi_op_and_stacksize 8 0\n",
        validation_func: validate_output_match
    };
    knob_da_append(&tests,multiop_stacksize);
    Test add_f = {
        name: "Add floats function",
        file_gen: "add_floats_test.c",
        content: 
        "float add_f(float num_a,float num_b){\n"
            "return num_a + num_b;\n"
        "}\n",
        expected: 
        "export add_f\n"
        "code\n"
        "proc add_f 0 0\n"
        "ADDRFP4 0\n"
        "INDIRF4\n"
        "ADDRFP4 4\n"
        "INDIRF4\n"
        "ADDF4\n"
        "RETF4\n"
        "LABELV $1\n"
        "endproc add_f 0 0\n",      
        validation_func: validate_output_match
    };
    knob_da_append(&tests,add_f);

    tests_run_all(&tests,false);
}
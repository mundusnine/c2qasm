#define KNOB_IMPLEMENTATION
#include "knob.h"

#include <time.h>

#include "./q3vm/q3vm_build.c"

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
    // U 
#define TESTED_EXE_FILENAME "./bin/chibicc.com"
#define EXE_CMDS(out,in) "-cc1","-cc1-output",out,"-cc1-input",in


#define ANSI_RESET "\e[0m"
#define ANSI_RED(a) "\e[31m" a ANSI_RESET
#define ANSI_GREEN(a) "\e[32m" a ANSI_RESET
#define ANSI_YELLOW(a) "\e[33m" a ANSI_RESET
#define ANSI_BLUE(a) "\e[34m" a ANSI_RESET
#define ANSI_MAGENTA(a) "\e[34m" a ANSI_RESET
#define ANSI_CYAN(a) "\e[36m" a ANSI_RESET

typedef struct {
    const char* name;
    const char* file_gen;
    const char* content;
    const char* test_file;
    int test_base_line;
    const char* expected;
    /* Lcc will sometimes emit asm that output's nonesense at runtime. We shouldn't fail test in that case. */
    int lcc_runtime_fails;
    int (*validation_func)(char*,const char*, const char *, int);
} Test;

typedef struct {
    Test* items;
    size_t count;
    size_t capacity;
} Tests;

int gen_file_do(const Test* curr){
    const char* testname = curr->name;
    const char* filename = curr->file_gen;
    const char* content = curr->content;
    const char* expect = curr->expected;
    int (*func_do)(char*,const char*, const char *, int) = curr->validation_func;

    char* filepath = knob_temp_sprintf("./tests/chibicc/%s",filename);
    char* lcc_filepath = knob_temp_sprintf("./tests/lcc/%s",filename);
    char* asmFilename = knob_temp_strdup(filename);
    asmFilename[strlen(asmFilename)-2] = '\0';
    char* outpath = knob_temp_sprintf("./tests/chibicc/%s.asm",asmFilename);
    //The order that we write the files is important since we use this quirk to recompile for 
    //the runtime tests...
    knob_write_entire_file(lcc_filepath,(void*)content,strlen(content));
    knob_write_entire_file(filepath,(void*)content,strlen(content));
    Knob_Cmd cmd = {0};
    knob_cmd_append(&cmd,TESTED_EXE_FILENAME,EXE_CMDS(outpath,filepath));
    if(!knob_cmd_run_sync(cmd)){
        knob_log(KNOB_ERROR,"Failed compilation of test file named: %s",filepath);
        return 0;
    }
    
    int result = func_do(outpath,expect,curr->test_file,curr->test_base_line);
    if(!result){
        knob_log(KNOB_ERROR,ANSI_RED(ANSI_RED("-------------[TEST] ") ANSI_CYAN("%s") ANSI_RED(" FAILED -------------")),testname);
    }
    if(curr->lcc_runtime_fails){
        knob_file_del(lcc_filepath);
        knob_file_del(filepath);
        knob_file_del(outpath);
    }
    return result;
}
#define MAX_INT_32 69
//(int)2147483647/2
int tests_run_all(Tests* tests,int stop_on_failure){

    srand(time(NULL));
    int val1 = rand() % MAX_INT_32;
    int val2 = rand() % (int)((MAX_INT_32 - val1) * 0.5f);
    Knob_String_Builder g_main_sb = {0};
    knob_sb_append_cstr(&g_main_sb,"#include \"../q3vm/scripts/bg_lib.h\"\n\n");
    knob_sb_append_cstr(&g_main_sb,"void main_init(void){\n");
    size_t succeeded_count = 0;
    for(int i =0; i < tests->count;++i){
        Test curr = tests->items[i];
        succeeded_count += gen_file_do(&curr);
        if(curr.lcc_runtime_fails){
          continue;  
        } 
        Knob_String_View sv = knob_sv_from_cstr(curr.content);
        Knob_String_View type_sv = knob_sv_chop_by_delim(&sv,' ');
        char temp_type[32] = {0}; 
        memcpy(temp_type,type_sv.data,type_sv.count);
        const char* type_format;
        if(knob_cstr_match(temp_type,"int")){
            type_format = " \%d";
        }
        if(knob_cstr_match(temp_type,"float")){
            type_format = " \%f";
        }
        char temp[64] = {0};
        int size = snprintf(temp,64,"%s",curr.file_gen);
        size-=2;
        temp[size] = '\0';
        knob_sb_append_cstr(&g_main_sb,"\t{\n\t\tchar temp[64] = {0};\n");
        knob_sb_append_cstr(&g_main_sb,knob_temp_sprintf("\t\t%s res = ",temp_type));
        knob_sb_append_buf(&g_main_sb,temp,size);
        knob_sb_append_cstr(&g_main_sb,knob_temp_sprintf("(%d,%d);\n",val1,val2));
        knob_sb_append_cstr(&g_main_sb,"\t\tsnprintf(temp,64,\"");
        knob_sb_append_buf(&g_main_sb,temp,size);
        knob_sb_append_cstr(&g_main_sb,knob_temp_sprintf("%s\\n\",res);\n",type_format));
        knob_sb_append_cstr(&g_main_sb,"\t\ttrap_Printf(temp);\n\t}\n");
    }
    knob_sb_append_cstr(&g_main_sb,"\texit();\n}\n");

    knob_file_del("./build/g_main_impl.asm");
    
    if(succeeded_count != tests->count){
        knob_log(KNOB_ERROR,ANSI_RED("-------------NOT ALL TESTS PASSED-------------"));
        return 0;
    }
    else{
        knob_log(KNOB_INFO,ANSI_GREEN("-------------ALL TESTS PASSED-------------"));
        knob_write_entire_file("./tests/g_main_impl.c",(void*)g_main_sb.items,g_main_sb.count);
        return 1;
    }
}

int validate_output_match(char* output_filename,const char* expect,const char* test_filename,int test_base_line){
    static Knob_String_Builder sb_out = {0};
    static Knob_String_View sv_out = {0};
    static Knob_String_View sv_exp = {0};
    sb_out.count = 0;
    sv_out.count = 0;
    sv_exp.count = 0;
    knob_read_entire_file(output_filename,&sb_out);
    knob_sb_append_null(&sb_out);
    sv_out = knob_sv_from_cstr(sb_out.items);
    sv_exp = knob_sv_from_cstr(expect);

    int matched = 1;
    int line_count =1;
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
            knob_log(KNOB_ERROR,ANSI_RED("%s :%s:%d"),temp1,output_filename,line_count);
            knob_log(KNOB_ERROR,ANSI_RED("%s") ANSI_YELLOW("^"),temp_err);
            knob_log(KNOB_ERROR,ANSI_YELLOW("Expected:"));
            knob_log(KNOB_ERROR,ANSI_RED("%s :%s%s:%d"),temp2,"./",test_filename,test_base_line+line_count);
            knob_log(KNOB_ERROR,ANSI_RED("%s") ANSI_YELLOW("^"),temp_err);
            matched = 0;
            break;
        }
        line_count++;
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
        "int add_test(int a,int b){\n"
        "    return a+b;\n"
        "}\n",
        test_file: __FILE__,
        test_base_line: __LINE__ + 2,
        expected: 
        "export add_test\n"
        "code\n"
        "proc add_test 0 0\n"
        "ADDRFP4 0\n"
        "INDIRI4\n"
        "ADDRFP4 4\n"
        "INDIRI4\n"
        "ADDI4\n"
        "RETI4\n"
        "LABELV $1\n"
        "endproc add_test 0 0\n",
        validation_func: validate_output_match
    };
    knob_da_append(&tests,add);
    Test conversion = {
        name: "Add function with type conversion generation",
        file_gen: "conv_add_test.c",
        content: 
        "int conv_add_test(char a,int b){\n"
        "    return a+b;\n"
        "}\n",
        test_file: __FILE__,
        test_base_line: __LINE__ + 2,
        expected: 
        "export conv_add_test\n"
        "code\n"
        "proc conv_add_test 0 0\n"
        "ADDRFP4 0\n"
        "INDIRI1\n"
        "CVII4 1\n"
        "ADDRFP4 4\n"
        "INDIRI4\n"
        "ADDI4\n"
        "RETI4\n"
        "LABELV $1\n"
        "endproc conv_add_test 0 0\n",
        validation_func: validate_output_match
    };
    knob_da_append(&tests,conversion);
    Test add_stacksize = {
        name: "Add function with a stacksize(local variable)",
        file_gen: "add_localvar_test.c",
        content: 
        "int add_localvar_test(int a,int b){\n"
        "   int c = a+b;\n"
        "   return c;\n"
        "}\n",
        test_file: __FILE__,
        test_base_line: __LINE__ + 2,
        expected: 
        "export add_localvar_test\n"
        "code\n"
        "proc add_localvar_test 4 0\n"
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
        "endproc add_localvar_test 4 0\n",
        validation_func: validate_output_match
    };
    knob_da_append(&tests,add_stacksize);
    Test multiop_stacksize = {
        name: "Multiop function with a stacksize for 2 local variables",
        file_gen: "multiop_localvar_test.c",
        content: 
        "int multiop_localvar_test(int a,int b){\n"
        "   int c = a-b;\n"
        "   int d = a+c;\n"
        "   return d*c;\n"
        "}\n",
        test_file: __FILE__,
        test_base_line: __LINE__ + 2,
        expected: 
        "export multiop_localvar_test\n"
        "code\n"
        "proc multiop_localvar_test 8 0\n"
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
        "endproc multiop_localvar_test 8 0\n",
        validation_func: validate_output_match
    };
    knob_da_append(&tests,multiop_stacksize);
    Test add_f = {
        name: "Add floats function",
        file_gen: "add_floats_test.c",
        content: 
        "float add_floats_test(float num_a,float num_b){\n"
        "   return num_a + num_b;\n"
        "}\n",
        test_file: __FILE__,
        test_base_line: __LINE__ + 2,
        expected: 
        "export add_floats_test\n"
        "code\n"
        "proc add_floats_test 0 0\n"
        "ADDRFP4 0\n"
        "INDIRF4\n"
        "ADDRFP4 4\n"
        "INDIRF4\n"
        "ADDF4\n"
        "RETF4\n"
        "LABELV $1\n"
        "endproc add_floats_test 0 0\n",      
        validation_func: validate_output_match
    };
    knob_da_append(&tests,add_f);
    Test add_uf_conv = {
        name: "Add an unsigned int to a float",
        file_gen: "add_u42f4_test.c",
        content: 
        "float add_u42f4_test(unsigned int num_a,float num_b){\n"
        "   return num_a + num_b;\n"
        "}\n",
        test_file: __FILE__,
        test_base_line: __LINE__ + 2,
        expected: 
        "export add_u42f4_test\n"
        "code\n"
        "proc add_u42f4_test 0 0\n"
        "ADDRFP4 0\n"
        "INDIRU4\n"
        "CVUI4 4\n"
        "ADDRFP4 4\n"
        "INDIRF4\n"
        "ADDF4\n"
        "RETF4\n"
        "LABELV $1\n"
        "endproc add_u42f4_test 0 0\n",      
        validation_func: validate_output_match,
        lcc_runtime_fails: 1
    };
    // Remove test temporarily since we can't validate output with lcc.
    knob_da_append(&tests,add_uf_conv);

    //@TODO: Readd later, floats seems to be imprecise to validate output, so we should probable do something like
    // validate that the number is close enough...
    // Test div_add_If_conv = {
    //     name: "Div a float by float and add an int to the local",
    //     file_gen: "div_add_i42f4_test.c",
    //     content: 
    //     "float div_add_i42f4_test(int num_a,float num_b){\n"
    //     "   float num_b_half = num_b/2.0f;\n"
    //     "   return num_a + num_b_half;\n"
    //     "}\n",
    //     test_file: __FILE__,
    //     test_base_line: __LINE__ + 2,
    //     expected: 
    //     "export div_add_i42f4_test\n"
    //     "code\n"
    //     "proc div_add_i42f4_test 4 0\n"
    //     "ADDRLP4 0\n"
    //     "ADDRFP4 4\n"
    //     "INDIRF4\n"
    //     "CNSTF4 2\n"
    //     "DIVF4\n"
    //     "ASGNF4\n"
    //     "ADDRFP4 0\n"
    //     "INDIRI4\n"
    //     "CVIF4 4\n"
    //     "ADDRLP4 0\n"
    //     "INDIRF4\n"
    //     "ADDF4\n"
    //     "RETF4\n"
    //     "LABELV $1\n"
    //     "endproc div_add_i42f4_test 4 0\n",      
    //     validation_func: validate_output_match
    // };
    // knob_da_append(&tests,div_add_If_conv);

    knob_mkdir_if_not_exists("./tests/chibicc");
    knob_mkdir_if_not_exists("./tests/lcc");
    int success = tests_run_all(&tests,false);
    if(success){
        // Knob_File_Paths childs = {0};
        // knob_read_entire_dir("./build",&childs);
        // for(int i =0; i < childs.count;++i){
        //     char* filename = childs.items[i];
        //     if(knob_cstr_match(filename,"..") || knob_cstr_match(filename,".")) continue;
        //     char* path = knob_temp_sprintf("./build/%s",childs.items[i]);
        //     if(knob_cstr_ends(path,".asm")){
        //         knob_file_del(path);
        //     }
        // }
        Knob_String_Builder sb = {0};
        q3vm_search_recursive = 0;
        q3vm_add_scripts_folder("./q3vm/main_script");
        q3vm_add_scripts_folder("./q3vm/scripts");
        q3vm_add_scripts_folder("./tests/");
        q3vm_add_scripts_folder("./tests/lcc");
        if(q3vm_build(".",Q3BUILD_VERIFY) == -1) return 1;
        Knob_Cmd cmd = {0};
        chdir("./Deployment");
        knob_cmd_append(&cmd,"./runtime_test.com");
        if(!knob_cmd_run_sync(cmd)){
            knob_log(KNOB_ERROR,"Failed running runtime !");
            return 0;
        }
        #define LCC_OUT "./lcc_output.txt"
        #define CHIBICC_OUT "./chibicc_output.txt"
        knob_file_del(LCC_OUT);
        knob_file_del(CHIBICC_OUT);

        knob_rename("./test_output.txt",LCC_OUT);
        chdir("..");
        q3vm_scripts.count = 0;
        q3vm_add_scripts_folder("./q3vm/main_script");
        q3vm_add_scripts_folder("./q3vm/scripts");
        q3vm_add_scripts_folder("./tests/");
        q3vm_add_scripts_folder("./tests/chibicc");
        for(int i =0; i < tests.count;++i){
            Test t = tests.items[i];
            char* asmFilename = knob_temp_strdup(t.file_gen);
            asmFilename[strlen(asmFilename)-2] = '\0';
            char* outpath = knob_temp_sprintf("./build/%s.asm",asmFilename);
            sb.count = 0;
            knob_read_entire_file(knob_temp_sprintf("./tests/chibicc/%s.asm",asmFilename),&sb);
            knob_file_del(outpath);
            knob_write_entire_file(outpath,sb.items,sb.count);
        }
        if(q3vm_build(".",Q3BUILD_LINK) == -1) return 1;
        chdir("./Deployment");
        knob_cmd_append(&cmd,"./runtime_test.com");
        if(!knob_cmd_run_sync(cmd)){
            knob_log(KNOB_ERROR,"Failed running runtime after chibicc compile!");
            return 0;
        }
        knob_rename("./test_output.txt",CHIBICC_OUT);

        Knob_String_Builder lcc_sb = {0};
        knob_read_entire_file(LCC_OUT,&lcc_sb);
        knob_sb_append_null(&lcc_sb);
        if(validate_output_match(CHIBICC_OUT,lcc_sb.items,&LCC_OUT[2],0)){
            knob_file_del(LCC_OUT);
            knob_file_del(CHIBICC_OUT);
            knob_log(KNOB_INFO,ANSI_GREEN("Both files had the same output, everything seems to be working !"));
        }
        chdir("..");
    }
    return 0;
}
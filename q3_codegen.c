#include "chibicc.h"

int cstr_ends(char* str,const char* end){
    int e_len = strlen(end);
    int s_len = strlen(str);
    int i =1;
    while(e_len-i > -1){
        if(str[s_len-i] != end[e_len-i]){
            return 0;
        }
        i++;
    } 
    return 1;
}

static FILE *output_file;
__attribute__((format(printf, 1, 2)))
static void println(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  fprintf(output_file, "\n");
}


static int is_first_data;
static void invalidate_statics_vars(void){
    is_first_data = true;
}
static void add_data_section(void){
    if(is_first_data){
        println("data");
        is_first_data = false;
    }
}
static void gen_data_section(Obj* prog){
    for (Obj *fn = prog; fn; fn = fn->next) {
        if (fn->is_function){
            println("export %s",fn->name);
        }
        else if(fn->is_definition && fn->name[0] != '.' && fn->init_data != NULL){
            add_data_section();
            println("export %s",fn->name);
            println("align %d",fn->align);
            println("LABELV %s",fn->name);
            int pos = 0;
            long combined = (fn->init_data[pos++] & 0xFF);//(byte1 & 0xFF) | ((byte2 & 0xFF) << 8) | ((byte3 & 0xFF) << 16) | ((byte4 & 0xFF) << 24);
            while(pos < fn->ty->size){
                int t_pos = pos;
                combined |= ((fn->init_data[pos++] & 0xFF) << (t_pos * 8));
            }
            println("byte %d %ld",fn->ty->size,combined);
            
        }
    }
}
static void gen_code_section(Obj* prog){

}
void codegen(Obj *prog, FILE *out){
    output_file = out;
    invalidate_statics_vars();
    gen_data_section(prog);
    gen_code_section(prog);
    // File **files = get_input_files();
    // for (int i = 0; files[i]; i++){
    //     if(cstr_ends(files[i]->name,".c")){
    //         println("file \"%s\"", files[i]->name);
    //     }
    // }
    return;
}

//Taken from codegen.c
// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
int align_to(int n, int align) {
  return (n + align - 1) / align * align;
}
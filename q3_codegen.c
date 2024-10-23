#define KNOB_IMPLEMENTATION
#include "knob.h"

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
__attribute__((format(printf, 1, 3)))
static void println(char *fmt,Knob_String_Builder* sb, ...) {
  char temp[512] = {0};
  va_list ap;
  va_start(ap, fmt);
  size_t b_len = vsnprintf(temp,512,fmt,ap);
  va_end(ap);
  knob_sb_append_buf(sb,temp,b_len);
  knob_sb_append_cstr(sb, "\n");
}


static int is_first_data;
static void invalidate_statics_vars(void){
    is_first_data = true;
}
// static void add_data_section(void){
//     if(is_first_data){
//         println("data");
//         is_first_data = false;
//     }
// }
// static void gen_data_section(Obj* prog){
//     for (Obj *fn = prog; fn; fn = fn->next) {
//         if (fn->is_function){
//             println("export %s",fn->name);
//         }
//         else if(fn->is_definition && fn->name[0] != '.' && fn->init_data != NULL){
//             add_data_section();
//             println("export %s",fn->name);
//             println("align %d",fn->align);
//             println("LABELV %s",fn->name);
//             int pos = 0;
//             long combined = (fn->init_data[pos++] & 0xFF);//(byte1 & 0xFF) | ((byte2 & 0xFF) << 8) | ((byte3 & 0xFF) << 16) | ((byte4 & 0xFF) << 24);
//             while(pos < fn->ty->size){
//                 int t_pos = pos;
//                 combined |= ((fn->init_data[pos++] & 0xFF) << (t_pos * 8));
//             }
//             println("byte %d %ld",fn->ty->size,combined);
            
//         }
//     }
// }
char* type_to_str(Type* type){
    static char temp[3] = {0};
    switch(type->kind){
        case TY_INT:{
            temp[0] = 'I';
            temp[1] = '0' + type->size;
            break;
        }
        //     TY_VOID,
        //   TY_BOOL,
        //   TY_CHAR,
        //   TY_SHORT,
        //   TY_LONG,
        //   TY_FLOAT,
        //   TY_DOUBLE,
        //   TY_LDOUBLE,
        //   TY_ENUM,
        //   TY_PTR,
        //   TY_FUNC,
        //   TY_ARRAY,
        //   TY_VLA, // variable-length array
        //   TY_STRUCT,
        //   TY_UNION,
        default:
        assert(0 &&  "Unreachable");
    }
    return temp;
}
// ND_NULL_EXPR, // Do nothing
// ND_ADD,       // +
// ND_SUB,       // -
// ND_MUL,       // *
// ND_DIV,       // /
// ND_NEG,       // unary -
// ND_MOD,       // %
// ND_BITAND,    // &
// ND_BITOR,     // |
// ND_BITXOR,    // ^
// ND_SHL,       // <<
// ND_SHR,       // >>
// ND_EQ,        // ==
// ND_NE,        // !=
// ND_LT,        // <
// ND_LE,        // <=
// ND_ASSIGN,    // =
// ND_COND,      // ?:
// ND_COMMA,     // ,
// ND_MEMBER,    // . (struct member access)
// ND_ADDR,      // unary &
// ND_DEREF,     // unary *
// ND_NOT,       // !
// ND_BITNOT,    // ~
// ND_LOGAND,    // &&
// ND_LOGOR,     // ||
// ND_RETURN,    // "return"
// ND_IF,        // "if"
// ND_FOR,       // "for" or "while"
// ND_DO,        // "do"
// ND_SWITCH,    // "switch"
// ND_CASE,      // "case"
// ND_BLOCK,     // { ... }
// ND_GOTO,      // "goto"
// ND_GOTO_EXPR, // "goto" labels-as-values
// ND_LABEL,     // Labeled statement
// ND_LABEL_VAL, // [GNU] Labels-as-values
// ND_FUNCALL,   // Function call
// ND_EXPR_STMT, // Expression statement
// ND_STMT_EXPR, // Statement expression
// ND_VAR,       // Variable
// ND_VLA_PTR,   // VLA designator
// ND_NUM,       // Integer
// ND_MEMZERO,   // Zero-clear a stack variable
static Obj* parameters = NULL;
static int is_func_parameter(Obj* var){
    int parm_i = 0;
    int out_val = -1;
    for(Obj* parm = parameters; parm;parm = parm->next){
        if(knob_cstr_match(parm->name,var->name)){
            out_val = parm_i;
            break;
        }
        parm_i++;
    }
    return out_val;
}
void gen_from_body(Node* body,Knob_String_Builder* sb){
    switch(body->kind){
        case ND_RETURN:{
            gen_from_body(body->lhs,sb);
            char* ret_type = type_to_str(body->lhs->ty);
            println("RET%s",sb,ret_type);
            break;
        }
        case ND_CAST:{      // Type cast
            gen_from_body(body->lhs,sb);
            if(body->ty->kind != body->lhs->ty->kind){
                assert(0 && "Add conversion...");
                //Think about doing conversion....
            }
            break;
        }
        case ND_ADD:{
            gen_from_body(body->lhs,sb);
            gen_from_body(body->rhs,sb);
            char* type_str = type_to_str(body->ty);
            println("ADD%s",sb,type_str);
            break;
        }
        case ND_VAR: {
            int parm_i = is_func_parameter(body->var);
            if(body->var->is_local){
                if(parm_i >= 0){
                    char* type_str = type_to_str(body->ty);
                    println("ADDRFP4 %ld",sb,parm_i * 4);
                    println("INDIR%s",sb,type_str);
                }
                else {
                    assert(0 && "Local variables need to be supported");
                }
            }
            else {
                assert(0 && "Global variables need to be supported");
            }
            break;
        }
        case ND_NUM: {
            char* type_str = type_to_str(body->ty);
            println("CNST%s %ld",sb,type_str,body->val);
            break;
        }
        default:
            break;
    }
}
void codegen(Obj *prog, FILE *out){
    output_file = out;
    Knob_String_Builder start = {0};
    Knob_String_Builder end = {0};
    int func_count = 1;
    Obj* last = NULL;
    for (Obj *fn = prog; fn; fn = fn->next) {
        if (fn->is_function){
            println("export %s",&end,fn->name);
            if(!last || !last->is_function){
                println("code",&end);
            }
            int locals_needed_bytes = 0;
            int args_marshalling_bytes = 0;
            println("proc %s %d %d",&end,fn->name,locals_needed_bytes,args_marshalling_bytes);
            Type* ret_type = fn->ty->return_ty;
            parameters = fn->params;
            for(Node* bod = fn->body->body;bod; bod = bod->next){
                gen_from_body(bod,&end);
                printf("Hello World !");
            }
            println("LABELV $%d",&end,func_count++);
            println("endproc %s %d %d",&end,fn->name,locals_needed_bytes,args_marshalling_bytes);


        }
        last = fn;
    }
    knob_sb_append_null(&end);
    fprintf(out,end.items);
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
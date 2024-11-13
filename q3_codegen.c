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
  va_start(ap,fmt);
  size_t b_len = vsnprintf(temp,512,fmt,ap);
  va_end(ap);
  knob_sb_append_buf(sb,temp,b_len);
  knob_sb_append_cstr(sb, "\n");
}


// static int is_first_data;
// static void invalidate_statics_vars(void){
//     is_first_data = true;
// }
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
    int size = type->size;
    switch(type->kind){

        case TY_CHAR:
        case TY_SHORT:
        case TY_LONG:
        case TY_INT:{
            if(type->is_unsigned){
                temp[0] = 'U';
            }
            else {
                temp[0] = 'I';
            }
            temp[1] = '0' + size;
            break;
        }
        case TY_LDOUBLE:
        case TY_DOUBLE:{
            size = 4;
            knob_log(KNOB_ERROR,"double is not supported, operations will be done with type: float");
            goto AS_FLOAT;
        }
        case TY_FLOAT:{
        AS_FLOAT:
            temp[0] = 'F';
            temp[1] = '0' + size;
            break;
        }
        //     TY_VOID,
        //   TY_BOOL,
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
typedef struct {
    Obj** items;
    size_t count;
    size_t capacity;
} Locals;
static Locals locals = {0};
static int get_local_offset(const char* name){
    int offset = 0;
    for(int i =locals.count-1; i >= 0;--i){
        if(knob_cstr_match(name,locals.items[i]->name)){
            break;
        }
        offset += 4;
    }
    return offset;
}
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
const char* ops[] = {
    ";NULL",//ND_NULL_EXPR We shouldn't get this
    "ADD%s",//ND_ADD
    "SUB%s",//ND_SUB
    "MUL%s",//ND_MUL
    "DIV%s",//ND_DIV
    "NEG%s",//ND_NEG
    "MOD%s",//ND_MOD
};
int gen_from_body(Node* body,Knob_String_Builder* sb){
    switch(body->kind){
        char* fmt = NULL;
        //OPS
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
        case ND_MOD:
        fmt = ops[body->kind];
        gen_from_body(body->lhs,sb);
        char* type_str = NULL;
        // if(end_kind == ND_VAR){
        //     type_str = type_to_str(body->lhs->ty);
        //     println("INDIR%s",sb,type_str);
        // }
        gen_from_body(body->rhs,sb);
        // if(end_kind == ND_VAR){
        //     type_str = type_to_str(body->rhs->ty);
        //     println("INDIR%s",sb,type_str);
        // }
        type_str = type_to_str(body->ty);
        println(fmt,sb,type_str);
        break;
        //END OPS
        case ND_COMMA:{
            gen_from_body(body->lhs,sb);
            gen_from_body(body->rhs,sb);
            return ND_COMMA;
        }
        case ND_EXPR_STMT:{
            gen_from_body(body->lhs,sb);
            break;
        }
        case ND_ASSIGN:{
            gen_from_body(body->lhs,sb);
            gen_from_body(body->rhs,sb);
            char* type_str = type_to_str(body->ty);

            // if (node->lhs->kind == ND_MEMBER && node->lhs->member->is_bitfield) {
            //     println("  mov %%rax, %%r8");

            //     // If the lhs is a bitfield, we need to read the current value
            //     // from memory and merge it with a new value.
            //     Member *mem = node->lhs->member;
            //     println("  mov %%rax, %%rdi");
            //     println("  and $%ld, %%rdi", (1L << mem->bit_width) - 1);
            //     println("  shl $%d, %%rdi", mem->bit_offset);

            //     println("  mov (%%rsp), %%rax");
            //     load(mem->ty);

            //     long mask = ((1L << mem->bit_width) - 1) << mem->bit_offset;
            //     println("  mov $%ld, %%r9", ~mask);
            //     println("  and %%r9, %%rax");
            //     println("  or %%rdi, %%rax");
            //     store(node->ty);
            //     println("  mov %%r8, %%rax");
            //     return;
            // }
            println("ASGN%s",sb,type_str);
            return ND_ASSIGN;
        }
        case ND_MEMZERO: {
            break;
        }
        case ND_BLOCK:{
            for (Node *n = body->body; n; n = n->next)
                gen_from_body(n,sb);
            break;
        }
        case ND_NULL_EXPR:{
            break;
        }
        case ND_VAR: {
            int parm_i = is_func_parameter(body->var);
            if(body->var->is_local){
                if(parm_i >= 0){
                    println("ADDRFP4 %d",sb,parm_i * 4);
                }
                else {
                    int offset = get_local_offset(body->var->name);
                    println("ADDRLP4 %d",sb,offset);
                }
            }
            else {
                assert(0 && "Global variables need to be supported");
            }
            return ND_VAR;
        }
        case ND_NUM: {
            char* type_str = type_to_str(body->ty);
            println("CNST%s %ld",sb,type_str,body->val);
            return ND_NUM;
        }
        case ND_DEREF:{
            gen_from_body(body->lhs,sb);
            println("INDIRP4",sb);
            break;
        }
        case ND_ADDR:{
            gen_from_body(body->lhs,sb);
            println("ADDRGP4 $%d",sb,1);
            break;
        }
        case ND_CAST:{      // Type cast
            int ret = gen_from_body(body->lhs,sb);
            char* type_str = NULL;
            if(ret == ND_VAR){
                type_str = type_to_str(body->lhs->ty);
                println("INDIR%s",sb,type_str);
            }
            if(body->ty->kind != body->lhs->ty->kind){
                // type_str = type_to_str(body->ty);
                //@TODO: We need to evaluate if what lcc outputs is valid...
                //Basically, when we have any kind of int and we convert it, lcc always outputs I4
                //Should we do the same ? Testing should help us here in validating what the end computation does
                //depending on what opcodes we output.
                if(type_str[0] == 'I'){
                    type_str[1] = '0' + 4;
                }
                if(body->ty->kind == TY_INT && body->ty->is_unsigned){                
                    println("CVU%s %d",sb,type_str,body->lhs->ty->size);
                }
                else if(body->ty->kind == TY_INT){
                    println("CVI%s %d",sb,type_str,body->lhs->ty->size);
                }
                else if(body->ty->kind == TY_FLOAT){
                    println("CVF%s %d",sb,type_str,body->lhs->ty->size);
                }
                else {
                    assert(0 && "UNREACHABLE");
                }
                //@TODO: Should we return ND_CAST since we should do the inderection before the conversion...
            }
            return ret;
        }
        case ND_RETURN:{
            gen_from_body(body->lhs,sb);
            char* ret_type = type_to_str(body->lhs->ty);
            println("RET%s",sb,ret_type);
            break;
        }
        default:
            assert(0 && "Unreachable, unless kind isn't supported");
            break;
    }
    return ND_NULL_EXPR;
}
typedef struct {
    char** items;
    size_t count;
    size_t capacity;
} Funcnames;

static Funcnames funcs = {0};
int is_func(char* name){
    for(int i =0; i < funcs.count;++i){
        if(knob_cstr_match(name,funcs.items[i])){
            return 1;
        }
    }
    return 0;
}
void codegen(Obj *prog, FILE *out){
    output_file = out;
    // Knob_String_Builder start = {0};
    Knob_String_Builder end = {0};
    int label_count = 1;
    for (Obj *fn = prog; fn; fn = fn->next) {
        if(fn->is_function){
            knob_da_append(&funcs,fn->name);
        }
    }
    Obj* last = NULL;
    for (Obj *fn = prog; fn; fn = fn->next) {
        if(fn->is_static && fn->is_definition && !is_func(fn->init_data)){
            if(!last || last->is_function){
                println("data",&end);
            }
            println("align %d",&end,fn->align);
            println("LABELV $%d",&end,label_count++);
            println("byte %d %d",&end,fn->ty->size,fn->init_data);
            last = fn;
        }
        if (fn->is_function){
            locals.count = 0;
            println("export %s",&end,fn->name);
            if(!last || !last->is_function){
                println("code",&end);
            }
            int locals_needed_bytes = 0;
            for(Obj* local = fn->locals;local;local = local->next){
                //@TODO: Validate that this is always alloca_size
                if(knob_cstr_match(local->name,"__alloca_size__")){
                    break;
                }
                locals_needed_bytes += local->ty->size;
                knob_da_append(&locals,local);
            }
            int args_marshalling_bytes = 0;
            println("proc %s %d %d",&end,fn->name,locals_needed_bytes,args_marshalling_bytes);
            // Type* ret_type = fn->ty->return_ty;
            parameters = fn->params;
            for(Node* bod = fn->body->body;bod; bod = bod->next){
                gen_from_body(bod,&end);
            }
            println("LABELV $%d",&end,label_count++);
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
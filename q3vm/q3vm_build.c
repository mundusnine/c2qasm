/*
* INCLUDE THIS FILE IN ONLY ONE C FILE AND AFTER knob.h and KNOB_IMPLEMENTATION 
*/
#ifndef KNOB_IMPLEMENTATION
#include "knob.h" // For developpement reasons, this isn't actually used.
#endif
typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} Scripts;

typedef enum {
    Q3BUILD_NO,
    Q3BUILD_VERIFY,
    Q3BUILD_LINK,
} Q3VM_Build_Opt;

static Scripts q3vm_scripts = {0};
static int q3vm_search_recursive = 1;
static void q3vm_add_scripts_folder(const char* folder_path){
    Knob_File_Paths files = {0};
    knob_read_entire_dir(folder_path,&files);
    for(int i =0; i < files.count;++i){
        const char* filename = files.items[i];
        if(filename[0] == '.') continue; // To avoid ".." and "."
        char* filepath = KNOB_REALLOC(NULL,260);
        memset(filepath,0,260);
        int wrote = snprintf(filepath,260,"%s"PATH_SEP"%s",folder_path,filename);
        assert(wrote >= 0);
        if(knob_get_file_type(filepath) == KNOB_FILE_DIRECTORY && q3vm_search_recursive){
            q3vm_add_scripts_folder(filepath);
        }
        else if(knob_cstr_ends(filepath,".c")){
            knob_da_append(&q3vm_scripts,filepath);
        }
    }
}
#define ASM_FOLDER "Deployment"
static int q3vm_build(const char* root, Q3VM_Build_Opt build_opt){
    size_t checkpoint = knob_temp_save();
    #ifdef CHIBICC_IMPL
    #define LCC "%s/bin/chibicc.com"
    #define CFLAGS(asmFile,scriptFile) "-cc1","-cc1-input",scriptFile,"-cc1-output",asmFile
    #else
    #define LCC "%s/bin/lcc.com"
    #define CFLAGS(asmFile,scriptFile) "-S", "-Wf-target=bytecode", "-Wf-g","-o",asmFile,scriptFile
    #endif

    #define ASSEMBLER "%s/bin/q3asm"

    Knob_Cmd  cmd = {0};
    cmd.count = 0;
    int isRebuild = 0;
    //@TODO: Make this a loop that uses the registered scripts
    //@TODO: Always have bg_lib.c in registered scripts aka the replacement c library
    char* asmFiles[1024] = {0};
    size_t num_asm = 0;
    size_t sv_len = 0;
    for(int i =0; i < q3vm_scripts.count; ++i){
        const char* scriptName = q3vm_scripts.items[i];
        Knob_String_View sv = knob_sv_from_cstr(knob_temp_strdup(scriptName));
        char* filename = NULL;
        while(sv.count > 0){
            knob_sv_chop_by_delim(&sv,PATH_SEP[0]);
            if(sv.count > 0){
                filename = (char*)sv.data;
                sv_len = sv.count;
            }
        }
        filename[sv_len-2] = '\0';

        char* asmName = knob_temp_sprintf("%s"PATH_SEP"build"PATH_SEP"%s.asm",root,filename);
        if(!build_opt || (build_opt && knob_needs_rebuild1(asmName,scriptName))){
            isRebuild = 1;
            knob_cmd_append(&cmd,knob_temp_sprintf(LCC,root),CFLAGS(asmName,scriptName),"-DQ3_VM");
            if(!knob_cmd_run_sync(cmd)){
                printf("Error: Couldn't build file !\n");
                return -1;
            }
        }
        asmFiles[num_asm++] = asmName;
        cmd.count = 0;
    }

    if(isRebuild || build_opt == Q3BUILD_LINK){
        char* output_path = strlen(root) == 1 && root[0] == '.' ? "."PATH_SEP"Deployment"PATH_SEP : "."PATH_SEP;
        knob_cmd_append(&cmd,knob_temp_sprintf(ASSEMBLER,root),"-v","-m","-o",knob_temp_sprintf("%sbytecode.qvm",output_path));
        for(int i =0; i < num_asm;++i){
            knob_cmd_append(&cmd,asmFiles[i]);    
        }
        knob_cmd_append(&cmd,knob_temp_sprintf("%s"PATH_SEP ASM_FOLDER PATH_SEP"g_syscalls",root));
        // Knob_String_Builder command = {0};
        // knob_cmd_render(cmd,&command);
        // knob_sb_append_null(&command);
        // printf("%s\n",command.items);
        if(!knob_cmd_run_sync(cmd)){
            printf("Error: Couldn't assemble file !\n");
            return -1;
        }
    }
    knob_temp_rewind(checkpoint);

    return isRebuild;
}
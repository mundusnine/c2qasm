
#define KNOB_IMPLEMENTATION
#include "knob.h"

#include "./q3vm/q3vm_build.c"

#ifdef CONFIGURED
#undef CONFIGURED
#endif
#define CONFIGURED
#ifdef CONFIGURED

// #include "build/config.h"


#ifdef _WIN32
#define LIB_EXT ".lib"
#define ZIG_PATH "./Tools/windows_x64/zig.exe"
#define ZIG_TARGET "x86_64-windows"
#else
#define ZIG_PATH "./Tools/linux_x64/zig"
#define LIB_EXT ".a"
#define ZIG_TARGET "x86_64-linux"
#endif


#define SCRIPT_FOLDER "scripts/"

void knob_clean_dir(const char* dirpath){
    Knob_File_Paths children = {0};
    knob_read_entire_dir(dirpath,&children);
    for(int i = 0; i < children.count;++i){
        const char* filename = children.items[i];
        if(filename[0] == '.' && strlen(filename) < 3){
            continue;
        } 
        char* filepath = knob_temp_sprintf("%s"PATH_SEP"%s",dirpath,filename);
        if(knob_cstr_ends(filepath,".o")){
            knob_file_del(filepath);
        }
        if(knob_path_is_dir(filepath)){
            knob_clean_dir(filepath);
        }
    }
}
int build_chibicc(Knob_File_Paths* end_cmd,Knob_Config* config){
    bool result = true;
    Knob_Cmd cmd = {0};

    if (!knob_mkdir_if_not_exists("./build/chibicc")) {
        knob_return_defer(false);
    }
    
    Knob_Config conf = {0};
    knob_config_init(&conf);
    conf.compiler = config->compiler;
    conf.debug_mode = config->debug_mode;
    
    Knob_File_Paths files = {0};
    knob_da_mult_append(&files,
        //Providers
        "./chibicc/hashmap.c",
        "./chibicc/main.c",
        "./chibicc/parse.c",
        "./chibicc/preprocess.c",
        "./chibicc/strings.c",
        "./chibicc/tokenize.c",
        "./chibicc/type.c",
        "./chibicc/unicode.c",
        "./q3_codegen.c",
        
    );
    knob_config_add_files(&conf,files.items,files.count);
    files.count = 0;

    knob_da_mult_append(&files,
        "./chibicc",
        "."
    );
    knob_config_add_includes(&conf,files.items,files.count);
    files.count = 0;
    knob_config_add_c_flag(&conf,"-std=c11");
    knob_config_add_c_flag(&conf,"-g");
    knob_config_add_c_flag(&conf,"-fno-common");
    knob_config_add_c_flag(&conf,"-Wall");
    knob_config_add_c_flag(&conf,"-Wno-switch");

    conf.build_to = "./build/chibicc";
    knob_config_build(&conf,&files,1);
    knob_cmd_add_compiler(&cmd,&conf);
    for(int i =0; i < files.count; ++i){
        knob_cmd_append(&cmd,files.items[i]);
    }
    knob_cmd_append(&cmd,"-o","./bin/chibicc.com");

    Knob_String_Builder render = {0};
    knob_cmd_render(cmd,&render);
    knob_log(KNOB_INFO,"CMD: %s",render.items);
    if(!knob_cmd_run_sync(cmd)){
        knob_return_defer(false);
    } 
defer:
    knob_cmd_free(cmd);
    return result;
}
int build_q3vm(Knob_File_Paths* end_cmd,Knob_Config* config){

    bool result = true;
    Knob_Cmd cmd = {0};

    if (!knob_mkdir_if_not_exists("./build/q3vm")) {
        knob_return_defer(false);
    }
    
    Knob_Config conf = {0};
    knob_config_init(&conf);
    conf.compiler = config->compiler;
    conf.debug_mode = config->debug_mode;

    Knob_File_Paths files = {0};
    knob_da_mult_append(&files,
        //Providers
        "./q3vm/q3vm.c",
        "./Libraries/q3vm/src/vm/vm.c",
    );
    knob_config_add_files(&conf,files.items,files.count);
    files.count = 0;

    knob_da_mult_append(&files,
        "./q3vm",
        "./Libraries/q3vm/src/vm",
        "."
    );
    knob_config_add_includes(&conf,files.items,files.count);
    files.count = 0;

    knob_config_add_define(&conf,"-DQ3_VM");
    // knob_config_add_define(&conf,"-DDEBUG_VM");
    knob_config_add_define(&conf,"-fPIC");

    conf.build_to = "./build/q3vm";
    knob_config_build(&conf,&files,1);
    for(int i =0; i < files.count; ++i){
        const char* output_path = files.items[i];
        knob_da_append(end_cmd,output_path);
    }
defer:
    knob_cmd_free(cmd);
    return result;
}


MAIN(Q3VM_test){
    // KNOB_GO_REBUILD_URSELF(argc,argv);

    if(!knob_mkdir_if_not_exists("build")){ return 1;}
    if(!knob_mkdir_if_not_exists("Deployment")){ return 1;}

    const char* program = knob_shift_args(&argc,&argv);
    char* subcommand = NULL;
    if(argc <= 0){
        subcommand = "build";
    } else {
        subcommand = (char*)knob_shift_args(&argc,&argv);
    }
    if(knob_cstr_match(subcommand,"clean")){
        knob_clean_dir("."PATH_SEP"build");
        // return 0;
    }

    Knob_Config config = {0};
    knob_config_init(&config);

    Knob_Cmd cmd = {0};
    config.compiler = COMPILER_GCC;

    knob_config_add_define(&config,"-ggdb3");

    if(!build_chibicc(NULL,&config)){
        return 1;
    }

    Knob_File_Paths o_files = {0};
    Knob_Cmd pass_cmds = {0};
    char* path_to_knobh = "..";
    build_q3vm(&o_files,&config);

    Knob_File_Paths files = {0};
    knob_da_mult_append(&files,
        "src/main.c",
    );
    config.build_to = "."PATH_SEP"build";
    knob_config_add_files(&config,files.items,files.count);
    files.count = 0;
    knob_da_mult_append(&files,
        ".",
        "./q3vm",
        "./src",
    );
    knob_config_add_includes(&config,files.items,files.count);

    Knob_File_Paths out_files = {0};
    if(!knob_config_build(&config,&out_files,0))return 1;

    cmd.count = 0;    
    // knob_cmd_append(&cmd,GET_COMPILER_NAME(config.compiler));
    knob_cmd_add_compiler(&cmd,&config);
    // knob_cmd_append(&cmd, "-ggdb3");
    knob_cmd_append(&cmd, "-fdata-sections","-ffunction-sections","-fno-strict-aliasing");
    if(config.compiler == COMPILER_GCC){
        //Apparently not on platforms using clang(e.g. Darwin)
        knob_cmd_append(&cmd, "-fno-crossjumping");
    }
    knob_cmd_append(&cmd, "-Wl,--gc-sections");

    knob_cmd_add_includes(&cmd,&config);
    for(int i =0; i < o_files.count;++i){
        knob_cmd_append(&cmd,o_files.items[i]);
    }
    for(int i =0; i < out_files.count;++i){
        knob_cmd_append(&cmd,out_files.items[i]);
    }

    knob_cmd_append(&cmd,"-o","./Deployment/test.com");
    knob_cmd_append(&cmd,"-lm");
    knob_cmd_append(&cmd,"-lstdc++");

    Knob_String_Builder render = {0};
    knob_cmd_render(cmd,&render);
    knob_log(KNOB_INFO,"CMD: %s",render.items);
    if(!knob_cmd_run_sync(cmd)) return 1;
    //BUILD SCRIPTS
    q3vm_add_scripts_folder("./q3vm/main_script");
    q3vm_add_scripts_folder("./scripts");
    q3vm_add_scripts_folder("./q3vm/scripts");
    if(q3vm_build(".",0) == -1) return 1;
    return 0;
}
#else
MAIN(STAGE_1){
    //@TODO: Add GO_REBUILD_YOURSELF tech TM
    knob_log(KNOB_INFO, "--- STAGE 1 ---");
    char* program = knob_shift_args(&argc,&argv);
    if (!knob_mkdir_if_not_exists("build")) return 1;

    Knob_Cmd cmd = {0};
    int config_exists = knob_file_exists(CONFIG_PATH);
    if (config_exists < 0) return 1;
    if (config_exists == 0) {
        knob_log(KNOB_INFO, "Generating %s", CONFIG_PATH);
        Knob_String_Builder content = {0};
        knob_create_default_config("Q3VM_test",&content,&cmd);
        if (!knob_write_entire_file(CONFIG_PATH, content.items, content.count)) return 1;
    } else {
        knob_log(KNOB_INFO, "file `%s` already exists", CONFIG_PATH);
    }

    cmd.count = 0;
    const char *configured_binary = "./build/knob.configured";
    knob_cmd_append(&cmd, KNOB_REBUILD_URSELF(configured_binary, "knob.c"), "-DCONFIGURED");
    if (!knob_cmd_run_sync(cmd)) return 1;

    cmd.count = 0;
    knob_cmd_append(&cmd, configured_binary);
    knob_da_append_many(&cmd, argv, argc);
    if (!knob_cmd_run_sync(cmd)) return 1;

    return 0;
}
#endif // CONFIGURED
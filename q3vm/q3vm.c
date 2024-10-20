#define KNOB_IMPLEMENTATION
#include "knob.h"

#include "q3vm_build.c"
#include "q3vm_impl.c"

#include "commands.h"

/* The compiled bytecode calls native functions, defined in this file.
 * Read README.md section "How to add a custom native function" for
 * details.
 * @param[in,out] vm Pointer to virtual machine, prepared by VM_Create.
 * @param[in,out] args Array with arguments of function call.
 * @return Return value handed back to virtual machine. */
// intptr_t systemCalls(vm_t* vm, intptr_t* args);

/* Load an image from a file. Data is allocated with malloc.
   Call free() to unload image.
   @param[in] filepath Path to virtual machine binary file.
   @param[out] size File size in bytes is written to this memory location.
   @return Pointer to virtual machine image file (raw bytes). */
uint8_t* loadImage(const char* filepath, int* size);

static vm_t vm = {0};
static int  retVal = -1;
static uint8_t* image = NULL;
static int  imageSize = 0;
const char*    filepath = "bytecode.qvm";


void q3vm_init(void){
    image = loadImage(filepath, &imageSize);
    q3vm_add_scripts_folder("../q3vm/main_script");
    q3vm_add_scripts_folder("../scripts");
    q3vm_add_scripts_folder("../q3vm/scripts");
    if (!image){
        retVal = -1;
    }
    if (VM_Create(&vm, filepath, image, imageSize, systemCalls) == 0){
        VM_Debug(2);
        retVal = VM_Call(&vm, COMM_INIT);
    }
    if(retVal < 0){
        knob_log(KNOB_ERROR,"Failed to initialize the script environnement.");
    }
}
void q3vm_pre_update(void){
    if(q3vm_build("..",1)){
        printf("Let's rebuild !\n");
        static uint8_t* temp_image    = NULL;
        static int  temp_imageSize = 0;
        static vm_t temp_vm = {0};
        temp_image = loadImage(filepath, &temp_imageSize);
        if(VM_Create(&temp_vm, filepath, temp_image, temp_imageSize, systemCalls) == 0){
            VM_Free(&vm);
            free(image); /* we can release the memory now */
            memcpy(&vm,&temp_vm,sizeof(vm_t));
            image = temp_image;
            imageSize = temp_imageSize;
            retVal = VM_Call(&vm, COMM_INIT);
        }
        else {
            printf("Error: When loading file !\n");
        }
        // We could call COMM_ASSET_RELOAD ?
    }
}

void q3vm_registerScript(const char* scriptPath){
    knob_da_append(&q3vm_scripts,scriptPath);// q3vm_scripts is defined in q3vm_build.c
}

/* Callback from the VM that something went wrong
 * @param[in] level Error id, see vmErrorCode_t definition.
 * @param[in] error Human readable error text. */
void Com_Error(vmErrorCode_t level, const char* error)
{
    fprintf(stderr, "Err (%i): %s\n", level, error);
    exit(level);
}

/** Memory allocation for the virtual machine.
 * @param[in] size Number of bytes to allocate.
 * @param[in] vm Pointer to vm requesting the memory.
 * @param[in] type What purpose has the requested memory, see vmMallocType_t.
 * @return pointer to allocated memory. */
void* Com_malloc(size_t size, vm_t* vm, vmMallocType_t type)
{
    (void)vm; /* simple malloc, we don't care about the vm */
    (void)type; /* we don't care what the VM wants to do with the memory */
    return malloc(size); /* just allocate the memory and return it */
}

/** Free memory for the virtual machine.
 * @param[in,out] p Pointer of memory allocated by Com_malloc to be released.
 * @param[in] vm Pointer to vm releasing the memory.
 * @param[in] type What purpose has the memory, see vmMallocType_t. */
void Com_free(void* p, vm_t* vm, vmMallocType_t type)
{
    (void)vm;
    (void)type;
    free(p);
}

uint8_t* loadImage(const char* filepath, int* size)
{
    FILE*    f;            /* bytecode input file */
    uint8_t* image = NULL; /* bytecode buffer */
    int      sz;           /* bytecode file size */

    *size = 0;
    f     = fopen(filepath, "rb");
    if (!f)
    {
        fprintf(stderr, "Failed to open file %s.\n", filepath);
        return NULL;
    }
    /* calculate file size */
    fseek(f, 0L, SEEK_END);
    sz = ftell(f);
    if (sz < 1)
    {
        fclose(f);
        return NULL;
    }
    rewind(f);

    image = (uint8_t*)malloc(sz);
    if (!image)
    {
        fclose(f);
        return NULL;
    }

    if (fread(image, 1, sz, f) != (size_t)sz)
    {
        free(image);
        fclose(f);
        return NULL;
    }

    fclose(f);
    *size = sz;
    return image;
}

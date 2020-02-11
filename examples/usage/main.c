
#define MGLSL_DEBUG
// uncomment to turn off addition of comments at the to of module in final shader
// #define MGLSL_NO_MODULE_HEADER_COMMENT

#include <stdio.h>
// mglsl logs on errors, we can define custom logging function if we wish
#define MGLSL_LOG(STR) puts(STR)

#include "../../mglsl.h"

// This is our program's file structure:
//
// # tree
//.
//├── a.out
//├── main.c
//├── shaders
//│   ├── main.glsl
//│   ├── never_required.glsl
//│   ├── quaternion.glsl
//│   └── structs.glsl
//└── uniforms.glsl
//
//1 directory, 7 files
//
// To compile simply:
// # gcc main.c && ./a.out

#define EASY_WAY 1
int main() {
    int ec;

    mglsl_ModuleArr arr;

#if EASY_WAY
    ec = mglsl_import_module_file_list_from_string
        (&arr, "main.glsl,uniforms.glsl, quaternion.glsl, structs.glsl, never_required.glsl", "shaders");

    if(ec) return -1;

#else
    const char * my_module_filenames[] =
        { "uniforms.glsl", "shaders/structs.glsl", "shaders/quaternion.glsl",
          "shaders/main.glsl", "shaders/never_required.glsl" };

    int module_count = sizeof(my_module_filenames)/sizeof(const char *);
    mglsl_Module modules[module_count];

    for(int i=0; i<module_count; ++i)
        if( ec = mglsl_create_module_from_file(modules + i, my_module_filenames[i]) ) return ec;

    arr.data = modules; arr.size = module_count;

#endif

    char * shader;
    ec = mglsl_assemble_shader(&shader, "main", arr);
    if(ec) return -1;

    puts("Our shader is ready to compile and use: ");
    puts(shader); 

    mglsl_free_shader(shader);

    for(int i=0; i<arr.size; ++i) 
        mglsl_free_module(arr.data + i);

#if EASY_WAY == 1
    mglsl_free_imported_module_arr(arr);
#endif

#if defined(MGLSL_DEBUG)
    printf("DEBUG SANITY CHECK:\nmalloc count = %d, free count = %d\n",
           _mglsl_debug_alloc_count, _mglsl_debug_free_count);
#endif
}

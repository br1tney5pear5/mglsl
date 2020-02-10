#include <stdio.h>
#define MGLSL_DEBUG
#define MGLSL_NO_MODULE_HEADER_COMMENT
#include "../../mglsl.h"

#include <unistd.h> // sleep()

int main() {
    int ec;

    mglsl_ModuleArr arr;

    ec = mglsl_import_module_file_list_from_string (&arr, "edit_me.glsl, maiin.glsl", "");
    if(ec) return -1;

    char * shader;
    ec = mglsl_assemble_shader(&shader, "main", arr);
    if(ec) return -1;

    puts(shader); 

    for(int i=0; i<1; i++) {

        ec = mglsl_file_change_watch(arr);
        if(ec) {
            if(ec == MGLSL_E_FILE_CHANGED) {

                printf("Some module files changed, rebuilding.\n");
                ec = mglsl_swap_dirty_modules(arr);
                if(ec) return ec;

                mglsl_free_shader(shader);

                ec = mglsl_assemble_shader(&shader, "main", arr);
                if(ec) return ec;

                puts(shader);
                fflush(stdout);
            }

        } else {
            printf("Watching for file change... %d\r", i);
            fflush(stdout);
        }
        sleep(1);
    }

    mglsl_free_shader(shader);

    for(int i=0; i<arr.size; ++i) 
        mglsl_free_module(arr.data + i);

    mglsl_free_imported_module_arr(arr);

#if defined(MGLSL_DEBUG)
    printf("DEBUG SANITY CHECK:\nmalloc count = %d, free count = %d\n",
           _mglsl_debug_alloc_count, _mglsl_debug_free_count);
#endif
}

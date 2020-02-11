## ABOUT
Modular GLSL is a single-header extension to OpenGL Shading Language aiming to simplify working with
hefty shaders and reduce code repetition by splitting them into modules. 
In essence it is a GLSL pre-pre-processor. It is written in C to be easily portable to other languages.

All it adds to the GLSL are the following directives:
``` c
#module /* this module name */
// If module is created from file, module directive is not mandatory and
// module name can be infered from file name.

#require /* comma separated list of one or more other modules used by this module */
// There can be many require directives, MGLSL also doesn't mind if requirements repeat.

#type /* module type */
// Refers to shader type (like Vertex, Fragment, Tesselation Control...).
// This feature is currently not used
```
When the module source is parsed added directives are stripped, finally modules are 
assembled into the final shader. Unlike in simple C Preprocessor inclusion, modules are sorted 
into dependency DAG and then concatenated, unused modules are not included. 
This may be beneficial, since AFAIK quality of GLSL compilers implemented in driver
may sometimes be questionable. Any circular or missing dependencies are reported.
MGLSL also provides simple interface to notify user app about changes in module files on disk, 
reload modified modules and reassemble the shader.

**Most importantly, MGLSL is a handy platform to implement automatic C++ to GLSL struct reflection, automatic uniform binding or live shader reloading in rendering applications.**

Previous draft of this library written in C++:  https://www.github.com/kcpikkt/modular-glsl
## DEPENDENCIES
All this library depends on is a small subset of C Standard Library.
## INTERFACE 

Below are listed all user-visible functions.
Each of them returns ```mglsl_ErrorCode``` as int.
If call to a given one was successful, it returns ```MGLSL_E_SUCCESS``` which is defined to be 0.
``` c
//mglsl_create_module
//   Used to create shader modules together with parsing their source.
//
//   module   - Pointer to mglsl_Module structure to which created module shall be written

int mglsl_create_module_from_file
    (mglsl_Module * module, const char * filepath);
//   filepath - Path to shader module source file.

int mglsl_create_module_from_source
    (mglsl_Module * module, const char * src);
//   src      - Source of this shader module.


// All successfuly created modules have to be deleted with following function:

int mglsl_free_module (mglsl_Module * module);

// This function resolve dependencies and assmebles final shader from modules.

int mglsl_assemble_shader
    (char ** bufptr, const char * root_module_name, mglsl_ModuleArr * module_arr);
//   bufptr           - Address to pointer to null-terminated, final shader source
//                      buffer this function allocates and returns.
//   root_module_name - Name of the root module from which to start, 
//                      in most cases it is your glsl main module.
//   module_arr       - Array of modules from which to assemble the shader.

// All Shaders created with above function must be freed with call following function:

int mglsl_free_shader (char * buf);
//   buf              - Assembled shader source buffer returned in bufptr 
//                      by mglsl_assemble_shader.

// mglsl_import_module_file_list

//    module_arr   - Pointer to mglsl_Module arr to which this function returns 
//                   an array of imported modules.
//    search_paths - String with colon-separated paths to search. Empty path is implied.

int mglsl_import_module_file_list_from_array
    (mglsl_ModuleArr * module_arr, mglsl_StringArr arr, const char * search_paths);

//    arr        - mglsl_Array of module file names strings.

int mglsl_import_module_file_list_from_string
    (mglsl_ModuleArr * module_arr, const char * str, const char * search_paths);

//    str        - String with comma-separated list of module file names.

int mglsl_import_module_file_list_from_file
    (mglsl_ModuleArr * module_arr, const char * filename, const char * search_paths);

//    filename   - Path to file which contains comma-separated list of module file names.

// Every array obtained with successful call to any of mglsl_import_module_file_list functon
// ought to be freed with call to:

int mglsl_free_imported_module_arr(mglsl_ModuleArr module_arr);

//   module_arr  - Array obtained by call to mglsl_import_file_list_function.

// Following are implementing mglsl live shader reload features and
// can be disabled with #define MGLSL_NO_FILE_CHANGE_WATCH

int mglsl_file_change_watch (mglsl_ModuleArr modules);
//   This function examines modules given by modules argument and
//   marks as dirty those which changed on disk.

//   modules - Array of modules to examine.

int mglsl_swap_dirty_modules(mglsl_ModuleArr modules);
//   This function examines modules given by modules argument and
//   reloads from disk those which were marked as dirty

//   modules - Array of modules to examine.
```

## EXAMPLE USAGE

Two examples are provided under examples directory in the project repo. One of them is basic usage showcase
and the other implements simple live watcher reloading the shader when any module is edited.

## CUSTOM MEMORY ALLOCATION

Custom allocation methods can be provided, by default library uses libc malloc and friends.
To do that all three `MGLS_ALLOC`, `MGLSL_REALLOC` and `MGLSL_FREE`.
Their signatures have to be identical to corresponding libc methods.
To overwrite them simply define following function-like macros:

``` c
#define MGLSL_ALLOC(SIZE) my_alloc(SIZE)
#define MGLSL_REALLOC(PTR,SIZE) my_realloc(PTR, SIZE)
#define MGLSL_FREE(SIZE) my_free(PTR)
```
## CUSTOM FILE IO

Custom file IO can be provided, otherwise library will just use default libc file interface.
This may be concern since, as of now (Feb 2020), library does account for windows paths and such.
All OS dependant file-related methods this library uses are the following:
``` c
int read_file(void ** bufptr, size_t * bufsizeptr, const char * filepath);
//    Function is supposed to open the file at filepath and write its entire contents,
//    together with null-terminator, to self-allocated buffer and return the latter.

//    bufptr     - Address of the pointer to memory buffer with contents of the file that
//                 this function, if successful, is supposed to return. It is crutial that the buffer
//                 functon is freeable with MGLSL_FREE.
//    bufsizeptr - Address to size_t integer to which buffer size, if function succeds, is returned.
//    filepath   - Path of file to be read.

// If successful, function shall return MGLSL_E_SUCCESS == 0, non-zero mglsl_ErrorCode value //o therwise.

// can be overwritten like that: 
#define MGLSL_READ_FILE(BUFPTR, BUFSIZEPTR, FILEPATH) my_read_file(BUFPTR, BUFSIZEPTR, FILEPATH)


int concat_path (char * buf, size_t buflen, const char * a, const char * b) 

//    bufptr     - Buffer to which concatenated, null-terminated path can be written.
//    buflen     - Length of this buffer.
//    a          - First part of the path.
//    b          - Second part of the path.

// If successful, function shall return MGLSL_E_SUCCESS == 0, nonzero mglsl_ErrorCode value otherwise.

// can be overwritten like that: 
#define MGLSL_CONCAT_PATH(BUF, BUFLEN, A, B) my_concat_path(BUF, BUFLEN, A, B)


int file_mtime (time_t * mtime, const char * filepath) 
//    Primarily this function is used to get last modification time for hoswap purposes but
//    it is also used as check if file exists by mglsl_import_module_list function, when searching
//    among specified search paths.

//    mtime      - If not NULL, it is an address of time_t integer to which to return last modification time.
//    filepath   - Path to the file.

// Thus this function should return MGLSL_E_FILE_NOT_FOUND if file specified by filepath does not exist or
// cannot be opened for reading. If successful, function shall return MGLSL_E_SUCCESS == 0.

// overwriting:
#define MGLSL_FILE_MTIME(TIME_T_PTR, FILEPATH) my_file_exist(TIME_T_PTR, FILEPATH)

```
## LOGGING

If MGLSL encounters an error it can log some helpful info like what happened or
where in source file syntax error occurred. By default it uses stdio for that.

You can provide custom logging method by:
``` c
#define MGLSL_LOG(STR) my_log_function(STR);
```
To disable logging completely, e.g. for release version of your code:
``` c
#define MGLSL_NO_LOGGGING
```
## OTHER OPTIONS
``` c
#define MGLSL_DEBUG 
//    enables debug features:
//      - alloc/free count variables:
//        _mglsl_debug_alloc_count, _mglsl_debug_realloc_count and _mglsl_debug_free_count.
//        Values of _mglsl_debug_alloc_count and _mglsl_debug_free_count should be equal,
//        provided that all successful module create_module calls were followed by successful free_module and
//        These still work if you define custom MGLSL_ALLOC, MGLSL_REALLOC and MGLSL_FREE.
//      - asserts

#define MGLSL_NO_MODULE_HEADER_COMMENT
//    Turns off comments indicating individual modules in final assembled shader

#define MGLSL_SHADER_MAX_NAME_LEN (96 - 1)
//    Set custom maximum module name lenght.

#define MGLSL_SHADER_MAX_PATH_LEN (255 - 1)
//    Set custom maximum file path lenght.
  ```
## LICENSE

>Copyright 2020 Kacper Kokot

>Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

>The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

>THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

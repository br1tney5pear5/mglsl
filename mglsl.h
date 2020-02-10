#ifndef _LIBMGLSL_
#define _LIBMGLSL_

#include <stdio.h>  // sprintf, fprintf
#include <string.h> // strcpy, strlen, unnecessary

//
// SWITCHES

#ifndef MGLSL_NO_MODULE_HEADER_COMMENT
# define _MGLSL_MODULE_HEADER_COMMENT
#endif

#ifndef MGLSL_SHADER_MAX_NAME_LEN
# define MGLSL_MAX_NAME_LEN (96 - 1)
#endif

#ifndef MGLSL_SHADER_MAX_PATH_LEN
# define MGLSL_MAX_PATH_LEN (255 - 1)
#endif


#ifndef MGLSL_NO_LOGGING
# ifndef MGLSL_LOG
#  define MGLSL_LOG(STR) fprintf(stderr, STR)
#  define _MGLSL_DEFAULT_READ_FILE
# endif
#else
# define _MGLSL_NO_LOGGING
#endif

#ifndef MGLSL_READ_FILE
# define MGLSL_READ_FILE(BUFPTR, BUFSIZEPTR, FILEPATH) _mglsl_read_file(BUFPTR, BUFSIZEPTR, FILEPATH)
# define _MGLSL_DEFAULT_READ_FILE
#endif

#ifndef MGLSL_CONCAT_PATH
# define MGLSL_CONCAT_PATH(BUF, BUFLEN, A, B) _mglsl_concat_path(BUF, BUFLEN, A, B)
# define _MGLSL_DEFAULT_CONCAT_PATH
#endif

#ifndef MGLSL_FILE_MTIME

#include <time.h>
#include <sys/stat.h>

# define MGLSL_FILE_MTIME(TIME_T_PTR, FILEPATH) _mglsl_file_mtime(TIME_T_PTR, FILEPATH)
# define _MGLSL_DEFAULT_FILE_MTIME
#endif


// DEFS

// NOTE(kacper): AFAIK some compilers warn about expression that expand to just nothing
#ifndef MGLSL_NOOP
# define MGLSL_NOOP ((void)0)
#endif

#ifdef MGLSL_DEBUG
#include <assert.h>
# define _MGLSL_ASSERT(...) assert(__VA_ARGS__)//if(!(__VA_ARGS__)) *(unsigned char*)0 = 0
#else
# define _MGLSL_ASSERT(...) MGLSL_NOOP
#endif

#ifndef MGLSL_NO_FILE_CHANGE_WATCH
# define _MGLSL_FILE_CHANGE_WATCH
# include <time.h> // time_t
#endif

//
// MEMORY ALLOCATION

#if defined(MGLSL_ALLOC) || defined(MGLSL_REALLOC) || defined(MGLSL_FREE)
# if !defined(MGLSL_ALLOC) || !defined(MGLSL_REALLOC) || !defined(MGLSL_FREE)
#  error mglsl: Either none or all of function-like macros MGLSL_ALLOC, MGLSL_REALLOC and MGLSL_FREE have to be defined.
# endif
#else 
# include <stdlib.h> // TODO(kacper): be sure you dont use anything else from stdlib than alloc stuff
#endif

#ifndef MGLSL_ALLOC
# define MGLSL_ALLOC(SIZE) (malloc(SIZE))
#endif
#ifndef MGLSL_REALLOC
# define MGLSL_REALLOC(PTR, SIZE) (realloc(PTR, SIZE))
#endif
#ifndef MGLSL_FREE
# define MGLSL_FREE(PTR) (free(PTR))
#endif


#if defined(MGLSL_DEBUG)
static size_t _mglsl_debug_alloc_count = 0;
static size_t _mglsl_debug_realloc_count = 0;
static size_t _mglsl_debug_free_count = 0;
#endif

void * _mglsl_alloc (size_t size) {
#if defined(MGLSL_DEBUG)
    _mglsl_debug_alloc_count++;
#endif
    return MGLSL_ALLOC(size);
}

void * _mglsl_realloc (void * ptr, size_t size) {
#if defined(MGLSL_DEBUG)
    _mglsl_debug_realloc_count++;
#endif
    return MGLSL_REALLOC(ptr, size);
}

void _mglsl_free (void * ptr) {
#if defined(MGLSL_DEBUG)
    _mglsl_debug_free_count++;
#endif
    MGLSL_FREE(ptr);
}

//
// ERROR HANDLING

enum mglsl_ErrorCode
{
    MGLSL_E_SUCCESS = 0,
    MGLSL_E_ALLOC,
    MGLSL_E_REALLOC,
    MGLSL_E_SYNTAX,
    MGLSL_E_SEMANTIC,
    MGLSL_E_FILE_OPEN,
    MGLSL_E_FILE_READ,
    MGLSL_E_FILE_NOT_FOUND,
    MGLSL_E_MODULE_CORRUPT,
    MGLSL_E_MODULE_NONAME,
    MGLSL_E_MODULE_NOT_FOUND,
    MGLSL_E_CIRCULAR_DEP,
    MGLSL_E_MISSING_DEP,
    MGLSL_E_BUF_TOO_SMALL,
#ifdef _MGLSL_FILE_CHANGE_WATCH
    MGLSL_E_FILE_CHANGED
#endif

};

struct _mglsl_ErrorDescPair { enum mglsl_ErrorCode code; const char * desc; };
struct _mglsl_ErrorDescPair _mglsl_err_desc_map[] =
{
    {MGLSL_E_SUCCESS, "Success"},
    {MGLSL_E_ALLOC, "Allocation failed"},
    {MGLSL_E_REALLOC, "Reallocation failed"},
    {MGLSL_E_SYNTAX, "Syntax error"},
    {MGLSL_E_SEMANTIC, "Semantic error"},
    {MGLSL_E_FILE_OPEN, "Failed to the open file"},
    {MGLSL_E_FILE_READ, "Failed to the read file"},
    {MGLSL_E_FILE_NOT_FOUND, "File could not be found"},
    {MGLSL_E_MODULE_CORRUPT, "Module is corrupt"},
    {MGLSL_E_MODULE_NONAME, "Module without name"},
    {MGLSL_E_MODULE_NOT_FOUND, "Module is missing"},
    {MGLSL_E_CIRCULAR_DEP, "Cirular dependency found"},
    {MGLSL_E_MISSING_DEP, "Missing required module"},
    {MGLSL_E_BUF_TOO_SMALL, "Provided buffer is too small"},
#ifdef _MGLSL_FILE_CHANGE_WATCH
    {MGLSL_E_FILE_CHANGED,  "File changed on disk"}
#endif
  
};

static const int _mglsl_err_desc_map_len = sizeof(_mglsl_err_desc_map)/sizeof(struct _mglsl_ErrorDescPair);


const char * mglsl_err_desc(int code) {
    for(int i=0; i<_mglsl_err_desc_map_len; i++) {
        if(code == _mglsl_err_desc_map[i].code)
            return _mglsl_err_desc_map[i].desc;
    }
    return "Unknown error.";
}

static int _mglsl_err_code = 0;
static int _mglsl_err_line = 0;
static int _mglsl_err_char_offset = 0;
static const char * _mglsl_err_file = "(unknown)";
static const char * _mglsl_err_msg = "Success.";
static const char * _mglsl_err_sec_msg = "";
static const char * _mglsl_err_level = "error";


char * _mglsl_errmsg() {
    size_t msglen = strlen(_mglsl_err_msg) + strlen(_mglsl_err_sec_msg) + strlen(_mglsl_err_level) + 255; // oof

    char * buf = _mglsl_alloc(msglen);
    _MGLSL_ASSERT(buf);

    const char * fmt = strlen(_mglsl_err_sec_msg) ?  "mglsl: %s: %s, %s." : "mglsl: %s: %s. %s";

    int result = snprintf(buf, msglen, fmt, _mglsl_err_level, _mglsl_err_msg, _mglsl_err_sec_msg);

    _MGLSL_ASSERT(result >= 0);
    
    return buf; // remember to free it
}

char * _mglsl_src_errmsg() {
    size_t msglen =
        strlen(_mglsl_err_file) + strlen(_mglsl_err_msg) +
        strlen(_mglsl_err_sec_msg) + strlen(_mglsl_err_level) + 255; // oof

    char * buf = _mglsl_alloc(msglen);
    _MGLSL_ASSERT(buf);

    const char * fmt = strlen(_mglsl_err_sec_msg) ?  "%s:%d:%d: %s: %s, %s." : "%s:%d:%d: %s: %s. %s";

    int result =
        snprintf(buf, msglen, fmt,
                 _mglsl_err_file, _mglsl_err_line, _mglsl_err_char_offset, _mglsl_err_level, _mglsl_err_msg, _mglsl_err_sec_msg);
    _MGLSL_ASSERT(result >= 0);
    
    return buf; // remember to free it
}


//
// LOGGING 

// library errors
#ifndef _MGLSL_NO_LOGGING
static int _mglsl_log_err(int code) {
    _mglsl_err_msg = mglsl_err_desc(code);

    char * msg = _mglsl_errmsg();
    MGLSL_LOG(msg);

    _MGLSL_ASSERT(msg);
    _mglsl_free(msg);
    return code;
}
#else
# define _mglsl_log_err(CODE) CODE
#endif

// errors in parsed source, displayed with file, line and char offset for fast jumps
#ifndef _MGLSL_NO_LOGGING
static int _mglsl_log_src_err(int line, int char_offset, int code) {
    _mglsl_err_line = line;
    _mglsl_err_char_offset = char_offset;
    _mglsl_err_msg = mglsl_err_desc(code);

    char * msg = _mglsl_src_errmsg();
    MGLSL_LOG(msg);

    _MGLSL_ASSERT(msg);
    _mglsl_free(msg);
    return code;
}
#else
# define _mglsl_log_src_err(LINE, OFFSET, CODE) CODE
#endif


enum mglsl_ShaderType { NONE, VERT, FRAG, GEOM, COMP, TESS_CTRL, TESS_EVAL };

enum mglsl_ModuleFlags {
    // for topo sorting
    MGLSL_PERM_MARK = (1 << 0),
    MGLSL_TEMP_MARK = (1 << 1),

#ifdef _MGLSL_FILE_CHANGE_WATCH
    MGLSL_DIRTY = (1 << 2),
#endif
};

typedef struct {
    size_t _sort_idx;

    char name[MGLSL_MAX_NAME_LEN + 1];
    enum mglsl_ShaderType type;

    char * source;

    char ** deps;
    size_t deps_len;

#ifdef _MGLSL_FILE_CHANGE_WATCH
    char path[MGLSL_MAX_PATH_LEN + 1];
    time_t mtime;
#endif
    unsigned char flags;

} mglsl_Module;

typedef struct {
    mglsl_Module * data;
    size_t size;
} mglsl_ModuleArr;

typedef struct {
    const char ** data;
    size_t size;
} mglsl_StringArr;


//
// INTERFACE
// user-visible functions.

int mglsl_create_module_from_file
    (mglsl_Module * module, const char * filepath);

int mglsl_create_module_from_source
    (mglsl_Module * module, const char * src);

int mglsl_free_module
    (mglsl_Module * module);

//

int mglsl_assemble_shader
    (char ** bufptr, const char * root_module_name, mglsl_ModuleArr module_arr);

int mglsl_free_shader
    (char * buf);

//

int mglsl_import_module_file_list_from_array
    (mglsl_ModuleArr * module_arr, mglsl_StringArr arr, const char * search_paths);

int mglsl_import_module_file_list_from_string
    (mglsl_ModuleArr * module_arr, const char * str, const char * search_paths);

int mglsl_import_module_file_list_from_file
    (mglsl_ModuleArr * module_arr, const char * filename, const char * search_paths);

int mglsl_free_imported_module_arr(mglsl_ModuleArr module_arr);

//
 
#ifdef _MGLSL_FILE_CHANGE_WATCH

int mglsl_file_change_watch
    (mglsl_ModuleArr modules);

int mglsl_swap_dirty_modules
    (mglsl_ModuleArr modules);
#endif
 
//
// PARSING HELPERS

static inline int _mglsl_is_space(char c) {
    return (c==' ' || c=='\t');
}

static inline int _mglsl_is_white(char c) {
    return (_mglsl_is_space(c) || c=='\f' || c=='\n' || c=='\r' || c=='\v');
}

static inline int _mglsl_is_number(char c) {
    return (48 <= c && c < 58);
}

static inline int _mglsl_is_letter(char c) {
    return (65 <= c && c < 91) || (97 <= c && c < 122);
}


// Valid module name can contian letters, numbers, '_' and '-',
// and cannot start with number or '-'.
static inline int _mglsl_is_valid_name(const char * str) {
    int len = strlen(str);
    if(len == 0 || len > MGLSL_MAX_NAME_LEN || _mglsl_is_number(str[0]) || str[0] == '-') return 0;

    for(int i=0; i<len; i++) {
        char c = str[i];
        if(!_mglsl_is_letter(c) && !_mglsl_is_number(c) && c != '_' && c != '-') return 0;
    }
    return 1;
}

//
//

static inline const char * _mglsl_cur_skip_line(const char * cur) {
    // short circuiting here
    for(;;) if(*cur == '\0' || *(cur++) == '\n') break; 
    return cur;
}

static inline const char * _mglsl_cur_skip_white(const char * cur) {
    while(_mglsl_is_white(*cur) && *cur != '\0') cur++;
    return cur;
}

static inline const char * _mglsl_cur_skip_white_and_count_nl(const char * cur, int * nlptr) {
    *nlptr = 0;

    while(_mglsl_is_white(*cur) && *cur != '\0') {
        if(*cur == '\n') (*nlptr)++;
        cur++;
    }

    return cur;
}

static inline const char * _mglsl_cur_skip_nonwhite(const char * cur) {
    while(!_mglsl_is_white(*cur) && *cur != '\0') cur++;
    return cur;
}

static inline const char * _mglsl_cur_skip_space(const char * cur) {
    while(_mglsl_is_space(*cur) && *cur != '\0') cur++;
    return cur;
}

//

// Splits str by character c, returns array of pointers to str.
// Argument str is expected to be comma delimited list of contagious nonwhite strings.
// All whitespace between strings and commas is dropped, strings are terminated
// with '\0' by replacing characters in str.

static int _mglsl_split
    (mglsl_StringArr * arrptr, char * str, char c)
{
    size_t arr_len = 1, arr_idx = 0;
    for(size_t i=0; i<strlen(str); i++) if(str[i] == c) arr_len++;

    const char ** arr = _mglsl_alloc(arr_len * sizeof(char*));

    if(!arr) return _mglsl_log_err(MGLSL_E_ALLOC);

    char * cur = str;
    int line = 1;

    while(cur) {
        // skipping leading whitespace
        int nl;
        char * word = cur = (char*)_mglsl_cur_skip_white_and_count_nl(cur, &nl);
        line += nl;

        // At this point module_name is not yet terminated but
        // unless there is an error it will be.
        _MGLSL_ASSERT(arr_idx < arr_len);
        arr[arr_idx++] = word;

        // Going over non-whitespace module file name
        while(!_mglsl_is_white(*cur) && *cur != c && *cur) cur++;

        // End of string, we are done
        /**/ if(*cur == '\0') break;

        // Comma, proceed to the next name
        else if(*cur ==    c) *(cur++) = '\0';

        // Whatever else, skip it making sure there it is only whitespace 
        else {
            *(cur++) = '\0';
            while(*cur != c && *cur) {
                if(!_mglsl_is_white(*cur)) {

                    _mglsl_err_line = line;
                    return MGLSL_E_SYNTAX;
                }
                cur++;
            }

            if(*cur) cur++;
            else break;
        }
    }
    _MGLSL_ASSERT(arr_idx == arr_len);

    arrptr->data = arr;
    arrptr->size = arr_len;
    return MGLSL_E_SUCCESS;
}

//
// MODULE UTIL

static int _mglsl_find_module(size_t * idxptr, const char * module_name, mglsl_ModuleArr module_arr)
{
    _MGLSL_ASSERT(module_name);
    if(!module_arr.data) return MGLSL_E_MODULE_NOT_FOUND;

    for(size_t i=0; i<module_arr.size; i++) {
        if( !strcmp(module_name, module_arr.data[i].name) ) {
            if(idxptr) *idxptr = i;
            return MGLSL_E_SUCCESS;
        }
    }

    return MGLSL_E_MODULE_NOT_FOUND;
}

//
// INDIVIDUAL KEYWORD PARSERS

#define _MGLSL_PROC_SIGNATURE(NAME, ...) int NAME(__VA_ARGS__)
#define _MGLSL_PARSE_PP_DIRECTIVE_PROC_SIGNATURE(NAME)                        \
    _MGLSL_PROC_SIGNATURE(NAME, mglsl_Module * module, char * args)

typedef _MGLSL_PARSE_PP_DIRECTIVE_PROC_SIGNATURE(_mglsl_ParsePPDirectiveProc);

//
//

_MGLSL_PARSE_PP_DIRECTIVE_PROC_SIGNATURE(_mglsl_parse_ppdir_module) {
    if(!args) {
        _mglsl_err_sec_msg = "'module' directive without module name argument";
        return MGLSL_E_SYNTAX;
    }

    if(strlen(module->name)) {
        _mglsl_err_sec_msg = "redefined module name";
        return MGLSL_E_SEMANTIC;
    }

    if(!_mglsl_is_valid_name(args)) {
        _mglsl_err_sec_msg = "invalid module name";
        return MGLSL_E_SYNTAX;
    }


    strcpy(module->name, args);
    return MGLSL_E_SUCCESS;
}

//
//

_MGLSL_PARSE_PP_DIRECTIVE_PROC_SIGNATURE(_mglsl_parse_ppdir_require) {
    if(!args) {
        _mglsl_err_sec_msg = "'require' directive without any arguments";
        return MGLSL_E_SYNTAX;
    }

    char * cur = args;
    size_t strbuf_overalloc = 64;

    int deps_count = 1;
    while(*cur != '\0') if(*(cur++) == ',') deps_count++;

    size_t args_len = strlen(args);

    char * strbuf, * strbuf_cur;
    size_t strbuf_len, strbuf_old_len, strbuf_left;

    if(module->deps_len != 0) {
        if(module->deps == NULL) return MGLSL_E_MODULE_CORRUPT;

        module->deps = _mglsl_realloc(module->deps, sizeof(char*) * (module->deps_len + deps_count));
        if(module->deps == NULL) return MGLSL_E_REALLOC;

        strbuf = module->deps[0];

        char * last_dep = module->deps[module->deps_len - 1];
        strbuf_old_len = (last_dep + strlen(last_dep) + 1) - strbuf;
        strbuf_len = strbuf_old_len + args_len + 2 + strbuf_overalloc;
        strbuf_left = strbuf_len - strbuf_old_len;

        if(module->deps[0] == NULL) return MGLSL_E_MODULE_CORRUPT;

        char * old_anchor = module->deps[0];
        char * new_anchor = _mglsl_realloc(module->deps[0], strbuf_len);
        if(module->deps == NULL) return MGLSL_E_REALLOC;

        if(new_anchor != old_anchor)// repoint pointers
            for(int i=0; i<module->deps_len; i++)
                module->deps[i] = new_anchor + (module->deps[i] - old_anchor);

        strbuf = module->deps[0];
        strbuf_cur = strbuf + strbuf_old_len;

    } else {
        if(module->deps != NULL) return MGLSL_E_MODULE_CORRUPT;

        module->deps = _mglsl_alloc(sizeof(char*) * deps_count);
        if(module->deps == NULL) return MGLSL_E_ALLOC;

        strbuf_old_len = 0;
        strbuf_len = strbuf_left = args_len + 2 + strbuf_overalloc;

        module->deps[0] = _mglsl_alloc(strbuf_len);
        if(module->deps == NULL) return MGLSL_E_ALLOC;

        strbuf = strbuf_cur = module->deps[0];
    }

    size_t dep_index = 0, dep_repeat = 0;

    for(cur = args;;) {
        char * arg = cur = (char*)_mglsl_cur_skip_white(cur);

        while( !_mglsl_is_white(*cur) && *cur != ',' && *cur != '\0') cur++;
        size_t arg_len = cur - arg;

        if(arg_len == 0) {
            _mglsl_err_sec_msg = "'require' directive argument cannot be empty";
            return MGLSL_E_SYNTAX;
        }

        _MGLSL_ASSERT(strbuf_left > (arg_len + 1));
        strbuf_left -= (arg_len + 1);

        char * dep = strbuf_cur;

        int already = 0;
        for(int i=0; i<module->deps_len; i++)
            if(already = !strncmp(arg, module->deps[i], arg_len)) break;

        if(!already) {
            memcpy(strbuf_cur, arg, arg_len);
            strbuf_cur[arg_len] = '\0';
            strbuf_cur += arg_len + 1;

            module->deps[module->deps_len + dep_index++] = dep;

            if(!_mglsl_is_valid_name(dep)) {
                _mglsl_err_sec_msg = "one of 'require' directive arguments is not valid module name";
                return MGLSL_E_SYNTAX;
            }
        } else {
            dep_repeat++;
            //TODO(kacper): add warning that module requirement is repeated
        }

        while(*cur != ',' && *cur != '\0') {
            if(!_mglsl_is_white(*cur)) {
                _mglsl_err_sec_msg = "one of 'require' directive arguments is not valid module name";
                return MGLSL_E_SYNTAX;
            }
            cur++;
        }
        if(*cur == '\0') break;
        else cur++;
    }

    _MGLSL_ASSERT(module->deps[0] == strbuf);
    _MGLSL_ASSERT(deps_count == dep_index + dep_repeat);
    module->deps_len += dep_index;

    return 0;
}

//
//

_MGLSL_PARSE_PP_DIRECTIVE_PROC_SIGNATURE(_mglsl_parse_ppdir_type) { return 0; }

struct _mglsl_KeywordProcPair { const char * keyword; _mglsl_ParsePPDirectiveProc * proc; };

#define _MGLSL_MAX_PP_KEYWORD_LEN 32
static struct _mglsl_KeywordProcPair _mglsl_keyword_proc_map[] =
    {
     {"module", _mglsl_parse_ppdir_module},
     {"require", _mglsl_parse_ppdir_require},
     {"type", _mglsl_parse_ppdir_type}
    };

static const int _mglsl_keyword_proc_map_len = sizeof(_mglsl_keyword_proc_map)/sizeof(struct _mglsl_KeywordProcPair);

//
//

static int _mglsl_parse(mglsl_Module * module, const char * src)
{
    size_t src_len = strlen(src);
    char * clean_src = _mglsl_alloc(src_len + 1);

    if(!clean_src)
        return _mglsl_log_err(MGLSL_E_ALLOC);

    size_t clean_src_len = 0;

    const char * cur = src;

    int line = 0;
    int _char_offset = 0;
    while(*cur != '\0') {
        const char * line_begin = cur;
        line++;
        cur = _mglsl_cur_skip_space(cur);

        if (*cur == '#') {
            cur++;
            cur = _mglsl_cur_skip_space(cur);

            char * keyword_end = (char*)_mglsl_cur_skip_nonwhite(cur);
            size_t keyword_len = keyword_end - cur;

            char keyword[_MGLSL_MAX_PP_KEYWORD_LEN + 1];

            if(keyword_len > _MGLSL_MAX_PP_KEYWORD_LEN) {
                _mglsl_err_line = line;
                _mglsl_err_char_offset = cur - line_begin;

                _MGLSL_ASSERT(clean_src); _mglsl_free(clean_src);
                return _mglsl_log_err(MGLSL_E_SYNTAX);
            }

            memcpy(keyword, cur, keyword_len);
            keyword[keyword_len] = '\0';

            for(int i=0; i<_mglsl_keyword_proc_map_len; i++) {

                if( strcmp(keyword, _mglsl_keyword_proc_map[i].keyword) ) continue;

                char * args_buf = NULL;
                char * args_begin = (char*)_mglsl_cur_skip_space(keyword_end);
                size_t args_buflen = _mglsl_cur_skip_line(args_begin) - args_begin;

                if(*args_begin != '\0' && args_buflen > 1) {

                    // \n at the end accounts for null terminator
                    args_buf = (char*)_mglsl_alloc(args_buflen); 

                    if(!args_buf) {
                        _mglsl_free(clean_src);
                        return _mglsl_log_src_err(line, cur - line_begin, MGLSL_E_ALLOC);
                    }

                    memcpy(args_buf, args_begin, args_buflen - 1);
                    args_buf[args_buflen - 1] = '\0';
                }

                int ec = _mglsl_keyword_proc_map[i].proc(module, args_buf);

                if(args_buf) _mglsl_free(args_buf);

                if(ec) {
                    _mglsl_free(clean_src);
                    return _mglsl_log_src_err(line, args_begin - line_begin, ec);
                }

                break;
            }
        } else {
            // everything we don't process is written back 
            size_t line_len = _mglsl_cur_skip_line(cur) - line_begin;
            memcpy(clean_src + clean_src_len, line_begin, line_len);
            clean_src_len += line_len;
        }

        cur = _mglsl_cur_skip_line(cur);
    }
    _MGLSL_ASSERT(clean_src); 

    module->source = clean_src = _mglsl_realloc(clean_src, clean_src_len + 1);
    clean_src[clean_src_len] = '\0';

    return MGLSL_E_SUCCESS;
}

//
// FILE INTERFACE

#ifdef _MGLSL_DEFAULT_READ_FILE
static int _mglsl_read_file(void ** bufptr, size_t * size, const char * filepath) {
    FILE * file = fopen(filepath, "r");

    if(!file)
        return MGLSL_E_FILE_OPEN;

    fseek(file, 0L, SEEK_END);
    size_t filesize = ftell(file);
    rewind(file);

    char * buf = _mglsl_alloc(filesize + 1);

    if(filesize != fread(buf, sizeof(char), filesize, file)) {
        fclose(file);
        return MGLSL_E_FILE_READ;
    }

    buf[filesize] = '\0';

    fclose(file);

    *bufptr = buf;
    *size = filesize;
    return MGLSL_E_SUCCESS;
}
#endif


#ifdef _MGLSL_DEFAULT_CONCAT_PATH
static int _mglsl_concat_path
    (char * buf, size_t buflen, const char * a, const char * b) 
{
    _MGLSL_ASSERT(a && b);

    size_t alen = strlen(a), blen = strlen(b);

    // If any of our two strings have no length, we dont need slash.
    // If one of them have it, we dont need it either.
    int slash_between =
        (alen && a[alen - 1] != '/') && (blen && b[blen - 1] != '/');

    size_t len = alen + blen + slash_between + 1;

    if(buflen < len)
        return MGLSL_E_BUF_TOO_SMALL;

    // NOTE(kacper): Calling memcpy with count zero is fine btw.
    char * cur = buf;

    memcpy(cur, a, alen);
    cur += alen;

    if(slash_between) *(cur++) = '/';

    memcpy(cur, b, blen);
    cur += blen;

    *(cur++) = '\0';

    _MGLSL_ASSERT(cur - buf == len);

    return MGLSL_E_SUCCESS;
}
#endif

#ifdef _MGLSL_DEFAULT_FILE_MTIME
static int _mglsl_file_mtime(time_t * mtime, const char * filepath) {
    struct stat attr;
    memset(&attr, 0, sizeof(attr));
    if( stat(filepath, &attr) ) return MGLSL_E_FILE_NOT_FOUND;

    if(mtime) *mtime = attr.st_mtime;

    return MGLSL_E_SUCCESS;
}
#endif

//
// FILE CHANGE WATCH



//
// TOPOLOGICAL SORT

static int _mglsl_toposort_rec_visit
    (size_t depth, mglsl_Module * module, mglsl_ModuleArr module_arr)
{
    _MGLSL_ASSERT(module);
    _MGLSL_ASSERT(module_arr.data);

    if(module->flags & MGLSL_TEMP_MARK) {
        _mglsl_err_sec_msg = module->name;
        return MGLSL_E_CIRCULAR_DEP;
    }

    module->flags |= MGLSL_TEMP_MARK;

    int ec; size_t module_idx;

    for(size_t i=0; i<module->deps_len; i++) {
        int ec = _mglsl_find_module(&module_idx, module->deps[i], module_arr);
        if(ec) {
            _mglsl_err_sec_msg = module->deps[i];
            return MGLSL_E_MISSING_DEP;
        }

        _MGLSL_ASSERT(module_idx < module_arr.size);

        mglsl_Module * dep = module_arr.data + module_idx;

        ec = _mglsl_toposort_rec_visit(depth + 1, dep, module_arr);
        if(ec) return MGLSL_E_CIRCULAR_DEP;

    }

    module->flags &= ~MGLSL_TEMP_MARK;
    module->flags |= MGLSL_PERM_MARK;

    module->_sort_idx = depth;

    return MGLSL_E_SUCCESS;
}

static int _mglsl_toposort_modules
    (mglsl_Module * root_module, mglsl_ModuleArr module_arr)
{
    for(size_t i=0; i<module_arr.size; i++)
        module_arr.data[i].flags &= ~(MGLSL_PERM_MARK | MGLSL_TEMP_MARK);

    return _mglsl_toposort_rec_visit(0, root_module, module_arr);
}


//
// LIBRARY INTERFACE IMPLEMENTATION

static int _mglsl_create_module
    (mglsl_Module * module, const char * src)
{
    int ec;
    _MGLSL_ASSERT(module);

    memset(module, 0, sizeof(mglsl_Module));

    module->name[0] = '\0';
    ec = _mglsl_parse(module, src);
    if(ec != MGLSL_E_SUCCESS) return ec;

    return MGLSL_E_SUCCESS;
}

//
//

int mglsl_create_module_from_source(mglsl_Module * module, const char * src)
{
    _mglsl_err_file = "(memory)";
    int ret = _mglsl_create_module(module, src);

    if(!strlen(module->name)) {
        _mglsl_err_sec_msg = "could not infer the name and thesource does not contain valid 'module' directive";
        return MGLSL_E_MODULE_NONAME;
    }

    return ret;
}

//
//

int mglsl_create_module_from_file(mglsl_Module * module, const char * filepath)
{
    void * filebuf; size_t filesize; int ec;

    _mglsl_err_file = filepath;

    ec = MGLSL_READ_FILE(&filebuf, &filesize, filepath);
    if(ec != MGLSL_E_SUCCESS) {
        _mglsl_err_file = filepath;
        return _mglsl_log_err(ec);
    }

    _MGLSL_ASSERT(strlen(filebuf) == filesize);

    _mglsl_err_file = filepath;

    int ret = _mglsl_create_module(module, filebuf);


    if(!strlen(module->name)) {
        size_t filepath_len = strlen(filepath);
        int basename_idx = 0, ext_idx = 0;

        for(int i=0; i<filepath_len; i++) {
            if(filepath[i] == '/') basename_idx = ext_idx = i + 1;
            if(filepath[i] == '.') ext_idx = i;
        }
        if(basename_idx == ext_idx) ext_idx = filepath_len - 1;
        int basename_len = ext_idx - basename_idx; 

        char basename_buf[MGLSL_MAX_NAME_LEN + 1];
        memcpy(basename_buf, filepath + basename_idx, basename_len);
        basename_buf[basename_len] = '\0';

        if(basename_len) {
            strcpy(module->name, basename_buf);
        } else {
            _mglsl_err_sec_msg = "could not infer the name and module source does not contain valid 'module' directive";
            return _mglsl_log_err(MGLSL_E_MODULE_NONAME);
        }
    }

#ifdef _MGLSL_FILE_CHANGE_WATCH
    ec = MGLSL_FILE_MTIME(&module->mtime, filepath);
    _MGLSL_ASSERT(!ec); // Opened this file moment ago, don't fail please

    size_t filepath_len = strlen(filepath);
    _MGLSL_ASSERT(filepath_len < MGLSL_MAX_PATH_LEN);

    memcpy(module->path, filepath, filepath_len);
    module->path[filepath_len] = '\0';
#endif

    _mglsl_free(filebuf);
    return ret;
}

//
//

int mglsl_free_module(mglsl_Module * module) {
 
    if(module->deps_len) {
        _MGLSL_ASSERT(module->deps != NULL && module->deps[0] != NULL);

// NOTE(kacper): Assumption is made that &(deps[0]) is the address with which memory region of this array can be freed.    
//               As well as that &(deps[0][0]) is the address to contagious memory region
//               to which all pointers in this array point, and can be freed with it.
        _mglsl_free(module->deps[0]);
        _mglsl_free(module->deps);
    }

    if(module->source) {
        _mglsl_free(module->source);
    }

    return MGLSL_E_SUCCESS;
}

int mglsl_assemble_shader
    (char ** bufptr, const char * root_module_name, mglsl_ModuleArr module_arr)
{
    _MGLSL_ASSERT(bufptr);
    int ec;

    size_t root_idx;
    ec = _mglsl_find_module(&root_idx, root_module_name, module_arr);
    if(ec) {
        _mglsl_err_sec_msg = root_module_name;
        return _mglsl_log_err(ec);
    }

    ec = _mglsl_toposort_modules(module_arr.data + root_idx, module_arr); 
    if(ec) return _mglsl_log_err(ec);

    size_t max_sort_idx = 0, bufsize = 0, module_count = 0;

    for(size_t i=0; i<module_arr.size; i++) {
        if(module_arr.data[i].flags & MGLSL_PERM_MARK) {
            _MGLSL_ASSERT(module_arr.data[i].source);

            module_count++;
            bufsize += strlen(module_arr.data[i].source);
            if(module_arr.data[i]._sort_idx > max_sort_idx)
                max_sort_idx = module_arr.data[i]._sort_idx;
        }
    }

    for(size_t i=0; i<module_arr.size; i++)
        if(module_arr.data[i].flags & MGLSL_PERM_MARK)
            module_arr.data[i]._sort_idx = max_sort_idx - module_arr.data[i]._sort_idx;

#ifdef _MGLSL_MODULE_HEADER_COMMENT
    const char * comment_header_fmt = "\n// ==== %s module ====\n";
    size_t comment_header_max_len = 127;
    char comment_header_buf[comment_header_max_len + 1];
    bufsize += comment_header_max_len * module_count; 
#endif

    char * buf = _mglsl_alloc(bufsize + 1);
    if(!buf) return _mglsl_log_err(MGLSL_E_ALLOC);

    size_t buflen = 0;

    for(size_t d=0; d < max_sort_idx + 1; d++) {
        for(size_t i=0; i<module_arr.size; i++) {
            mglsl_Module * module = module_arr.data + i;

            if(module->flags & MGLSL_PERM_MARK && module->_sort_idx == d) { 

#           ifdef _MGLSL_MODULE_HEADER_COMMENT
                int comment_header_len =
                    snprintf(comment_header_buf, comment_header_max_len, comment_header_fmt, module->name);

                _MGLSL_ASSERT(comment_header_len > 0);

                _MGLSL_ASSERT(bufsize - buflen >= comment_header_len);
                memcpy(buf + buflen, comment_header_buf, comment_header_len);
                buflen += comment_header_len;
#           endif

                size_t src_len = strlen(module->source);

                _MGLSL_ASSERT(bufsize - buflen >= src_len);
                memcpy(buf + buflen, module->source, src_len);
                buflen += src_len;
            }
        }
    }

    buf[buflen] = '\0';
    buf = _mglsl_realloc(buf, buflen + 1);
    if(!buf) return _mglsl_log_err(MGLSL_E_REALLOC);

    *bufptr = buf;
    return MGLSL_E_SUCCESS;
}

//
//

int mglsl_free_shader(char * buf) {
    _MGLSL_ASSERT(buf);
    _mglsl_free(buf);
    return MGLSL_E_SUCCESS;
}

//
//
// All import_module functions end up calling this one

static int _mglsl_import_module_file_list_from_array
    (mglsl_ModuleArr * module_arr, mglsl_StringArr modules, mglsl_StringArr search_paths)
{
    int ec;
    char pathbuf[MGLSL_MAX_PATH_LEN + 1];
    void * filebuf; size_t bufsize;

    module_arr->data = _mglsl_alloc(modules.size * sizeof(mglsl_Module));
    if(!module_arr->data)
        return _mglsl_log_err(MGLSL_E_ALLOC);

    module_arr->size = modules.size;
    size_t idx = 0;

    for(size_t m_idx=0; m_idx < modules.size; ++m_idx) {
        int found = 0;
        _mglsl_err_sec_msg = modules.data[m_idx];

        for(int p_idx=-1; p_idx < (int)search_paths.size; ++p_idx) {
            const char * dir = p_idx < 0 ? "" : search_paths.data[p_idx];

            ec = MGLSL_CONCAT_PATH(pathbuf, MGLSL_MAX_PATH_LEN + 1,
                                   dir, modules.data[m_idx]);
            if(ec) {
                _mglsl_free(module_arr->data);
                return ec;
            }

            ec = MGLSL_FILE_MTIME(NULL, pathbuf);

            if(ec == MGLSL_E_SUCCESS) {

                ec = mglsl_create_module_from_file(module_arr->data + (idx++), pathbuf);

                if(ec) {
                    _mglsl_free(module_arr->data);
                    return ec;
                } else {
                    found = 1; break; }
                
            } else if(ec == MGLSL_E_FILE_NOT_FOUND) {
                continue;

            } else {
                _mglsl_free(module_arr->data);
                return _mglsl_log_err(ec);
            }
        }

        if(!found) {
            _mglsl_free(module_arr->data);
            return _mglsl_log_err(MGLSL_E_FILE_NOT_FOUND);
        }
    }

    return MGLSL_E_SUCCESS;
}

static int _mglsl_import_module_file_list_from_string
    (mglsl_ModuleArr * module_arr, const char * str, const char * search_paths)
{
    _MGLSL_ASSERT(str);
    _mglsl_err_file = "(memory)";

    size_t str_len = strlen(str);
    size_t sp_len = strlen(search_paths);

    if(str_len == 0) return MGLSL_E_SUCCESS;

    // joint search_paths and str buffer, less freeing, less fragmentation
    char * buf = _mglsl_alloc(str_len + sp_len + 2);
    if(!buf) return _mglsl_log_err(MGLSL_E_ALLOC);

    char * sp_buf = buf + str_len + 1;

    memcpy(buf, str, str_len);
    buf[str_len] = '\0';

    memcpy(sp_buf, search_paths, sp_len);
    buf[str_len + sp_len + 1] = '\0';

    int ec;

    mglsl_StringArr arr, sp_arr;

    ec = _mglsl_split(&arr, buf, ',');
    if(ec) {
        _MGLSL_ASSERT(buf); _mglsl_free(buf);
        return _mglsl_log_err(ec);
    }

    ec = _mglsl_split(&sp_arr, sp_buf, ':');
    if(ec) {
        _MGLSL_ASSERT(buf); _mglsl_free(arr.data);
        _MGLSL_ASSERT(buf); _mglsl_free(buf);
        return _mglsl_log_err(ec);
    }


    ec = _mglsl_import_module_file_list_from_array(module_arr, arr, sp_arr);

    _MGLSL_ASSERT(sp_arr.data); _mglsl_free(sp_arr.data);
    _MGLSL_ASSERT(arr.data); _mglsl_free(arr.data);

    _MGLSL_ASSERT(buf); _mglsl_free(buf);
    return ec;
}

//
//

int mglsl_import_module_file_list_from_array
    (mglsl_ModuleArr * module_arr, mglsl_StringArr arr, const char * search_paths)
{
    _MGLSL_ASSERT(arr.data);
    _mglsl_err_file = "(memory)";

    if(arr.size == 0) return MGLSL_E_SUCCESS;

    size_t sp_len = strlen(search_paths);

    char * sp_buf = _mglsl_alloc(sp_len + 1);
    if(!sp_buf) return _mglsl_log_err(MGLSL_E_ALLOC);

    memcpy(sp_buf, search_paths, sp_len);
    sp_buf[sp_len] = '\0';

    int ec;
    mglsl_StringArr sp_arr;

    ec = _mglsl_split(&sp_arr, sp_buf, ':');
    if(ec) {
        _MGLSL_ASSERT(sp_buf); _mglsl_free(sp_buf);
        return _mglsl_log_err(ec);
    }
 
    ec = _mglsl_import_module_file_list_from_array(module_arr, arr, sp_arr);

    _MGLSL_ASSERT(sp_buf); _mglsl_free(sp_buf);
    return ec;
}

int mglsl_import_module_file_list_from_string
    (mglsl_ModuleArr * module_arr, const char * str, const char * search_paths)
{
    _MGLSL_ASSERT(str);
    _mglsl_err_file = "(memory)";
    return _mglsl_import_module_file_list_from_string(module_arr, str, search_paths);
}

int mglsl_import_module_file_list_from_file
    (mglsl_ModuleArr * module_arr, const char * filepath, const char * search_paths)
{
    _MGLSL_ASSERT(filepath);
    _mglsl_err_file = filepath;

     void * filebuf; size_t filesize; int ec;

    _mglsl_err_file = filepath;

    ec = MGLSL_READ_FILE(&filebuf, &filesize, filepath);
    if(ec != MGLSL_E_SUCCESS) {
        _mglsl_err_file = filepath;
        return _mglsl_log_err(ec);
    }

    _MGLSL_ASSERT(strlen(filebuf) == filesize);

    ec = _mglsl_import_module_file_list_from_string(module_arr, filebuf, search_paths);

    _MGLSL_ASSERT(filebuf); _mglsl_free(filebuf);
    return ec;
}

int mglsl_free_imported_module_arr(mglsl_ModuleArr module_arr) {

    _MGLSL_ASSERT(module_arr.data);
    _mglsl_free(module_arr.data);
    module_arr.data = NULL;
    return MGLSL_E_SUCCESS;
}
 
#ifdef _MGLSL_FILE_CHANGE_WATCH

int mglsl_file_change_watch(mglsl_ModuleArr modules) {
    for(size_t i=0; i<modules.size; ++i){
        mglsl_Module * module = modules.data + i;
    }

    int anydirty = 0, ec;

    for(size_t i=0; i<modules.size; ++i){
        mglsl_Module * module = modules.data + i;

        if(module->path[0]) {
            time_t new_mtime;
            ec = MGLSL_FILE_MTIME(&new_mtime, module->path);
            if(ec) return _mglsl_log_err(ec);

            if(module->mtime != new_mtime) {
                module->mtime = new_mtime;
                module->flags |= MGLSL_DIRTY;
                anydirty = 1;
            }
        }
    }

    return anydirty ? MGLSL_E_FILE_CHANGED : MGLSL_E_SUCCESS;
}

int mglsl_swap_dirty_modules(mglsl_ModuleArr modules) {
    mglsl_Module new_module; int ec;

    for(size_t i=0; i<modules.size; ++i){
        mglsl_Module * module = modules.data + i;

        if(module->flags & MGLSL_DIRTY) {

            _MGLSL_ASSERT(module->path[0]);
            ec = mglsl_create_module_from_file(&new_module, module->path);
            if(ec) return _mglsl_log_err(ec);

            _MGLSL_ASSERT(!mglsl_free_module(module));

            memcpy(module, &new_module, sizeof(mglsl_Module));
        }
    }
    return MGLSL_E_SUCCESS;
}
#endif


#endif // _LIBMGLSL_

#ifndef _LISP_H_
#define _LISP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef BUILDING_CF_LIB
        #define CF_API __declspec(dllexport)
    #else
        #define CF_API __declspec(dllimport)
    #endif
#else
    #if __GNUC__ >= 4
        #define CF_API __attribute__((visibility("default")))
    #else
        #define CF_API
    #endif
#endif

typedef uintptr_t value_t;
typedef int number_t;

// lisp_read.c
CF_API value_t cf_read_file(const char* name);

// lisp_print.c
CF_API void cf_print(value_t v);
CF_API void cf_println(value_t v);
CF_API void cf_mprint(value_t v);

// lisp_eval.c
CF_API value_t cf_eval_toplevel(value_t l);
CF_API number_t cf_num_val(value_t x);

// lisp_core.c
CF_API void cf_lisp_init(void);
CF_API bool cf_isNIL(value_t v);

// lisp_repl.c
CF_API void cf_lisp_repl(void);

#ifdef __cplusplus
}
#endif

#endif /* _LISP_H_ */

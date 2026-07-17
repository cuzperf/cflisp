#include "lisp.h"
#include <stdio.h>
#include <string.h>

// LCOV_EXCL_START

static const char* g_dump_file = NULL;

int main(int argc, char* argv[])
{
    cf_lisp_init();

    bool no_image   = false;
    bool has_script = false;
    const char* script_file = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--no-image") == 0) {
            no_image = true;
        } else if (strcmp(argv[i], "--dump") == 0) {
            if (i + 1 < argc) {
                g_dump_file = argv[++i];
            } else {
                fprintf(stderr, "Error: --dump requires a filename\n");
                return 1;
            }
        } else if (argv[i][0] != '-') {
            script_file = argv[i];
            has_script = true;
        }
    }

    if (!no_image && cf_lisp_load_image("cflisp.img")) {
        /* image loaded successfully, skip system.lsp */
    } else {
        if (!no_image)
            printf("--- image not found or invalid, loading system.lsp ---\n");
        value_t sexp = cf_read_file("system.lsp");
        cf_eval_toplevel(sexp);
    }

    if (has_script) {
        value_t user_sexpr = cf_read_file(script_file);
        cf_eval_toplevel(user_sexpr);
    } else if (!g_dump_file) {
        cf_lisp_repl();
    }

    if (g_dump_file) {
        cf_lisp_dump(g_dump_file);
    }

    return 0;
}

// LCOV_EXCL_STOP

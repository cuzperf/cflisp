#include "lisp.h"

#include <string.h>

// LCOV_EXCL_START

#ifndef DEFAULT_IMAGE
#define DEFAULT_IMAGE "system.img"
#endif

int main(int argc, char* argv[])
{
    cf_lisp_init();

    const char* image = DEFAULT_IMAGE;
    bool load_image = true;     // 默认启动时加载镜像
    bool save_image = false;    // 默认执行完不生成镜像
    const char* script = NULL;  // 用户脚本（可选）

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--no-load") == 0) {
            load_image = false;
        } else if (strcmp(argv[i], "--save") == 0) {
            save_image = true;
        } else if (strcmp(argv[i], "--image") == 0 && i + 1 < argc) {
            image = argv[++i];
        } else if (strncmp(argv[i], "--image=", 8) == 0) {
            image = argv[i] + 8;
        } else if (script == NULL) {
            script = argv[i];
        }
    }

    bool loaded = false;
    if (load_image) {
        loaded = cf_load_image(image);
    }

    if (!loaded) {
        // NOTE: 镜像未找到或加载失败，回退到加载 system.lsp [陈智鹏@2026-7-17]
        //       工作路径必须是 system.lsp 所在路径
        value_t sexp = cf_read_file("system.lsp");
        cf_eval_toplevel(sexp);
    }

    if (script != NULL) {
        value_t user_sexpr = cf_read_file(script);
        cf_eval_toplevel(user_sexpr);
        if (save_image) {
            cf_save_image(image);
        }
    } else {
        if (save_image) {
            // 交互模式下在进 REPL 前落盘，避免进程退出时丢失
            cf_save_image(image);
        }
        cf_lisp_repl();
    }
    return 0;
}

// LCOV_EXCL_STOP

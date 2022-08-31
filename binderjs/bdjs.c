/*
BSD 3-Clause License

Copyright (c) 2022, DiShi Xu
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>

#include <quickjs.h>
#include <quickjs-libc.h>
#include <cutils.h>

#include <binderjs/binderjs.h>

/* extern */
JSValue JSX_Binder_EvalModuleFunction(
    JSContext *ctx, JSValue module, 
    const char *func_name, int argc, JSValueConst *argv);

static JSContext *binderjs_new_context(JSRuntime *rt)
{
    JSContext *ctx;
    ctx = JS_NewContext(rt);
    if (!ctx)
        return NULL;

    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");
    js_init_binderjs(ctx);
    return ctx;
}

static int binderjs_eval_buf(JSContext *ctx, const void *buf, int buf_len,
                    const char *filename)
{
    JSValue val, retval;
    int ret;

    val = JS_Eval(ctx, buf, buf_len, filename,
                    JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    
    if (!JS_IsException(val)) {
        js_module_set_import_meta(ctx, val, TRUE, TRUE);
        /*
        val = JS_DupValue(ctx, val);
        retval = JS_EvalFunction(ctx, val);
        */
        
        retval = JSX_Binder_EvalModuleFunction(
            ctx, val,
            "entry", 0, NULL
        );
    }
    
    if (JS_IsException(retval)) {
        js_std_dump_error(ctx);
        ret = -1;
    } else {
        ret = 0;
    }

    JS_FreeValue(ctx, retval);
    return ret;
}


static int binderjs_eval_file(JSContext *ctx, const char *filename)
{
    uint8_t *buf;
    int ret, eval_flags;
    size_t buf_len;
    
    buf = js_load_file(ctx, &buf_len, filename);
    if (!buf) {
        perror(filename);
        exit(1);
    }

    ret = binderjs_eval_buf(ctx, buf, buf_len, filename);
    js_free(ctx, buf);
    return ret;
}

void binderjs_string_append(char *dest, char *src)
{
    strcat(dest, src);
}

typedef struct targ
{
    char filename[BINDERJS_MODULE_ROOT_DIR_PATH_LEN_MAX];
}targ_t;

void *eval_thread_module(void *arg)
{
    JSRuntime* rt = JS_NewRuntime();
    JSContext *ctx = binderjs_new_context(rt);

    js_std_set_worker_new_context_func(binderjs_new_context);
    js_std_init_handlers(rt);
    js_std_add_helpers(ctx, 0, NULL);
    JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);

    binderjs_eval_file(ctx, ((targ_t *)arg)->filename);

    /*
    js_std_free_handlers(rt);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    */

    return 0;
}


static JSRuntime *service_rt;


int main(int argc, char *argv[], char *env[])
{
    char *dir;
    char *dir_tmp_buf;
    size_t dir_len;
    DIR *dir_ent;
    struct dirent *tmp_dir_ent;
    FILE *js_module_entry;
    pthread_t dummy_thread;

    fprintf(stdout, "::BinderJS::\nBind up the whole world!\n        <------------>>>\n\n");
    
    dir = argv[1];
    dir_len = strlen(dir);

    /* append '/' */
    if(dir[dir_len - 1] != '/')
    {
        dir_tmp_buf = (char *)malloc(dir_len + 2);
        memcpy(dir_tmp_buf, dir, dir_len);
        dir_tmp_buf[dir_len] = '/';
        dir_tmp_buf[dir_len + 1] = '\0';
        dir = dir_tmp_buf;
    }

    dir_ent = opendir(dir);

    fprintf(stdout, BINDERJS_LOG_PREFIX"init ...\n");

    service_rt = JS_NewRuntime();

    js_std_set_worker_new_context_func(binderjs_new_context);
    js_std_init_handlers(service_rt);
    JS_SetModuleLoaderFunc(service_rt, NULL, js_module_loader, NULL);

    while(1)
    {
        tmp_dir_ent = readdir(dir_ent);
        if(tmp_dir_ent == NULL)
            break;
        
        if(tmp_dir_ent->d_namlen == 1 && !strncmp(".", &tmp_dir_ent->d_name[0], 1)) 
            continue;
        
        if(tmp_dir_ent->d_namlen == 2 && !strncmp("..", &tmp_dir_ent->d_name[0], 2)) 
            continue;

        if(tmp_dir_ent->d_namlen > 6 && !strncmp("thread", &tmp_dir_ent->d_name[0], 6))
        {
            fprintf(stdout, BINDERJS_LOG_PREFIX"load thread module %s\n", tmp_dir_ent->d_name);

            targ_t *arg = (targ_t *)malloc(sizeof(targ_t));
            memset(arg->filename, 0, BINDERJS_MODULE_ROOT_DIR_PATH_LEN_MAX);
            binderjs_string_append(arg->filename, dir);
            binderjs_string_append(arg->filename, tmp_dir_ent->d_name);
            binderjs_string_append(arg->filename, "/");
            binderjs_string_append(arg->filename, BINDERJS_MODULE_ENTRY_NAME);

            pthread_create(&dummy_thread, NULL, eval_thread_module, arg);
        }
#if 0
        else if(tmp_dir_ent->d_namlen > 7 && !strncmp("service", &tmp_dir_ent->d_name[0], 7))
        {
            fprintf(stdout, BINDERJS_LOG_PREFIX"load service module %s\n", tmp_dir_ent->d_name);
            char filename[BINDERJS_MODULE_ROOT_DIR_PATH_LEN_MAX] = {0x00};
            memset(filename, 0, BINDERJS_MODULE_ROOT_DIR_PATH_LEN_MAX);
            binderjs_string_append(filename, dir);
            binderjs_string_append(filename, tmp_dir_ent->d_name);
            binderjs_string_append(filename, "/");
            binderjs_string_append(filename, BINDERJS_MODULE_ENTRY_NAME);

            JSContext *ctx = binderjs_new_context(service_rt);
            js_std_add_helpers(ctx, 0, NULL);

            binderjs_eval_file(ctx, filename);

            JS_FreeContext(ctx);

            /*
            js_std_free_handlers(rt);
            JS_FreeRuntime(rt);
            */

        }
#endif 
        else 
        {
            fprintf(stdout, BINDERJS_LOG_PREFIX"module name should start with \"service\" or \"thread\", but is \"%s\"\n", tmp_dir_ent->d_name);
        }
    }

    pthread_join(dummy_thread, NULL);

    return 0;
}



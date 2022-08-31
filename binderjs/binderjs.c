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
#include <string.h>

#include <quickjs.h>
#include <quickjs-libc.h>
#include <cutils.h>

#include <sys/syslimits.h>

#include <binderjs/binderjs.h>

static JSValue binderjs_hello(
    JSContext *ctx, 
    JSValueConst this_val, 
    int argc, JSValueConst *argv) 
{
    fprintf(stdout, "hello, binderjs\n");
    return JS_UNDEFINED;
}

/* extern */
JSValue JSX_Binder_EvalModuleFunction(
    JSContext *ctx, JSValue module, 
    const char *func_name, int argc, JSValueConst *argv);

static JSValue binderjs_call(
    JSContext *ctx, 
    JSValueConst this_val, 
    int argc, JSValueConst *argv) 
{
    const char *module_name, *func_name;
    struct list_head *module;
    binderjs_context_t *bctx;
    binderjs_module_t *m;
    JSValue retval;

    bctx = (binderjs_context_t *) JS_GetContextOpaque(ctx);

    module_name = JS_ToCString(ctx, argv[0]);
    if (!module_name)
        goto fail;
    func_name = JS_ToCString(ctx, argv[1]);
    if (!func_name)
        goto fail;

    list_for_each(module, &bctx->module_list)
    {   
        m = list_entry(module, binderjs_module_t, list);
        if(!strcmp(m->name, module_name))
        {
            retval = JSX_Binder_EvalModuleFunction(
                ctx, m->val,
                func_name, argc - 2, &argv[2]
            );
            break;
        }
    }

    JS_FreeCString(ctx, module_name);
    JS_FreeCString(ctx, func_name);

    return retval;

fail:
    JS_FreeCString(ctx, module_name);
    JS_FreeCString(ctx, func_name);
    return JS_EXCEPTION;
}

static JSValue binderjs_cast(
    JSContext *ctx, 
    JSValueConst this_val, 
    int argc, JSValueConst *argv) 
{
    return JS_UNDEFINED;
}

static JSValue binderjs_add_endpoint(
    JSContext *ctx, 
    JSValueConst this_val, 
    int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue binderjs_import(
    JSContext *ctx, 
    JSValueConst this_val, 
    int argc, JSValueConst *argv)
{
    const char *name, *path;
    uint8_t *buf = NULL;
    size_t name_len, head, tail, buf_len;
    binderjs_context_t *bctx;
    binderjs_module_t *bm, *m;
    char path_buf[PATH_MAX];
    BOOL abs_path = FALSE;
    JSValue val;
    
    
    char *module_name;
    struct list_head *module;
    size_t i;

    memset(&path_buf[0], 0, PATH_MAX);

    name = JS_ToCString(ctx, argv[0]);
    if (!name)
        goto fail;

    if(strlen(name) < 1)
        goto fail;

    bctx = (binderjs_context_t *) JS_GetContextOpaque(ctx);
    
    /* create js file path */
    if(*name == '@')
       { path = &name[1]; abs_path = TRUE; }
    else path = name;
    
    if(abs_path)
    {
        strcpy(path_buf, bctx->root_path);
        strcat(path_buf, path);
        path = &path_buf[0];
    }

    /* if already imported */
    list_for_each(module, &bctx->module_list)
    {   
        m = list_entry(module, binderjs_module_t, list);
        if(!strcmp(m->name, module_name))
        {
            goto end;
            break;
        }
    }

    /* load and compile */
    buf = js_load_file(ctx, &buf_len, path);

    val = JS_Eval(ctx, (const char *)buf, buf_len, path,
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    
    if (!JS_IsException(val)) {
        js_module_set_import_meta(ctx, val, TRUE, TRUE);
    }
    
    if (JS_IsException(val))
        goto fail;
 
    name_len = strlen(name);
    
    tail = name_len - 1;
    for(head = tail; head >= 0; head--)
    {
        if(name[head] == '.')
            { if(head > 1)
                { tail = head - 1; continue; } 
            else { goto fail; } }

        if(name[head] == '/')
            { head++; break; }
    }

    if(head < 0) head = 0;

    name_len = tail - head + 1;
    module_name = (char *) js_mallocz(ctx, name_len + 1);
    for(i = 0; head <= tail; head++, i++)
        module_name[i] = name[head];
    module_name[i] = '\0';

    bm = (binderjs_module_t *) js_mallocz(ctx, sizeof(binderjs_module_t));

    /* add module to bctx */
    bm->val = val;
    bm->name = module_name;
    list_add_tail(&bm->list, &bctx->module_list);

    /* val should not free here */
end:
    JS_FreeCString(ctx, name);
    js_free(ctx, buf);
    return JS_UNDEFINED;
fail:
    JS_FreeCString(ctx, name);
    if(buf)
        js_free(ctx, buf);
    return JS_EXCEPTION;
}

static JSValue binderjs_release(
    JSContext *ctx, 
    JSValueConst this_val, 
    int argc, JSValueConst *argv)
{
    const char *module_name;
    struct list_head *module;
    binderjs_context_t *bctx;
    binderjs_module_t *m;
    JSValue retval;

    bctx = (binderjs_context_t *) JS_GetContextOpaque(ctx);

    module_name = JS_ToCString(ctx, argv[0]);
    if (!module_name)
        goto fail;

    list_for_each(module, &bctx->module_list)
    {   
        m = list_entry(module, binderjs_module_t, list);
        if(!strcmp(m->name, module_name))
        {
            list_del(module);
            js_free(ctx, (void *) m->name);
            JS_FreeValue(ctx, m->val);
            js_free(ctx, m);
            break;
        }
    }

fail:
    JS_FreeCString(ctx, module_name);
    return JS_UNDEFINED;
}

void js_init_binderjs(JSContext *ctx)
{
    JSModuleDef *m;
    binderjs_context_t *bctx;

    JSValue global_obj, binderjs;

    global_obj = JS_GetGlobalObject(ctx);
    binderjs = JS_NewObject(ctx);

    JS_SetPropertyStr(ctx, binderjs, "hello", JS_NewCFunction(ctx, binderjs_hello, "hello", 0));
    JS_SetPropertyStr(ctx, binderjs, "import", JS_NewCFunction(ctx, binderjs_import, "import", 1));
    JS_SetPropertyStr(ctx, binderjs, "release", JS_NewCFunction(ctx, binderjs_release, "release", 1));
    /* call 方法的参数是不定长的 */
    JS_SetPropertyStr(ctx, binderjs, "call", JS_NewCFunction(ctx, binderjs_call, "call", 0));

    JS_SetPropertyStr(ctx, global_obj, "binderjs", binderjs);

    /* test */
    bctx = (binderjs_context_t *) js_mallocz(ctx, sizeof(binderjs_context_t));
    bctx->root_path = "../binderjs_test/";
    init_list_head(&bctx->module_list);

    JS_SetContextOpaque(ctx, (void *) bctx);

    JS_FreeValue(ctx, global_obj);
}




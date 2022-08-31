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

#ifndef __BINDERJS_H__
#define __BINDERJS_H__

#define BINDERJS_LOG_PREFIX "[BinderJS] "
#define BINDERJS_MODULE_NAME_LEN_MAX 256
#define BINDERJS_MODULE_ROOT_DIR_PATH_LEN_MAX 256
#define BINDERJS_MODULE_ENTRY_NAME "module.js"

#include <quickjs.h>
#include <list.h>

typedef struct binderjs_module 
{  
    const char *name;
    JSValue val;
    struct list_head list;
} binderjs_module_t;

typedef struct binderjs_msg 
{

} binderjs_msg_t;

typedef struct binderjs_context
{
    const char *root_path;

    /* 如果这里改成红黑树，搜索会快一些 */
    struct list_head module_list;

} binderjs_context_t;

void js_init_binderjs(JSContext *ctx);

#endif 

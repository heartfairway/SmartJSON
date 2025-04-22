/******
* JSON Parser & Utilities
* 
* by. Cory Chiang
* 
*   V. 3.0.0 (2025/04/20)
*

BSD 3-Clause License

Copyright (c) 2025, Cory Chiang
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

******/

#ifndef __JSONRPC_H__
#define __JSONRPC_H__

#include "json.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JSONRPC_REQTYPE_NULL    0
#define JSONRPC_REQTYPE_INT     1
#define JSONRPC_REQTYPE_STRING  2
#define JSONRPC_REQTYPE_NOTIFY  3

typedef struct _jsonrpc {
    char version[8];
    char *method;
    uint8_t reqType;
    union {
        int64_t idNum;
        char *idString;
    };

    int errorCode;
    char *errorMessage;
    json_t *errorData;

    json_t *params;
    json_t *result;

    int _sock_fd;
    struct _jsonrpc *next;
} jsonrpc_t;

jsonrpc_t *jsonrpcParse(char *str);
void jsonrpcFillError(jsonrpc_t *rpc, int code, const char *mesage);
char *jsonrpcResult(jsonrpc_t *rpc);
void jsonrpcFree(jsonrpc_t *rpc);

#ifdef __cplusplus
}
#endif

#endif /* __JSONRPC_H__ */

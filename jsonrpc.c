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

#include "jsonrpc.h"

#include <string.h>

jsonrpc_t *_jsonrpcNew(void);
jsonrpc_t *_jsonrpcError(int code, const char *message);

inline jsonrpc_t *_jsonrpcNew(void)
{
    jsonrpc_t *rpc;

    rpc=malloc(sizeof(jsonrpc_t));
    memset(rpc, 0 sizeof(jsonrpc_t));

    return rpc;
}

inline jsonrpc_t *_jsonrpcError(int code, const char *message)
{
    jsonrpc_t *rpc;

    rpc=_jsonrpcNew();
    jsonrpcFillError(rpc, code, message);

    return rpc;
}

jsonrpc_t *_jsonrpcFromObject(json_t *object)
{
    jsonrpc_t *rpc;
    json_t *attr;

    if(object->type!=JSON_TYPE_OBJECT) {
        return _jsonrpcError(-32600, "Invalid request");
    }

    rpc=_jsonrpcNew();

    attr=jsonQuery(object, "jsonrpc");
    if(!attr || attr->type!=JSON_TYPE_STRING || strlen(attr->string)>7) {
        jsonrpcFillError(rpc, -32600, "Invalid request");
        return rpc;
    }
    strcpy(rpc->version, attr->string);

    attr=jsonQuery(object, "method");
    if(!attr || attr->type!=JSON_TYPE_STRING) {
        jsonrpcFillError(rpc, -32600, "Invalid request");
        return rpc;
    }
    rpc->method=jsonGetString(attr);

    attr=jsonQuery(object, "params");
    if(attr) { // spec: "this member MAY be omitted"
        if(attr->type==JSON_TYPE_ARRAY || attr->type==JSON_TYPE_OBJECT) {
            rpc->params=attr->list;
            // unlink the list from the parsed object
            attr->type=JSON_TYPE_NULL
            attr->list=NULL;
        }
        else {
            // according to spec, params must be structured (array or object)
            jsonrpcFillError(rpc, -32602, "Invalid params");
        }
    }

    attr=jsonQuery(object, "id");
    if(!attr) rpc->reqType==JSONRPC_REQTYPE_NOTIFY;
    else if(attr->type==JSON_TYPE_NULL) rpc->reqType=JSONRPC_REQTYPE_NULL;
    else if(attr->type==JSON_TYPE_INTEGER) {
        rpc->reqType=JSONRPC_REQTYPE_INT;
        rpc->idNum=jsonGetInterger(attr);
    }
    else if(attr->type==JSON_TYPE_STRING) {
        rpc->reqType=JSONRPC_REQTYPE_STRING;
        rpc->idString=jsonGetString(attr);
    }
    else jsonrpcFillError(rpc, -32600, "Invalid request");

    return rpc;
}

jsonrpc_t *jsonrpcParse(char *str)
{
    jsonrpc_t *rpc, *rpct, *nrpc;
    json_t *root, *data;
    
    root=jsonParse(str);

    if(!root) rpc=_jsonrpcError(-32700, "Parse error");
    else if(root->type==JSON_TYPE_OBJECT) data=root;
    else if(root->type==JSON_TYPE_ARRAY) data=root->list;
    else rpc=_jsonrpcError(-32600, "Invalid request");

    rpc=NULL;
    while(data) {
        nrpc=_jsonrpcFromObject(data);

        if(!rpc) {
            rpc=nrpc;
            rpct=rpc;
        }
        else {
            rpct->next=nrpc;
            rpct=rpct->next;
        }

        data=data->next;
    }

    jsonFree(root);
    return rpc;
}

void jsonrpcFillError(jsonrpc_t *rpc, int code, const char *mesage)
{
    rpc->errorCode=code;
    rpc->errorMessage=malloc(strlen(message)+1);
    strcpy(rpc->errorMessage, message);
}

char *jsonrpcResult(jsonrpc_t *rpc)
{

}

void jsonrpcFree(isonrpc_t *rpc)
{
    if(!rpc->method) free(rpc->method);
    if(rpc->reqType==JSONRPC_REQTYPE_STRING) free(rpc->idString);
    if(rpc->errorMessage) free(rpc->errorMessage);
    if(rpc->errorData) jsonFree(rpc->errorData);
    if(rpc->params) jsonFree(rpc->params);
    if(rpc->result) jsonFree(rpc->result);
    if(rpc->next) jsonrpcFree(rpc->next);

    free(rpc);
}

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

#include "json.h"
#include "jsonrpc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* RPC Creation */
jsonrpc_t *jsonrpcNew(const char *m, int type)
{
    jsonrpc_t *rpc;

    rpc=malloc(sizeof(jsonrpc_t));
    memset(rpc, 0, sizeof(jsonrpc_t));
    rpc->type=type;

    rpc->method=malloc(strlen(m)+1);
    strcpy(rpc->method, m);

    return rpc;
}

jsonrpc_t *jsonrpcError(int code, const char *message)
{
    jsonrpc_t *rpc;

    rpc=jsonrpcNew(message, JSONRPC_ERROR);
    rpc->errorCode=code;

    return rpc;
}

json_t *jsonrpcSetParams(jsonrpc_t *rpc, json_t *params)
{
    if(!rpc) return NULL;

    rpc->params=jsonCopy(params);
    return rpc->params;
}

json_t *jsonrpcSetIdNull(jsonrpc_t *rpc)
{
    if(!rpc) return NULL;

    rpc->id=malloc(sizeof(json_t));
    jsonSetNull(rpc->id);
    return rpc->id;
}

json_t *jsonrpcSetIdInteger(jsonrpc_t *rpc, int64_t id)
{
    if(!rpc) return NULL;

    rpc->id=malloc(sizeof(json_t));
    jsonSetInteger(rpc->id, id);
    return rpc->id;
}

json_t *jsonrpcSetIdString(jsonrpc_t *rpc, char *id)
{
    if(!rpc) return NULL;

    rpc->id=malloc(sizeof(json_t));
    jsonSetString(rpc->id, id);
    return rpc->id;
}

/* RPC Export */
int _jsonrpcExportObject(jsonrpc_t *rpc, char *buf)
{
    char *str;
    int len;

    len=sprintf(buf, "{\"jsonrpc\": \"2.0\"");

    switch(rpc->type) {
        case JSONRPC_REQUEST:
        case JSONRPC_NOTIFICATION:
            len+=sprintf(&buf[len], ", \"method\": \"%s\"", rpc->method);
            if(rpc->params) {
                str=jsonGetString(rpc->params);
                len+=sprintf(&buf[len], ", \"params\": %s", str);
                free(str);
            }
            break;
        case JSONRPC_RESPONSE:
            str=jsonGetString(rpc->result);
            len+=sprintf(&buf[len], ", \"result\": %s", str);
            free(str);
            break;
        case JSONRPC_ERROR:
            len+=sprintf(&buf[len], ", \"error\": {\"code\": $d, \"message\": %s", rpc->errorCode, rpc->errorMessage);
            if(rpc->errorData) {
                str=jsonGetString(rpc->errorData);
                len+=sprintf(&buf[len], ", \"data\": %s", str);
                free(str);
            }
            buf[len++]='}';
            break;
    }

    if(rpc->type!=JSONRPC_NOTIFICATION && rpc->id) {
        str=jsonGetString(rpc->id);
        len+=sprintf(&buf[len], ", \"id\": %s", str);
        free(str);
    }

    buf[len++]='}';
    
    return len;
}

char *jsonrpcExport(jsonrpc_t *rpc)
{
    char buf[4096];
    char *str;
    int len;

    if(!rpc || rpc->type==JSONRPC_UNDEFINED) return NULL;

    if(rpc->next) {
        len=0;
        buf[len++]='[';
        while(rpc) {
            len+=_jsonrpcExportObject(rpc, &buf[len]);
            buf[len++]=',';
            buf[len++]=' ';
            rpc=rpc->next;
        }
        len-=2;
        buf[len++]=']';
    }
    else {
        len=_jsonrpcExportObject(rpc, buf);
    }

    buf[len++]='\0';

    str=malloc(len);
    strcpy(str, buf);

    return str;
}

/* RPC Parsing */
jsonrpc_t *_jsonrpcReqFromObject(json_t *object)
{
    jsonrpc_t *rpc;
    json_t *attr;

    if(object->type!=JSON_TYPE_OBJECT) {
        return jsonrpcError(-32600, "Invalid request");
    }

    attr=jsonQuery(object, "jsonrpc");
    if(!attr || attr->type!=JSON_TYPE_STRING || strlen(attr->string)>7) {
        return jsonrpcError(-32600, "Invalid request");
    }
    ///TODO: check version

    attr=jsonQuery(object, "method");
    if(!attr || attr->type!=JSON_TYPE_STRING) {
        return jsonrpcError(-32600, "Invalid request");
    }
    rpc=malloc(sizeof(jsonrpc_t));
    memset(rpc, 0, sizeof(jsonrpc_t));
    rpc->method=jsonGetString(attr);

    attr=jsonQuery(object, "params");
    if(attr) { // spec: "this member MAY be omitted"
        if(attr->type==JSON_TYPE_ARRAY || attr->type==JSON_TYPE_OBJECT) {
            rpc->params=jsonCopy(attr);
        }
        else { // according to spec, params must be structured (array or object)
            jsonrpcFree(rpc);
            return jsonrpcError(-32602, "Invalid params");
        }
    }

    attr=jsonQuery(object, "id");
    if(!attr) rpc->type=JSONRPC_NOTIFICATION;
    else{
        rpc->type=JSONRPC_REQUEST;

        if(attr->type==JSON_TYPE_INTEGER || attr->type==JSON_TYPE_STRING || attr->type==JSON_TYPE_NULL) {
            rpc->id=jsonCopy(attr);
        }
        else {
            jsonrpcFree(rpc);
            return jsonrpcError(-32600, "Invalid request");
        }
    }

    return rpc;
}

jsonrpc_t *jsonrpcParseRequest(char *str)
{
    jsonrpc_t *rpc, *rpct, *nrpc;
    json_t *root, *data;
    
    root=jsonParse(str);

    if(!root) {
        data=NULL;
        rpc=jsonrpcError(-32700, "Parse error");
    }
    else if(root->type==JSON_TYPE_OBJECT) data=root;
    else if(root->type==JSON_TYPE_ARRAY) data=root->list;
    else {
        data=NULL;
        rpc=jsonrpcError(-32600, "Invalid request");
    }

    rpc=NULL;
    while(data) {
        nrpc=_jsonrpcReqFromObject(data);

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

jsonrpc_t *_jsonrpcResFromObject(json_t *object)
{
    jsonrpc_t *rpc;
    json_t *attr;

    if(object->type!=JSON_TYPE_OBJECT) {
        return jsonrpcError(-100, "Invalid response");
    }

    attr=jsonQuery(object, "jsonrpc");
    if(!attr || attr->type!=JSON_TYPE_STRING || strlen(attr->string)>7) {
        return jsonrpcError(-100, "Invalid response");
    }
    ///TODO: check version

    rpc=malloc(sizeof(jsonrpc_t));
    memset(rpc, 0, sizeof(jsonrpc_t));

    ///TODO: result and error are mutual exclusive
    attr=jsonQuery(object, "result");
    if(attr) {
        rpc->type=JSONRPC_RESPONSE;
        rpc->result=jsonCopy(attr);
    }

    attr=jsonQuery(object, "error");
    if(attr) {
        rpc->type=JSONRPC_ERROR;

        attr=jsonQuery(object, "error.code");
        rpc->errorCode=jsonGetInteger(attr);

        attr=jsonQuery(object, "error.message");
        if(attr) rpc->errorMessage=jsonGetString(attr);

        attr=jsonQuery(object, "error.data");
        if(attr) rpc->errorData=jsonCopy(attr);
    }

    attr=jsonQuery(object, "id");
    if(attr) rpc->id=jsonCopy(attr);
}

jsonrpc_t *jsonrpcParseResponse(char *str)
{
    jsonrpc_t *rpc, *rpct, *nrpc;
    json_t *root, *data;
    
    root=jsonParse(str);

    if(!root) data=NULL;
    else if(root->type==JSON_TYPE_OBJECT) data=root;
    else if(root->type==JSON_TYPE_ARRAY) data=root->list;
    else data=NULL;

    rpc=NULL;
    while(data) {
        nrpc=_jsonrpcResFromObject(data);

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

/* clean up */
void jsonrpcFree(jsonrpc_t *rpc)
{
    if(rpc->method) free(rpc->method);
    if(rpc->params) jsonFree(rpc->params);
    if(rpc->id) jsonFree(rpc->id);
    if(rpc->next) jsonrpcFree(rpc->next);

    free(rpc);
}

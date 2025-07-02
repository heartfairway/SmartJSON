#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"
#include "jsonrpc.h"

int main(void)
{
    json_t *js1, *js2;
    jsonrpc_t *rpc1, *rpc2;
    char input[512];
    char *output, *temp;

    strcpy(input, "{\"jsonrpc\":\"2.0\", \"method\":\"TestMethod\", \"params\": [29, 67], \"id\":123}");
    printf("Input:\n  %s \n", input);
    rpc1=jsonrpcParseRequest(input);

    if(rpc1) {
        printf("method: %s \n", rpc1->method);
        temp=jsonGetString(rpc1->params);
        printf("params: %s \n", temp);
        free(temp);
    }
    else printf("Parse error. \n");

    rpc1->type=JSONRPC_RESPONSE;
    rpc1->result=malloc(sizeof(json_t));
    jsonSetInteger(rpc1->result, 96);

    output=jsonrpcExport(rpc1);
    printf("Output:\n  %s \n", output);
    free(output);

    jsonrpcFree(rpc1);

    printf("\n======\n\n");

    js1=jsonParse("{\"a\": 1, \"b\": 2}");
    js2=jsonParse("[100, 200, 300]");

    rpc1=jsonrpcNew("Call", JSONRPC_REQUEST);
    jsonrpcSetParams(rpc1, js1);
    jsonrpcSetIdInteger(rpc1, 123);

    rpc2=jsonrpcNew("Notify", JSONRPC_NOTIFICATION);
    jsonrpcSetParams(rpc2, js2);

    output=jsonrpcExport(rpc1);
    printf("Output:\n  %s \n", output);
    free(output);

    output=jsonrpcExport(rpc2);
    printf("Output:\n  %s \n", output);
    free(output);

    rpc1->next=rpc2;
    output=jsonrpcExport(rpc1);
    printf("Output:\n  %s \n", output);

    printf("------\n");
    jsonrpcFree(rpc1); // rpc2 is freed automatically, since it's rpc1->next

    rpc1=jsonrpcParseRequest(output);
    free(output);

    printf("type(1): %d \n", rpc1->type);
    printf("method(1): %s \n", rpc1->method);
    temp=jsonGetString(rpc1->params);
    printf("params(1): %s \n", temp);
    free(temp);
    rpc2=rpc1->next;
    printf("type(2): %d \n", rpc2->type);
    printf("method(2): %s \n", rpc2->method);
    temp=jsonGetString(rpc2->params);
    printf("params(2): %s \n", temp);
    free(temp);

    printf("------\n");
    jsonrpcFree(rpc1);

    rpc1=jsonrpcResult(js2);
    jsonrpcSetIdInteger(rpc1, 17);
    output=jsonrpcExport(rpc1);
    printf("Output:\n  %s \n", output);

    return 0;
}

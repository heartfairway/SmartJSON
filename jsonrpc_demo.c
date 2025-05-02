#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"
#include "jsonrpc.h"

int main(void)
{
	json_t *js1;
	jsonrpc_t *rpc1;
	char input[512];
	char *output, *temp;

	strcpy(input, "{\"jsonrpc\":\"2.0\", \"method\":\"TestMethod\", \"params\": [29, 67], \"id\":123}");
	
	printf("Input:\n  %s \n", input);

	//js1=jsonParse(input);
	rpc1=jsonrpcParseRequest(input);

	if(rpc1) {
		printf("method: %s \n", rpc1->method);
		temp=jsonGetString(rpc1->params);
		printf("params: %s \n", temp);
		free(temp);
	}
	else printf("Parse error. \n");

	rpc1->result=malloc(sizeof(json_t));
	jsonFillInteger(rpc1->result, 96);

	output=jsonrpcResult(rpc1);
	printf("Output:\n  %s \n", output);

	/*free(output);

	js2=jsonQuery(js1, "nameB.BA[2]");
	output=jsonGetString(js2);
	printf("Output:\n  %s \n", output);

	jsonFree(js1);*/

	return 0;
}

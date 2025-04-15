#include <stdio.h>
#include <string.h>
#include "json.h"

int main(void)
{
	//struct SJSN_SHELL shell;
	//struct SJSN_VA *js;
	json_t *js, *js2;
	char buf_a[128]; //, buf_b[128];
	char *output;

	//strcpy(buf_a, "Null"); // null
	//strcpy(buf_a, "false"); // false
	//strcpy(buf_a, "true"); // true
	//strcpy(buf_a, "{\"boolean\":true, \"integer\":123}"); // simple object
	//strcpy(buf_a, "[True, False]"); // simple array
	//strcpy(buf_a, "[1, 2.3, true, false, \"String\", null]"); // array
	//strcpy(buf_a, "-1.23"); // real number
	//strcpy(buf_a, "123000"); // int number
	//strcpy(buf_a, "\"This is a string.\""); //string
	strcpy(buf_a, "{\"string\":\"A String\", \"number\":123, \"array\":[123, \"string\", true, false, null]}"); // mixed object
	
	printf("Buffer A:\n  %s \n", buf_a);

	//shell.str=buf_a;
	//js=SJNSParse(&shell);
	js=jsonParse(buf_a);

	//printf("Parsing status: %d / %d / %d \n", shell.error_code, shell.p_ptr, shell.p_level);

	//SJSNExport(js, buf_b, 128);
	output=jsonGetString(js);

	//printf("Buffer B:\n  %s \n", buf_b);
	printf("Output:\n  %s \n", output);

	js2=jsonQuery(js, "array[2]");
	output=jsonGetString(js2);
	printf("Output:\n  %s \n", output);

	return 0;
}

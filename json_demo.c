#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"

int main(void)
{
	json_t *js1, *js2;
	char input[512];
	char *output;

	//strcpy(input, "Null"); // null
	//strcpy(input, "false"); // false
	//strcpy(input, "true"); // true
	//strcpy(input, "{\"boolean\":true, \"integer\":123}"); // simple object
	//strcpy(input, "[True, False]"); // simple array
	//strcpy(input, "[1, 2.3, true, false, \"String\", null]"); // array
	//strcpy(input, "-1.23"); // real number
	//strcpy(input, "123000"); // int number
	//strcpy(input, "\"This is a string.\""); //string
	//strcpy(input, "{\"string\":\"A String\", \"number\":123, \"array\":[123, \"string\", true, false, null]}"); // mixed object
	strcpy(input, "{\"nameA\":{\"AA\":[\"s1\", \"s2\", \"s3\" ], \"AB\":[\"s4\", \"s5\", \"s6\" ], \"AC\":[\"s7\", \"s8\", \"s9\"]}, \"nameB\":{\"BA\":[\"s10\", \"s11\", \"s12\"], \"BB\":[\"s13\",\"s14\",\"s15\"],\"BC\":[\"s16\", \"s17\", \"s18\" ]}, \"nameC\":{\"CA\":[\"s19\", \"s20\", \"s21\" ], \"CB\":[\"s22\", \"s23\", \"s24\" ], \"CC\":[\"s25\", \"s26\", \"s27\" ]}, \"NameC2\":[ 0, 1, 2, 3 ], \"nameD\":[ 10, 20, 30 ] }"); // mixed object
	
	printf("Input:\n  %s \n", input);

	js1=jsonParse(input);

	output=jsonGetString(js1);
	printf("Output:\n  %s \n", output);

	free(output);

	js2=jsonQuery(js1, "nameB.BA[2]");
	output=jsonGetString(js2);
	printf("Output:\n  %s \n", output);

	return 0;
}

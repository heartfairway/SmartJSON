#include <stdio.h>
#include <string.h>
#include "json.h"

int main(void)
{
#if 0
	struct SJSN_SHELL shell;
	struct SJSN_VA *js, *query;
	char buf_a[512], buf_b[512];

	/* source JSON string */
	strcpy(buf_a, "{\"nameA\":{\"AA\":[\"s1\", \"s2\", \"s3\" ], \"AB\":[\"s4\", \"s5\", \"s6\" ], \"AC\":[\"s7\", \"s8\", \"s9\"]}, \"nameB\":{\"BA\":[\"s10\", \"s11\", \"s12\"], \"BB\":[\"s13\",\"s14\",\"s15\"],\"BC\":[\"s16\", \"s17\", \"s18\" ]}, \"nameC\":{\"CA\":[\"s19\", \"s20\", \"s21\" ], \"CB\":[\"s22\", \"s23\", \"s24\" ], \"CC\":[\"s25\", \"s26\", \"s27\" ]}, \"NameC2\":[ 0, 1, 2, 3 ], \"nameD\":[ 10, 20, 30 ] }"); // mixed object
	
	printf("Buffer A:\n  %s \n", buf_a);

	shell.str=buf_a;
	js=SJNSParse(&shell);

	printf("Parsing status: %d / %d / %d \n", shell.error_code, shell.p_ptr, shell.p_level);

	SJSNExport(js, buf_b, 512);

	printf("Buffer B:\n  %s \n", buf_b);

	/* Example of query */
	query=SJSNQuery(js, "nameB.BA[2]", 0);
	//query=SJSNQuery(js, "NameC2[2]", 0);

	SJSNExport(query, buf_b, 512);
	printf("(query result) Buffer B:\n  %s \n", buf_b);

	/* Example of query and auto create */
	query=SJSNQuery(js, "nameC.CB[3].alpha", 1);

	query->type=SJSN_VAL_INTEGER;
	query->data.integer=2017;

	query=SJSNQuery(js, "nameC.CC[5]", 1);

	SJSNExport(js, buf_b, 512);
	printf("(auto create result) Buffer B:\n  %s \n", buf_b);
#endif
	return 0;
}

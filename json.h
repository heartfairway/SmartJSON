/******
* JSON Parser & Exporter   (code Stella)
* 
* by. Cory Chiang
* 
******/

#ifndef __JSON_H_
#define __JSON_H_

#define SJSN_MAX_LEVEL		32

/* data types */
#define SJSN_VAL_NULL		0
#define SJSN_VAL_FALSE		1
#define SJSN_VAL_TRUE		2
#define SJSN_VAL_OBJECT		3
#define SJSN_VAL_ARRAY		4
#define SJSN_VAL_REALNUM	5
#define SJSN_VAL_INTEGER	6
#define SJSN_VAL_STRING		7

/* error codes */
#define SJSN_OK				0
#define SJSN_PHRASE_ERROR	1
#define SJSN_SYNTAX_ERROR	2
#define SJSN_LEVEL_EXCEED	3
#define SJSN_UNEXP_SUFFIX	4
#define SJSN_UNEXP_NUL		5
#define SJSN_OUT_OF_BUF		6
#define SJSN_EXPORT_TYPE	7

// forward declaration
struct SJSN_ARR;
struct SJSN_OBJ;

/* data definitions */

union SJSNvalue
{
	char*				string;
	int					integer;
	double				realnum;
	struct SJSN_ARR*	array;
	struct SJSN_OBJ*	object;
};

/*
* Value descriptor
*/
struct SJSN_VAL
{
	unsigned char	type;
	union SJSNvalue	res;
};

/*
* Array descriptor
*/
struct SJSN_ARR
{
	struct SJSN_VAL*	value;
	struct SJSN_ARR*	next;
};

/*
* Object descriptor
*/
struct SJSN_OBJ
{
	char*				name;
	unsigned int		hidx;
	struct SJSN_VAL*	value;
	struct SJSN_OBJ*	next;
};

struct SJSN_SHELL
{
	char*				str;
	
	int					error_code;
	int					p_ptr;
	int					p_level;
	struct SJSN_VAL*	p_stack[SJSN_MAX_LEVEL];
};

/* functions */
struct SJSN_VAL* SJNSParse(struct SJSN_SHELL* sh);
struct SJSN_VAL* SJNSQuickParse(char* src);

int SJSNExport(struct SJSN_VAL* val, char* buf, int size);
void SJSNFree(struct SJSN_VAL* val);

void SJSNObjectIdx(struct SJSN_OBJ* obj);
struct SJSN_VAL* SJSNQuery(struct SJSN_VAL* val, char* path, int mode);

unsigned int SJSNhidx(char* str);

//struct SJSN_VAL* SJSNArrayInsert(struct SJSN_ARR **arrd);
//struct SJSN_VAL* SJSNObjectInsert(struct SJSN_OBJ **objd, char *name);

#endif

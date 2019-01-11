/******
* JSON Parser & Exporter   (code Stella)
* 
* by. Cory Chiang
* 

BSD 3-Clause License

Copyright (c) 2017, Cory Chiang
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

#ifndef __JSON_H_
#define __JSON_H_

#include <stdint.h>

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
	int64_t				integer;
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
struct SJSN_VAL* SJSNObjMultiQuery(struct SJSN_VAL* val, char* id);

unsigned int SJSNhidx(char* str);

//struct SJSN_VAL* SJSNArrayInsert(struct SJSN_ARR **arrd);
//struct SJSN_VAL* SJSNObjectInsert(struct SJSN_OBJ **objd, char *name);

#endif

/******
* JSON Parser & Exporter
* 
* by. Cory Chiang
* 
*   V. 2.2.1 (2023/07/10)
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

#ifndef __JSON_H__
#define __JSON_H__

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SJSN_ENABLE_QUERY	1
#define SJSN_NUM_POWER		1
#define SJSN_NUM_INT		0

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

// for 3.0
#define JSON_TYPE_NULL     0
#define JSON_TYPE_BOOLEAN  1
#define JSON_TYPE_STRING   2
#define JSON_TYPE_INTEGER  3
#define JSON_TYPE_NUMERIC  4
#define JSON_TYPE_ARRAY    5
#define JSON_TYPE_OBJECT   6
// --

/* error codes */
#define SJSN_OK				0
#define SJSN_PHRASE_ERROR	1
#define SJSN_SYNTAX_ERROR	2
#define SJSN_LEVEL_EXCEED	3
#define SJSN_UNEXP_SUFFIX	4
#define SJSN_UNEXP_NUL		5
#define SJSN_OUT_OF_BUF		6
#define SJSN_EXPORT_TYPE	7

// for 3.0
#define JSON_ERROR_NONE    0
#define JSON_ERRPR_PHRASE  1  
// --

// for 3.0
typedef struct _json_t {
    uint8_t type;
    union {  // this should be 64 bits / 8 bytes in modern 64-bit systems
        bool           boolean;
        char           *string;
        int64_t        integer;
        double         numeric;
        struct _json_t *list;
    };
    struct _json_t *next;
} json_t;

typedef struct _json_labeled_t {
    json_t value;
    char *label;
} json_labeled_t;
// --

// forward declaration
struct SJSN_VA;
struct SJSN_VB;

/* data definitions */

// data descriptors
union SJSNval
{
	char*				string;
	int64_t				integer;
	double				realnum;
	struct SJSN_VA*		array;
	struct SJSN_VB*		object;
};

struct SJSN_VA
{
	uint8_t			type;
	union SJSNval	data;
	struct SJSN_VA*	next;
};

struct SJSN_VB
{
	uint8_t			type;
	union SJSNval	data;
	struct SJSN_VB*	next;
	
	char*			name;
	uint32_t		hidx;
};

struct SJSN_SHELL
{
	char*			str;
	
	int				error_code;
	int				p_ptr;
	int				p_level;
	struct SJSN_VA*	p_stack[SJSN_MAX_LEVEL];
};

/* functions */
struct SJSN_VA* SJNSParse(struct SJSN_SHELL* sh);
struct SJSN_VA* SJNSQuickParse(char* src);

// for 3.0
bool jsonFillNull(json_t *dst);
bool jsonFillBoolean(json_t *dst, bool value);
bool jsonFillString(json_t *dst, char *value);
bool jsonFillInteger(json_t *dst, int64_t value);
bool jsonFillNumeric(json_t *dst, double value);
bool jsonAttachArray(json_t *dst, json_t *value);
bool jsonAttachObject(json_t *dst, json_labeled_t *value);

json_t *jsonParse(char *str);
json_t *jsonQuery(json_t *root, char *str);

bool jsonEqNull(json_t *value);
bool jsonEqBoolean(json_t *value);
int64_t jsonGetInteger(json_t *value);
double jsonGetNumeric(json_t *value);
char *jsonGetString(json_t *value);
// --

int SJSNExport(struct SJSN_VA* val, char* buf, int size);
void SJSNFree(void* val);

void SJSNObjectIdx(struct SJSN_VB* obj);

#if defined(SJSN_ENABLE_QUERY) && SJSN_ENABLE_QUERY

struct SJSN_VA* SJSNQuery(struct SJSN_VA* val, char* path, int mode);
struct SJSN_VA* SJSNObjMultiQuery(struct SJSN_VA* val, char* id);

#endif

unsigned int SJSNhidx(char* str);

#ifdef __cplusplus
}
#endif

#endif /* __JSON_H__ */
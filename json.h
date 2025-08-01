/******
* JSON Parser & Utilities
* 
* by. Cory Chiang
* 
*   V. 3.0.0 (2025/04/16)
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

#ifdef __cplusplus
extern "C" {
#endif

/* data types */
#define JSON_TYPE_NULL     0
#define JSON_TYPE_BOOLEAN  1
#define JSON_TYPE_STRING   2
#define JSON_TYPE_INTEGER  3
#define JSON_TYPE_NUMERIC  4
#define JSON_TYPE_ARRAY    5
#define JSON_TYPE_OBJECT   6

/* error codes */
#define JSON_ERROR_NONE    0
#define JSON_ERRPR_PHRASE  1  

typedef struct json_t {
    uint8_t fixed:1;
    uint8_t reference:1;
    uint8_t type:6;

    union {
        bool         boolean;
        char         *string;
        int64_t      integer;
        double       numeric;
        struct json_t *list;
    };

    struct json_t *next;
    char *label;
} json_t;

bool jsonSetNull(json_t *dst);
bool jsonSetBoolean(json_t *dst, bool value);
bool jsonSetString(json_t *dst, const char *value);
bool jsonRefString(json_t *dst, char *value);
bool jsonSetInteger(json_t *dst, int64_t value);
bool jsonSetNumeric(json_t *dst, double value);

bool jsonSetArray(json_t *dst, json_t *value);
bool jsonRefArray(json_t *dst, json_t *value);
bool jsonSetObject(json_t *dst, json_t *value);
bool jsonRefObject(json_t *dst, json_t *value);

bool jsonInsertList(json_t *dst, json_t *value);

bool jsonLabelName(json_t *dst, const char *str);

json_t *jsonParse(char *str);
json_t *jsonQuery(json_t *root, const char *str);

bool jsonEqNull(json_t *value);
bool jsonEqBoolean(json_t *value);
int64_t jsonGetInteger(json_t *value);
double jsonGetNumeric(json_t *value);
char *jsonGetString(json_t *value);
int jsonListCount(json_t *value);

json_t *jsonCopy(json_t *value);
void jsonFree(json_t *value);

#ifdef __cplusplus
}
#endif

#endif /* __JSON_H__ */
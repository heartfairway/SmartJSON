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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "json.h"

#ifndef NAN
#define NAN    0
#endif

int json_error = 0;

///TODO: Check parsing empty array or object

/* forward reference declaration */
bool _jsonFillZero(json_t *dst);

char *_skipWhitespace(char **src);
int _getString(char **src, char *buf);
json_t *_buildValue(char **src);

bool _jsonSetArray(json_t *dst, json_t *value, bool ref);
bool _jsonSetObject(json_t *dst, json_t *value, bool ref);

int _strcpyToJsonEsc(char *dest, char *src);

json_t *_queryArray(json_t *value, char **src);
json_t *_queryObject(json_t *value, char **src);

void _fillPureValStrBuf(json_t *value, bool esc, char *buf, int *idx);
void _fillOptStrBuf(json_t *value, char *buf, int *idx);

json_t *_jsonCopy(json_t *value, bool expand);

/************************************
 **  #internat# Utility Functions  **
 ************************************/
inline bool _jsonFillZero(json_t *dst)
{
    if(!dst) return false;

    memset(dst, 0, sizeof(json_t));
    return true;
}

/*************************
 **  Filling Functions  **
 *************************/
bool jsonSetNull(json_t *dst)
{
    return _jsonFillZero(dst);  // equals to null
}

bool jsonSetBoolean(json_t *dst, bool value)
{
    if(!_jsonFillZero(dst)) return false;

    dst->type=JSON_TYPE_BOOLEAN;
    dst->boolean=value;

    return true;
}

bool jsonSetString(json_t *dst, char *value)
{
    int len;

    if(!value || !_jsonFillZero(dst)) return false;

    len=strlen(value);
    dst->string=malloc(len+1);
    if(!dst->string) return false;

    dst->type=JSON_TYPE_STRING;
    strcpy(dst->string, value);
    dst->string[len]='\0';

    return true;
}

bool jsonRefString(json_t *dst, char *value)
{
    if(!value || !_jsonFillZero(dst)) return false;

    dst->type=JSON_TYPE_STRING;
    dst->string=value;
    dst->reference=true;

    return true;
}

bool jsonSetInteger(json_t *dst, int64_t value)
{
    if(!_jsonFillZero(dst)) return false;

    dst->type=JSON_TYPE_INTEGER;
    dst->integer=value;

    return true;
}

bool jsonSetNumeric(json_t *dst, double value)
{
    if(!_jsonFillZero(dst)) return false;

    dst->type=JSON_TYPE_NUMERIC;
    dst->numeric=value;

    return true;
}

inline bool _jsonSetArray(json_t *dst, json_t *value, bool ref)
{
    if(!_jsonFillZero(dst)) return false;

    dst->type=JSON_TYPE_ARRAY;
    dst->list=value;
    dst->reference=ref;

    return true;
}

bool jsonSetArray(json_t *dst, json_t *value)
{
    return _jsonSetArray(dst, value, false);
}

bool jsonRefArray(json_t *dst, json_t *value)
{
    return _jsonSetArray(dst, value, true);
}

inline bool _jsonSetObject(json_t *dst, json_t *value, bool ref)
{
    if(!_jsonFillZero(dst)) return false;

    dst->type=JSON_TYPE_OBJECT;
    dst->list=value;
    dst->reference=ref;

    return true;
}

bool jsonSetObject(json_t *dst, json_t *value)
{
    return _jsonSetObject(dst, value, false);
}

bool jsonRefObject(json_t *dst, json_t *value)
{
    return _jsonSetObject(dst, value, true);
}

bool jsonInsertList(json_t *dst, json_t *value)
{
    json_t *ptr;

    if(!dst || (dst->type!=JSON_TYPE_ARRAY && dst->type!=JSON_TYPE_OBJECT)) return false;

    ptr=dst->list;
    if(dst->list) {
        ptr=dst->list;
        while(ptr->next) ptr=ptr->next;

        ptr->next=value;
    }
    else dst->list=value;
}

bool jsonLabelName(json_t *dst, const char *str)
{
    if(!dst || !str) return false;
    if(dst->label) free(dst->label);

    dst->label=malloc(strlen(str)+1);
    strcpy(dst->label, str);
}

/*************************************
 **  #internal# Matching Functions  **
 *************************************/
/* src := (char **) scanning pointer for whole input string, modify 
 *     while scanning/parsing in order to keep the progress
 * cptr := (char *) local temp pointer in each matching function
 */
inline char *_skipWhitespace(char **src)
{
    while(**src==' ' || **src=='\n' || **src=='\r' || **src=='\t') {
        (*src)++;
    }

    return *src;
}

json_t *_matchNull(char **src)
{
    json_t *rval;

    if((*src)[0]!='n' && (*src)[0]!='N') return NULL;
    if((*src)[1]!='u' && (*src)[1]!='U') return NULL;
    if((*src)[2]!='l' && (*src)[2]!='L') return NULL;
    if((*src)[3]!='l' && (*src)[3]!='L') return NULL;
    if(isalnum((*src)[4])) return NULL;

    (*src)+=4;
    rval=malloc(sizeof(json_t));
    jsonSetNull(rval);

    return rval;
}

json_t *_matchBooleanTrue(char **src)
{
    json_t *rval;

    if((*src)[0]!='t' && (*src)[0]!='T') return NULL;
    if((*src)[1]!='r' && (*src)[1]!='R') return NULL;
    if((*src)[2]!='u' && (*src)[2]!='U') return NULL;
    if((*src)[3]!='e' && (*src)[3]!='E') return NULL;
    if(isalnum((*src)[4])) return NULL;

    (*src)+=4;
    rval=malloc(sizeof(json_t));
    jsonSetBoolean(rval, true);

    return rval;
}

json_t *_matchBooleanFalse(char **src)
{
    json_t *rval;

    if((*src)[0]!='f' && (*src)[0]!='F') return NULL;
    if((*src)[1]!='a' && (*src)[1]!='A') return NULL;
    if((*src)[2]!='l' && (*src)[2]!='L') return NULL;
    if((*src)[3]!='s' && (*src)[3]!='S') return NULL;
    if((*src)[4]!='e' && (*src)[4]!='E') return NULL;
    if(isalnum((*src)[5])) return NULL;

    (*src)+=5;
    rval=malloc(sizeof(json_t));
    jsonSetBoolean(rval, false);

    return rval;
}

inline int _getString(char **src, char *buf)
{
    int pLen = 0;

    if(**src!='\"') return 0;
    (*src)++;

    // scan to the end double quote or string end   
    while(**src!='\"' && **src!='\0') {
        if(**src=='\\') { // escape character
            (*src)++; // (the escape character)
            switch(**src) { // look ahead                    
                case '\"':
                    buf[pLen++]='\"';
                    break;
                case '\\':
                    buf[pLen++]='\\';
                    break;
                case '/':
                    buf[pLen++]='/';
                    break;
                case 'n':
                    buf[pLen++]='\n';
                    break;
                case 'r':
                    buf[pLen++]='\r';
                    break;
                case 't':
                    buf[pLen++]='\t';
                    break;
                case 'b':
                case 'f':
                case 'u':
                    // transparent
                    buf[pLen++]='\\';
                    buf[pLen++]='u'; 
                    break;
                default: // including '\0'
                    // syntax error
                    return 0;
            }

            (*src)++; // (the one after escape character)
        }
        else {
            buf[pLen++]=**src;
            (*src)++;
        }
    }
    
    // syntax error, should end with a double quote
    if(**src=='\0') return 0; 
    else (*src)++; // shift the '\"'

    buf[pLen]='\0';

    return pLen;
}

json_t *_matchString(char **src)
{
    json_t *rval;
    char buf[2048];

    if(!_getString(src, buf)) {
        // syntax error
        return NULL;
    }

    rval=malloc(sizeof(json_t));
    if(!jsonSetString(rval, buf)) {
        // error
        return NULL;
    }

    return rval;
}

json_t *_matchNumber(char **src)
{
    json_t *rval;
    char buf[2048];
    bool numeric = false;
    int i = 0;

    if(**src=='-') {
        buf[i++]='-';
        (*src)++;
    }

    while(isdigit(**src)) {
        buf[i++]=**src;
        (*src)++;
    }

    if(**src=='.') {
        buf[i++]='.';
        numeric=true;
        (*src)++;
    }

    while(isdigit(**src)) {
        buf[i++]=**src;
        (*src)++;
    }

    if(**src=='e' || **src=='E') {
        buf[i++]='e';
        numeric=true;
        (*src)++;
    }

    if(**src=='-' || **src=='+') {
        buf[i++]=**src;
        (*src)++;
    }

    while(isdigit(**src)) {
        buf[i++]=**src;
        (*src)++;
    }

    buf[i]='\0';

    rval=malloc(sizeof(json_t));
    if(numeric) jsonSetNumeric(rval, strtod(buf, NULL));
    else jsonSetInteger(rval, strtoll(buf, NULL, 10));

    return rval;
}

json_t *_matchArray(char **src)
{
    json_t *rval;
    json_t *arrayHead, *arrayTail, *matchedItem;

    if(**src!='[') return NULL;
    (*src)++;

    arrayHead=NULL;
    while(1) {
        _skipWhitespace(src);

        matchedItem=_buildValue(src);
        if(matchedItem) {
            if(arrayHead==NULL) {
                arrayHead=matchedItem;
                arrayTail=arrayHead;
            }
            else {
                arrayTail->next=matchedItem;
                arrayTail=arrayTail->next;
            }
        }
        else break; // syntax error

        _skipWhitespace(src);

        if(**src==',') (*src)++;
        else break;
    }

    if(**src!=']') {
        // syntax error
        return NULL;
    }
    else (*src)++;

    rval=malloc(sizeof(json_t));
    jsonSetArray(rval, arrayHead);

    return rval;
}

json_t *_matchObject(char **src)
{
    json_t *rval;
    json_t *objectHead, *objectTail, *matchedItem;
    char buf[2048];
    int len;

    if(**src!='{') return NULL;
    (*src)++;

    objectHead=NULL;
    while(1) {
        _skipWhitespace(src);

        if(!_getString(src, buf)) {
            // syntax error
            return NULL;
        }

        _skipWhitespace(src);

        if(**src!=':') {
            // syntax error
            return NULL;
        }
        else (*src)++; // shift the ':'

        _skipWhitespace(src);

        matchedItem=_buildValue(src);
        if(matchedItem) {
            len=strlen(buf);
            matchedItem->label=malloc(len+1);
            strcpy(matchedItem->label, buf);
            matchedItem->label[len]='\0';

            if(objectHead==NULL) {
                objectHead=matchedItem;
                objectTail=objectHead;
            }
            else {
                objectTail->next=matchedItem;
                objectTail=matchedItem;
            }
        }
        else break; // syntax error

        _skipWhitespace(src);

        if(**src==',') (*src)++;
        else break;
    }

    if(**src!='}') {
        // syntax error
        return NULL;
    }
    else (*src)++;

    rval=malloc(sizeof(json_t));
    jsonSetObject(rval, objectHead);

    return rval;
}

inline json_t *_buildValue(char **src)
{
    switch(**src) {
        case '\"':
            return _matchString(src);
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return _matchNumber(src);
        case 'F':
            return _matchBooleanFalse(src); 
        case 'N':
            return _matchNull(src);
        case 'T':
            return _matchBooleanTrue(src);
        case '[':
            return _matchArray(src);
        case 'f':
            return _matchBooleanFalse(src); 
        case 'n':
            return _matchNull(src);
        case 't':
            return _matchBooleanTrue(src);
        case '{':
            return _matchObject(src);
        default:
            // phrase error
            return NULL;
    }
}

/*************************
 **  Utility Functions  **
 *************************/

json_t *jsonParse(char *str)
{
    _skipWhitespace(&str);
    return _buildValue(&str);
}

inline json_t *_queryArray(json_t *value, char **src)
{
    json_t *accessPtr;
    char buf[32];
    int i, n;

    if(**src!='[') return NULL;  // syntax error
    else (*src)++;

    i=0;
    while(isdigit(**src)) {
        buf[i++]=**src;
        (*src)++;
    }

    if(**src!=']') return NULL;  // syntax error
    (*src)++;
    buf[i]='\0';

    n=atoi(buf);
    accessPtr=value->list;

    for(i=0; i<n; i++) {
        accessPtr=accessPtr->next;
        if(accessPtr==NULL) break;
    }

    return accessPtr;
}

inline json_t *_queryObject(json_t *value, char **src)
{
    json_t *accessPtr;
    char buf[256];
    int i;

    i=0;
    while(isalnum(**src)) {
        buf[i++]=**src;
        (*src)++;
    }
    if(**src!='.' && **src!='[' && **src!='\0') return NULL; // syntax error
    if(**src=='.') (*src)++;
    buf[i]='\0';

    accessPtr=value->list;

    while(accessPtr) {
        if(strcmp(buf, accessPtr->label)==0) break;
        accessPtr=accessPtr->next;
    }
    
    if(accessPtr) return accessPtr;
    else return NULL;
}

json_t *jsonQuery(json_t *root, const char *str)
{
    json_t *currentLevel, *accessPtr;

    if(!root) return NULL;

    if(!str) return root;
    if(strlen(str)==0) return root;

    currentLevel=root;
    while(*str!='\0') {
        if(currentLevel->type==JSON_TYPE_ARRAY) accessPtr=_queryArray(currentLevel, (char **)&str);
        else if(currentLevel->type==JSON_TYPE_OBJECT) accessPtr=_queryObject(currentLevel, (char **)&str);
        else return NULL;

        if(!accessPtr) break; 
        currentLevel=accessPtr;
    }

    return accessPtr;
}

bool jsonEqNull(json_t *value)
{
    if(!value) return true;

    switch(value->type) {
        case JSON_TYPE_NULL:
            return true;
        case JSON_TYPE_STRING:
            if(strlen(value->string)==0) return true;
            else return false;
        case JSON_TYPE_INTEGER:
            if(value->integer==0) return true;
            else return false;
        case JSON_TYPE_NUMERIC:
            if(value->numeric==0) return true;
            else return false;
        case JSON_TYPE_ARRAY:
        case JSON_TYPE_OBJECT:
            if(value->list==NULL) return true;
            else return false;
        default: // JSON_TYPE_BOOLEAN
            return false;
    }
}

bool jsonEqBoolean(json_t *value)
{
    if(!value) return false;

    switch(value->type) {
        case JSON_TYPE_BOOLEAN:
            return value->boolean;
        case JSON_TYPE_STRING:
            if(strlen(value->string)) return true;
            else return false;
        case JSON_TYPE_INTEGER:
            if(value->integer) return true;
            else return false;
        case JSON_TYPE_NUMERIC:
            if(value->numeric) return true;
            else return false;
        case JSON_TYPE_ARRAY:
        case JSON_TYPE_OBJECT:
            if(value->list) return true;
            else return false;
        default: // JSON_TYPE_NULL
            return false;
    }
}

int64_t jsonGetInteger(json_t *value)
{
    if(!value) return 0;

    switch(value->type) {
        case JSON_TYPE_BOOLEAN:
            return (int64_t)value->boolean;
        case JSON_TYPE_STRING:
            return strtoll(value->string, NULL, 10);
        case JSON_TYPE_INTEGER:
            return value->integer;
        case JSON_TYPE_NUMERIC:
            return (int64_t)value->numeric;
        default: // JSON_TYPE_NULL, JSON_TYPE_ARRAY, JSON_TYPE_OBJECT
            return 0;
    }
}

double jsonGetNumeric(json_t *value)
{
    if(!value) return 0;

    switch(value->type) {
        case JSON_TYPE_NULL:
            return 0;
        case JSON_TYPE_BOOLEAN:
            if(value->boolean) return 1;
            else return 0;
        case JSON_TYPE_STRING:
            return strtod(value->string, NULL);
        case JSON_TYPE_INTEGER:
            return (double)value->integer;
        case JSON_TYPE_NUMERIC:
            return value->numeric;
        default: // JSON_TYPE_ARRAY, JSON_TYPE_OBJECT
            return NAN;
    }
}

inline int _strcpyToJsonEsc(char *dest, char *src)
{
    int i;
    int pLen = 0;

    for(i=0; src[i]!='\0'; i++) {
        switch(src[i]) {
            case '\t':
                dest[pLen++]='\\';
                dest[pLen++]='t';
                break;
            case '\n':
                dest[pLen++]='\\';
                dest[pLen++]='n';
                break;
            case '\r':
                dest[pLen++]='\\';
                dest[pLen++]='r';
                break;
            case '\"':
                dest[pLen++]='\\';
                dest[pLen++]='\"';
                break;
            case '\\':
                dest[pLen++]='\\';
                dest[pLen++]='\\';
                break;
            default:
                dest[pLen++]=src[i];
        }
    }

    return pLen;
}

inline void _fillPureValStrBuf(json_t *value, bool esc, char *buf, int *idx)
{
    switch(value->type) {
        case JSON_TYPE_NULL:
            (*idx)+=sprintf(&buf[*idx], "null");
            break;
        case JSON_TYPE_BOOLEAN:
            if(value->boolean) (*idx)+=sprintf(&buf[*idx], "true");
            else (*idx)+=sprintf(&buf[*idx], "false");
            break;
        case JSON_TYPE_STRING:
            if(esc) (*idx)+=_strcpyToJsonEsc(&buf[*idx], value->string);
            else {
                strcpy(&buf[*idx], value->string);
                (*idx)+=strlen(value->string);
            }
            break;
        case JSON_TYPE_INTEGER:
            (*idx)+=sprintf(&buf[*idx], "%lld", value->integer);
            break;
        case JSON_TYPE_NUMERIC:
            (*idx)+=sprintf(&buf[*idx], "%f", value->numeric);
            break;
        default:
            (*idx)+=sprintf(&buf[*idx], "(type error)");
    }
    buf[*idx]='\0';
}

void _fillOptStrBuf(json_t *value, char *buf, int *idx)
{
    int i;
    json_t *arrayPtr, *objectPtr;

    switch(value->type) {
        case JSON_TYPE_STRING:
            buf[(*idx)++]='\"';
            _fillPureValStrBuf(value, true, buf, idx);
            buf[(*idx)++]='\"';
            break;
        case JSON_TYPE_ARRAY:
            buf[(*idx)++]='[';
            arrayPtr=value->list;
            while(arrayPtr) {
                buf[(*idx)++]=' '; // for pretty   XD
                _fillOptStrBuf(arrayPtr, buf, idx);
                buf[(*idx)++]=',';
                arrayPtr=arrayPtr->next;
            }
            if(value->list) (*idx)--; // not empty array, over-write the last comma
            buf[(*idx)++]=' '; // for pretty
            buf[(*idx)++]=']';
            break;
        case JSON_TYPE_OBJECT:
            buf[(*idx)++]='{';
            objectPtr=value->list;
            while(objectPtr) {
                buf[(*idx)++]=' '; // for pretty
                buf[(*idx)++]='\"';
                (*idx)+=sprintf(&buf[*idx], "%s", objectPtr->label);
                buf[(*idx)++]='\"';
                buf[(*idx)++]=':';
                buf[(*idx)++]=' '; // for pretty
                _fillOptStrBuf(objectPtr, buf, idx);
                buf[(*idx)++]=',';
                objectPtr=objectPtr->next;
            }
            if(value->list) (*idx)--; // not empty object, over-write the last comma
            buf[(*idx)++]=' '; // for pretty
            buf[(*idx)++]='}';
            break;
        default:
            _fillPureValStrBuf(value, true, buf, idx);
    }
    buf[*idx]='\0';
}

char *jsonGetString(json_t *value)
{
    char buf[4096];
    char *rval;
    int len = 0;

    if(!value) return NULL;

    if(value->type==JSON_TYPE_ARRAY || value->type==JSON_TYPE_OBJECT) {
        _fillOptStrBuf(value, buf, &len);
    }
    else _fillPureValStrBuf(value, false, buf, &len);

    rval=malloc(len+1);
    strcpy(rval, buf);

    return rval;
}

int jsonListCount(json_t *value)
{
    int count;
    json_t *ptr;

    if(!value) return -1;

    if(value->type==JSON_TYPE_ARRAY || value->type==JSON_TYPE_OBJECT) {
        count=0;
        for(ptr=value->list; ptr!=NULL; ptr=ptr->next) count++;
    }
    return -1;

    return count;
}

inline json_t *_jsonCopy(json_t *value, bool expand)
{
    json_t *rval;

    if(!value) return NULL;

    rval=malloc(sizeof(json_t));
    memcpy(rval, value, sizeof(json_t));

    rval->label=NULL;
    rval->next=NULL;

    if(rval->type==JSON_TYPE_STRING) {
        rval->string=malloc(strlen(value->string)+1);
        strcpy(rval->string, value->string);
    }
    else if(rval->type==JSON_TYPE_ARRAY || rval->type==JSON_TYPE_OBJECT) {
        rval->list=_jsonCopy(value->list, true);
    }

    if(expand) {
        if(value->label) {
            rval->label=malloc(strlen(value->label)+1);
            strcpy(rval->label, value->label);
        }
        if(value->next) rval->next=_jsonCopy(value->next, true);
    }

    return rval;
}

json_t *jsonCopy(json_t *value)
{
    return _jsonCopy(value, false);
}

void jsonFree(json_t *value)
{
    if(!value || value->fixed) return;

    if(value->type==JSON_TYPE_STRING) if(!value->reference) free(value->string);
    else if(value->type==JSON_TYPE_ARRAY|| value->type==JSON_TYPE_OBJECT) {
        if(!value->reference) jsonFree(value->list);
    }

    if(value->next) jsonFree(value->next);
    if(value->label) free(value->label);

    free(value);
}
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "json.h"

// for 3.0
#ifndef NAN
#define NAN    0
#endif

int json_error = 0;
// --

///TODO: When parsing, detect empty array or object, that will cause phrase error.

/* forward reference declaration */
void _SJSNbuildVAL(struct SJSN_SHELL* sh);
char* _SJSNexportVAL(struct SJSN_VA* val, char* buf, int size, int* err);

void _SJSNskipBLANK(struct SJSN_SHELL* sh);
unsigned int _SJSNhashIDX(char* name);

struct SJSN_VB* _SJSNObjMultiQuery_last_ptr=NULL;

// 3.0
bool _jsonFillZero(json_t *dst);
json_t *_getLabeledValue(json_labeled_t *labeled);

char *_skipWhitespace(char **src);
int _getString(char **src, char *buf);
json_t *_buildValue(char **src);

int _strcpyToJsonEsc(char *dest, char *src);

void _fillPureValStrBuf(json_t *value, bool esc, char *buf, int *idx);
void _fillOptStrBuf(json_t *value, char *buf, int *idx);

/* parsing */

// for 3.0

/************************************
 **  #internat# Utility Functions  **
 ************************************/
inline bool _jsonFillZero(json_t *dst)
{
    if(!dst) return false;

    memset(dst, 0, sizeof(json_t));
    return true;
}

inline json_t *_getLabeledValue(json_labeled_t *labeled)
{
    return &labeled->value;
}

/*************************
 **  Filling Functions  **
 *************************/
bool jsonFillNull(json_t *dst)
{
    return _jsonFillZero(dst);  // equals to null
}

bool jsonFillBoolean(json_t *dst, bool value)
{
    if(!_jsonFillZero(dst)) return false;

    dst->type=JSON_TYPE_BOOLEAN;
    dst->boolean=value;

    return true;
}

bool jsonFillString(json_t *dst, char *value)
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

bool jsonFillInteger(json_t *dst, int64_t value)
{
    if(!_jsonFillZero(dst)) return false;

    dst->type=JSON_TYPE_INTEGER;
    dst->integer=value;

    return true;
}

bool jsonFillNumeric(json_t *dst, double value)
{
    if(!_jsonFillZero(dst)) return false;

    dst->type=JSON_TYPE_NUMERIC;
    dst->numeric=value;

    return true;
}

bool jsonAttachArray(json_t *dst, json_t *value)
{
    if(!value || !_jsonFillZero(dst)) return false;

    dst->type=JSON_TYPE_ARRAY;
    dst->list=value;

    return true;
}

bool jsonAttachObject(json_t *dst, json_labeled_t *value)
{
    if(!value || !_jsonFillZero(dst)) return false;

    dst->type=JSON_TYPE_OBJECT;
    dst->list=(json_t *)value;

    return true;
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
    jsonFillNull(rval);

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
    jsonFillBoolean(rval, true);

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
    jsonFillBoolean(rval, false);

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
    if(!jsonFillString(rval, buf)) {
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
    if(numeric) jsonFillNumeric(rval, strtod(buf, NULL));
    else jsonFillInteger(rval, strtoll(buf, NULL, 10));

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
    jsonAttachArray(rval, arrayHead);

    return rval;
}

json_t *_matchObject(char **src)
{
    json_t *rval;
    json_labeled_t *objectHead, *objectTail, *matchedItem;
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

        matchedItem=(json_labeled_t *)_buildValue(src);
        if(matchedItem) {
            len=strlen(buf);
            matchedItem->label=malloc(len+1);
            strcpy(matchedItem->label, buf);
            matchedItem->label[len+1]='\0';

            if(objectHead==NULL) {
                objectHead=matchedItem;
                objectTail=objectHead;
            }
            else {
                objectTail->value.next=(json_t *)matchedItem;
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
    jsonAttachObject(rval, objectHead);

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

json_t *jsonParse(char *str)
{
    _skipWhitespace(&str);
    return _buildValue(&str);
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

int64_t jsonGetInterger(json_t *value)
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
    json_t *arrayPtr;
    json_labeled_t *objectPtr;

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
            objectPtr=(json_labeled_t *)value->list;
            while(objectPtr) {
                buf[(*idx)++]=' '; // for pretty
                buf[(*idx)++]='\"';
                (*idx)+=sprintf(&buf[*idx], "%s", objectPtr->label);
                buf[(*idx)++]='\"';
                buf[(*idx)++]=':';
                buf[(*idx)++]=' '; // for pretty
                _fillOptStrBuf(&objectPtr->value, buf, idx);
                buf[(*idx)++]=',';
                objectPtr=(json_labeled_t *)objectPtr->value.next;
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

// -- (3.0)

inline void _SJSNskipBLANK(struct SJSN_SHELL* sh)
{
    while(!isgraph(sh->str[sh->p_ptr]) && sh->str[sh->p_ptr]!='\0') sh->p_ptr++;
}

char* _SJSNgetSTRING(struct SJSN_SHELL* sh)
{
    int len, t_ptr;
    char* rval;
    
    if(sh->str[sh->p_ptr]!='\"') return NULL;
    else sh->p_ptr++; // move to first character of string
    
    // find tail
    t_ptr=sh->p_ptr;
    
    while(sh->str[t_ptr]!='\"' && sh->str[t_ptr]!='\0') {
        // escape
        if(sh->str[t_ptr]=='\\') {
            t_ptr++; // shift one more character
            if(sh->str[t_ptr]=='\0') return NULL; // an error condition
        }
        
        t_ptr++;
    }
    
    if(sh->str[t_ptr]=='\0') return NULL; // otherwise, t_ptr now on the end of string ... on the right double quote
    
    len=t_ptr-sh->p_ptr; // with out ending NUL character
    rval=malloc(sizeof(char)*(len+1));
    
    ///TODO: handle escape & unicode advanced in the future
    strncpy(rval, &sh->str[sh->p_ptr], len);
    rval[len]='\0';
    
    // move pointer to next of right double quote and return
    sh->p_ptr=t_ptr+1;
    return rval;
}

void _SJSNmatchSTRING(struct SJSN_SHELL* sh)
{
    char* str;
    
    str=_SJSNgetSTRING(sh);
    
    if(!str) {
        sh->error_code=SJSN_SYNTAX_ERROR;
        return;
    }
    
    sh->p_stack[sh->p_level]->type=SJSN_VAL_STRING;
    sh->p_stack[sh->p_level]->data.string=str;
    
    return;
}

// we don't check syntax in matchNUMBER because there might be "123," "123]" "123}" ...
void _SJSNmatchNUMBER(struct SJSN_SHELL* sh)
{
    char num[40];
    char exp[8];
    int real, i, j;
    
    real=0;
    i=0;
    j=0;
    
    // format:
    //  [-][<part0>.<part1>][eE][+-][<part2>]
    
    // both atoi & atof accept signed expression, '-', just shift it to buffer,
    // but there can be only one before part0.
    if(sh->str[sh->p_ptr]) {
        num[i++]=sh->str[sh->p_ptr];
        sh->p_ptr++;
    }
    
    // part0
    while(isdigit(sh->str[sh->p_ptr])) num[i++]=sh->str[sh->p_ptr++];
    
    // '.'
    if(sh->str[sh->p_ptr]=='.') {
        num[i++]=sh->str[sh->p_ptr++];
        
        real=1;
        exp[0]='\0';
    }

    // part1
    while(isdigit(sh->str[sh->p_ptr])) num[i++]=sh->str[sh->p_ptr++];
    
    num[i]='\0';
    
    // [eE]
    if(sh->str[sh->p_ptr]=='e' || sh->str[sh->p_ptr]=='E') {
        // [+-]
        real=1;
        sh->p_ptr++;
        
        if(sh->str[sh->p_ptr]=='-') exp[j++]=sh->str[sh->p_ptr++];
        else if(sh->str[sh->p_ptr]=='+') sh->p_ptr++;
        
        // part2
        while(isdigit(sh->str[sh->p_ptr])) exp[j++]=sh->str[sh->p_ptr++];
        
        exp[j]='\0';
    }
    
    if(real) {
        sh->p_stack[sh->p_level]->type=SJSN_VAL_REALNUM;
#if defined(SJSN_NUM_POWER) && SJSN_NUM_POWER
        sh->p_stack[sh->p_level]->data.realnum=atof(num)*pow(10, atoi(exp)); // atof() prototype returns double
#else
        sh->p_stack[sh->p_level]->data.realnum=atof(num); 
#endif
    }
    else {
        sh->p_stack[sh->p_level]->type=SJSN_VAL_INTEGER;
        sh->p_stack[sh->p_level]->data.integer=atoll(num);
    }
}

void _SJSNmatchOBJECT(struct SJSN_SHELL* sh)
{
    struct SJSN_VB *OPtr, *FOPtr;
    
    if(sh->str[sh->p_ptr]!='{') {
        sh->error_code=SJSN_SYNTAX_ERROR;
        return;
    }
    
    // generate object descriptor node
    OPtr=malloc(sizeof(struct SJSN_VB));
    
    sh->p_stack[sh->p_level]->type=SJSN_VAL_OBJECT;
    sh->p_stack[sh->p_level]->data.object=OPtr;

    sh->p_ptr++;
    
    // prepare 1st obkect element for parsing
    FOPtr=NULL; // latest finished object element
    
    OPtr->type=SJSN_VAL_NULL;
    OPtr->next=NULL;
    
    sh->p_level++;
    sh->p_stack[sh->p_level]=(struct SJSN_VA*)OPtr;
    
    while(1) {
        _SJSNskipBLANK(sh);
        
        OPtr->name=_SJSNgetSTRING(sh);
        if(!OPtr->name) {
            sh->error_code=SJSN_SYNTAX_ERROR;
            return;
        }
        SJSNObjectIdx(OPtr); // generate hash index
        
        _SJSNskipBLANK(sh);
        
        if(sh->str[sh->p_ptr]!=':') {
            sh->error_code=SJSN_SYNTAX_ERROR;
            return;
        }
        sh->p_ptr++;

        _SJSNskipBLANK(sh);

        _SJSNbuildVAL(sh);
        if(sh->error_code==SJSN_OK) {
            // if no error, prepare next object element
            //OPtr->type|=0x80; ///TODO: it's necessary?
            FOPtr=OPtr;
            OPtr->next=malloc(sizeof(struct SJSN_VB));
            OPtr=OPtr->next;
            
            OPtr->type=SJSN_VAL_NULL;
            OPtr->next=NULL;

            sh->p_stack[sh->p_level]=(struct SJSN_VA*)OPtr; // same layer, just next value
        }

        _SJSNskipBLANK(sh);
        
        if(sh->str[sh->p_ptr]!=',') break;
        else sh->p_ptr++;
    }
    
    if(sh->str[sh->p_ptr]=='}') {
        // remove the last node
        if(FOPtr) FOPtr->next=NULL;
        
        free(OPtr);
        
        // array finished, decrease one level
        sh->p_level--;
        
        sh->p_ptr++;
        return;
    }
    else if(sh->str[sh->p_ptr]=='\0') {
        sh->error_code=SJSN_UNEXP_NUL;
        return;
    }
    else {
        sh->error_code=SJSN_SYNTAX_ERROR;
        return;
    }
}

void _SJSNmatchARRAY(struct SJSN_SHELL* sh)
{
    struct SJSN_VA *APtr, *FAPtr;
    
    if(sh->str[sh->p_ptr]!='[') {
        sh->error_code=SJSN_SYNTAX_ERROR;
        return;
    }
    
    // generate first array element
    APtr=malloc(sizeof(struct SJSN_VA));
    
    sh->p_stack[sh->p_level]->type=SJSN_VAL_ARRAY;
    sh->p_stack[sh->p_level]->data.array=APtr;

    sh->p_ptr++;
    
    // prepare 1st array element for parsing
    FAPtr=NULL; // latest finished array element
    
    APtr->type=SJSN_VAL_NULL;
    APtr->next=NULL;
    
    sh->p_level++;
    sh->p_stack[sh->p_level]=APtr;
    
    while(1) {
        _SJSNskipBLANK(sh);
        
        _SJSNbuildVAL(sh);
        if(sh->error_code==SJSN_OK) {
            // if no error, prepare next array element
            FAPtr=APtr;
            APtr->next=malloc(sizeof(struct SJSN_VA));
            APtr=APtr->next;
            
            APtr->type=SJSN_VAL_NULL;
            APtr->next=NULL;
            
            sh->p_stack[sh->p_level]=APtr; // same layer, just next value
        }
        else return;

        _SJSNskipBLANK(sh);
        
        if(sh->str[sh->p_ptr]!=',') break;
        else sh->p_ptr++;
    }
    
    if(sh->str[sh->p_ptr]==']') {
        // remove the last node
        if(FAPtr) FAPtr->next=NULL;
        
        free(APtr);
        
        // array finished, decrease one level
        sh->p_level--;
                
        sh->p_ptr++;
        return;
    }
    else if(sh->str[sh->p_ptr]=='\0') {
        sh->error_code=SJSN_UNEXP_NUL;
        return;
    }
    else {
        sh->error_code=SJSN_SYNTAX_ERROR;
        return;
    }
}

void _SJSNmatchTRUE(struct SJSN_SHELL* sh)
{
        
    if(sh->str[sh->p_ptr]!='t' && sh->str[sh->p_ptr]!='T') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(sh->str[sh->p_ptr]!='r' && sh->str[sh->p_ptr]!='R') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(sh->str[sh->p_ptr]!='u' && sh->str[sh->p_ptr]!='U') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(sh->str[sh->p_ptr]!='e' && sh->str[sh->p_ptr]!='E') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(isalnum(sh->str[sh->p_ptr])) {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_stack[sh->p_level]->type=SJSN_VAL_TRUE;
    
    return;
}

void _SJSNmatchFALSE(struct SJSN_SHELL* sh)
{

    if(sh->str[sh->p_ptr]!='f' && sh->str[sh->p_ptr]!='F') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(sh->str[sh->p_ptr]!='a' && sh->str[sh->p_ptr]!='A') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(sh->str[sh->p_ptr]!='l' && sh->str[sh->p_ptr]!='L') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(sh->str[sh->p_ptr]!='s' && sh->str[sh->p_ptr]!='S') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(sh->str[sh->p_ptr]!='e' && sh->str[sh->p_ptr]!='E') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(isalnum(sh->str[sh->p_ptr])) {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_stack[sh->p_level]->type=SJSN_VAL_FALSE;
    
    return;
}

void _SJSNmatchNULL(struct SJSN_SHELL* sh)
{
    
    if(sh->str[sh->p_ptr]!='n' && sh->str[sh->p_ptr]!='N') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(sh->str[sh->p_ptr]!='u' && sh->str[sh->p_ptr]!='U') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(sh->str[sh->p_ptr]!='l' && sh->str[sh->p_ptr]!='L') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(sh->str[sh->p_ptr]!='l' && sh->str[sh->p_ptr]!='L') {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_ptr+=1;
    if(isalnum(sh->str[sh->p_ptr])) {
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
    
    sh->p_stack[sh->p_level]->type=SJSN_VAL_NULL;
    
    return;
}

void _SJSNbuildVAL(struct SJSN_SHELL* sh)
{
    // assuming this value is null, in case of error
    sh->p_stack[sh->p_level]->type=SJSN_VAL_NULL;
    sh->p_stack[sh->p_level]->next=NULL;
    
    // skip non-graph characters
    _SJSNskipBLANK(sh);
    if(sh->str[sh->p_ptr]=='\0') {
        // unexpected termination
        sh->error_code=SJSN_UNEXP_NUL;
        return;
    }

    if(sh->str[sh->p_ptr]=='\"') _SJSNmatchSTRING(sh); //string
    else if(sh->str[sh->p_ptr]=='-' || isdigit(sh->str[sh->p_ptr])) _SJSNmatchNUMBER(sh); // number
    else if(sh->str[sh->p_ptr]=='{') _SJSNmatchOBJECT(sh); //object
    else if(sh->str[sh->p_ptr]=='[') _SJSNmatchARRAY(sh); // array
    else if(sh->str[sh->p_ptr]=='t' || sh->str[sh->p_ptr]=='T') _SJSNmatchTRUE(sh); // true
    else if(sh->str[sh->p_ptr]=='f' || sh->str[sh->p_ptr]=='F') _SJSNmatchFALSE(sh); // false
    else if(sh->str[sh->p_ptr]=='n' || sh->str[sh->p_ptr]=='N') _SJSNmatchNULL(sh); // null
    else {	// phrase error
        sh->error_code=SJSN_PHRASE_ERROR;
        return;
    }
}

struct SJSN_VA* SJNSParse(struct SJSN_SHELL* sh)
{
    struct SJSN_VA* rval;
    
    /* error hand */
    if(!sh) return NULL;
    if(!sh->str) return NULL;

    /* initial */
    rval=malloc(sizeof(struct SJSN_VA));
    
    sh->error_code=SJSN_OK; // mark for continuous parsing
    sh->p_ptr=0;
    sh->p_level=0;
    sh->p_stack[0]=rval;
    
    _SJSNbuildVAL(sh);
    
    if(sh->error_code==SJSN_OK) {
        // check suffix
        _SJSNskipBLANK(sh);
        if(sh->str[sh->p_ptr]!='\0') {
            // unexpected suffix
            sh->error_code=SJSN_UNEXP_SUFFIX;
            return NULL;
        }
    }

    return rval;
}

struct SJSN_VA* SJNSQuickParse(char* src)
{
    struct SJSN_SHELL shell;
    
    shell.str=src;
    
    return SJNSParse(&shell);
}

/* exporting */

///TODO: escape & unicode
char* _SJSNcapSTRING(char* dst, char* src)
{
    int i,j;
    
    i=0;
    j=0;
    
    dst[i++]='\"';
    
    while(src[j]!='\0') {
        switch (src[j]) {
            case '\n':
                dst[i++]='\\';
                dst[i++]='n';
                break;
            case '\r':
                dst[i++]='\\';
                dst[i++]='r';
                break;
            case '\t':
                dst[i++]='\\';
                dst[i++]='t';
                break;
            case '\"':
            case '\\':
                dst[i++]='\\';
            default:
                dst[i++]=src[j];
        }
        
        j++;
    }
    
    dst[i++]='\"';
    dst[i]='\0';
    
    return &dst[i];
}

char* _SJSNexportOBJ(struct SJSN_VB* obj, char* buf, int size, int* err)
{
    char* tbuf;
    
    tbuf=buf;
    
    *(tbuf++)='{';
    
    while(obj) {
        tbuf=_SJSNcapSTRING(tbuf, obj->name);
        *(tbuf++)=':';
        
        tbuf=_SJSNexportVAL((struct SJSN_VA*)obj, tbuf, size-(int)(tbuf-buf), err);
        *(tbuf++)=',';
        
        // error return
        if(*err!=SJSN_OK) return tbuf;
        
        obj=obj->next;
    }
    
    if(*(tbuf-1)==',') tbuf--;
    
    *(tbuf++)='}';
    *tbuf='\0';
    
    return tbuf;
}

char* _SJSNexportARR(struct SJSN_VA* arr, char* buf, int size, int* err)
{
    char* tbuf;
    
    tbuf=buf;
    
    *(tbuf++)='[';
    
    while(arr) {
        tbuf=_SJSNexportVAL(arr, tbuf, size-(int)(tbuf-buf), err);
        *(tbuf++)=',';
        
        // error return
        if(*err!=SJSN_OK) return tbuf;
        
        arr=arr->next;
    }
    
    if(*(tbuf-1)==',') tbuf--;
    
    *(tbuf++)=']';
    *tbuf='\0';
    
    return tbuf;
}

char* _SJSNexportVAL(struct SJSN_VA* val, char* buf, int size, int* err)
{	
    switch(val->type) {
        case SJSN_VAL_NULL:
            if(size<5) {
                *err=SJSN_OUT_OF_BUF;
                return buf;
            }
            
            strcpy(buf, "null");			
            break;
        case SJSN_VAL_FALSE:
            if(size<6) {
                *err=SJSN_OUT_OF_BUF;
                return buf;
            }
            
            strcpy(buf, "false");
            break;
        case SJSN_VAL_TRUE:
            if(size<5) {
                *err=SJSN_OUT_OF_BUF;
                return buf;
            }
            
            strcpy(buf, "true");			
            break;
        case SJSN_VAL_OBJECT:
            if(size<3) {
                *err=SJSN_OUT_OF_BUF;
                return buf;
            }
            
            return _SJSNexportOBJ(val->data.object, buf, size, err);
        case SJSN_VAL_ARRAY:
            if(size<3) {
                *err=SJSN_OUT_OF_BUF;
                return buf;
            }
            
            return _SJSNexportARR(val->data.array, buf, size, err);
        case SJSN_VAL_REALNUM:
            ///TODO: digit accuracy
            if(size<16) {
                *err=SJSN_OUT_OF_BUF;
                return buf;
            }
            
            sprintf(buf, "%lf", val->data.realnum);			
            break;
        case SJSN_VAL_INTEGER:
            if(size<33) {
                *err=SJSN_OUT_OF_BUF;
                return buf;
            }
            
            //itoa(val->res.integer, buf, 10);
            sprintf(buf, "%ld", val->data.integer);
            break;
        case SJSN_VAL_STRING:
            // add some overhead for escape 
            if(size < strlen(val->data.string)+20) {
                *err=SJSN_OUT_OF_BUF;
                return buf;
            }
            
            return _SJSNcapSTRING(buf, val->data.string);
        default:
            *err=SJSN_EXPORT_TYPE;
            return buf;
    }
    
    // for others, find tail and return
    while(*buf!='\0') buf++;
    
    return buf;
}

int SJSNExport(struct SJSN_VA* val, char* buf, int size)
{
    int err;
    
    err=SJSN_OK;
    _SJSNexportVAL(val, buf, size, &err);
    
    return err;
}

/* free, memory recycle */

void _SJSNfreeOBJECT(struct SJSN_VB* obj)
{
    struct SJSN_VB* temp;
    
    while(obj) {
        SJSNFree(obj->data.object);
        free(obj->name);
        
        temp=obj;
        obj=obj->next;
        
        free(temp);
    }
}

void _SJSNfreeARRAY(struct SJSN_VA* arr)
{
    struct SJSN_VA* temp;
    
    while(arr) {
        SJSNFree(arr->data.array);
        
        temp=arr;
        arr=arr->next;
        
        free(temp);
    }
}

void SJSNFree(void* val)
{
    struct SJSN_VA *tval;
    
    if(!val) return;
    
    tval=(struct SJSN_VA*)val;
    
    switch(tval->type) {
        case SJSN_VAL_OBJECT:
            _SJSNfreeOBJECT((struct SJSN_VB*)tval->data.object);
            break;
        case SJSN_VAL_ARRAY:
            _SJSNfreeARRAY(tval->data.array);
            break;
        case SJSN_VAL_STRING:
            free(tval->data.string);
            break;
    }
    
    free(val);
    
    return;
}

inline unsigned int _SJSNhashIDX(char* name)
{
    unsigned char* rp; 
    unsigned int rval;
    int i;
    
    rval=0xe3a1c280;
    rp=(unsigned char*)&rval;
    
    for(i=0; name[i]!='\0'; i++) {
        rp[i%4]^=name[i];
    }
    
    return rval;
}

void SJSNObjectIdx(struct SJSN_VB* obj)
{
    if(!obj) return;
    if(!obj->name) return;
    
    obj->hidx=_SJSNhashIDX(obj->name);
    return;
}

#if defined(SJSN_ENABLE_QUERY) && SJSN_ENABLE_QUERY

struct SJSN_VA* SJSNQuery(struct SJSN_VA* val, char* path, int mode)
{
    char nbf[128];
    unsigned int i;
    uint8_t array_closure;
    
    struct SJSN_VA *aptr;
    struct SJSN_VB *optr, *optr_p;
    
    if( !val || !path ) return NULL;
    
    i=0;
    array_closure=0;
    
    while(1) {
        if(*path=='.' || *path=='\0' || *path=='[') {
            if(array_closure) {
                array_closure=0;

                if(*path=='\0') break; // finish, and return val
                else { // skip '.' or ']' and prepare for next
                    i=0;
                    path++;
                    continue;
                }
            }

            nbf[i]='\0';
            
            // data process
            if(val->type==SJSN_VAL_OBJECT) {
                optr=val->data.object;
                i=_SJSNhashIDX(nbf);
                
                optr_p=NULL;
                while(optr) {
                    if(optr->hidx==i) break;
                    optr_p=optr;
                    optr=optr->next;
                }
                
                if(!optr) {
                    if(mode) {
                        optr_p->next=malloc(sizeof(struct SJSN_VB));
                        
                        optr=optr_p->next;
                        optr->name=malloc(strlen(nbf));
                        strcpy(optr->name, nbf);
                        optr->hidx=i;
                        
                        optr->type=SJSN_VAL_NULL;
                        optr->next=NULL;
                    }
                    else return NULL;
                }
                
                val=(struct SJSN_VA*)optr;
            }
            else if(val->type==SJSN_VAL_NULL && mode) {
                ///TODO: detect type (array/object)?
                
                val->type=SJSN_VAL_OBJECT;
                val->data.object=malloc(sizeof(struct SJSN_VB));
                
                optr=val->data.object;
                
                optr->name=malloc(strlen(nbf));
                strcpy(optr->name, nbf);
                optr->hidx=_SJSNhashIDX(nbf);
                optr->type=SJSN_VAL_NULL;
                optr->next=NULL;
                
                val=(struct SJSN_VA*)optr;
            }
            else return NULL;   // invalid type 
            
            if(*path=='\0') break; // finish, and return val
            else { // skip '.' or ']' and prepare for next
                i=0;
                path++;
                continue;
            }
        }
        else if(*path==']') {
            if(val->type==SJSN_VAL_ARRAY) {
                nbf[i]='\0';

                aptr=val->data.array;

                ///TODO: if wrong digit format
                for(i=atoi(nbf); i>0; i--) {
                    if(aptr->next) aptr=aptr->next;
                    else if(mode) {
                        aptr->next=malloc(sizeof(struct SJSN_VA));
                        aptr=aptr->next;
                        
                        aptr->type=SJSN_VAL_NULL;
                        aptr->next=NULL;
                    }
                    else return NULL;
                }
                
                val=aptr;
                array_closure=1;
                i=0;
            }
            else return NULL;   // invalid type 
        }
        else if(*path=='\\') {
            path++; // skip one character
            ///TODO: escape, unicode
        }
        
        nbf[i++]=*path;
        path++;
    }
    
    return val;
}

struct SJSN_VA* SJSNObjMultiQuery(struct SJSN_VA* val, char* id)
{
    struct SJSN_VB *optr;
    //struct SJSN_VA *rval;
    unsigned int i;
    
    if(!val) {
        if(!_SJSNObjMultiQuery_last_ptr) return NULL;
        else optr=_SJSNObjMultiQuery_last_ptr;
    }
    else {
        if(val->type!=SJSN_VAL_OBJECT) return NULL;
        else optr=val->data.object;
    }
    
    i=_SJSNhashIDX(id);
    
    while(optr) {
        if(optr->hidx==i) return (struct SJSN_VA*)optr;
        else optr=optr->next;
    }
    
    return NULL;
}

#endif

unsigned int SJSNhidx(char* str)
{
    return _SJSNhashIDX(str);
}

/*struct SJSN_VAL* SJSNArrayInsert(struct SJSN_ARR **arrd)
{
    if(!*arrd) {
        *arrd=malloc(sizeof(struct SJSN_ARR));
        
    }
}

struct SJSN_VAL* SJSNObjectInsert(struct SJSN_OBJ **objd, char *name)
{
    
}*/
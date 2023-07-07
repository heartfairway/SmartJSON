/******
* JSON Parser & Exporter
* 
* by. Cory Chiang
* 
*   V. 2.2.0 (2020/11/19)
*

BSD 3-Clause License

Copyright (c) 2020, Cory Chiang
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

///TODO: When parsing, detect empty array or object, that will cause phrase error.

/* forward reference declaration */
void _SJSNbuildVAL(struct SJSN_SHELL* sh);
char* _SJSNexportVAL(struct SJSN_VA* val, char* buf, int size, int* err);

void _SJSNskipBLANK(struct SJSN_SHELL* sh);
unsigned int _SJSNhashIDX(char* name);

struct SJSN_VB* _SJSNObjMultiQuery_last_ptr=NULL;

/* parsing */
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
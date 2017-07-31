/******
* JSON Parser & Exporter   (code Stella)
* 
* by. Cory Chiang
* 
******/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "json.h"

/* forward reference declaration */
void _SJSNbuildVAL(struct SJSN_SHELL* sh);
char* _SJSNexportVAL(struct SJSN_VAL* val, char* buf, int size, int* err);

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
	sh->p_stack[sh->p_level]->res.string=str;
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
		sh->p_stack[sh->p_level]->res.realnum=atof(num)*pow(10, atoi(exp)); // atof() prototype returns double
	}
	else {
		sh->p_stack[sh->p_level]->type=SJSN_VAL_INTEGER;
		sh->p_stack[sh->p_level]->res.integer=atoi(num);
	}
}

void _SJSNmatchOBJECT(struct SJSN_SHELL* sh)
{
	struct SJSN_VAL* VPtr;
	struct SJSN_OBJ* OPtr;
	struct SJSN_OBJ* FOPtr;
	
	if(sh->str[sh->p_ptr]!='{') {
		sh->error_code=SJSN_SYNTAX_ERROR;
		return;
	}
	
	// generate object descriptor node
	VPtr=sh->p_stack[sh->p_level];
	OPtr=malloc(sizeof(struct SJSN_OBJ));
	
	VPtr->type=SJSN_VAL_OBJECT;
	VPtr->res.object=OPtr;

	sh->p_ptr++;
	
	// prepare 1st obkect element for parsing
	FOPtr=NULL;
	
	OPtr->value=malloc(sizeof(struct SJSN_VAL));
	OPtr->value->type=SJSN_VAL_NULL;
	OPtr->next=NULL;
	
	sh->p_level++;
	sh->p_stack[sh->p_level]=OPtr->value;
	
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
			FOPtr=OPtr;
			OPtr->next=malloc(sizeof(struct SJSN_OBJ));
			OPtr=OPtr->next;
			
			OPtr->value=malloc(sizeof(struct SJSN_VAL));
			OPtr->value->type=SJSN_VAL_NULL;
			OPtr->next=NULL;

			sh->p_stack[sh->p_level]=OPtr->value; // same layer, just next value
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
	struct SJSN_VAL* VPtr;
	struct SJSN_ARR* APtr;
	struct SJSN_ARR* FAPtr;
	
	if(sh->str[sh->p_ptr]!='[') {
		sh->error_code=SJSN_SYNTAX_ERROR;
		return;
	}
	
	// generate array descriptor node
	VPtr=sh->p_stack[sh->p_level];
	APtr=malloc(sizeof(struct SJSN_ARR));
	
	VPtr->type=SJSN_VAL_ARRAY;
	VPtr->res.array=APtr;

	sh->p_ptr++;
	
	// prepare 1st array element for parsing
	FAPtr=NULL;
	
	APtr->value=malloc(sizeof(struct SJSN_VAL));
	APtr->value->type=SJSN_VAL_NULL;
	APtr->next=NULL;
	
	sh->p_level++;
	sh->p_stack[sh->p_level]=APtr->value;
	
	while(1) {
		_SJSNskipBLANK(sh);
		
		_SJSNbuildVAL(sh);
		if(sh->error_code==SJSN_OK) {
			// if no error, prepare next array element
			FAPtr=APtr;
			APtr->next=malloc(sizeof(struct SJSN_ARR));
			APtr=APtr->next;
			
			APtr->value=malloc(sizeof(struct SJSN_VAL));
			APtr->value->type=SJSN_VAL_NULL;
			APtr->next=NULL;

			sh->p_stack[sh->p_level]=APtr->value; // same layer, just next value
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
	else {
		// phrase error
		sh->error_code=SJSN_PHRASE_ERROR;
		return;
	}
}

struct SJSN_VAL* SJNSParse(struct SJSN_SHELL* sh)
{
	struct SJSN_VAL* rval;
	
	/* error hand */
	if(!sh) return NULL;
	if(!sh->str) return NULL;

	/* initial */
	rval=malloc(sizeof(struct SJSN_VAL));
	
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
			return;
		}
	}

	return rval;
}

struct SJSN_VAL* SJNSQuickParse(char* src)
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

char* _SJSNexportOBJ(struct SJSN_OBJ* obj, char* buf, int size, int* err)
{
	char* tbuf;
	
	tbuf=buf;
	
	*(tbuf++)='{';
	
	while(obj) {
		tbuf=_SJSNcapSTRING(tbuf, obj->name);
		*(tbuf++)=':';
		
		tbuf=_SJSNexportVAL(obj->value, tbuf, size-(int)(tbuf-buf), err);
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

char* _SJSNexportARR(struct SJSN_ARR* arr, char* buf, int size, int* err)
{
	char* tbuf;
	
	tbuf=buf;
	
	*(tbuf++)='[';
	
	while(arr) {
		tbuf=_SJSNexportVAL(arr->value, tbuf, size-(int)(tbuf-buf), err);
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

char* _SJSNexportVAL(struct SJSN_VAL* val, char* buf, int size, int* err)
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
			
			return _SJSNexportOBJ(val->res.object, buf, size, err);
		case SJSN_VAL_ARRAY:
			if(size<3) {
				*err=SJSN_OUT_OF_BUF;
				return buf;
			}
			
			return _SJSNexportARR(val->res.array, buf, size, err);
		case SJSN_VAL_REALNUM:
			///TODO: digit accuracy
			if(size<16) {
				*err=SJSN_OUT_OF_BUF;
				return buf;
			}
			
			sprintf(buf, "%lf", val->res.realnum);			
			break;
		case SJSN_VAL_INTEGER:
			if(size<33) {
				*err=SJSN_OUT_OF_BUF;
				return buf;
			}
			
			//itoa(val->res.integer, buf, 10);
			sprintf(buf, "%d", val->res.integer);
			break;
		case SJSN_VAL_STRING:
			// add some overhead for escape 
			if(size < strlen(val->res.string)+20) {
				*err=SJSN_OUT_OF_BUF;
				return buf;
			}
			
			return _SJSNcapSTRING(buf, val->res.string);
		default:
			*err=SJSN_EXPORT_TYPE;
			return buf;
	}
	
	// for others, find tail and return
	while(*buf!='\0') buf++;
	
	return buf;
}

int SJSNExport(struct SJSN_VAL* val, char* buf, int size)
{
	int err;
	
	err=SJSN_OK;
	_SJSNexportVAL(val, buf, size, &err);
	
	return err;
}

/* free, memory recycle */

void _SJSNfreeOBJECT(struct SJSN_OBJ* obj)
{
	struct SJSN_OBJ* temp;
	
	while(obj) {
		SJSNFree(obj->value);
		free(obj->name);
		
		temp=obj;
		obj=obj->next;
		
		free(temp);
	}
}

void _SJSNfreeARRAY(struct SJSN_ARR* arr)
{
	struct SJSN_ARR* temp;
	
	while(arr) {
		SJSNFree(arr->value);
		
		temp=arr;
		arr=arr->next;
		
		free(temp);
	}
}

void SJSNFree(struct SJSN_VAL* val)
{
	if(!val) return;
	
	switch(val->type) {
		case SJSN_VAL_OBJECT:
			_SJSNfreeOBJECT(val->res.object);
			break;
		case SJSN_VAL_ARRAY:
			_SJSNfreeARRAY(val->res.array);
			break;
		case SJSN_VAL_STRING:
			free(val->res.string);
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

void SJSNObjectIdx(struct SJSN_OBJ* obj)
{
	if(!obj) return;
	if(!obj->name) return;
	
	obj->hidx=_SJSNhashIDX(obj->name);
	return;
}

struct SJSN_VAL* SJSNQuery(struct SJSN_VAL* val, char* path, int mode)
{
	char nbf[128];
	unsigned int i;

	struct SJSN_ARR *aptr;
	struct SJSN_OBJ *optr, *optr_p;
	
	i=0;
	
	while(1) {
		if(*path=='.' || *path=='\0') {
			nbf[i]='\0';
			
			// data process
			if(val->type==SJSN_VAL_OBJECT) {
				optr=val->res.object;
				i=_SJSNhashIDX(nbf);
				
				optr_p=NULL;
				while(optr) {
					if(optr->hidx==i) break;
					optr_p=optr;
					optr=optr->next;
				}
				
				if(optr) val=optr->value;
				else if(mode) {
					optr_p->next=malloc(sizeof(struct SJSN_OBJ));
					
					optr=optr_p->next;
					optr->name=malloc(strlen(nbf));
					strcpy(optr->name, nbf);
					optr->hidx=i;
					optr->value=malloc(sizeof(struct SJSN_VAL));
					optr->value->type=SJSN_VAL_NULL;
					optr->next=NULL;
				}
				else return NULL;
				
				val=optr->value;
			}
			else if(val->type==SJSN_VAL_ARRAY) {
				aptr=val->res.array;
				
				///TODO: if wrong digit format
				for(i=atoi(nbf); i>0; i--) {
					if(aptr->next) aptr=aptr->next;
					else if(mode) {
						aptr->next=malloc(sizeof(struct SJSN_ARR));
						aptr=aptr->next;
						
						aptr->value=malloc(sizeof(struct SJSN_VAL));
						aptr->value->type=SJSN_VAL_NULL;
						aptr->next=NULL;
					}
					else return NULL;
				}
				
				val=aptr->value;
			}
			else if(val->type==SJSN_VAL_NULL && mode) {
				///TODO: detect type (array/object)?
				
				val->type=SJSN_VAL_OBJECT;
				val->res.object=malloc(sizeof(struct SJSN_OBJ));
				
				optr=val->res.object;
				
				optr->name=malloc(strlen(nbf));
				strcpy(optr->name, nbf);
				optr->hidx=_SJSNhashIDX(nbf);
				optr->value=malloc(sizeof(struct SJSN_VAL));
				optr->value->type=SJSN_VAL_NULL;
				optr->next=NULL;
				
				val=optr->value;
			}
			else {
				// invalid type !!
				return NULL;
			}
			
			if(*path=='\0') break; // exit loop
			else {
				// skip '.' and prepare for next
				i=0;
				path++;
				continue;
			}
		}
		
		if(*path=='\\') {
			path++; // skip one character
			///TODO: escape, unicode
		}
		
		nbf[i++]=*path;
		path++;
	}
	
	return val;
}

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
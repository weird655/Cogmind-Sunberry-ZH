#ifndef SUNBERRY_TERMINAL_INPUT_H
#define SUNBERRY_TERMINAL_INPUT_H
#include "text_core.h"
#include <string.h>
#define INPUT_ROWS 256
#define INPUT_TEXT 768
#define INPUT_NATIVE 256
typedef struct {char alias[INPUT_TEXT],native[INPUT_NATIVE];uint64_t target;} InputRow;
typedef struct {uint64_t scope;unsigned count;InputRow rows[INPUT_ROWS];} InputRegistry;
/* Exact registered terms only. Fullwidth punctuation is an explicit spelling
   variant; inner whitespace, words and parameters are never guessed. */
static int input_normalize(const char *s,size_t n,char *out,size_t cap){
 if(!s||!out||!cap||n>=INPUT_TEXT)return 0;char temp[INPUT_TEXT];size_t i=0,at=0;
 while(i<n){size_t used;uint32_t cp;if(!zh_decode(s+i,n-i,&used,&cp)||cp<32||cp==127)return 0;
  if(cp==0xff08)cp='(';else if(cp==0xff09)cp=')';else if(cp==0x201c||cp==0x201d)cp='"';else if(cp==0x3000)cp=' ';
  if(cp<128)temp[at++]=(char)cp;else{memcpy(temp+at,s+i,used);at+=used;}i+=used;
 }
 size_t begin=0;while(begin<at&&temp[begin]==' ')begin++;while(at>begin&&temp[at-1]==' ')at--;
 if(at==begin||at-begin>=cap)return 0;memcpy(out,temp+begin,at-begin);out[at-begin]=0;return 1;
}
static int input_register(InputRegistry *r,const char *alias,const char *native,uint64_t target){
 if(!r||!alias||!native||r->count>=INPUT_ROWS||!target||!native[0]||strlen(native)>=INPUT_NATIVE)return 0;
 for(const unsigned char *p=(const void*)native;*p;p++)if(*p<32||*p>126)return 0;
 InputRow row={0};if(!input_normalize(alias,strlen(alias),row.alias,sizeof row.alias))return 0;
 strcpy(row.native,native);row.target=target;
 for(unsigned i=0;i<r->count;i++)if(!strcmp(r->rows[i].alias,row.alias)&&r->rows[i].target==target&&!strcmp(r->rows[i].native,native))return 1;
 r->rows[r->count++]=row;return 1;
}
/* 1 unique, 0 unknown, -2 ambiguous, -3 stale, -1 capacity/invalid.
   Failure never changes the caller's command buffer. */
static int input_resolve(const InputRegistry *r,uint64_t scope,const char *text,char *out,size_t cap){
 if(!r||!text||!out||!cap)return -1;if(!scope||scope!=r->scope)return -3;
 char key[INPUT_TEXT];if(!input_normalize(text,strlen(text),key,sizeof key))return -1;
 const InputRow *chosen=NULL;
 for(unsigned i=0;i<r->count;i++)if(!strcmp(key,r->rows[i].alias)){
  if(chosen&&(chosen->target!=r->rows[i].target||strcmp(chosen->native,r->rows[i].native)))return -2;chosen=&r->rows[i];
 }
 if(!chosen)return 0;size_t bytes=strlen(chosen->native);if(bytes>=cap)return -1;memcpy(out,chosen->native,bytes+1);return 1;
}
/* Candidates are discoverability only; submission still requires exact resolution.
   Prefix first, then substring. Ambiguous aliases and stale scopes stay hidden. */
static inline unsigned input_suggest(const InputRegistry *r,uint64_t scope,const char *query,unsigned *indices,unsigned capacity){
 if(!r||!indices||!capacity||!scope||r->scope!=scope||!query)return 0;
 char key[INPUT_TEXT],native[INPUT_NATIVE];if(!input_normalize(query,strlen(query),key,sizeof key))return 0;
 unsigned count=0;size_t length=strlen(key);
 for(unsigned pass=0;pass<2;pass++)for(unsigned i=0;i<r->count&&count<capacity;i++){
  const char *match=strstr(r->rows[i].alias,key);if(!match||(pass==0?match!=r->rows[i].alias:match==r->rows[i].alias))continue;
  if(length>strlen(r->rows[i].alias)||input_resolve(r,scope,r->rows[i].alias,native,sizeof native)!=1)continue;
  int duplicate=0;for(unsigned j=0;j<count;j++)if(!strcmp(r->rows[indices[j]].alias,r->rows[i].alias))duplicate=1;
  if(!duplicate)indices[count++]=i;
 }
 return count;
}
#endif

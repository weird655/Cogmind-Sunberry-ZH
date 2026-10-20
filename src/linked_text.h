#ifndef COGMIND_LINKED_TEXT_H
#define COGMIND_LINKED_TEXT_H
#include <stdlib.h>
#include <string.h>
#include "text_core.h"
#define LINKED_TEXT_LIMIT 16384u
#define LINKED_COUNT_LIMIT 32u
typedef struct {size_t begin,end;} LinkedSpan;
/* An existing native command marker is literal text, never an executable hook.
   Only this exact grammar is supported; every stage must retain the same IDs. */
static size_t linked_literal_size(const char *s,size_t n){
 if(n<9||memcmp(s,"<mhack=",7))return 0;size_t end=7;
 while(end<n&&end<71&&((s[end]>='A'&&s[end]<='Z')||(s[end]>='0'&&s[end]<='9')||s[end]=='_'))end++;
 return end>7&&end<n&&s[end]=='>'?end+1:0;
}
static int linked_literals_equal(const char *a,size_t an,const char *b,size_t bn){
 if(!a||!b||an>LINKED_TEXT_LIMIT||bn>LINKED_TEXT_LIMIT)return 0;size_t x=0,y=0;
 for(;;){
  while(x<an&&a[x]!='<')x++;while(y<bn&&b[y]!='<')y++;
  if(x==an||y==bn)return x==an&&y==bn;
  size_t al=linked_literal_size(a+x,an-x),bl=linked_literal_size(b+y,bn-y);
  if(!al||al!=bl||memcmp(a+x,b+y,al))return 0;x+=al;y+=bl;
 }
}

/* Reject control/format syntax; LF is permitted only outside bracket spans.
   ASCII source labels may have a single leading native seen marker '!'. */
static int linked_spans(const char *s,size_t n,int ascii_labels,LinkedSpan spans[LINKED_COUNT_LIMIT],size_t *count){
 if(!s||n>LINKED_TEXT_LIMIT)return 0;size_t at=0,open=0,k=0;int inside=0;
 while(at<n){size_t used;uint32_t cp;if(!zh_decode(s+at,n-at,&used,&cp))return 0;
  if(cp=='<'){size_t literal=linked_literal_size(s+at,n-at);if(inside||!literal)return 0;at+=literal;continue;}
  if((cp<32&&cp!='\n')||cp==127||cp=='`'||cp=='<'||cp=='>'||cp=='{'||cp=='}')return 0;
  if(cp=='['){if(inside||k>=LINKED_COUNT_LIMIT)return 0;inside=1;open=at;}
  else if(cp==']'){
   if(!inside||at==open+1)return 0;
   if(ascii_labels&&at==open+2&&s[open+1]=='!')return 0;
   spans[k].begin=open;spans[k++].end=at+1;inside=0;
  }else if(inside&&(cp=='\n'||(ascii_labels&&cp>126)))return 0;
  at+=used;
 }
 if(inside)return 0;*count=k;return 1;
}
/* Recover brackets omitted by a reviewed translation only when every native
   link label has one exact dictionary translation occurring once, in order.
   Ambiguous prose, partial markup and unknown labels remain untouched. */
static inline int linked_target_restore(const char *raw,size_t rn,const char *target,size_t tn,size_t expected,const ZhDictionary *const *dicts,size_t nd,char *out,size_t cap){
 LinkedSpan source[LINKED_COUNT_LIMIT],marked[LINKED_COUNT_LIMIT],found[LINKED_COUNT_LIMIT];size_t ns,nt;
 if(!linked_literals_equal(raw,rn,target,tn))return 0;
 if(!out||!expected||!linked_spans(raw,rn,1,source,&ns)||ns!=expected||!linked_spans(target,tn,0,marked,&nt))return 0;
 if(nt==expected){if(tn>=cap)return -1;memmove(out,target,tn);out[tn]=0;return 1;}
 if(nt)return 0;size_t previous=0;
 char *text=malloc(tn+1);if(!text)return -1;memcpy(text,target,tn);text[tn]=0;
 for(size_t i=0;i<expected;i++){
  size_t begin=source[i].begin+1,end=source[i].end-1;if(raw[begin]=='!')begin++;
  char label[1024];int match=0;
  for(size_t d=0;d<nd&&!match;d++)if(dicts[d])match=zh_translate_exact(dicts[d],raw+begin,end-begin,label,sizeof label,NULL);
  char *at=NULL;size_t length=0;
  if(match==1&&label[0]&&!strpbrk(label,"[]\n\r`")){
   at=strstr(text,label);length=strlen(label);
   if(at&&(strstr(at+1,label)||(size_t)(at-text)<previous))at=NULL;
  }
  /* A reviewed paragraph may retain the original technical term. Accept it
     only once, at ASCII word boundaries; never guess a translated synonym. */
  if(!at){
   length=end-begin;if(!length||length>=sizeof label){free(text);return 0;}
   memcpy(label,raw+begin,length);label[length]=0;at=strstr(text,label);
   if(!at||strstr(at+1,label)||(size_t)(at-text)<previous){free(text);return 0;}
   unsigned char left=at==text?0:(unsigned char)at[-1],right=(unsigned char)at[length];
   if((left>='A'&&left<='Z')||(left>='a'&&left<='z')||(left>='0'&&left<='9')||left=='_'||
      (right>='A'&&right<='Z')||(right>='a'&&right<='z')||(right>='0'&&right<='9')||right=='_'){free(text);return 0;}
  }
  found[i].begin=(size_t)(at-text);found[i].end=found[i].begin+length;previous=found[i].end;
 }
 size_t size=tn+expected*2;if(size>=cap||size>LINKED_TEXT_LIMIT){free(text);return -1;}
 char *stage=malloc(size+1);if(!stage){free(text);return -1;}size_t at=0,pos=0;
 for(size_t i=0;i<expected;i++){
  size_t n=found[i].begin-pos;memcpy(stage+at,text+pos,n);at+=n;stage[at++]='[';
  n=found[i].end-found[i].begin;memcpy(stage+at,text+found[i].begin,n);at+=n;stage[at++]=']';pos=found[i].end;
 }
 memcpy(stage+at,text+pos,tn-pos);stage[size]=0;memcpy(out,stage,size+1);free(stage);free(text);return 1;
}
/* Return 1 success, 0 unsupported/malformed, -1 output capacity. No failure
   changes out. Target bracket labels may be Chinese; source labels stay ASCII.
   Raw and processed link order is validated, target labels map by ordinal. */
static int linked_text_prepare(const char *raw,size_t raw_length,const char *processed,size_t processed_length,const char *target,size_t target_length,size_t expected,int columns,char *out,size_t capacity){
 LinkedSpan a[LINKED_COUNT_LIMIT],b[LINKED_COUNT_LIMIT],c[LINKED_COUNT_LIMIT];size_t na,nb,nc;
 if(!out||columns<1||columns>512||!expected||expected>LINKED_COUNT_LIMIT)return 0;
 if(!linked_literals_equal(raw,raw_length,processed,processed_length)||!linked_literals_equal(raw,raw_length,target,target_length))return 0;
 if(!linked_spans(raw,raw_length,1,a,&na)||!linked_spans(processed,processed_length,1,b,&nb)||!linked_spans(target,target_length,0,c,&nc)||na!=expected||nb!=expected||nc!=expected)return 0;
 size_t total=target_length;
 for(size_t i=0;i<expected;i++){
  size_t ab=a[i].begin+1,bb=b[i].begin+1,ae=a[i].end-1,be=b[i].end-1;
  if(raw[ab]=='!')ab++;if(processed[bb]=='!')bb++;
  if(ae-ab!=be-bb||memcmp(raw+ab,processed+bb,ae-ab))return 0;
  size_t length=b[i].end-b[i].begin;if(length>(size_t)columns)return 0;
  total-=c[i].end-c[i].begin;total+=length;
 }
 if(total>LINKED_TEXT_LIMIT||total>=capacity)return -1;
 char *temporary=malloc(total+1);if(!temporary)return -1;size_t pos=0,written=0;
 for(size_t i=0;i<expected;i++){
  size_t n=c[i].begin-pos;memcpy(temporary+written,target+pos,n);written+=n;
  n=b[i].end-b[i].begin;memcpy(temporary+written,processed+b[i].begin,n);written+=n;pos=c[i].end;
 }
 memcpy(temporary+written,target+pos,target_length-pos);temporary[total]=0;
 memcpy(out,temporary,total+1);free(temporary);return 1;
}
/* Independent native glyph plan: CJK=2, ASCII=1; complete bracket spans are
   indivisible ASCII atoms. Keeps normal ASCII words together when they fit.
   1 success, 0 malformed/unsupported glyph, -1 layout/allocation failure.
   Only commits out on success, matching zh_plan ownership conventions. */
static int linked_plan_atomic(const char *text,size_t length,int columns,int rows,ZhPlan *out){
 LinkedSpan spans[LINKED_COUNT_LIMIT];size_t count;
 if(!out||columns<1||columns>512||rows<1||rows>512||!linked_spans(text,length,1,spans,&count))return 0;
 for(size_t i=0;i<count;i++)if(spans[i].end-spans[i].begin>(size_t)columns)return -1;
 ZhPlan p={0};p.runs=calloc(length+1,sizeof *p.runs);if(!p.runs)return -1;
 p.columns=columns;p.rows=rows;p.style_count=1;int x=0,y=0;size_t at=0,span=0;
 while(at<length){
  if(span<count&&at==spans[span].begin){
   size_t size=spans[span].end-at;if(x+(int)size>columns){x=0;y++;}if(y>=rows)goto overflow;
   for(size_t i=0;i<size;i++){ZhRun *r=&p.runs[p.count++];r->scalar=(unsigned char)text[at+i];r->token[0]=r->scalar;r->x=x++;r->y=y;r->width=1;}
   at+=size;span++;continue;
  }
  size_t used;uint32_t cp;if(!zh_decode(text+at,length-at,&used,&cp))goto invalid;
  if(cp=='\n'){x=0;y++;at+=used;if(y>=rows&&at<length)goto overflow;continue;}
  if(cp>=33&&cp<127&&(!at||text[at-1]==' '||text[at-1]=='\n')){
   size_t end=at;while(end<length&&(unsigned char)text[end]>=33&&(unsigned char)text[end]<127&&text[end]!='['&&text[end]!=']')end++;
   size_t word=end-at;if(word<=(size_t)columns&&x+(int)word>columns){x=0;y++;}
  }
  int width=cp<128?1:2,index=cp<128?-1:zh_glyph(cp);if(cp>=128&&index<0)goto invalid;
  if(width>columns)goto overflow;if(x+width>columns){x=0;y++;}if(y>=rows)goto overflow;
  ZhRun *r=&p.runs[p.count++];r->scalar=cp;r->x=x;r->y=y;r->width=width;r->token[0]=cp<128?cp:320+index*2;r->token[1]=width==2?r->token[0]+1:0;x+=width;at+=used;
 }
 p.used_rows=p.count?y+1:0;*out=p;return 1;
overflow:zh_plan_free(&p);return -1;
invalid:zh_plan_free(&p);return 0;
}
#endif

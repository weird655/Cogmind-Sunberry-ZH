#ifndef COGMIND_EVENT_DYNAMIC_H
#define COGMIND_EVENT_DYNAMIC_H
/* Reviewed complete native event scaffolds. Original records remain untouched.
   Parameters are typed; no free sentence capture or global word substitution. */
typedef enum {EV_NONE,EV_ATOM,EV_NUMBER,EV_IDENTITY,EV_MESSAGE,EV_LINES,EV_CODE,EV_STATS,EV_ACTOR,EV_ENTITY} EventValueKind;
typedef struct {const char *source,*target;EventValueKind kinds[5];} EventDynamicRule;
#include "event_dynamic_rules.h"
typedef struct {const ZhDictionary *extra,*items,*shorts,*paragraphs;} EventDictionaries;
static inline int event_dynamic_inner(const EventDictionaries*,const char*,size_t,char*,size_t,unsigned);
static inline int event_number(const char *s,size_t n){
 if(!n||n>32)return 0;size_t i=(s[0]=='+'||s[0]=='-')?1:0,start=i;
 while(i<n&&s[i]>='0'&&s[i]<='9')i++;if(i==start)return 0;
 if(i<n&&s[i]=='.'){start=++i;while(i<n&&s[i]>='0'&&s[i]<='9')i++;if(i==start)return 0;}
 return i==n;
}
static inline int event_copy(const char *s,size_t n,char *out,size_t cap){if(n>=cap)return -1;memmove(out,s,n);out[n]=0;return 1;}
static inline int event_identity(const char *s,size_t n){
 if(!n||n>80||s[0]==' '||s[n-1]==' ')return 0;
 for(size_t i=0;i<n;i++)if(!((s[i]>='A'&&s[i]<='Z')||(s[i]>='a'&&s[i]<='z')||(s[i]>='0'&&s[i]<='9')||strchr(" -_.'",s[i])))return 0;
 return 1;
}
static inline int event_leaf(const EventDictionaries *d,const char *s,size_t n,char *out,size_t cap,unsigned depth){
 if(!n||n>4096||memchr(s,0,n))return 0;
 const ZhDictionary *dicts[]={d->extra,d->items,d->shorts,d->paragraphs};size_t rule=0;
 for(unsigned i=0;i<4;i++)if(dicts[i]){int r=zh_translate_exact(dicts[i],s,n,out,cap,&rule);if(r)return r;}
 int r=dialogue_dynamic_translate(d->extra,d->items,d->shorts,d->paragraphs,s,n,out,cap);if(r)return r;
 r=event_dynamic_inner(d,s,n,out,cap,depth+1);if(r)return r;
 for(unsigned i=0;i<4;i++)if(dicts[i]){
  char candidate[8192];r=zh_translate(dicts[i],s,n,candidate,sizeof candidate,&rule);
  if(r<0)return r;if(r>0&&zh_rule_is_numeric_template(dicts[i],rule))return event_copy(candidate,strlen(candidate),out,cap);
 }
 return content_name_lookup(d->extra,d->items,d->shorts,d->paragraphs,s,n,out,cap);
}
static inline int event_value(const EventDictionaries*,EventValueKind,const char*,size_t,char*,size_t,unsigned);
#include <stdio.h>
#include "event_stats.h"
static inline int event_value(const EventDictionaries *d,EventValueKind kind,const char *s,size_t n,char *out,size_t cap,unsigned depth){
 if(!n||n>4096||memchr(s,0,n)||memchr(s,'\r',n))return 0;
 if(kind==EV_STATS)return event_stats(d,s,n,out,cap,depth);
 if(kind==EV_ENTITY){
  /* Only a registered atomic name, a finite native proper name, or a bounded
     generated code is admitted. Never preserve arbitrary unknown prose. */
  if(n>160||!dialogue_parameter_safe(s,n))return 0;
  int r=dialogue_atom(d->extra,d->items,d->shorts,d->paragraphs,s,n,out,cap);
  if(r)return r;
  static const char *names[]={"Optimus","Warlord","MAIN.C","Zhirov","Imprinter","Data Miner"};
  for(unsigned i=0;i<sizeof names/sizeof names[0];i++)if(strlen(names[i])==n&&!memcmp(s,names[i],n))return event_copy(s,n,out,cap);
  /* A generated code has two 1..8-character uppercase/digit groups, exactly
     one hyphen, and at least one digit: e.g. RA-1N3. Letter-only proper names
     such as EX-DEC are accepted above only when already registered. */
  if(n<3||n>17||s[0]<'A'||s[0]>'Z')return 0;size_t dash=n;unsigned digits=0;
  for(size_t i=0;i<n;i++){
   if(s[i]>='0'&&s[i]<='9'){digits++;continue;}
   if(s[i]>='A'&&s[i]<='Z')continue;
   if(s[i]=='-'&&dash==n){dash=i;continue;}
   return 0;
  }
  return digits&&dash>=1&&dash<=8&&n-dash-1>=1&&n-dash-1<=8?event_copy(s,n,out,cap):0;
 }
 if(kind==EV_ACTOR){
  if(n>80||!event_identity(s,n))return 0;
  int code=memchr(s,'-',n)!=NULL;
  for(size_t i=0;i<n;i++)if(s[i]==' '||!((s[i]>='A'&&s[i]<='Z')||(s[i]>='0'&&s[i]<='9')||s[i]=='-'))code=0;
  if(!code){static const char *names[]={"Optimus","Warlord","MAIN.C","Zhirov","Imprinter","Data Miner"};
   for(unsigned i=0;i<sizeof names/sizeof names[0];i++)if(strlen(names[i])==n&&!memcmp(s,names[i],n))code=1;
  }
  return code?event_copy(s,n,out,cap):0;
 }
 if(kind==EV_NUMBER)return event_number(s,n)?event_copy(s,n,out,cap):0;
 if(kind==EV_ATOM)return dialogue_atom(d->extra,d->items,d->shorts,d->paragraphs,s,n,out,cap);
 if(kind==EV_IDENTITY){int r=dialogue_atom(d->extra,d->items,d->shorts,d->paragraphs,s,n,out,cap);if(r)return r;return event_identity(s,n)?event_copy(s,n,out,cap):0;}
 if(kind==EV_CODE){if(n>96)return 0;for(size_t i=0;i<n;i++)if(!((s[i]>='A'&&s[i]<='Z')||(s[i]>='a'&&s[i]<='z')||(s[i]>='0'&&s[i]<='9')||strchr("_-./@",s[i])))return 0;return event_copy(s,n,out,cap);}
 if(kind==EV_MESSAGE)return event_leaf(d,s,n,out,cap,depth);
 if(kind==EV_LINES){
  char staged[8192];size_t at=0,pos=0;unsigned lines=0;
  while(pos<n){size_t end=pos;while(end<n&&s[end]!='\n')end++;if(++lines>24||end==pos)return 0;
   size_t start=pos;while(start<end&&s[start]==' ')start++;char value[4096];
   int r=event_leaf(d,s+start,end-start,value,sizeof value,depth);if(r<=0)return r;
   size_t z=strlen(value),indent=start-pos;if(at+indent+z+1>=sizeof staged)return -1;
   memcpy(staged+at,s+pos,indent);at+=indent;memcpy(staged+at,value,z);at+=z;
   if(end<n)staged[at++]='\n';pos=end+1;
  }
  staged[at]=0;return event_copy(staged,at,out,cap);
 }
 return 0;
}
static inline int event_marker(const char *s,size_t *width){
 if(s[0]=='%'&&s[1]>='1'&&s[1]<='3'){*width=2;return s[1]-'1';}
 if(!strncmp(s,"{name}",6)){*width=6;return 3;}
 if(!strncmp(s,"{ufd_name}",10)){*width=10;return 4;}
 if(!strncmp(s,"[name]",6)){*width=6;return 3;}
 return -1;
}
typedef struct {
 const EventDictionaries *dicts;const EventDynamicRule *rule;const char *source;size_t bytes;
 const char *literal[7];size_t length[7];int marker[6];unsigned count,depth,attempts;
 size_t begin[5],size[5];unsigned present;int have,ambiguous,limited;char result[32768];
} EventSearch;
static inline void event_finish(EventSearch *c){
 char value[5][8192];for(unsigned i=0;i<5;i++)if(c->present&(1u<<i)){
  int r=event_value(c->dicts,c->rule->kinds[i],c->source+c->begin[i],c->size[i],value[i],sizeof value[i],c->depth);if(r!=1)return;
 }
 char staged[32768];size_t at=0;const char *p=c->rule->target;
 while(*p){size_t width=0;int id=event_marker(p,&width);
  if(id>=0){if(!(c->present&(1u<<id)))return;size_t z=strlen(value[id]);if(at+z>=sizeof staged)return;memcpy(staged+at,value[id],z);at+=z;p+=width;}
  else{if(at+1>=sizeof staged)return;staged[at++]=*p++;}}
 staged[at]=0;if(c->have&&strcmp(c->result,staged))c->ambiguous=1;
 else if(!c->have){memcpy(c->result,staged,at+1);c->have=1;}
}
static inline void event_walk(EventSearch *c,unsigned index,size_t start){
 if(c->ambiguous||c->limited)return;
 if(index==c->count){if(start==c->bytes)event_finish(c);return;}
 int id=c->marker[index];size_t z=c->length[index+1];
 for(size_t end=start+1;end+z<=c->bytes;end++){
  if(z&&memcmp(c->source+end,c->literal[index+1],z))continue;
  if(index+1==c->count&&end+z!=c->bytes)continue;
  if(++c->attempts>512){c->limited=1;return;}
  unsigned bit=1u<<id;int repeated=(c->present&bit)!=0;
  if(repeated&&(c->size[id]!=end-start||memcmp(c->source+c->begin[id],c->source+start,end-start)))continue;
  if(!repeated){c->present|=bit;c->begin[id]=start;c->size[id]=end-start;}
  event_walk(c,index+1,end+z);if(!repeated)c->present&=~bit;
 }
}
static inline int event_dynamic_inner(const EventDictionaries *d,const char *s,size_t n,char *out,size_t cap,unsigned depth){
 if(depth>3||!s||!out||!cap||n>16384||memchr(s,0,n))return 0;
 for(unsigned r=0;r<sizeof event_dynamic_rules/sizeof event_dynamic_rules[0];r++){
  const EventDynamicRule *rule=event_dynamic_rules+r;if(!rule->source[0])continue;
  EventSearch search={0};search.dicts=d;search.rule=rule;search.source=s;search.bytes=n;search.depth=depth;
  const char *p=rule->source,*last=p;
  while(*p){size_t width=0;int id=event_marker(p,&width);
   if(id<0){p++;continue;}if(search.count>=6){search.limited=1;break;}
   search.literal[search.count]=last;search.length[search.count]=(size_t)(p-last);search.marker[search.count++]=id;p+=width;last=p;}
  if(search.limited||!search.count)continue;search.literal[search.count]=last;search.length[search.count]=strlen(last);
  size_t prefix=search.length[0];if(n<=prefix||memcmp(s,search.literal[0],prefix))continue;
  event_walk(&search,0,prefix);if(search.ambiguous)return -2;if(search.limited)return 0;
  if(search.have)return event_copy(search.result,strlen(search.result),out,cap);
 }
 return 0;
}
static inline int event_dynamic_translate(const ZhDictionary *extra,const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 EventDictionaries d={extra,items,shorts,paragraphs};return event_dynamic_inner(&d,s,n,out,cap,0);
}
#endif

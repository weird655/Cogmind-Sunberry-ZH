#ifndef COGMIND_DIALOGUE_DYNAMIC_H
#define COGMIND_DIALOGUE_DYNAMIC_H
/* Reviewed display scaffolds. No command dispatch and no free-form matching.
   Unknown atoms, controls, mismatched repeated parameters and overflow miss
   atomically. The old exact dictionaries retain priority in terminal_text.h. */
typedef enum {DD_ATOM,DD_BUILD,DD_LIST,DD_IDENTITY,DD_SLOT} DialogueDynamicKind;
typedef struct {const char *source,*target;DialogueDynamicKind kind;} DialogueDynamicRule;
#include "dialogue_dynamic_rules.h"
static inline int dialogue_parameter_safe(const char *s,size_t n){
 if(!n||n>512||s[0]==' '||s[n-1]==' ')return 0;
 for(size_t i=0;i<n;i++)if((unsigned char)s[i]<32||(unsigned char)s[i]==127||strchr("%{}[]<>`\\",s[i]))return 0;
 return 1;
}
static inline int dialogue_atom(const ZhDictionary *extra,const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 if(!dialogue_parameter_safe(s,n)||n>160)return 0;
 /* Equipment membership is established before applying reviewed aliases. */
 char membership[1024];int r=canonical_item_name_translate(NULL,items,NULL,NULL,s,n,membership,sizeof membership);
 if(r<0)return r;
 if(r==1)return canonical_item_name_translate(extra,items,shorts,paragraphs,s,n,out,cap);
 /* Native robot identity prefix remains byte-for-byte, including code-only IDs. */
 if(n>=4&&s[0]>='A'&&s[0]<='Z'&&s[1]=='-'){
  size_t p=2;while(p<n&&s[p]>='0'&&s[p]<='9')p++;
  if(p>=4&&p<=6){
   if(p==n){if(n>=cap)return -1;memcpy(out,s,n);out[n]=0;return 1;}
   if(s[p]==' '){char label[1024];r=canonical_item_name_literal(extra,NULL,shorts,NULL,s+p+1,n-p-1,label,sizeof label);
    if(r==1){size_t z=strlen(label);if(p+1+z>=cap)return -1;memcpy(out,s,p+1);memcpy(out+p+1,label,z+1);return 1;}if(r<0)return r;}
  }
 }
 /* Topic/class atoms may come from the literal UI table, never from sentence
    templates. Sentence punctuation disqualifies this fallback. */
 for(size_t i=0;i<n;i++)if(strchr(".,!?;:\"()=+@",s[i]))return 0;
 return canonical_item_name_literal(extra,NULL,shorts,NULL,s,n,out,cap);
}
static inline int dialogue_build(const ZhDictionary *extra,const ZhDictionary *shorts,const char *s,size_t n,char *out,size_t cap){
 static const struct {const char *key,*value;} labels[]={
  {"flight","飞行"},{"flying","飞行"},{"hover","悬浮"},{"hovering","悬浮"},
  {"treads","履带"},{"treading","履带"},{"wheels","轮式"},{"rolling","轮式"},
  {"legs","腿式"},{"walking","腿式"},{"melee","近战"},{"ranged","远程"},
  {"stealth","潜行"},{"hacking","黑入"},{"combat","战斗"}
 };
 if(!dialogue_parameter_safe(s,n)||n>80)return 0;
 char lower[81];for(size_t i=0;i<n;i++)lower[i]=s[i]>='A'&&s[i]<='Z'?(char)(s[i]+32):s[i];lower[n]=0;
 for(unsigned i=0;i<sizeof labels/sizeof labels[0];i++)if(!strcmp(lower,labels[i].key)){size_t z=strlen(labels[i].value);if(z>=cap)return -1;memcpy(out,labels[i].value,z+1);return 1;}
 /* A literal dictionary label can describe configurations added by this build. */
 for(size_t i=0;i<n;i++)if(!((s[i]>='A'&&s[i]<='Z')||(s[i]>='a'&&s[i]<='z')||s[i]==' '||s[i]=='-'))return 0;
 return canonical_item_name_literal(extra,NULL,shorts,NULL,s,n,out,cap);
}
static inline int dialogue_slot(const char *s,size_t n,char *out,size_t cap){
 static const struct {const char *raw,*zh;} slots[]={{"power","动力"},{"propulsion","推进"},{"utility","功能"},{"weapon","武器"}};
 if(!s||!n||n>16)return 0;char lower[17];
 for(size_t i=0;i<n;i++)lower[i]=s[i]>='A'&&s[i]<='Z'?(char)(s[i]+32):s[i];lower[n]=0;
 for(unsigned i=0;i<sizeof slots/sizeof slots[0];i++)if(!strcmp(lower,slots[i].raw)){size_t z=strlen(slots[i].zh);if(z>=cap)return -1;memcpy(out,slots[i].zh,z+1);return 1;}
 return 0;
}
static inline int dialogue_list(const ZhDictionary *extra,const ZhDictionary *items,const char *s,size_t n,char *out,size_t cap,unsigned depth){
 if(depth>=8||!dialogue_parameter_safe(s,n))return 0;
 char atom[1024];int r=canonical_item_name_translate(NULL,items,NULL,NULL,s,n,atom,sizeof atom);
 if(r==1)return canonical_item_name_translate(extra,items,NULL,NULL,s,n,out,cap);if(r<0)return r;
 static const struct {const char *raw,*zh;} separators[]={{", and ","和"},{", ","、"},{" and ","和"}};
 for(size_t p=1;p<n;p++)for(unsigned k=0;k<sizeof separators/sizeof separators[0];k++){
  size_t z=strlen(separators[k].raw);if(p+z>=n||memcmp(s+p,separators[k].raw,z))continue;
  char first[1024],tail[4096];r=canonical_item_name_translate(NULL,items,NULL,NULL,s,p,first,sizeof first);if(r!=1)continue;
  r=canonical_item_name_translate(extra,items,NULL,NULL,s,p,first,sizeof first);if(r!=1)continue;
  r=dialogue_list(extra,items,s+p+z,n-p-z,tail,sizeof tail,depth+1);if(r!=1)continue;
  size_t a=strlen(first),b=strlen(separators[k].zh),c=strlen(tail);if(a+b+c>=cap)return -1;
  memcpy(out,first,a);memcpy(out+a,separators[k].zh,b);memcpy(out+a+b,tail,c+1);return 1;
 }
 return 0;
}
static inline int dialogue_dynamic_translate(const ZhDictionary *extra,const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 if(!s||!out||!cap||n>16384||memchr(s,0,n))return 0;
 for(unsigned i=0;i<sizeof dialogue_dynamic_rules/sizeof dialogue_dynamic_rules[0];i++){
  const DialogueDynamicRule *r=dialogue_dynamic_rules+i;const char *mark=strstr(r->source,"%1"),*second=strstr(mark+2,"%1");
  size_t pre=(size_t)(mark-r->source),suffix=strlen(second?second+2:mark+2),middle=second?(size_t)(second-mark-2):0;
  if(n<=pre+suffix+middle||memcmp(s,r->source,pre)||memcmp(s+n-suffix,second?second+2:mark+2,suffix))continue;
  size_t bytes=n-pre-suffix-middle;if(second){if(bytes%2)continue;bytes/=2;if(memcmp(s+pre+bytes,mark+2,middle)||memcmp(s+pre,s+pre+bytes+middle,bytes))continue;}
  const char *capture=s+pre;char value[4096];int found=0;
  if(r->kind==DD_ATOM)found=dialogue_atom(extra,items,shorts,paragraphs,capture,bytes,value,sizeof value);
  else if(r->kind==DD_BUILD)found=dialogue_build(extra,shorts,capture,bytes,value,sizeof value);
  else if(r->kind==DD_LIST)found=dialogue_list(extra,items,capture,bytes,value,sizeof value,0);
  else if(r->kind==DD_SLOT)found=dialogue_slot(capture,bytes,value,sizeof value);
  else if(dialogue_parameter_safe(capture,bytes)&&bytes<=80){
   /* NPC-generated display names are identities, not translatable prose. */
   found=1;for(size_t j=0;j<bytes;j++)if(!((capture[j]>='A'&&capture[j]<='Z')||(capture[j]>='a'&&capture[j]<='z')||(capture[j]>='0'&&capture[j]<='9')||strchr(" -_.'",capture[j])))found=0;
   if(found){memcpy(value,capture,bytes);value[bytes]=0;}
  }
  if(found<0)return found;if(!found)continue;
  char staged[8192];size_t at=0,z=strlen(value);const char *p=r->target;
  while(*p){if(p[0]=='%'&&p[1]=='1'){if(at+z>=sizeof staged)return -1;memcpy(staged+at,value,z);at+=z;p+=2;}
   else{if(at+1>=sizeof staged)return -1;staged[at++]=*p++;}}
  if(at>=cap)return -1;staged[at]=0;memmove(out,staged,at+1);return 1;
 }
 return 0;
}
#endif

#ifndef COGMIND_CANONICAL_ITEM_NAMES_H
#define COGMIND_CANONICAL_ITEM_NAMES_H
#include "item_name_text.h"
/* One literal item-name policy for UI, hover, logs and repair lists.
   Dictionary precedence applies before aliases: a reviewed extra alias wins
   over an item_names exact spelling. No templates or arbitrary prefix match. */
static inline int canonical_item_name_literal(const ZhDictionary *extra,const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *source,size_t length,char *out,size_t capacity){
 if(!source||!out)return -1;
 const ZhDictionary *dictionaries[]={extra,items,shorts,paragraphs};
 static const struct {const char *full,*short_name;} aliases[]={
  {"Improved ","Imp. "},{"Advanced ","Adv. "},{"Experimental ","Exp. "},
  {"Light ","Lgt. "},{"Heavy ","Hvy. "},{"Small ","Sml. "},{"Large ","Lrg. "},
  {"Micro ","Mic. "},{"Compact ","Cpt. "},{"Enhanced ","Enh. "},{"Precision ","Prc. "},
  {"Armored ","Arm. "},{"Cooled ","Cld. "},{"Hardened ","Hrd. "},{"High-capacity ","Hcp. "}
 };
 for(size_t d=0;d<sizeof dictionaries/sizeof dictionaries[0];d++){
  if(!dictionaries[d])continue;
  int r=item_name_translate_exact(dictionaries[d],source,length,out,capacity);if(r)return r;
  for(size_t a=0;a<sizeof aliases/sizeof aliases[0];a++){
   size_t p=strlen(aliases[a].full),q=strlen(aliases[a].short_name);
   if(length<=p||length>160||memcmp(source,aliases[a].full,p))continue;
   char candidate[180];size_t n=q+length-p;
   if(n>=sizeof candidate)continue;
   memcpy(candidate,aliases[a].short_name,q);memcpy(candidate+q,source+p,length-p);candidate[n]=0;
   r=item_name_translate_exact(dictionaries[d],candidate,n,out,capacity);if(r)return r;
  }
 }
 return 0;
}
/* Bounded native modifier grammar. Resolve the underlying item first, then
   use only the reviewed modifier template; never enable general NAME templates.
   Complete literal names still win, including Broken Shaft Lining. */
static inline int canonical_item_name_translate(const ZhDictionary *extra,const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *source,size_t length,char *out,size_t capacity){
 int literal=canonical_item_name_literal(extra,items,shorts,paragraphs,source,length,out,capacity);if(literal)return literal;
 /* Native left-facing hover puts the count before the name. Preserve the
    exact prefix and resolve the whole name through the same literal policy. */
 size_t at=length&&source[0]==' '?1:0;
 if(at<length&&source[at]=='['){
  size_t start=++at;while(at<length&&source[at]>='0'&&source[at]<='9')at++;
  if(at>start&&at-start<=16&&at+2<length&&source[at]==']'&&source[at+1]==' '){
   size_t prefix_bytes=at+2;char label[672],assembled[864];
   int match=canonical_item_name_literal(extra,items,shorts,paragraphs,source+prefix_bytes,length-prefix_bytes,label,sizeof label);
   if(match!=1)return match;size_t bytes=strlen(label);
   if(prefix_bytes+bytes>=sizeof assembled||prefix_bytes+bytes>=capacity)return -1;
   memcpy(assembled,source,prefix_bytes);memcpy(assembled+prefix_bytes,label,bytes+1);memmove(out,assembled,prefix_bytes+bytes+1);return 1;
  }
 }
 static const char prefix[]="Broken ";
 if(length<=sizeof prefix-1||length>160||memcmp(source,prefix,sizeof prefix-1))return 0;
 const char *base=source+sizeof prefix-1;size_t n=length-(sizeof prefix-1);
 if(memchr(base,'\n',n)||memchr(base,'\r',n)||memchr(base,'`',n)||memchr(base,':',n))return 0;
 char name[672],candidate[704],translated[1024];
 int found=canonical_item_name_literal(extra,items,shorts,paragraphs,base,n,name,sizeof name);if(found!=1)return found;
 size_t size=strlen(name);if(size+sizeof prefix>sizeof candidate)return -1;
 memcpy(candidate,prefix,sizeof prefix-1);memcpy(candidate+sizeof prefix-1,name,size+1);
 const ZhDictionary *ds[]={extra,items,shorts,paragraphs};
 for(unsigned i=0;i<sizeof ds/sizeof ds[0];i++){
  if(!ds[i])continue;size_t rule=0;int match=zh_translate(ds[i],candidate,strlen(candidate),translated,sizeof translated,&rule);
  if(match<0)return match;
  if(match>0&&zh_rule_key_is(ds[i],rule,"Broken {*}")){
   size_t bytes=strlen(translated);if(bytes>=capacity)return -1;memmove(out,translated,bytes+1);return 1;
  }
 }
 return 0;
}
/* Dedicated repair grammar: plain NAME, space, (decimal/decimal).
   Delimiters and both decimal operands are preserved byte for byte.
   No suffix chains, percentages, nested parentheses or partial matches.
   Failure, including capacity failure, leaves output unchanged. */
static inline int canonical_item_name_translate_health(const ZhDictionary *extra,const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *source,size_t length,char *out,size_t capacity){
 if(!source||!out)return -1;
 if(length<7||length>180||source[length-1]!=')')return 0;
 size_t open=length-1;while(open&&source[open]!='(')open--;
 if(open<2||source[open-1]!=' ')return 0;
 size_t slash=open+1;while(slash<length-1&&source[slash]>='0'&&source[slash]<='9')slash++;
 if(slash==open+1||slash-open-1>16||slash>=length-1||source[slash]!='/')return 0;
 size_t end=slash+1;while(end<length-1&&source[end]>='0'&&source[end]<='9')end++;
 if(end==slash+1||end-slash-1>16||end!=length-1)return 0;
 size_t base=open-1;
 for(size_t i=0;i<base;i++)if(strchr("[]()+",source[i]))return 0;
 char translated[672];
 int r=canonical_item_name_translate(extra,items,shorts,paragraphs,source,base,translated,sizeof translated);if(r!=1)return r;
 size_t n=strlen(translated),suffix=length-base;
 if(n+suffix>=sizeof translated||n+suffix>=capacity)return -1;
 memcpy(translated+n,source+base,suffix);translated[n+suffix]=0;
 memcpy(out,translated,n+suffix+1);return 1;
}
#endif

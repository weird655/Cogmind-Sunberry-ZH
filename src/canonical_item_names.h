#ifndef COGMIND_CANONICAL_ITEM_NAMES_H
#define COGMIND_CANONICAL_ITEM_NAMES_H
#include "item_name_text.h"
/* One literal item-name policy for UI, hover, logs and repair lists.
   Dictionary precedence applies before aliases: a reviewed extra alias wins
   over an item_names exact spelling. No templates or arbitrary prefix match. */
static inline int canonical_item_name_translate(const ZhDictionary *extra,const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *source,size_t length,char *out,size_t capacity){
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

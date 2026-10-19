#ifndef COGMIND_ITEM_NAME_TEXT_H
#define COGMIND_ITEM_NAME_TEXT_H
#include <string.h>
#include <stdint.h>
#include "text_core.h"
#define ITEM_NAME_BASE_LIMIT 96u
#define ITEM_NAME_TAG_LIMIT 32u
static int item_name_safe_target(const char *p,size_t n){
 if(!n)return 0;for(size_t at=0;at<n;){size_t used;uint32_t cp;if(!zh_decode(p+at,n-at,&used,&cp))return 0;
  if(cp<32||cp==127||cp=='`'||cp=='['||cp==']'||cp=='{'||cp=='}'||cp=='<'||cp=='>')return 0;at+=used;
 }return 1;
}
/* Dedicated literal item-name dictionary only. Accept an exact plain name or
   one suffix: NAME+, NAME (digits), NAME [ASCII-tag], or native NAME! with an optional bracket tag. Preserve the entire suffix
   byte-for-byte; no template lookup, normalization, trimming, or chain parsing.
   1 success, 0 unsupported/not found, -1 bad argument/capacity. Failure atomic. */
static int item_name_translate_exact(const ZhDictionary *items,const char *source,size_t length,char *out,size_t capacity){
 if(!items||!source||!out)return -1;if(!length||length>160)return 0;
 for(size_t i=0;i<length;i++){unsigned char c=(unsigned char)source[i];if(c<32||c>126||c=='`'||c=='{'||c=='}'||c=='<'||c=='>')return 0;}
 size_t base=length;
 if(source[length-1]=='+')base=length-1;
 else if(source[length-1]==')'){
  size_t open=length-1;while(open&&source[open]!='(')open--;
  if(open<2||source[open-1]!=' '||length-open-2<1||length-open-2>16)return 0;
  for(size_t i=open+1;i<length-1;i++)if(source[i]<'0'||source[i]>'9')return 0;base=open-1;
 }else if(source[length-1]==']'){
  size_t open=length-1;while(open&&source[open]!='[')open--;
  size_t tags=length-open-2;if(open<2||source[open-1]!=' '||!tags||tags>ITEM_NAME_TAG_LIMIT)return 0;
  for(size_t i=open+1;i<length-1;i++){
   unsigned char c=(unsigned char)source[i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||strchr(".+*/_%:#?=!,;-",c)))return 0;
  }base=open-1;
 }
 if(base&&source[base-1]=='!')base--;
 if(!base||base>ITEM_NAME_BASE_LIMIT||source[0]==' '||source[base-1]==' ')return 0;
 for(size_t i=0;i<base;i++)if(strchr("[]()+!",source[i]))return 0;
 char translated[512],temporary[672];size_t rule;
 int r=zh_translate_exact(items,source,base,translated,sizeof translated,&rule);if(r<=0)return r<0?-1:0;
 size_t n=strlen(translated);if(!item_name_safe_target(translated,n))return 0;
 size_t suffix=length-base;if(n+suffix>=sizeof temporary||n+suffix>=capacity)return -1;
 memcpy(temporary,translated,n);memcpy(temporary+n,source+base,suffix);temporary[n+suffix]=0;
 memcpy(out,temporary,n+suffix+1);return 1;
}
#endif

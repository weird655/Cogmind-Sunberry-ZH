#ifndef COGMIND_CONTENT_NAMES_H
#define COGMIND_CONTENT_NAMES_H
#include "canonical_item_names.h"
/* Translate only complete, typed display atoms. Identifiers and numeric
   prefixes remain byte-for-byte; this never substitutes inside unknown words. */
static inline int content_name_lookup(const ZhDictionary *extra,const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 int r=canonical_item_name_translate(extra,items,shorts,paragraphs,s,n,out,cap);if(r)return r;
 r=canonical_item_name_translate_health(extra,items,shorts,paragraphs,s,n,out,cap);if(r)return r;
 /* Native component stockpile IDs: IDENTIFIER: Item Name. */
 size_t colon=0;while(colon<n&&((s[colon]>='A'&&s[colon]<='Z')||(s[colon]>='0'&&s[colon]<='9')))colon++;
 if(colon>=6&&colon<=16&&colon+2<n&&s[colon]==':'&&s[colon+1]==' '){
  char label[1024];r=canonical_item_name_translate(extra,items,shorts,paragraphs,s+colon+2,n-colon-2,label,sizeof label);
  if(r==1){size_t size=strlen(label);if(colon+2+size>=cap)return -1;char temporary[1536];memcpy(temporary,s,colon+2);memcpy(temporary+colon+2,label,size+1);memcpy(out,temporary,colon+2+size+1);return 1;}if(r<0)return r;
 }
 /* Typed numeric fields only: a known label, colon, decimal + optional 'p'. */
 const char *separator=memchr(s,':',n);
 if(separator&&(size_t)(separator-s)+2<n&&separator[1]==' '){
  size_t label_size=(size_t)(separator-s),start=label_size+2,end=start;while(end<n&&s[end]>='0'&&s[end]<='9')end++;
  if(end>start&&end-start<=16&&(end==n||(end+1==n&&s[end]=='p'))){
   char label[1024];r=canonical_item_name_translate(extra,items,shorts,paragraphs,s,label_size,label,sizeof label);
   if(r==1){size_t size=strlen(label),tail=n-label_size;if(size+tail>=cap)return -1;char temporary[1536];if(size+tail>=sizeof temporary)return -1;memcpy(temporary,label,size);memcpy(temporary+size,s+label_size,tail);temporary[size+tail]=0;memcpy(out,temporary,size+tail+1);return 1;}if(r<0)return r;
  }
 }
 size_t prefix=0;
 if(n>5&&s[0]>='A'&&s[0]<='Z'&&s[1]=='-'){
  size_t at=2;while(at<n&&s[at]>='0'&&s[at]<='9')at++;
  if(at>=3&&at<=6&&at+1<n&&s[at]==' ')prefix=at+1;
 }
 if(!prefix){size_t at=0;while(at<n&&s[at]>='0'&&s[at]<='9')at++;
  if(at&&at<=16&&at+2<n&&s[at]=='x'&&s[at+1]==' ')prefix=at+2;
 }
 if(!prefix&&n>6&&s[0]=='('){
  size_t at=1,start=at;while(at<n&&s[at]>='0'&&s[at]<='9')at++;
  if(at>start&&at-start<=5&&at<n&&s[at++]==','){
   start=at;while(at<n&&s[at]>='0'&&s[at]<='9')at++;
   if(at>start&&at-start<=5&&at+2<n&&s[at]==')'&&s[at+1]==' ')prefix=at+2;
  }
 }
 if(!prefix)return 0;
 char label[1024];r=canonical_item_name_translate(extra,items,shorts,paragraphs,s+prefix,n-prefix,label,sizeof label);
 if(r!=1)return r;size_t bytes=strlen(label);if(prefix+bytes>=cap)return -1;
 char temporary[1536];if(prefix+bytes>=sizeof temporary)return -1;
 memcpy(temporary,s,prefix);memcpy(temporary+prefix,label,bytes+1);memcpy(out,temporary,prefix+bytes+1);return 1;
}
/* Refine whole captured ASCII name runs after a template has translated the
   surrounding prose. Link labels and native style tags are kept unchanged. */
static inline int content_refine_names(const ZhDictionary *extra,const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,char *text,size_t cap){
 char result[32768];size_t n=strlen(text),at=0,pos=0;int changed=0;
 if(n>=sizeof result||n>=cap)return 0;
 while(pos<n){
  size_t end=pos;
  if(text[pos]=='['){end=pos+1;while(end<n&&text[end]!=']')end++;if(end<n)end++;}
  else if(text[pos]=='`'){end=pos+1;while(end<n&&text[end]!='`')end++;if(end<n)end++;}
  else if((unsigned char)text[pos]>=128){size_t used;uint32_t cp;if(!zh_decode(text+pos,n-pos,&used,&cp))return 0;end=pos+used;}
  else{
   end=pos;while(end<n&&(unsigned char)text[end]<128&&text[end]!='['&&text[end]!='`'&&text[end]!='\n'&&text[end]!='\r')end++;
   if(end==pos)end++;
   size_t begin=pos,tail=end;
   while(begin<tail&&(text[begin]==' '||text[begin]=='"'||text[begin]=='\''))begin++;
   while(tail>begin&&strchr(" .,:;!?\"",text[tail-1]))tail--;
   char translated[2048];int r=tail>begin?content_name_lookup(extra,items,shorts,paragraphs,text+begin,tail-begin,translated,sizeof translated):0;
   if(r==1){size_t bytes=strlen(translated),need=(begin-pos)+bytes+(end-tail);
    if(at+need>=sizeof result||at+need>=cap)return 0;
    memcpy(result+at,text+pos,begin-pos);at+=begin-pos;memcpy(result+at,translated,bytes);at+=bytes;memcpy(result+at,text+tail,end-tail);at+=end-tail;pos=end;changed=1;continue;
   }
  }
  if(at+end-pos>=sizeof result||at+end-pos>=cap)return 0;memcpy(result+at,text+pos,end-pos);at+=end-pos;pos=end;
 }
 if(changed){result[at]=0;memcpy(text,result,at+1);}return changed;
}
#endif

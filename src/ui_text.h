#ifndef COGMIND_UI_TEXT_H
#define COGMIND_UI_TEXT_H
#include "text_core.h"
#include <string.h>
/* Display-only exact lookup. No wildcard expansion or business-string mutation.
   All successful plans stay within the original visible footprint. */
static inline int ui_visible_width(const char *raw,size_t n){
 int columns=0;
 for(size_t i=0;i<n;i++){
  unsigned char c=raw[i];if(c=='`'){size_t end=i+1;while(end<n&&raw[end]!='`')end++;
   if(end==n||end==i+1||end-i>32)return -1;
   for(size_t j=i+1;j<end;j++)if((unsigned char)raw[j]<32||(unsigned char)raw[j]>=127)return -1;
   i=end;continue;
  }
  if(c<32||c>=127)return -1;columns++;
 }
 return columns;
}
static inline int ui_text_prepare(const ZhDictionary *d,const char *raw,size_t n,int columns,char *out,size_t cap,ZhPlan *plan,size_t *rule){
 if(!d||!raw||!n||n>4096||columns<1||columns>512||!out||cap<2||!plan)return 0;
 int footprint=ui_visible_width(raw,n);if(footprint<1||footprint>columns)return 0;
 char translated[4096],combined[8192];size_t selected=0;
 size_t start=0,end=n;
 while(start<end&&raw[start]=='`'){size_t q=start+1;while(q<end&&raw[q]!='`')q++;if(q==end)return 0;start=q+1;}
 while(end>=start+3&&!memcmp(raw+end-3,"`x`",3))end-=3;
 while(start<end&&raw[start]==' ')start++;
 while(end>start&&raw[end-1]==' ')end--;
 if(start==end||memchr(raw+start,'`',end-start))return 0;
 if(zh_translate_exact(d,raw+start,end-start,translated,sizeof translated,&selected)!=1)return 0;
 if(strchr(translated,'{')||strchr(translated,'}')||!strcmp(translated,""))return 0;
 size_t m=strlen(translated),tail=n-end;
 if(start+m+tail>=sizeof combined||start+m+tail>=cap)return 0;
 memcpy(combined,raw,start);memcpy(combined+start,translated,m);memcpy(combined+start+m,raw+end,tail);combined[start+m+tail]=0;
 ZhPlan prepared={0};if(zh_plan(combined,start+m+tail,footprint,1,&prepared))return 0;
 memcpy(out,combined,start+m+tail+1);*plan=prepared;if(rule)*rule=selected;return 1;
}
#endif

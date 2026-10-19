#ifndef COGMIND_TERMINAL_TEXT_H
#define COGMIND_TERMINAL_TEXT_H
#include "translation_overlay.h"
#include "terminal_normalize.h"
#include "content_names.h"
static const ZhDictionary *terminal_item_dictionary;
static const ZhDictionary *terminal_runtime_dictionary;
/* Preserve reviewed paragraph rules. Exact strings in the UI corpus must
   also reach terminal output before broad paragraph templates are tried.
   Additional rule IDs are diagnostic sentinels, never native link IDs. */
static inline int terminal_translate_core(const ZhDictionary *paragraphs,const ZhDictionary *shorts,const ZhDictionary *extra,const char *source,size_t bytes,char *out,size_t capacity,size_t *rule){
 size_t found=0;int r;
 if(terminal_runtime_dictionary){r=zh_translate_exact(terminal_runtime_dictionary,source,bytes,out,capacity,&found);if(r){if(r>0&&rule)*rule=(size_t)-4;return r;}}
 if(extra){r=zh_translate_exact(extra,source,bytes,out,capacity,&found);if(r){if(r>0&&rule)*rule=(size_t)-2;return r;}}
 r=zh_translate_exact(paragraphs,source,bytes,out,capacity,&found);if(r){if(r>0&&rule)*rule=found;return r;}
 if(shorts){r=zh_translate_exact(shorts,source,bytes,out,capacity,&found);if(r){if(r>0&&rule)*rule=(size_t)-3;return r;}}
 /* All literal exact matches above retain their original precedence. For
    normalized matches, any conflicting reviewed output fails atomically. */
 if(capacity>32768)return -1;
 const ZhDictionary *dicts[]={terminal_runtime_dictionary,extra,paragraphs,shorts};
 char chosen[32768],candidate[32768];int have=0;size_t chosen_rule=0;
 for(unsigned i=0;i<4;i++)if(dicts[i]){
  r=zh_translate_normalized_exact(dicts[i],source,bytes,candidate,capacity,&found);
  if(r<0)return r;
  if(r>0){if(have&&strcmp(chosen,candidate))return -2;
   if(!have){strcpy(chosen,candidate);chosen_rule=i==0?(size_t)-4:i==1?(size_t)-2:i==3?(size_t)-3:found;have=1;}}
 }
 if(have){strcpy(out,chosen);if(rule)*rule=chosen_rule;return 1;}
 if(terminal_runtime_dictionary){r=zh_translate(terminal_runtime_dictionary,source,bytes,out,capacity,&found);if(r){if(r>0&&rule)*rule=(size_t)-4;return r;}}
 r=zh_translate_overlay(paragraphs,extra,source,bytes,out,capacity,rule);if(r||!shorts)return r;
 r=zh_translate(shorts,source,bytes,out,capacity,&found);if(r>0&&rule)*rule=(size_t)-3;return r;
}
/* Line composition is a fallback after complete paragraph matching. Native
   dynamic lists are assembled from independent strings, not one TSV key. */
static inline int terminal_translate_leaf(const ZhDictionary *items,const ZhDictionary *paragraphs,const ZhDictionary *shorts,const ZhDictionary *extra,const char *s,size_t n,char *out,size_t cap,size_t *rule){
 int r=canonical_item_name_translate(extra,items,NULL,NULL,s,n,out,cap);if(r)return r;
 r=terminal_translate_core(paragraphs,shorts,extra,s,n,out,cap,rule);
 if(r!=1){char atom[16385];int ar=content_name_lookup(extra,items,shorts,paragraphs,s,n,atom,sizeof atom);if(ar==1){size_t bytes=strlen(atom);if(bytes>=cap)return -1;memcpy(out,atom,bytes+1);return 1;}if(ar<0)return ar;}
 if(r==0){char canonical[16385];if(terminal_normalize(s,n,canonical,sizeof canonical)==1){size_t bytes=strlen(canonical);if(bytes!=n||memcmp(canonical,s,n))r=terminal_translate_core(paragraphs,shorts,extra,canonical,bytes,out,cap,rule);}}
 if(r==0)r=content_name_lookup(extra,items,shorts,paragraphs,s,n,out,cap);
 if(r>0)content_refine_names(extra,items,shorts,paragraphs,out,cap);return r;
}
static inline int terminal_compose_lines(const ZhDictionary *items,const ZhDictionary *paragraphs,const ZhDictionary *shorts,const ZhDictionary *extra,const char *s,size_t n,char *out,size_t cap,unsigned *matched,unsigned *retained){
 if(!out||n>16384||cap>32768)return -1;
 char assembled[32768],line[16385];size_t pos=0,at=0;unsigned changed=0,kept=0;
 while(pos<n){
  size_t end=pos;while(end<n&&s[end]!='\r'&&s[end]!='\n')end++;
  size_t left=pos,right=end;while(left<right&&(s[left]==' '||s[left]=='\t'))left++;while(right>left&&(s[right-1]==' '||s[right-1]=='\t'))right--;
  int r=left<right?terminal_translate_leaf(items,paragraphs,shorts,extra,s+left,right-left,line,sizeof line,NULL):0;
  if(r<0)return r;
  size_t bytes=r>0?strlen(line):right-left,need=left-pos+bytes+end-right;
  if(at+need>=sizeof assembled||at+need>=cap)return -1;
  memcpy(assembled+at,s+pos,left-pos);at+=left-pos;
  if(r>0){memcpy(assembled+at,line,bytes);changed++;}else{memcpy(assembled+at,s+left,bytes);if(right>left)kept++;}at+=bytes;
  memcpy(assembled+at,s+right,end-right);at+=end-right;
  if(end<n){if(at+1>=cap||at+1>=sizeof assembled)return -1;assembled[at++]='\n';if(s[end]=='\r'&&end+1<n&&s[end+1]=='\n')end++;end++;}
  pos=end;
 }
 if(matched)*matched=changed;if(retained)*retained=kept;
 if(!changed)return 0;assembled[at]=0;memcpy(out,assembled,at+1);return 1;
}
static inline int terminal_translate(const ZhDictionary *paragraphs,const ZhDictionary *shorts,const ZhDictionary *extra,const char *source,size_t bytes,char *out,size_t capacity,size_t *rule){
 int r=terminal_translate_core(paragraphs,shorts,extra,source,bytes,out,capacity,rule);
 if(r==1)return r;
 if(r>=0&&(memchr(source,'\n',bytes)||memchr(source,'\r',bytes))){
  char composed[32768];unsigned matched=0,kept=0;
  int cr=terminal_compose_lines(terminal_item_dictionary,paragraphs,shorts,extra,source,bytes,composed,sizeof composed,&matched,&kept);
  if(cr>0&&(r==0||(matched>=2&&kept==0))){size_t length=strlen(composed);if(length>=capacity){if(r>0)return r;return -1;}memcpy(out,composed,length+1);if(rule)*rule=(size_t)-5;return 1;}
  if(r==0&&cr<0)return cr;
 }
 if(r){if(r>1)content_refine_names(extra,terminal_item_dictionary,shorts,paragraphs,out,capacity);return r;}
 char canonical[16385];if(terminal_normalize(source,bytes,canonical,sizeof canonical)!=1)return 0;
 size_t n=strlen(canonical);if(n==bytes&&!memcmp(source,canonical,n))return 0;
 r=terminal_translate_core(paragraphs,shorts,extra,canonical,n,out,capacity,rule);if(r>1)content_refine_names(extra,terminal_item_dictionary,shorts,paragraphs,out,capacity);return r;
}
#endif

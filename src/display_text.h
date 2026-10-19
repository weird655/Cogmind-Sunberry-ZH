#ifndef COGMIND_DISPLAY_TEXT_H
#define COGMIND_DISPLAY_TEXT_H
#include "content_names.h"
#include "terminal_text.h"
#include "translation_overlay.h"
#include <stdlib.h>
#include <stdio.h>
static int display_columns(const char *s){int width=0;for(size_t at=0,n=strlen(s);at<n;){size_t used;uint32_t cp;if(!zh_decode(s+at,n-at,&used,&cp)||cp<32||cp==127)return -1;width+=cp<128?1:2;at+=used;}return width;}
static int display_literal_base(const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 int canonical=content_name_lookup(display_extra_dictionary,items,shorts,paragraphs,s,n,out,cap);if(canonical)return canonical;
 int r=zh_translate_exact(shorts,s,n,out,cap,NULL);if(r)return r;
 r=zh_translate_exact(paragraphs,s,n,out,cap,NULL);if(r)return r;
 /* Robot display codes remain byte-for-byte; translate only a known type. */
 if(n>5&&s[0]>='A'&&s[0]<='Z'&&s[1]=='-'){
  size_t at=2;while(at<n&&s[at]>='0'&&s[at]<='9')at++;
  if(at>=3&&at<=5&&at+1<n&&s[at]==' '){char type[512];r=zh_translate_exact(shorts,s+at+1,n-at-1,type,sizeof type,NULL);
   if(r!=1)r=zh_translate_exact(paragraphs,s+at+1,n-at-1,type,sizeof type,NULL);
   if(r==1&&at+1+strlen(type)<cap){memcpy(out,s,at+1);strcpy(out+at+1,type);return 1;}
  }
 }
 return 0;
}
static int display_literal(const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 if(display_extra_dictionary){int r=zh_translate_exact(display_extra_dictionary,s,n,out,cap,NULL);if(r)return r;}
 int r=display_literal_base(items,shorts,paragraphs,s,n,out,cap);if(r)return r;
 if(n>2&&s[0]=='('&&s[n-1]==')'){
  char inner[2048];r=display_literal_base(items,shorts,paragraphs,s+1,n-2,inner,sizeof inner);
  if(r==1){size_t m=strlen(inner);if(m+3>cap)return -1;out[0]='(';memcpy(out+1,inner,m);out[m+1]=')';out[m+2]=0;return 1;}
 }
 if(n>7&&!memcmp(s,"Faulty ",7)){
  char inner[2048];r=display_literal_base(items,shorts,paragraphs,s+7,n-7,inner,sizeof inner);
  if(r==1){const char *prefix="有故障";size_t p=strlen(prefix),m=strlen(inner);if(p+m>=cap)return -1;memcpy(out,prefix,p);memcpy(out+p,inner,m+1);return 1;}
 }
 return 0;
}
/* Combine complete UI and paragraph corpora only at display boundaries. */
static int display_lookup(const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 if(terminal_runtime_dictionary){int patch=zh_translate(terminal_runtime_dictionary,s,n,out,cap,NULL);if(patch)return patch;}
 static const struct {const char *source,*target;} difficulty[]={
  {"E X P L O R E R","探 险 家"},{"A D V E N T U R E R","冒 险 家"},{"R O G U E","浪 人"}
 };
 for(unsigned i=0;i<sizeof difficulty/sizeof difficulty[0];i++)if(n==strlen(difficulty[i].source)&&!memcmp(s,difficulty[i].source,n)){
  size_t bytes=strlen(difficulty[i].target);if(bytes>=cap)return -1;memcpy(out,difficulty[i].target,bytes+1);return 1;
 }
 static const struct {const char *a,*b;} movement[]={
  {"MOVEMENT: Rolling","移动: 滚动"},{"MOVEMENT: Walking","移动: 步行"},
  {"MOVEMENT: Treading","移动: 履带"},{"MOVEMENT: Flying","移动: 飞行"},
  {"MOVEMENT: Hovering","移动: 悬浮"},{"MOVEMENT: Running","移动: 行驶"},
  {"MOVEMENT: Weaving","移动: 闪避"},{"MOVEMENT: Core","移动: 核心"}
 };
 for(unsigned i=0;i<sizeof movement/sizeof movement[0];i++)if(n==strlen(movement[i].a)&&!memcmp(s,movement[i].a,n)){
  if(strlen(movement[i].b)>=cap)return -1;strcpy(out,movement[i].b);return 1;
 }
 /* Repair selection widgets print NAME (current/maximum) as one string.
    Keep the native durability column rather than shortening the whole row. */
 char health[1024];int health_result=canonical_item_name_translate_health(display_extra_dictionary,items,shorts,paragraphs,s,n,health,sizeof health);
 if(health_result<0)return health_result;
 if(health_result==1){
  size_t base=n-1;while(base&&s[base]!='(')base--;if(base)base--;
  char *suffix=strrchr(health,'(');if(!suffix||suffix==health)return 0;
  size_t translated_name=(size_t)(suffix-health)-1;health[translated_name]=0;
  int width=display_columns(health);if(width<0||(size_t)width>base)return 0;
  size_t padding=base-(size_t)width,tail=n-base;
  if(translated_name+padding+tail>=cap)return -1;
  memcpy(out,health,translated_name);memset(out+translated_name,' ',padding);
  memcpy(out+translated_name+padding,s+base,tail);out[translated_name+padding+tail]=0;return 1;
 }
 int literal=display_literal(items,shorts,paragraphs,s,n,out,cap);if(literal)return literal;
 int atom=content_name_lookup(display_extra_dictionary,items,shorts,paragraphs,s,n,out,cap);if(atom)return atom;
 /* Native command descriptors stay English; only the display label changes.
    Preserve the seen marker and quoted argument used by the menu artwork. */
 const char *verbs[]={"Record","Schematic"};const char *labels[]={"记录","图纸"};
 for(unsigned i=0;i<2;i++){
  size_t k=strlen(verbs[i]),mark=k;if(n>k&&s[k]=='!')mark++;
  if(n>mark+3&&!memcmp(s,verbs[i],k)&&s[mark]==' '&&s[mark+1]=='"'&&s[n-1]=='"'&&!memchr(s+mark+2,'"',n-mark-3)){
   char arg[1024];size_t length=n-mark-3;int known=display_literal(items,shorts,paragraphs,s+mark+2,length,arg,sizeof arg);
   if(known!=1){if(length>=sizeof arg)return -1;memcpy(arg,s+mark+2,length);arg[length]=0;}
   int verb_width=display_columns(labels[i]),arg_width=display_columns(arg);if(verb_width<0||arg_width<0||verb_width>(int)k||arg_width>(int)length)return 0;
   int written=snprintf(out,cap,"%s%s \"%s\"",labels[i],mark>k?"!":"",arg);return written>=0&&(size_t)written<cap?1:-1;
  }
 }
 size_t a=0,b=0;int r=zh_translate_exact(shorts,s,n,out,cap,&a);if(r)return r;
 r=zh_translate_exact(paragraphs,s,n,out,cap,&b);if(r)return r;
 char first[16384],second[16384];if(cap>sizeof first)return -1;
 int ra=zh_translate(shorts,s,n,first,cap,&a),rb=zh_translate(paragraphs,s,n,second,cap,&b);
 /* A conflict inside one corpus must not veto an unambiguous rule in the
    other corpus (e.g. numeric Acquired vs broad acquired-item patterns). */
 int conflict=ra<0?ra:rb<0?rb:0;
 if(ra<0)ra=0;if(rb<0)rb=0;
 if(display_extra_dictionary){size_t extra_rule=0;char extra[16384];int re=zh_translate(display_extra_dictionary,s,n,extra,cap,&extra_rule);
  if(re>0){unsigned se=zh_rule_specificity(display_extra_dictionary,extra_rule),sa=ra?zh_rule_specificity(shorts,a):0,sb=rb?zh_rule_specificity(paragraphs,b):0;
   if(se>=sa&&se>=sb){strcpy(out,extra);return 1;}
  }
 }
 if(!ra&&!rb)return conflict;
 if(ra&&rb){unsigned sa=zh_rule_specificity(shorts,a),sb=zh_rule_specificity(paragraphs,b);
  if(sa==sb&&strcmp(first,second))return -2;
  strcpy(out,sa>=sb?first:second);
 }else strcpy(out,ra?first:second);
 return 1;
}
static int display_append(char *out,size_t cap,size_t *at,const char *p,size_t n){
 if(n>=cap-*at)return 0;memcpy(out+*at,p,n);*at+=n;out[*at]=0;return 1;
}
static void display_refine_names(const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,char *text,size_t capacity){
 char assembled[16384];size_t n=strlen(text),at=0,pos=0;
 while(pos<n){
  if((unsigned char)text[pos]>=128){size_t used;uint32_t cp;if(!zh_decode(text+pos,n-pos,&used,&cp)||!display_append(assembled,sizeof assembled,&at,text+pos,used))return;pos+=used;continue;}
  size_t end=pos;while(end<n&&(unsigned char)text[end]<128&&text[end]!='`')end++;
  if(end==pos){if(!display_append(assembled,sizeof assembled,&at,text+pos,1))return;pos++;continue;}
  size_t begin=pos,tail=end;while(begin<tail&&!((text[begin]>='A'&&text[begin]<='Z')||(text[begin]>='a'&&text[begin]<='z')||(text[begin]>='0'&&text[begin]<='9')))begin++;
  while(tail>begin&&strchr(" .,:;!?\"",text[tail-1]))tail--;
  char replacement[1024];int translated=begin<tail?display_literal(items,shorts,paragraphs,text+begin,tail-begin,replacement,sizeof replacement):0;
  if(translated==1&&!strchr(replacement,'{')&&!strchr(replacement,'`')){
   if(!display_append(assembled,sizeof assembled,&at,text+pos,begin-pos)||!display_append(assembled,sizeof assembled,&at,replacement,strlen(replacement))||!display_append(assembled,sizeof assembled,&at,text+tail,end-tail))return;
  }else if(!display_append(assembled,sizeof assembled,&at,text+pos,end-pos))return;
  pos=end;
 }
 if(at<capacity)memcpy(text,assembled,at+1);
}
static int display_piece(const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 size_t left=0,right=n;while(left<right&&s[left]==' ')left++;while(right>left&&s[right-1]==' ')right--;
 if(left==right)return 0;char translated[16384];
 /* Full literal overrides may deliberately include their surrounding spaces. */
 if(display_extra_dictionary){int exact=zh_translate_exact(display_extra_dictionary,s,n,translated,sizeof translated,NULL);
  if(exact<0)return exact;if(exact>0){size_t m=strlen(translated);if(m>=cap)return -1;memcpy(out,translated,m+1);return m!=n||memcmp(out,s,n)?1:0;}
 }
 int r=display_lookup(items,shorts,paragraphs,s+left,right-left,translated,sizeof translated);if(r<=0)return r;
 display_refine_names(items,shorts,paragraphs,translated,sizeof translated);
 size_t at=0;if(!display_append(out,cap,&at,s,left)||!display_append(out,cap,&at,translated,strlen(translated))||!display_append(out,cap,&at,s+right,n-right))return -1;
 return at!=n||memcmp(out,s,n)?1:0;
}
/* Adjacent bracketed fields are fixed columns even with one separator space. */
static int display_bracket_columns(const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 if(n<3||s[0]!='[')return 0;char assembled[32768],piece[32768];size_t pos=0,at=0;unsigned groups=0,changed=0;
 while(pos<n){
  if(s[pos]!='[')return 0;size_t end=pos+1;while(end<n&&s[end]!=']'&&s[end]!='['&&s[end]!='`')end++;if(end==n||s[end]!=']')return 0;end++;
  int r=display_piece(items,shorts,paragraphs,s+pos,end-pos,piece,sizeof piece);int cols=r==1?display_columns(piece):-1;
  if(r==1&&cols>=0&&(size_t)cols<=end-pos){changed++;if(!display_append(assembled,sizeof assembled,&at,piece,strlen(piece)))return -1;
   for(size_t q=(size_t)cols;q<end-pos;q++)if(!display_append(assembled,sizeof assembled,&at," ",1))return -1;
  }else if(!display_append(assembled,sizeof assembled,&at,s+pos,end-pos))return -1;
  groups++;pos=end;while(pos<n&&s[pos]==' '){if(!display_append(assembled,sizeof assembled,&at,s+pos,1))return -1;pos++;}
 }
 if(groups<2||!changed)return 0;if(at>=cap)return -1;memcpy(out,assembled,at+1);return 1;
}
/* Preserve formatting tokens, explicit newlines and HUD column separators.
   Whole sentences/templates are preferred; fragments are used only when the
   native string itself contains explicit formatting/column boundaries. */
static int display_translate_inner(const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 if(!s||!out||!n||n>16384||cap>32768)return 0;
 for(size_t i=0;i<n;i++)if((unsigned char)s[i]>=127||((unsigned char)s[i]<32&&s[i]!='\n'))return 0;
 int bracket=display_bracket_columns(items,shorts,paragraphs,s,n,out,cap);if(bracket)return bracket;
 int segmented=memchr(s,'`',n)!=NULL;for(size_t i=1;i<n;i++)if(s[i]==' '&&s[i-1]==' ')segmented=1;
 char temporary[32768],piece[32768];int r=segmented?0:display_piece(items,shorts,paragraphs,s,n,temporary,sizeof temporary);
 if(r==1){if(strlen(temporary)>=cap)return -1;strcpy(out,temporary);return 1;}
 size_t at=0,pos=0;int changed=0;
 while(pos<n){
  if(s[pos]=='`'){size_t end=pos+1;while(end<n&&s[end]!='`')end++;if(end==n||end-pos>32)return 0;
   if(!display_append(temporary,sizeof temporary,&at,s+pos,end-pos+1))return -1;pos=end+1;continue;
  }
  if(s[pos]=='\n'||(s[pos]==' '&&pos+1<n&&s[pos+1]==' ')){
   size_t end=pos+1;if(s[pos]==' ')while(end<n&&s[end]==' ')end++;
   if(!display_append(temporary,sizeof temporary,&at,s+pos,end-pos))return -1;pos=end;continue;
  }
  size_t end=pos;while(end<n&&s[end]!='`'&&s[end]!='\n'&&!(s[end]==' '&&end+1<n&&s[end+1]==' '))end++;
  r=display_piece(items,shorts,paragraphs,s+pos,end-pos,piece,sizeof piece);
  if(r==1){
   size_t physical=0;int valid=1;for(size_t q=0;q<strlen(piece);){size_t used;uint32_t cp;if(!zh_decode(piece+q,strlen(piece)-q,&used,&cp)){valid=0;break;}physical+=cp<128?1:2;q+=used;}
   if(segmented&&(!valid||physical>end-pos)){if(!display_append(temporary,sizeof temporary,&at,s+pos,end-pos))return -1;}
   else{changed=1;if(!display_append(temporary,sizeof temporary,&at,piece,strlen(piece)))return -1;
    if(segmented)for(size_t pad=physical;pad<end-pos;pad++)if(!display_append(temporary,sizeof temporary,&at," ",1))return -1;
   }
  }
  else if(!display_append(temporary,sizeof temporary,&at,s+pos,end-pos))return -1;
  pos=end;
 }
 if(!changed||at>=cap)return changed?-1:0;
 memcpy(out,temporary,at+1);return 1;
}
/* Aggregation suffix is a separate typed field, never a wildcard spelling.
   Translate the body, then retain the exact native count marker and digits. */
static int display_translate(const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 if(!s||!out||!n||n>16384||cap>32768)return 0;
 if(n>=6&&s[n-1]=='>'){
  size_t open=n-2;while(open&&s[open]!='<')open--;
  if(open>1&&s[open-1]==' '&&(s[open+1]=='x'||s[open+1]=='X')&&n-open-3>=1&&n-open-3<=16){
   int digits=1;for(size_t i=open+2;i<n-1;i++)if(s[i]<'0'||s[i]>'9')digits=0;
   if(digits){char translated[32768];int r=display_translate_inner(items,shorts,paragraphs,s,open-1,translated,sizeof translated);
    if(r!=1)return r;size_t m=strlen(translated),suffix=n-open+1;if(m+suffix>=cap)return -1;
    memcpy(out,translated,m);memcpy(out+m,s+open-1,suffix);out[m+suffix]=0;return 1;
   }
  }
 }
 return display_translate_inner(items,shorts,paragraphs,s,n,out,cap);
}
#endif

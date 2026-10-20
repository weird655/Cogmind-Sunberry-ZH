#include "text_core.h"
#include "terminal_normalize.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
typedef struct {char *key,*value;const char *anchor;size_t keylen,prefix,next_pattern,anchor_length;unsigned specificity;int pattern;} Rule;
typedef struct {char *key;size_t rule;int conflict;} NormalRule;
struct ZhDictionary {char *data;Rule *rules;size_t count,*table,table_size,pattern_heads[257];NormalRule *normal;size_t *normal_table;Rule **literals;size_t literal_count;};
int zh_rule_key_is(const ZhDictionary *d,size_t rule,const char *key){return d&&key&&rule<d->count&&!strcmp(d->rules[rule].key,key);}
unsigned zh_rule_specificity(const ZhDictionary *d,size_t rule){return d&&rule<d->count?d->rules[rule].specificity:0;}
int zh_rule_is_template(const ZhDictionary *d,size_t rule){return d&&rule<d->count?d->rules[rule].pattern:0;}
typedef struct {const char *p;size_t n;int set,numeric;} Capture;
int zh_decode(const char *text,size_t n,size_t *used,uint32_t *out){
 const unsigned char *s=(const unsigned char*)text;if(!s||!n)return 0;
 uint32_t c=s[0],minimum;unsigned bytes;
 if(c<128){*used=1;*out=c;return 1;}
 if(c>=0xc2&&c<=0xdf){bytes=2;c&=31;minimum=128;}
 else if(c>=0xe0&&c<=0xef){bytes=3;c&=15;minimum=2048;}
 else if(c>=0xf0&&c<=0xf4){bytes=4;c&=7;minimum=65536;}else return 0;
 if(n<bytes)return 0;
 for(unsigned i=1;i<bytes;i++){if((s[i]&0xc0)!=0x80)return 0;c=(c<<6)|(s[i]&63);}
 if(c<minimum||c>0x10ffff||(c>=0xd800&&c<=0xdfff))return 0;
 *used=bytes;*out=c;return 1;
}
static int valid_utf8(const char *s,size_t n){
 for(size_t pos=0;pos<n;){size_t used;uint32_t cp;if(!zh_decode(s+pos,n-pos,&used,&cp)||!cp)return 0;pos+=used;}return 1;
}
int zh_glyph(uint32_t cp){
 size_t lo=0,hi=zh_registry_count;while(lo<hi){size_t mid=lo+(hi-lo)/2;if(zh_registry[mid]<cp)lo=mid+1;else hi=mid;}
 return lo<zh_registry_count&&zh_registry[lo]==cp?(int)lo:-1;
}
static uint64_t hash_key(const char *p,size_t n){uint64_t h=14695981039346656037ULL;for(size_t i=0;i<n;i++){h^=(unsigned char)p[i];h*=1099511628211ULL;}return h;}
static int placeholder(const char *p){return p[0]=='{'&&p[1]&&p[2]=='}'&&(p[1]=='n'||p[1]=='*'||(p[1]>='1'&&p[1]<='9'));}
static void error_text(char *out,size_t cap,const char *s){if(out&&cap)snprintf(out,cap,"%s",s);}
void zh_dictionary_free(ZhDictionary *d){if(d){if(d->normal){for(size_t i=0;i<d->count;i++)free(d->normal[i].key);}free(d->literals);free(d->normal);free(d->normal_table);free(d->table);free(d->rules);free(d->data);free(d);}}
static int literal_compare(const void *a,const void *b){return strcmp((*(Rule *const*)a)->key,(*(Rule *const*)b)->key);}
ZhDictionary *zh_dictionary_load(const char *path,size_t *count,char *error,size_t cap){
 FILE *f=fopen(path,"rb");if(!f){error_text(error,cap,"open failed");return NULL;}
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long length=ftell(f);rewind(f);
 if(length<1||length>16*1024*1024){fclose(f);error_text(error,cap,"size rejected");return NULL;}
 ZhDictionary *d=calloc(1,sizeof *d);if(!d){fclose(f);return NULL;}
 d->data=malloc((size_t)length+1);d->rules=calloc(100000,sizeof *d->rules);
 if(!d->data||!d->rules){fclose(f);zh_dictionary_free(d);return NULL;}
 size_t got=fread(d->data,1,length,f);fclose(f);if(got!=(size_t)length){zh_dictionary_free(d);return NULL;}d->data[got]=0;
 if(!valid_utf8(d->data,got)){error_text(error,cap,"invalid UTF-8");zh_dictionary_free(d);return NULL;}
 char *p=d->data;if(got>=3&&!memcmp(p,"\xef\xbb\xbf",3))p+=3;
 while(*p){char *line=p,*end=strchr(p,'\n');if(end){*end=0;p=end+1;}else p+=strlen(p);
  size_t len=strlen(line);if(len&&line[len-1]=='\r')line[--len]=0;
  char *nonspace=line;while(*nonspace==' '||*nonspace=='\t')nonspace++;
  if(!*nonspace||*nonspace=='#')continue;
  char *tab=strchr(line,'\t');if(!tab||tab==line||!tab[1]||strchr(tab+1,'\t')||d->count>=100000){error_text(error,cap,"malformed TSV row");zh_dictionary_free(d);return NULL;}
  *tab=0;Rule *r=&d->rules[d->count++];r->key=line;r->value=tab+1;r->keylen=strlen(line);
  for(size_t i=0;i<r->keylen;){if(placeholder(line+i)){r->pattern=1;i+=3;}else {r->specificity++;i++;}}
 }
 d->table_size=1;while(d->table_size<d->count*2)d->table_size*=2;
 d->table=calloc(d->table_size,sizeof *d->table);if(!d->table){zh_dictionary_free(d);return NULL;}
 for(size_t i=0;i<d->count;i++){
  size_t at=(size_t)hash_key(d->rules[i].key,d->rules[i].keylen)&(d->table_size-1);
  while(d->table[at]){Rule *prev=&d->rules[d->table[at]-1];if(!strcmp(prev->key,d->rules[i].key)){error_text(error,cap,"duplicate key");zh_dictionary_free(d);return NULL;}at=(at+1)&(d->table_size-1);}
  d->table[at]=i+1;
 }
 /* Build once, never scan the full corpus from the per-frame fallback. */
 d->normal=calloc(d->count?d->count:1,sizeof *d->normal);
 d->normal_table=calloc(d->table_size,sizeof *d->normal_table);
 if(!d->normal||!d->normal_table){zh_dictionary_free(d);return NULL;}
 for(size_t i=0;i<d->count;i++){
  Rule *r=&d->rules[i];char key[16385];
  if(r->pattern||terminal_normalize(r->key,r->keylen,key,sizeof key)!=1||!key[0])continue;
  size_t at=(size_t)hash_key(key,strlen(key))&(d->table_size-1);
  while(d->normal_table[at]){
   NormalRule *old=&d->normal[d->normal_table[at]-1];
   if(!strcmp(old->key,key)){if(strcmp(d->rules[old->rule].value,r->value))old->conflict=1;break;}
   at=(at+1)&(d->table_size-1);
  }
  if(d->normal_table[at])continue;
  d->normal[i].key=malloc(strlen(key)+1);if(!d->normal[i].key){zh_dictionary_free(d);return NULL;}
  strcpy(d->normal[i].key,key);d->normal[i].rule=i;d->normal_table[at]=i+1;
 }
 /* Reverse insertion gives ascending source rule indices in each bucket. */
 for(size_t i=d->count;i>0;i--){Rule *r=&d->rules[i-1];if(!r->pattern)continue;
  while(r->prefix<r->keylen&&!placeholder(r->key+r->prefix))r->prefix++;
  /* Any matched source must contain every literal chunk. The longest chunk
     cheaply rejects unrelated suffix/name rules before bounded backtracking. */
  for(size_t at=0;at<r->keylen;){
   if(placeholder(r->key+at)){at+=3;continue;}
   size_t start=at;while(at<r->keylen&&!placeholder(r->key+at))at++;
   if(at-start>r->anchor_length){r->anchor=r->key+start;r->anchor_length=at-start;}
  }
  unsigned bucket=r->prefix?(unsigned char)r->key[0]:256;
  r->next_pattern=d->pattern_heads[bucket];d->pattern_heads[bucket]=i;
 }
 d->literals=calloc(d->count?d->count:1,sizeof *d->literals);
 if(!d->literals){zh_dictionary_free(d);return NULL;}
 for(size_t i=0;i<d->count;i++)if(!d->rules[i].pattern)d->literals[d->literal_count++]=&d->rules[i];
 qsort(d->literals,d->literal_count,sizeof *d->literals,literal_compare);
 if(count)*count=d->count;error_text(error,cap,"ok");return d;
}
int zh_translate_exact(const ZhDictionary *d,const char *src,size_t n,char *out,size_t cap,size_t *rule){
 if(!d||!src||!out||!cap||n>16384)return -1;
 size_t at=(size_t)hash_key(src,n)&(d->table_size-1);
 while(d->table[at]){size_t i=d->table[at]-1;Rule *r=&d->rules[i];if(r->keylen==n&&!memcmp(r->key,src,n)){size_t m=strlen(r->value);if(m>=cap)return -1;memcpy(out,r->value,m+1);if(rule)*rule=i;return 1;}at=(at+1)&(d->table_size-1);}return 0;
}
int zh_translate_normalized_exact(const ZhDictionary *d,const char *src,size_t n,char *out,size_t cap,size_t *rule){
 if(!d||!src||!out||!cap)return -1;char key[16385];
 int normalized=terminal_normalize(src,n,key,sizeof key);if(normalized!=1)return normalized<0?-1:0;
 size_t length=strlen(key),at=(size_t)hash_key(key,length)&(d->table_size-1);
 while(d->normal_table[at]){
  NormalRule *r=&d->normal[d->normal_table[at]-1];
  if(!strcmp(r->key,key)){
   if(r->conflict)return -2;const char *value=d->rules[r->rule].value;size_t m=strlen(value);
   if(m>=cap)return -1;memcpy(out,value,m+1);if(rule)*rule=r->rule;return 1;
  }
  at=(at+1)&(d->table_size-1);
 }
 return 0;
}
static int match(const char *p,const char *s,size_t n,size_t pos,Capture c[9],unsigned *budget){
 if(!*budget)return 0;(*budget)--;
 while(*p&&!placeholder(p)){if(pos==n||*p!=s[pos])return 0;p++;pos++;}
 if(!*p)return pos==n;
 int id=p[1]>='1'&&p[1]<='9'?p[1]-'1':-1;
 if(id<0){id=0;while(id<9&&c[id].set)id++;if(id==9)return 0;}
 if(c[id].set){if(pos+c[id].n>n||memcmp(c[id].p,s+pos,c[id].n))return 0;return match(p+3,s,n,pos+c[id].n,c,budget);}
 size_t first=pos+1,last=n;
 if(p[1]=='n'){
  size_t q=pos;if(q<n&&(s[q]=='+'||s[q]=='-'))q++;
  size_t digits=q;while(q<n&&s[q]>='0'&&s[q]<='9')q++;if(q==digits)return 0;first=digits+1;
  if(q<n&&s[q]=='.'&&q+1<n&&s[q+1]>='0'&&s[q+1]<='9'){q++;while(q<n&&s[q]>='0'&&s[q]<='9')q++;}last=q;
 }
 for(size_t end=last;end>=first;end--){
  if(p[1]=='n'&&(s[end-1]<'0'||s[end-1]>'9'))continue;
  Capture copy[9];memcpy(copy,c,sizeof copy);copy[id]=(Capture){s+pos,end-pos,1,p[1]=='n'};
  if(match(p+3,s,n,end,copy,budget)){memcpy(c,copy,sizeof copy);return 1;}
  if(!*budget)break;
 }
 return 0;
}
static int expand(const char *p,const Capture c[9],char *out,size_t cap){
 size_t at=0;unsigned number=0,wild=0;
 while(*p){if(placeholder(p)){
   int id=p[1]>='1'&&p[1]<='9'?p[1]-'1':-1;
   if(id<0){int matches[9],count=0;for(int i=0;i<9;i++)if(c[i].set&&c[i].numeric==(p[1]=='n'))matches[count++]=i;
    unsigned which=p[1]=='n'?number++:wild++;if(!count||(count>1&&which>=(unsigned)count))return 0;id=matches[count==1?0:which];
   }
   if(!c[id].set||at+c[id].n>=cap)return 0;memcpy(out+at,c[id].p,c[id].n);at+=c[id].n;p+=3;
  }else {if(at+1>=cap)return 0;out[at++]=*p++;}
 }
 out[at]=0;return 1;
}
static int contains_literal(const char *text,size_t n,const char *literal,size_t length){
 if(!length)return 1;if(length>n)return 0;
 for(size_t at=0;at<=n-length;at++)if(text[at]==literal[0]&&!memcmp(text+at,literal,length))return 1;return 0;
}
int zh_translate(const ZhDictionary *d,const char *src,size_t n,char *out,size_t cap,size_t *rule){
 int exact=zh_translate_exact(d,src,n,out,cap,rule);if(exact)return exact;
 if(!valid_utf8(src,n)||cap>65536)return -1;
 char *candidate=malloc(cap),*best=malloc(cap);if(!candidate||!best){free(candidate);free(best);return -1;}
 int found=0,ambiguous=0;unsigned score=0;size_t winner=0;
 size_t literal=n?d->pattern_heads[(unsigned char)src[0]]:0,wild=d->pattern_heads[256];
 while(literal||wild){
  /* Merge source-ordered buckets, retaining original same-output winner ID. */
  size_t selected=!wild||(literal&&literal<wild)?literal:wild;
  size_t i=selected-1;Rule *r=&d->rules[i];
  if(selected==literal)literal=r->next_pattern;else wild=r->next_pattern;
  if((found&&r->specificity<score)||r->prefix>n||memcmp(r->key,src,r->prefix)||!contains_literal(src,n,r->anchor,r->anchor_length))continue;
  Capture captures[9]={0};unsigned budget=8192;
  if(!match(r->key,src,n,0,captures,&budget)||!expand(r->value,captures,candidate,cap))continue;
  if(!found||r->specificity>score){strcpy(best,candidate);score=r->specificity;winner=i;found=1;ambiguous=0;}
  else if(strcmp(best,candidate))ambiguous=1;
 }
 if(found&&!ambiguous){strcpy(out,best);if(rule)*rule=winner;}
 free(candidate);free(best);return ambiguous?-2:found?2:0;
}
void zh_plan_free(ZhPlan *p){if(p){free(p->runs);memset(p,0,sizeof *p);}}
int zh_plan_row(const ZhPlan *source,int row,ZhPlan *out){
 if(!source||!out||row<0||row>=source->used_rows)return -1;
 ZhPlan p={0};p.rows=1;p.used_rows=1;p.style_count=source->style_count;memcpy(p.styles,source->styles,sizeof p.styles);
 for(size_t i=0;i<source->count;i++)if(source->runs[i].y==row)p.count++;
 p.runs=calloc(p.count?p.count:1,sizeof *p.runs);if(!p.runs)return -1;
 size_t at=0;for(size_t i=0;i<source->count;i++)if(source->runs[i].y==row){p.runs[at]=source->runs[i];p.runs[at].y=0;int end=p.runs[at].x+p.runs[at].width;if(end>p.columns)p.columns=end;at++;}
 if(!p.columns)p.columns=1;*out=p;return 0;
}
int zh_plan(const char *text,size_t n,int cols,int rows,ZhPlan *out){
 if(!text||!out||n>16384||cols<1||cols>512||rows<1||rows>512)return -1;
 ZhPlan p={0};p.runs=calloc(n+1,sizeof *p.runs);if(!p.runs)return -1;p.columns=cols;p.rows=rows;p.style_count=1;
 int x=0,y=0;unsigned style=0;
 for(size_t pos=0;pos<n;){
  if(text[pos]=='`'){
   size_t end=pos+1;while(end<n&&text[end]!='`')end++;
   size_t length=end-pos-1;if(end==n||!length||length>=32||p.style_count>=32)goto invalid;
   for(size_t i=pos+1;i<end;i++)if((unsigned char)text[i]<32||(unsigned char)text[i]>=127)goto invalid;
   unsigned next=p.style_count++;size_t previous=strlen(p.styles[style]);
   if(length==1&&text[pos+1]=='x')previous=0;
   if(previous+length+2>=sizeof p.styles[0])goto invalid;
   memcpy(p.styles[next],p.styles[style],previous);p.styles[next][previous]='`';memcpy(p.styles[next]+previous+1,text+pos+1,length);p.styles[next][previous+length+1]='`';style=next;pos=end+1;continue;
  }
  size_t used;uint32_t cp;if(!zh_decode(text+pos,n-pos,&used,&cp))goto invalid;
  if(cp=='\n'){pos+=used;x=0;y++;if(y>=rows&&pos<n)goto overflow;continue;}
  if(cp<32||cp==127)goto invalid;
  if(cp>=33&&cp<127&&(!pos||text[pos-1]==' '||text[pos-1]=='\n')){
   size_t end=pos;while(end<n&&(unsigned char)text[end]>=33&&(unsigned char)text[end]<127&&text[end]!='`')end++;
   size_t word=end-pos;if(word<=(size_t)cols&&x+(int)word>cols){x=0;y++;}
  }
  int width=cp<128?1:2,index=cp<128?-1:zh_glyph(cp);if(cp>=128&&index<0)goto invalid;
  if(width>cols)goto overflow;if(x+width>cols){x=0;y++;}if(y>=rows)goto overflow;
  ZhRun *r=&p.runs[p.count++];r->scalar=cp;r->x=x;r->y=y;r->width=width;r->style=style;
  r->token[0]=cp<128?cp:320+index*2;r->token[1]=width==2?r->token[0]+1:0;x+=width;pos+=used;
 }
 p.used_rows=p.count?y+1:0;*out=p;return 0;
overflow:zh_plan_free(&p);return -2;
invalid:zh_plan_free(&p);return -3;
}

/* For native ellipsis labels only. Binary search literal keys; never templates.
   Any second completion is ambiguous, even if its translation is identical. */
int zh_translate_unique_prefix(const ZhDictionary *d,const char *src,size_t n,char *out,size_t cap){
 if(!d||!src||!out||n<12)return 0;
 size_t lo=0,hi=d->literal_count;
 while(lo<hi){size_t mid=lo+(hi-lo)/2;if(strncmp(d->literals[mid]->key,src,n)<0)lo=mid+1;else hi=mid;}
 if(lo==d->literal_count||strncmp(d->literals[lo]->key,src,n))return 0;
 if(lo+1<d->literal_count&&!strncmp(d->literals[lo+1]->key,src,n))return -2;
 const Rule *r=d->literals[lo];if(r->keylen<=n)return 0;
 size_t size=strlen(r->value);if(size>=cap)return -1;memcpy(out,r->value,size+1);return 1;
}

#ifndef COGMIND_REPAIR_TEXT_H
#define COGMIND_REPAIR_TEXT_H
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "canonical_item_names.h"
static const ZhDictionary *repair_extra_dictionary,*repair_short_dictionary,*repair_paragraph_dictionary;

/* Strict complete five-line repair scan response. 1=translated, 0=not this
   grammar, -1=output/translation capacity failure. Failure never changes out.
   Numbers are strings, never parsed or reformatted. Input is length-delimited. */
#define REPAIR_PART_LIMIT 96u
#define REPAIR_NUMBER_LIMIT 16u
static int repair_safe_label(const char *p,size_t n,int ascii_only){
 size_t at=0;uint32_t cp;
 if(!n||p[0]==' '||p[n-1]==' ')return 0;
 while(at<n){
  size_t used;if(zh_decode(p+at,n-at,&used,&cp)!=1)return 0;at+=used;
  if(cp<32||cp==127||(ascii_only&&cp>126)||cp=='`'||cp=='['||cp==']'||cp=='{'||cp=='}'||cp=='<'||cp=='>')return 0;
 }
 return 1;
}
static int repair_digits(const char *p,size_t n,int rating){
 if(rating&&n&&p[n-1]=='p')n--;
 if(!n||n>REPAIR_NUMBER_LIMIT)return 0;
 for(size_t i=0;i<n;i++)if(p[i]<'0'||p[i]>'9')return 0;
 return 1;
}
static int repair_translate_stage(const ZhDictionary *dictionary,const char *input,size_t length,char *out,size_t capacity){
 const char *line[5];size_t size[5],at=0;
 static const char first[]="Scanning ",second[]="Ready to Repair:",fourth[]="  Rating: ",fifth[]="  Time: ";
 if(!dictionary||!input||!out)return -1;
 if(length>512||memchr(input,0,length)||memchr(input,'\r',length))return 0;
 for(unsigned i=0;i<5;i++){
  line[i]=input+at;const char *end=memchr(input+at,'\n',length-at);
  if(i<4){if(!end)return 0;size[i]=(size_t)(end-line[i]);at=(size_t)(end-input)+1;}
  else {if(end)return 0;size[i]=length-at;}
 }
 if(size[0]<sizeof first-1+4||memcmp(line[0],first,sizeof first-1)||memcmp(line[0]+size[0]-3,"...",3))return 0;
 size_t part_length=size[0]-(sizeof first-1)-3;const char *part=line[0]+sizeof first-1;
 if(part_length>REPAIR_PART_LIMIT||!repair_safe_label(part,part_length,1))return 0;
 if(size[1]!=sizeof second-1||memcmp(line[1],second,sizeof second-1))return 0;
 if(size[2]!=part_length+2||memcmp(line[2],"  ",2)||memcmp(line[2]+2,part,part_length))return 0;
 if(size[3]<=sizeof fourth-1||memcmp(line[3],fourth,sizeof fourth-1))return 0;
 if(size[4]<=sizeof fifth-1||memcmp(line[4],fifth,sizeof fifth-1))return 0;
 const char *rating=line[3]+sizeof fourth-1,*time=line[4]+sizeof fifth-1;
 size_t rating_length=size[3]-(sizeof fourth-1),time_length=size[4]-(sizeof fifth-1);
 if(!repair_digits(rating,rating_length,1)||!repair_digits(time,time_length,0))return 0;
 char translated[512],assembled[1536];
 int match=canonical_item_name_translate(repair_extra_dictionary,dictionary,repair_short_dictionary,repair_paragraph_dictionary,part,part_length,translated,sizeof translated);
 if(match<0)return -1;
 const char *label=part;size_t label_length=part_length;
 if(match==1){label=translated;label_length=strlen(translated);if(!repair_safe_label(label,label_length,0))return 0;}
 int count=snprintf(assembled,sizeof assembled,"正在扫描 %.*s……\n准备修理：\n  %.*s\n  评级：%.*s\n  耗时：%.*s",
  (int)label_length,label,(int)label_length,label,(int)rating_length,rating,(int)time_length,time);
 if(count<0||(size_t)count>=sizeof assembled||(size_t)count>=capacity)return -1;
 memcpy(out,assembled,(size_t)count+1);return 1;
}
static int repair_translate_begin(const ZhDictionary *dictionary,const char *input,size_t length,char *out,size_t capacity){
 static const char prefix[]="Repairing ",etc[]="ETC: ";
 if(!dictionary||!input||!out)return -1;if(length>512||memchr(input,0,length)||memchr(input,'\r',length))return 0;
 const char *lf=memchr(input,'\n',length);if(!lf||memchr(lf+1,'\n',length-(size_t)(lf+1-input)))return 0;
 size_t first=(size_t)(lf-input);if(first<sizeof prefix+3||memcmp(input,prefix,sizeof prefix-1)||memcmp(lf-3,"...",3))return 0;
 const char *part=input+sizeof prefix-1;size_t n=first-(sizeof prefix-1)-3;
 if(n>REPAIR_PART_LIMIT||!repair_safe_label(part,n,1))return 0;
 const char *second=lf+1;size_t tail=length-(size_t)(second-input);if(tail<sizeof etc||memcmp(second,etc,sizeof etc-1))return 0;
 const char *number=second+sizeof etc-1;size_t digits=tail-(sizeof etc-1);if(!repair_digits(number,digits,0))return 0;
 char label[512],assembled[1536];int match=canonical_item_name_translate(repair_extra_dictionary,dictionary,repair_short_dictionary,repair_paragraph_dictionary,part,n,label,sizeof label);if(match<0)return -1;
 if(match==1){part=label;n=strlen(label);if(!repair_safe_label(part,n,0))return 0;}
 int written=snprintf(assembled,sizeof assembled,"正在修理 %.*s……\n预计耗时：%.*s",(int)n,part,(int)digits,number);
 if(written<0||(size_t)written>=sizeof assembled||(size_t)written>=capacity)return -1;
 memcpy(out,assembled,(size_t)written+1);return 1;
}
#endif

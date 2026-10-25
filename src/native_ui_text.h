#ifndef COGMIND_NATIVE_UI_TEXT_H
#define COGMIND_NATIVE_UI_TEXT_H
#include "native_ui_items.h"
#include "native_ui_robots.h"
/* Display forms recovered from the native HUD and Pay2Buy constructors.
   Only complete forms are accepted. Item slots require dictionary membership;
   native colors, turn numbers and wrapping remain owned by the caller. */
static int native_ui_copy(const char *s,size_t n,char *out,size_t cap){
 if(n>=cap)return -1;memcpy(out,s,n);out[n]=0;return 1;
}
static int native_ui_uint(const char *s,size_t n,unsigned limit){
 if(!n||n>10)return 0;unsigned value=0;
 for(size_t i=0;i<n;i++){
  if(s[i]<'0'||s[i]>'9')return 0;
  unsigned digit=(unsigned)(s[i]-'0');if(digit>limit||value>(limit-digit)/10)return 0;
  value=value*10+digit;
 }
 return 1;
}
/* Complete observed descriptors only; strict integer and native robot slots. */
static int native_ui_descriptor(const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 if(n>4&&!memcmp(s,">>",2)&&((n>11&&!memcmp(s+2,"Analysis!",9))||(n>8&&!memcmp(s+2,"Query(",6)))){
  char body[4096];int r=native_ui_descriptor(items,shorts,paragraphs,s+2,n-2,body,sizeof body);if(r!=1)return r;
  int w=snprintf(out,cap,">>%s",body);return w>=0&&(size_t)w<cap?1:-1;
 }
 const char *time="Time ";size_t a=strlen(time);
 if(n>a&&!memcmp(s,time,a)&&native_ui_uint(s+a,n-a,2147483647u)){
  int w=snprintf(out,cap,"时间 %.*s",(int)(n-a),s+a);return w>=0&&(size_t)w<cap?1:-1;
 }
 const char *link="Linking terminal botnet (",*tail=")...";a=strlen(link);size_t b=strlen(tail);
 if(n>a+b&&!memcmp(s,link,a)&&!memcmp(s+n-b,tail,b)&&native_ui_uint(s+a,n-a-b,2147483647u)){
  int w=snprintf(out,cap,"正在连接终端僵尸网络 (%.*s)...",(int)(n-a-b),s+a);return w>=0&&(size_t)w<cap?1:-1;
 }
 const char *analysis="Analysis! \"";a=strlen(analysis);
 if(n>a+1&&!memcmp(s,analysis,a)&&s[n-1]=='"'&&!memchr(s+a,'"',n-a-1)){
  if(!native_robot_name_known(s+a,n-a-1))return 0;
  char name[2048];int r=content_name_lookup(display_extra_dictionary,items,shorts,paragraphs,s+a,n-a-1,name,sizeof name);if(r!=1)return r;
  int w=snprintf(out,cap,"分析! \"%s\"",name);return w>=0&&(size_t)w<cap?1:-1;
 }
 static const char *facilities[]={"Scanalyzer","Garrison Access","Terminal"};
 for(unsigned i=0;i<sizeof facilities/sizeof facilities[0];i++){
  a=strlen(facilities[i]);if(n!=a+7||memcmp(s,"Query(",6)||memcmp(s+6,facilities[i],a)||s[n-1]!=')')continue;
  char name[2048];int r=content_name_lookup(display_extra_dictionary,items,shorts,paragraphs,facilities[i],a,name,sizeof name);if(r!=1)return r;
  int w=snprintf(out,cap,"查询(%s)",name);return w>=0&&(size_t)w<cap?1:-1;
 }
 return 0;
}
static int native_ui_text(const ZhDictionary *items,const ZhDictionary *shorts,const ZhDictionary *paragraphs,const char *s,size_t n,char *out,size_t cap){
 int dynamic=native_ui_descriptor(items,shorts,paragraphs,s,n,out,cap);if(dynamic)return dynamic;
 static const struct {const char *source,*target;} literals[]={
  {">>> MATTER WARNING <<<",">>> 物质警告 <<<"},
  {"CogCoins","齿币"},{"Loot Box","战利品箱"},{"What's inside?","里面有什么？"},
  {"You open your loot box...","你打开了战利品箱……"},{"It's empty?!","竟然是空的？！"},
  {"\\ C O G S H O P \\","\\ 齿 轮 商 店 \\"},
  {"[BUY (9)]","[购买(9)]"},
  {"Congratulations on your consolation prize:","恭喜获得安慰奖："},
  {"Congratulations on your very own...","恭喜获得属于你的……"},
  {"[SPECIAL MODE ACTIVATED: AFD2019/Pay2Buy]","[特殊模式已启用：AFD2019/Pay2Buy]"},
  {"[Congratulations! You are victorious! The exit is now open...]","[恭喜！你胜利了！出口现已开启……]"}
 };
 for(unsigned i=0;i<sizeof literals/sizeof literals[0];i++){
  size_t bytes=strlen(literals[i].source);
  if(n==bytes&&!memcmp(s,literals[i].source,n))return native_ui_copy(literals[i].target,strlen(literals[i].target),out,cap);
  if(n==bytes+2&&s[0]=='['&&s[n-1]==']'&&literals[i].source[0]!='['&&!memcmp(s+1,literals[i].source,bytes)){
   int written=snprintf(out,cap,"[%s]",literals[i].target);
   return written>=0&&(size_t)written<cap?1:-1;
  }
 }
 /* A colorized exposure field is printed separately from core durability. */
 const char *suffix="% exposed)";size_t z=strlen(suffix);
 if(n>z+1&&s[0]=='('&&!memcmp(s+n-z,suffix,z)&&native_ui_uint(s+1,n-z-1,100)){
  int written=snprintf(out,cap,"(暴露%.*s%%)",(int)(n-z-1),s+1);
  return written>=0&&(size_t)written<cap?1:-1;
 }
 /* Native log wrappers may enclose the complete reward message in brackets. */
 const char *source=s;size_t bytes=n;int bracket=n>=2&&s[0]=='['&&s[n-1]==']';
 if(bracket){source++;bytes-=2;}
 static const struct {const char *prefix,*suffix,*target_prefix,*target_suffix;} named[]={
  {"Congratulations on your very own ","!","恭喜获得属于你的","！"},
  {"Purchased: ","","已购买：",""}
 };
 for(unsigned i=0;i<sizeof named/sizeof named[0];i++){
  size_t a=strlen(named[i].prefix),b=strlen(named[i].suffix);
  if(bytes<=a+b||memcmp(source,named[i].prefix,a)||(b&&memcmp(source+bytes-b,named[i].suffix,b)))continue;
  if(!native_ui_item_name_known(source+a,bytes-a-b))return 0;
  char name[2048];int r=canonical_item_name_translate(display_extra_dictionary,items,shorts,paragraphs,source+a,bytes-a-b,name,sizeof name);
  if(r!=1)return r;
  int written=snprintf(out,cap,"%s%s%s%s%s",bracket?"[":"",named[i].target_prefix,name,named[i].target_suffix,bracket?"]":"");
  return written>=0&&(size_t)written<cap?1:-1;
 }
 return 0;
}
#endif

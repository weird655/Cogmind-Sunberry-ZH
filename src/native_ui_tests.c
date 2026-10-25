#include "terminal_text.h"
static const ZhDictionary *item_dictionary,*short_dictionary,*dictionary;
#include "message_resolver.h"
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"native UI line %d\n",__LINE__);return 1;}}while(0)
int main(int argc,char **argv){
 if(argc!=6)return 2;char error[256],out[32768];
 dictionary=zh_dictionary_load(argv[1],NULL,error,sizeof error);
 short_dictionary=zh_dictionary_load(argv[2],NULL,error,sizeof error);
 display_extra_dictionary=zh_dictionary_load(argv[3],NULL,error,sizeof error);
 item_dictionary=zh_dictionary_load(argv[4],NULL,error,sizeof error);
 terminal_runtime_dictionary=zh_dictionary_load(argv[5],NULL,error,sizeof error);
 CHECK(dictionary&&short_dictionary&&display_extra_dictionary&&item_dictionary&&terminal_runtime_dictionary);
 terminal_item_dictionary=item_dictionary;
 static const struct {const char *source,*target;} cases[]={
  {"CogCoins","齿币"},{"Loot Box","战利品箱"},{"What's inside?","里面有什么？"},
  {"You open your loot box...","你打开了战利品箱……"},{"[You open your loot box...]","[你打开了战利品箱……]"},
  {"It's empty?!","竟然是空的？！"},{"[It's empty?!]","[竟然是空的？！]"},
  {"\\ C O G S H O P \\","\\ 齿 轮 商 店 \\"},{"[BUY (9)]","[购买(9)]"},
  {"(0% exposed)","(暴露0%)"},{"(22% exposed)","(暴露22%)"},{"(100% exposed)","(暴露100%)"},
  {"Congratulations on your very own Ion Engine!","恭喜获得属于你的离子引擎！"},
  {"[Congratulations on your very own Wheel!]","[恭喜获得属于你的轮！]"},
  {"Purchased: Ion Engine","已购买：离子引擎"},
  {"[Purchased: Lgt. Assault Rifle]","[已购买：轻型突击步枪]"},
  {"[SPECIAL MODE ACTIVATED: AFD2019/Pay2Buy]","[特殊模式已启用：AFD2019/Pay2Buy]"},
  {"[Congratulations! You are victorious! The exit is now open...]","[恭喜！你胜利了！出口现已开启……]"}
 };
 for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++){
  int r=message_resolve_entry(TEXT_ENTRY_DISPLAY,cases[i].source,strlen(cases[i].source),out,sizeof out,NULL);
  if(r!=1||strcmp(out,cases[i].target))fprintf(stderr,"source=%s actual=%s expected=%s\n",cases[i].source,out,cases[i].target);
  CHECK(r==1&&!strcmp(out,cases[i].target));
  strcpy(out,"sentinel");CHECK(message_resolve_entry(TEXT_ENTRY_DISPLAY,cases[i].source,strlen(cases[i].source),out,2,NULL)<0&&!strcmp(out,"sentinel"));
 }
 const char *bad[]={"(101% exposed)","(-1% exposed)","(1.5% exposed)","(1e2% exposed)","(% exposed)","(22% exposed) garbage",
  "(4294967296% exposed)","(22% exposed", "Congratulations on your very own Unknown Test Thing!",
  "Congratulations on your very own Ion Engine", "[Congratulations on your very own Ion Engine!",
  "Purchased: Access granted.","Purchased: RA-1N3","Purchased: Ion Engine\nWiping memory..."};
 for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++){
  strcpy(out,"sentinel");CHECK(native_ui_text(item_dictionary,short_dictionary,dictionary,bad[i],strlen(bad[i]),out,sizeof out)==0&&!strcmp(out,"sentinel"));
 }
 const char *colored="`f0` (22% exposed)`x`";
 CHECK(message_resolve_entry(TEXT_ENTRY_DISPLAY,colored,strlen(colored),out,sizeof out,NULL)==1&&!strcmp(out,"`f0` (暴露22%)    `x`"));
 const char *timestamp="01234_ [Congratulations on your very own Wheel!]";
 CHECK(message_resolve_entry(TEXT_ENTRY_DISPLAY,timestamp,strlen(timestamp),out,sizeof out,NULL)==1&&!strcmp(out,"01234_ [恭喜获得属于你的轮！]"));
 strcpy(out,"sentinel");CHECK(message_resolve_profile(MESSAGE_NAME,"(22% exposed)",13,out,sizeof out,NULL)==0&&!strcmp(out,"sentinel"));
 printf("Native UI complete forms, typed arguments, atomic output and colors: %u checks.\n",checks);
 zh_dictionary_free((ZhDictionary*)dictionary);zh_dictionary_free((ZhDictionary*)short_dictionary);
 zh_dictionary_free((ZhDictionary*)display_extra_dictionary);zh_dictionary_free((ZhDictionary*)item_dictionary);zh_dictionary_free((ZhDictionary*)terminal_runtime_dictionary);
 return 0;
}

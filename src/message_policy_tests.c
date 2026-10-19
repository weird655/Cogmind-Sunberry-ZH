#include "terminal_text.h"
static const ZhDictionary *item_dictionary,*short_dictionary,*dictionary;
#include "message_resolver.h"
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){
 CHECK(argc==6);size_t count;char error[256];
 dictionary=zh_dictionary_load(argv[1],&count,error,sizeof error);short_dictionary=zh_dictionary_load(argv[2],&count,error,sizeof error);
 display_extra_dictionary=zh_dictionary_load(argv[3],&count,error,sizeof error);item_dictionary=zh_dictionary_load(argv[4],&count,error,sizeof error);
 terminal_item_dictionary=item_dictionary;terminal_runtime_dictionary=zh_dictionary_load(argv[5],&count,error,sizeof error);
 CHECK(dictionary&&short_dictionary&&display_extra_dictionary&&item_dictionary&&terminal_runtime_dictionary);
 const char *sources[]={"Lgt. Ion Engine","Beam Cannon","Sunberry Melta Gun","Retrieving latest reported positions...\nFound 6 active transport squads.\nDownloaded coordinate data.","Scanning Imp. Spread Laser...\nReady to Repair:\n  Imp. Spread Laser\n  Rating: 4p\n  Time: 23","Unknown untranslated message 93276"};
 for(unsigned i=0;i<sizeof sources/sizeof sources[0];i++)for(int profile=0;profile<=MESSAGE_NAME;profile++){
  const char *input=sources[i];size_t n=strlen(input),rule=937;char output[32768];memset(output,0x55,sizeof output);
  int result=message_resolve_profile(profile,input,n,output,sizeof output,&rule);
  if(result>0){
   char alias[32768];strcpy(alias,input);CHECK(message_resolve_profile(profile,alias,n,alias,sizeof alias,NULL)==result);CHECK(!strcmp(alias,output));
   size_t need=strlen(output)+1;char exact[32768];CHECK(message_resolve_profile(profile,input,n,exact,need,NULL)==result);CHECK(!strcmp(exact,output));
   char tiny[32768],before[32768];memset(tiny,0x66,sizeof tiny);memcpy(before,tiny,sizeof tiny);size_t old_rule=283;
   CHECK(message_resolve_profile(profile,input,n,tiny,need-1,&old_rule)==-1);CHECK(!memcmp(tiny,before,sizeof tiny));CHECK(old_rule==283);
   for(int width=40;width<=80;width+=20){ZhPlan plan={0};CHECK(!zh_plan(output,strlen(output),width,128,&plan));zh_plan_free(&plan);}
  }else{CHECK(output[0]==0x55);CHECK(rule==937);}
 }
 for(unsigned i=0;i<3;i++){
  char named[32768],display[32768],terminal[32768];size_t n=strlen(sources[i]);
  CHECK(message_resolve_profile(MESSAGE_NAME,sources[i],n,named,sizeof named,NULL)>0);
  CHECK(message_resolve(sources[i],n,display,sizeof display)>0);CHECK(!strcmp(display,named));
  CHECK(message_resolve_terminal(sources[i],n,terminal,sizeof terminal,NULL)>0);CHECK(!strcmp(terminal,named));
 }
 char output[32768]="unchanged";size_t rule=123;
 CHECK(message_resolve_profile((MessageProfile)99,"A",1,output,sizeof output,&rule)==-1);CHECK(!strcmp(output,"unchanged")&&rule==123);
 CHECK(message_resolve_profile(MESSAGE_DISPLAY,NULL,1,output,sizeof output,NULL)==-1);
 CHECK(message_resolve_profile(MESSAGE_DISPLAY,"A\0B",3,output,sizeof output,NULL)==-1);CHECK(!strcmp(output,"unchanged"));
 CHECK(message_resolve_profile(MESSAGE_NAME,sources[3],strlen(sources[3]),output,sizeof output,NULL)==0);CHECK(!strcmp(output,"unchanged"));
 const char *counts[]={"0","1","6","0006","9999999999999999"};
 const char *patterns[]={"Found %s hauler:","Found %s haulers:","Found %s active surveillance squad.","Found %s active surveillance squads."};
 for(unsigned i=0;i<sizeof counts/sizeof counts[0];i++)for(unsigned j=0;j<sizeof patterns/sizeof patterns[0];j++){
  char input[256],resolved[32768],direct[32768];snprintf(input,sizeof input,patterns[j],counts[i]);size_t length=strlen(input);
  CHECK(terminal_translate(dictionary,short_dictionary,display_extra_dictionary,input,length,direct,sizeof direct,NULL)==2);
  CHECK(message_resolve(input,length,resolved,sizeof resolved)==1);CHECK(!strcmp(resolved,direct));CHECK(strstr(resolved,counts[i]));
  CHECK(!strstr(resolved,"hauler")&&!strstr(resolved,"surveillance"));
  CHECK(message_resolve_terminal(input,length,resolved,sizeof resolved,NULL)==1);CHECK(!strcmp(resolved,direct));
 }
 const char *variants[]={"Huge Storage Unit","Huge Storage Unit!","Huge Storage Unit [1]","Huge Storage Unit! [1]"};
 for(unsigned i=0;i<4;i++){char named[32768],shown[32768];size_t n=strlen(variants[i]);CHECK(message_resolve_profile(MESSAGE_NAME,variants[i],n,named,sizeof named,NULL)==1);CHECK(strstr(named,"超大型存储单元"));CHECK(message_resolve(variants[i],n,shown,sizeof shown)==1);CHECK(!strcmp(named,shown));}
 printf("PASS %u typed-set, alias, atomic commit, rule preservation and cross-path/layout checks\n",checks);return 0;
}

#include "terminal_text.h"
#include "display_text.h"
#include <stdio.h>
static const ZhDictionary *item_dictionary,*short_dictionary,*dictionary;
#include "message_resolver.h"
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){
 CHECK(argc==6);size_t count;char error[128];ZhDictionary *para=zh_dictionary_load(argv[1],&count,error,sizeof error),*shorts=zh_dictionary_load(argv[2],&count,error,sizeof error),*extra=zh_dictionary_load(argv[3],&count,error,sizeof error);
 terminal_item_dictionary=zh_dictionary_load(argv[4],&count,error,sizeof error);terminal_runtime_dictionary=zh_dictionary_load(argv[5],&count,error,sizeof error);display_extra_dictionary=extra;CHECK(para&&shorts&&extra&&terminal_item_dictionary&&terminal_runtime_dictionary);
 item_dictionary=terminal_item_dictionary;short_dictionary=shorts;dictionary=para;
 const struct {const char *source,*must,*absent;} cases[]={
  {"Downloading prototype IDs...\n  Imp. VTOL Module\n  Adv. Beam Rifle","改进型VTOL模块","Adv. Beam Rifle"},
  {"Retrieving dispatch records...\nFound 15 active squads:\n  5x Security\n  3x Surveillance\n  1x Patrol\n  6x Transport","5x 安保","Security"},
  {"Retrieving manifest data...\nFound 6 haulers:\n  A-02 Transporter\n    2x Adv. Beam Cannon","2x 高级光束加农炮","haulers"},
  {"Retrieving component inventory records...\nFound 4 registered component stockpiles:\n  RMO60CJCY7: Sml. Laser\nDownloaded coordinate data.","RMO60CJCY7: 小型激光","stockpiles"},
  {"Found 1 branch access point:\n  (49,50) Mines\nMap updated.","(49,50) 矿场","access point"},
  {"Found 2 recycling units.\nDownloaded coordinate data.","坐标数据已下载","recycling"},
  {"Retrieving latest reported positions...\nNo active investigation squads found.","调查小队","found"},
  {"Establishing remote squad link...\nNo tasked investigation squads found.","已指派","found"},
  {"No scanalyzers found.","未发现扫描分析站","scanalyzers"}
 };
 char out[32768];
 for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++){
  CHECK(terminal_translate(para,shorts,extra,cases[i].source,strlen(cases[i].source),out,sizeof out,NULL)>0);CHECK(strstr(out,cases[i].must));CHECK(!strstr(out,cases[i].absent));
  CHECK(message_resolve(cases[i].source,strlen(cases[i].source),out,sizeof out)>0);CHECK(strstr(out,cases[i].must));CHECK(!strstr(out,cases[i].absent));
  char routed[32768];size_t rule;CHECK(message_resolve_terminal(cases[i].source,strlen(cases[i].source),routed,sizeof routed,&rule)>0);CHECK(!strcmp(out,routed));
  ZhPlan p={0};CHECK(!zh_plan(out,strlen(out),46,128,&p));zh_plan_free(&p);
 }
 CHECK(display_translate(terminal_item_dictionary,shorts,para,"Insufficient inventory capacity (1)",35,out,sizeof out)==1);CHECK(strstr(out,"容量不足"));
 const char *mixed="Downloading prototype IDs...\r\n  Imp. VTOL Module\r\n  Completely Unreviewed New Part";unsigned matched=0,retained=0;
 CHECK(terminal_compose_lines(terminal_item_dictionary,para,shorts,extra,mixed,strlen(mixed),out,sizeof out,&matched,&retained)==1);CHECK(matched==2&&retained==1);CHECK(strstr(out,"Completely Unreviewed New Part"));
 strcpy(out,"sentinel");CHECK(terminal_compose_lines(terminal_item_dictionary,para,shorts,extra,mixed,strlen(mixed),out,4,NULL,NULL)==-1);CHECK(!strcmp(out,"sentinel"));
 strcpy(out,"sentinel");const char *conflict="Unknown untranslated header\nUnknown untranslated body";CHECK(terminal_compose_lines(terminal_item_dictionary,para,shorts,extra,conflict,strlen(conflict),out,sizeof out,NULL,NULL)==0);CHECK(!strcmp(out,"sentinel"));
 CHECK(content_name_lookup(extra,terminal_item_dictionary,shorts,para,"027x Lgt. Ion Engine",strlen("027x Lgt. Ion Engine"),out,sizeof out)==1);CHECK(!strcmp(out,"027x 轻型离子引擎"));
 strcpy(out,"sentinel");CHECK(message_resolve(conflict,strlen(conflict),out,sizeof out)==0);CHECK(!strcmp(out,"sentinel"));
 strcpy(out,"sentinel");CHECK(message_resolve(cases[0].source,strlen(cases[0].source),out,4)<0);CHECK(!strcmp(out,"sentinel"));
 const char *repair="Scanning Imp. Spread Laser...\nReady to Repair:\n  Imp. Spread Laser\n  Rating: 4p\n  Time: 23";
 char expected[32768];size_t repair_rule=0;
 CHECK(repair_translate_stage(item_dictionary,repair,strlen(repair),expected,sizeof expected)==1);
 CHECK(message_resolve_profile(MESSAGE_REPAIR,repair,strlen(repair),out,sizeof out,&repair_rule)==1);CHECK(!strcmp(out,expected));CHECK(repair_rule==(size_t)-1);
 strcpy(out,"sentinel");CHECK(message_resolve_profile(MESSAGE_REPAIR,repair,strlen(repair),out,4,&repair_rule)<0);CHECK(!strcmp(out,"sentinel"));
 /* Reviewed numeric effects have identical priority with native line breaks.
    Conflicting older paragraph wording must not veto an explicit override. */
 const char *numeric_effect="Enables slashing damage resistance: 3%.\n <no_stack>";
 CHECK(message_resolve(numeric_effect,strlen(numeric_effect),out,sizeof out)==1);
 CHECK(!strcmp(out,"获得 3% 切割伤害抗性。<不叠加>"));
 strcpy(out,"sentinel");CHECK(message_resolve(numeric_effect,strlen(numeric_effect),out,4)<0);CHECK(!strcmp(out,"sentinel"));
 printf("PASS %u dynamic paragraphs, names, coordinates, preserved unknowns and atomic-capacity checks\n",checks);return 0;
}


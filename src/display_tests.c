#include "display_text.h"
#include "ui_text.h"
#include <assert.h>
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do{checks++;assert(x);}while(0)
static int marker_column(const char *s,const char *marker){
 const char *p=strstr(s,marker);CHECK(p!=NULL);size_t at=0;int columns=0;
 while(s+at<p){if(s[at]=='`'){at++;while(s[at]&&s[at]!='`')at++;CHECK(s[at]=='`');at++;continue;}size_t n;uint32_t cp;CHECK(zh_decode(s+at,strlen(s)-at,&n,&cp));columns+=cp<128?1:2;at+=n;}
 return columns;
}
int main(int argc,char **argv){
 CHECK(argc==5);size_t count;char error[128];ZhDictionary *items=zh_dictionary_load(argv[1],&count,error,sizeof error);CHECK(items&&count==1045);
 ZhDictionary *shorts=zh_dictionary_load(argv[2],&count,error,sizeof error);CHECK(shorts&&count==56249);
 ZhDictionary *paragraphs=zh_dictionary_load(argv[3],&count,error,sizeof error);CHECK(paragraphs&&count==16378);
 display_extra_dictionary=zh_dictionary_load(argv[4],&count,error,sizeof error);CHECK(display_extra_dictionary!=NULL);
 const struct {const char *source,*must,*preserve;} cases[]={
  {"Record! \"Lightweight Power Sour...","轻型动力源","!"},
 {"Light Angular Momentum Engine (11%)","轻型","(11%)"},
 {"Carbon-fiber Leg (15%) x2","碳纤维腿","(15%) x2"},
 {"Improved Heat Sink (3%) x2","改进型","(3%) x2"},
 {"Huge Storage Unit! [1]","超大型存储单元","! [1]"},
  {"Huge Storage Unit!","超大型存储单元","!"},
  {"Assault Rifle(66-1=66%)","突击步枪","(66-1=66%)"},
  {"Lgt. Assault Rifle(67-2=65%)","轻型突击步枪","(67-2=65%)"},
  {" E X P L O R E R ","探 险 家",NULL},{" A D V E N T U R E R ","冒 险 家",NULL},{" R O G U E ","浪 人",NULL},
  {" VIRUS DETECTED ","检出病毒",NULL},{" FRAGMENTED ","发生碎片",NULL},{" FAILED ","失败",NULL},{" SECURED ","已加固",NULL},{" STABLE ","稳定",NULL},{"SEQUENCE INITIATED","序列启动",NULL},{"PREPARING CONTROL OVERLAY","控制界面",NULL},{"Systems online...","系统",NULL},{"Loading variables...","变量",NULL},{"CORE=UNSTABLE","不稳定",NULL},{"Type","类型",NULL},{"Overview","概述",NULL},{"Integrity","耐久度",NULL},
  {"Standard","标准",NULL},{"Effect","效果",NULL},{"34 MATTER","物质","34"},
  {"Record! \"Derelict\"","流亡者","!"},{"Record! \"Derelict Rail Weapons\"","流亡者轨道武器","!"},
  {"Schematic \"Imp. Coolant Injector\"","改进型冷却液注入器",NULL},
  {"Terminal F009 - Restricted Access","受限访问","F009"},{"SECURITY LEVEL 2","安保等级","2"},
  {"Scanning nodes...","扫描节点",NULL},{"Network defenses at 100%...","网络防御","100"},
  {"Building attack tree...","攻击树",NULL},{"Bypassing authorization...","绕过授权",NULL},
  {"Increases inventory capacity by 20.\n<no_stack>","库存容量","20"},
  {"Chain reaction triggered in Lgt. Nuclear Core.","轻型核核心",NULL},
  {"Acquired Imp. Spread Laser.","改进型散射激光",NULL},
  {"Acquired 5 matter.","获得","5"},
  {"Acquired 42 matter. <x2>","获得","<x2>"},
  {"U-05 Engineer lays Concrete. <x3>","混凝土","<x3>"},
  {"EM Disruption Field","电磁干扰场",NULL},{"System Shield","系统护盾",NULL},
  {"(Improved Power Shielding)","改进型电源护盾",NULL},
  {"Faulty Adv. Beam Cannon","高级光束加农炮",NULL},
  {"K-01 Serf cleans the Construction Platform. <x2>","清理建造平台","<x2>"},
  {"Repair Station unresponsive.","维修站无响应",NULL},
  {"[1 - Hostile]  [3 - Parts]","敌对","[3"}
 };
 char out[32768];
 for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++){
  int result=display_translate(items,shorts,paragraphs,cases[i].source,strlen(cases[i].source),out,sizeof out);
  if(result!=1||!strstr(out,cases[i].must)){char aa[16384]="",bb[16384]="";size_t ia=0,ib=0;int ra=zh_translate(shorts,cases[i].source,strlen(cases[i].source),aa,sizeof aa,&ia),rb=zh_translate(paragraphs,cases[i].source,strlen(cases[i].source),bb,sizeof bb,&ib);fprintf(stderr,"Missing case %u: %s => result=%d output=%s; short=%d/%zu:%s para=%d/%zu:%s\n",i,cases[i].source,result,out,ra,ia,aa,rb,ib,bb);return 1;}
  CHECK(result==1);CHECK(strstr(out,cases[i].must));if(cases[i].preserve)CHECK(strstr(out,cases[i].preserve));
  ZhPlan p={0};CHECK(!zh_plan(out,strlen(out),80,64,&p));zh_plan_free(&p);
 }
 const char *movement=" MOVEMENT: Rolling `f0` SLOW `x` (62%)";
 CHECK(display_translate(items,shorts,paragraphs,movement,strlen(movement),out,sizeof out)==1);CHECK(strstr(out,"滚动")&&strstr(out,"低速")&&strstr(out,"(62%)"));
 CHECK(marker_column(movement,"`f0`")==marker_column(out,"`f0`"));CHECK(marker_column(movement,"`x`")==marker_column(out,"`x`"));CHECK(marker_column(movement,"(62%)")==marker_column(out,"(62%)"));
 const char *hud=" CORE   272/700";CHECK(display_translate(items,shorts,paragraphs,hud,strlen(hud),out,sizeof out)==1);CHECK(strstr(out,"272/700"));
 CHECK(marker_column(hud,"272")==marker_column(out,"272"));
 const char *menu="Record! \"Derelict Rail Weapons\"";CHECK(display_translate(items,shorts,paragraphs,menu,strlen(menu),out,sizeof out)==1);CHECK(strstr(out,"记录! \"流亡者轨道武器\""));CHECK(!strstr(out,"  "));CHECK(display_columns(out)<(int)strlen(menu));
 const char *scan[]={"[1 - Hostile]  [3 - Parts]","[2 - Friendly] [4 - Exits]"};
 for(unsigned i=0;i<2;i++){CHECK(display_translate(items,shorts,paragraphs,scan[i],strlen(scan[i]),out,sizeof out)==1);CHECK(marker_column(out,i?"[4":"[3")==15);CHECK(display_columns(out)==(int)strlen(scan[i]));}
 size_t overlay_rule=0;const char *overlay="Repair Station unresponsive.";
 CHECK(zh_translate_overlay(paragraphs,display_extra_dictionary,overlay,strlen(overlay),out,sizeof out,&overlay_rule)>0);CHECK(strstr(out,"维修站无响应"));CHECK(overlay_rule==(size_t)-2);
 overlay="Programmers report to Factory Zone 2V.";CHECK(zh_translate_overlay(paragraphs,display_extra_dictionary,overlay,strlen(overlay),out,sizeof out,&overlay_rule)>0);CHECK(strstr(out,"2V"));CHECK(strstr(out,"工厂区域"));
 memset(out,0x53,sizeof out);CHECK(display_translate(items,shorts,paragraphs,"Unknown UI QA9283",17,out,sizeof out)==0);CHECK(out[0]==0x53);
 CHECK(display_translate(items,shorts,paragraphs,"Type",4,out,2)==-1);CHECK(out[0]==0x53);
 const char *ambiguous="Lightweight ";memset(out,0x53,sizeof out);
 CHECK(zh_translate_unique_prefix(paragraphs,ambiguous,strlen(ambiguous),out,sizeof out)==-2);CHECK(out[0]==0x53);
 CHECK(zh_translate_unique_prefix(paragraphs,"Lightweight Power Sour",22,out,2)==-1);CHECK(out[0]==0x53);
 const char *bad_combat[]={"Assault Rifle(foo%)","Assault Rifle(66%%)","Assault Rifle(66)","Unknown Rifle(66%)"};
 for(unsigned i=0;i<4;i++){memset(out,0x53,sizeof out);CHECK(display_combat_name(items,shorts,paragraphs,bad_combat[i],strlen(bad_combat[i]),out,sizeof out)==0);CHECK(out[0]==0x53);}
 const char *bad_condition[]={"Carbon-fiber Leg (101%)","Carbon-fiber Leg (-1%)","Carbon-fiber Leg (15%) x0","Carbon-fiber Leg (15%) x2tail"};
 for(unsigned i=0;i<4;i++){memset(out,0x53,sizeof out);CHECK(display_part_condition(items,shorts,paragraphs,bad_condition[i],strlen(bad_condition[i]),out,sizeof out)==0);CHECK(out[0]==0x53);}
 zh_dictionary_free(items);zh_dictionary_free(shorts);zh_dictionary_free(paragraphs);printf("PASS %u checks: real UI gaps, hover, terminal states, detail text, capture names, fixed color columns, numbers and atomic fallback.\n",checks);return 0;
}

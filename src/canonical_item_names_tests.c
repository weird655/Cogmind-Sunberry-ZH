#include <stdio.h>
#include <stdlib.h>
#include "canonical_item_names.h"
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static ZhDictionary *load(const char *name,const char *data){FILE *f=fopen(name,"wb");CHECK(f!=NULL);CHECK(fwrite(data,1,strlen(data),f)==strlen(data));fclose(f);size_t count;char error[256];ZhDictionary *d=zh_dictionary_load(name,&count,error,sizeof error);CHECK(d!=NULL);return d;}
int main(void){
 ZhDictionary *extra=load("canonical-extra-test.tsv","Imp. Carbon-fiber Leg\t审核腿\nLgt. Ion Engine\t审核引擎\nSunberry Melta Gun\t太阳莓牌热熔枪\n");
 ZhDictionary *items=load("canonical-items-test.tsv","Imp. Carbon-fiber Leg\t旧腿\nImproved Carbon-fiber Leg\t旧全称腿\nLgt. Ion Engine\t旧引擎\nHvy. Plasma Cannon\t物品表炮\n");
 ZhDictionary *shorts=load("canonical-shorts-test.tsv","Hvy. Plasma Cannon\t短表炮\nFallback Part\t短表物品\n");
 ZhDictionary *paras=load("canonical-paras-test.tsv","Fallback Part\t段落表物品\nOnly Paragraph Part\t段落独有\n");
 char out[672];
 const char *sources[]={"Imp. Carbon-fiber Leg","Improved Carbon-fiber Leg","Light Ion Engine","Light Ion Engine+","Light Ion Engine (004)","Light Ion Engine [4p]","Sunberry Melta Gun","Hvy. Plasma Cannon","Fallback Part","Only Paragraph Part"};
 const char *targets[]={"审核腿","审核腿","审核引擎","审核引擎+","审核引擎 (004)","审核引擎 [4p]","太阳莓牌热熔枪","物品表炮","短表物品","段落独有"};
 for(size_t i=0;i<sizeof sources/sizeof sources[0];i++){CHECK(canonical_item_name_translate(extra,items,shorts,paras,sources[i],strlen(sources[i]),out,sizeof out)==1);CHECK(!strcmp(out,targets[i]));}
 CHECK(item_name_translate_exact(items,"Improved Carbon-fiber Leg",strlen("Improved Carbon-fiber Leg"),out,sizeof out)==1);CHECK(!strcmp(out,"旧全称腿"));
 CHECK(item_name_translate_exact(items,"Light Ion Engine",16,out,sizeof out)==0);
 const char *health="Improved Carbon-fiber Leg (027/035)";
 CHECK(canonical_item_name_translate_health(extra,items,shorts,paras,health,strlen(health),out,sizeof out)==1);CHECK(!strcmp(out,"审核腿 (027/035)"));
 CHECK(canonical_item_name_translate(extra,items,shorts,paras,health,strlen(health),out,sizeof out)==0);
 const char *bad[]={"Unknown Part","Light Ion Engine++","Light Ion Engine (1) [a]","Light Ion Engine [x y]"," Light Ion Engine","Light Ion Engine\n","Light Ion Engine (2/)","Light Ion Engine (/3)","Light Ion Engine (-2/3)","Light Ion Engine (2/3%)","Light Ion Engine+ (2/3)","Light Ion Engine (2/3)extra","Light Ion Engine (2 /3)"};
 for(size_t i=0;i<sizeof bad/sizeof bad[0];i++){
  memset(out,0x53,sizeof out);char before[672];memcpy(before,out,sizeof out);
  CHECK(canonical_item_name_translate_health(extra,items,shorts,paras,bad[i],strlen(bad[i]),out,sizeof out)==0);CHECK(!memcmp(before,out,sizeof out));
 }
 char tiny[2]={1,2};CHECK(canonical_item_name_translate(extra,items,shorts,paras,sources[0],strlen(sources[0]),tiny,2)==-1);CHECK(tiny[0]==1&&tiny[1]==2);
 CHECK(canonical_item_name_translate_health(extra,items,shorts,paras,health,strlen(health),tiny,2)==-1);CHECK(tiny[0]==1&&tiny[1]==2);
 strcpy(out,"Light Ion Engine (1/2)");CHECK(canonical_item_name_translate_health(extra,items,shorts,paras,out,strlen(out),out,sizeof out)==1);CHECK(!strcmp(out,"审核引擎 (1/2)"));
 zh_dictionary_free(extra);zh_dictionary_free(items);zh_dictionary_free(shorts);zh_dictionary_free(paras);
 printf("PASS %u canonical item-name checks\n",checks);return 0;
}

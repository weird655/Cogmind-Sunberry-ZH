#include "display_text.h"
#include "repair_text.h"
#include <stdio.h>
#include <assert.h>
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line=%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){
 unsigned checks=0;CHECK(argc==5);size_t count;char err[128];
 ZhDictionary *items=zh_dictionary_load(argv[1],&count,err,sizeof err),*shorts=zh_dictionary_load(argv[2],&count,err,sizeof err),*paragraphs=zh_dictionary_load(argv[3],&count,err,sizeof err);
 display_extra_dictionary=zh_dictionary_load(argv[4],&count,err,sizeof err);CHECK(items&&shorts&&paragraphs&&display_extra_dictionary);
 repair_extra_dictionary=display_extra_dictionary;repair_short_dictionary=shorts;repair_paragraph_dictionary=paragraphs;
 const char *names[]={"Lrg. Battery","Beam Cannon","Core Shielding","Lgt. Ion Engine","Ion Engine","Sunberry Melta Gun","Lgt. Treads","Lgt. Treads"};
 const char *health[]={" (27/35)"," (75/100)"," (42/50)"," (26/40)"," (44/80)"," (81/160)"," (112/140)"," (116/140)"};
 for(unsigned i=0;i<8;i++){
  char canonical[512],hover[1024],popup[1024],source[512],pickup[1024],expected[1024];
  CHECK(canonical_item_name_translate(display_extra_dictionary,items,shorts,paragraphs,names[i],strlen(names[i]),canonical,sizeof canonical)==1);
  CHECK(display_translate(items,shorts,paragraphs,names[i],strlen(names[i]),hover,sizeof hover)==1);CHECK(!strcmp(hover,canonical));
  snprintf(source,sizeof source,"%s%s",names[i],health[i]);
  CHECK(display_translate(items,shorts,paragraphs,source,strlen(source),popup,sizeof popup)==1);
  CHECK(!strncmp(popup,canonical,strlen(canonical)));char *suffix=strrchr(popup,'(');CHECK(suffix&&!strcmp(suffix,health[i]+1));
  *suffix=0;CHECK(display_columns(popup)==(int)strlen(names[i])+1);
  snprintf(source,sizeof source,"Acquired %s.",names[i]);CHECK(display_translate(items,shorts,paragraphs,source,strlen(source),pickup,sizeof pickup)==1);
  CHECK(strstr(pickup,canonical)!=NULL);
  snprintf(source,sizeof source,"Repairing %s...\nETC: 027",names[i]);CHECK(repair_translate_begin(items,source,strlen(source),expected,sizeof expected)==1);CHECK(strstr(expected,canonical)&&strstr(expected,"027"));
  printf("PASS popup-name-pickup-repair %s\n",names[i]);
 }
 char staged[1024];const char *scan="Scanning Lrg. Battery...\nReady to Repair:\n  Lrg. Battery\n  Rating: 1\n  Time: 027";CHECK(repair_translate_stage(items,scan,strlen(scan),staged,sizeof staged)==1);CHECK(strstr(staged,"027"));
 char out[1024]="sentinel";const char *bad[]={"Beam Cannon (2/)","Beam Cannon (2/3) trailing","Beam Cannon (-2/3)","Unknown Item (2/3)"};
 for(unsigned i=0;i<4;i++){CHECK(canonical_item_name_translate_health(display_extra_dictionary,items,shorts,paragraphs,bad[i],strlen(bad[i]),out,sizeof out)==0);CHECK(!strcmp(out,"sentinel"));}
 printf("PASS %u screenshot regression checks\n",checks);return 0;
}

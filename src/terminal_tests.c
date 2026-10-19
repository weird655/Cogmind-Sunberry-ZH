#include "terminal_text.h"
#include "linked_text.h"
#include <assert.h>
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){
 CHECK(argc==5);size_t count;char error[128],out[32768],old[32768];
 ZhDictionary *para=zh_dictionary_load(argv[1],&count,error,sizeof error);CHECK(para&&count==16378);
 ZhDictionary *shorts=zh_dictionary_load(argv[2],&count,error,sizeof error);CHECK(shorts&&count==56249);
 ZhDictionary *extra=zh_dictionary_load(argv[3],&count,error,sizeof error);CHECK(extra);
 terminal_runtime_dictionary=zh_dictionary_load(argv[4],&count,error,sizeof error);CHECK(terminal_runtime_dictionary&&count>=1);
 const char *sources[]={"Current Alert Level: Low Security","Connecting with Door Terminal...","Connecting with QA-17/TEST...","Deep Access #37"};
 for(unsigned i=0;i<sizeof sources/sizeof sources[0];i++){
  size_t n=strlen(sources[i]),rule=0;int a=zh_translate_overlay(para,extra,sources[i],n,old,sizeof old,&rule);
  CHECK(a>0);CHECK(terminal_translate(para,shorts,extra,sources[i],n,out,sizeof out,&rule)>0);CHECK(!strcmp(out,old));
 }
 FILE *f=fopen(argv[2],"rb");CHECK(f);char line[32768];unsigned newly_connected=0,exact_precedence=0;
 while(fgets(line,sizeof line,f)){
  char *tab=strchr(line,'\t');if(!tab||line[0]=='#')continue;*tab=0;size_t n=strlen(line);
  if(strchr(line,'{')||strchr(line,'\\')||!n)continue;
  if(zh_translate_exact(para,line,n,old,sizeof old,NULL)||zh_translate_exact(extra,line,n,old,sizeof old,NULL))continue;
  int exact=zh_translate_exact(shorts,line,n,old,sizeof old,NULL);if(exact<=0)continue;
  size_t rule=0;CHECK(terminal_translate(para,shorts,extra,line,n,out,sizeof out,&rule)==1);CHECK(!strcmp(out,old));CHECK(rule==(size_t)-3);
  char prior[32768];int before=zh_translate_overlay(para,extra,line,n,prior,sizeof prior,NULL);
  if(!before)newly_connected++;else exact_precedence++;
 }
 fclose(f);
 f=fopen(argv[1],"rb");CHECK(f);unsigned retained_paragraphs=0;
 while(fgets(line,sizeof line,f)){
  char *tab=strchr(line,'\t');if(!tab||line[0]=='#')continue;*tab=0;size_t n=strlen(line);
  if(strchr(line,'{')||strchr(line,'\\')||!n)continue;
  if(zh_translate_overlay(para,extra,line,n,old,sizeof old,NULL)<=0)continue;
  CHECK(terminal_translate(para,shorts,extra,line,n,out,sizeof out,NULL)>0);CHECK(!strcmp(out,old));retained_paragraphs++;
 }
 fclose(f);printf("PASS paragraph exact regression: %u existing outputs retained.\n",retained_paragraphs);
 char fixture[4096];CHECK(strlen(argv[1])+32<sizeof fixture);snprintf(fixture,sizeof fixture,"%s.terminal-test.tmp",argv[1]);
 f=fopen(fixture,"wb");CHECK(f);fputs("A{1}C\tfirst {1}\nA{2}C\tsecond {2}\nExact\tparagraph\n",f);fclose(f);
 ZhDictionary *a=zh_dictionary_load(fixture,NULL,error,sizeof error);CHECK(a);
 f=fopen(fixture,"wb");CHECK(f);fputs("A{1}C\tshort {1}\nExact\tshort\nOnly {n}\tshort {n}\n",f);fclose(f);
 ZhDictionary *b=zh_dictionary_load(fixture,NULL,error,sizeof error);CHECK(b);remove(fixture);
 strcpy(out,"unchanged");CHECK(terminal_translate(a,b,NULL,"AXC",3,out,sizeof out,NULL)==-2);CHECK(!strcmp(out,"unchanged"));
 CHECK(terminal_translate(a,b,NULL,"Exact",5,out,sizeof out,NULL)==1);CHECK(!strcmp(out,"paragraph"));
 CHECK(terminal_translate(a,b,NULL,"Only 37",7,out,sizeof out,NULL)==2);CHECK(!strcmp(out,"short 37"));
 strcpy(out,"unchanged");CHECK(terminal_translate(a,b,NULL,"Exact",5,out,1,NULL)<0);CHECK(!strcmp(out,"unchanged"));
 zh_dictionary_free(b);zh_dictionary_free(a);
 f=fopen(argv[4],"rb");CHECK(f);char *runtime_tab=NULL;while(fgets(line,sizeof line,f))if(line[0]!='#'&&(runtime_tab=strchr(line,'\t')))break;fclose(f);CHECK(runtime_tab);*runtime_tab=0;
 CHECK(strlen(line)==554);CHECK(terminal_translate(para,shorts,extra,line,strlen(line),out,sizeof out,NULL)==1);CHECK(strstr(out,"[Complex 0b10]"));
 char linked[32768];CHECK(linked_text_prepare(line,strlen(line),line,strlen(line),out,strlen(out),1,46,linked,sizeof linked)==1);
 ZhPlan linked_plan={0};CHECK(linked_plan_atomic(linked,strlen(linked),46,128,&linked_plan)==1);CHECK(linked_plan.used_rows>1);zh_plan_free(&linked_plan);
 strcpy(out,"unchanged");CHECK(!terminal_translate(para,shorts,extra,"not a real Cogmind string 9283",29,out,sizeof out,NULL));CHECK(!strcmp(out,"unchanged"));
 CHECK(terminal_translate(para,shorts,extra,"Current Alert Level: Low Security",33,out,1,NULL)<0);
 printf("PASS terminal corpus: checks=%u newly-connected-exact=%u exact-before-template=%u; existing alert/dynamic paragraph outputs preserved.\n",checks,newly_connected,exact_precedence);
 zh_dictionary_free(extra);zh_dictionary_free(shorts);zh_dictionary_free(para);return 0;
}

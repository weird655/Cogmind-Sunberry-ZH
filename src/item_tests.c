#include "text_core.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
int main(int argc,char **argv){
 assert(argc==2);char error[128];size_t count=0;
 ZhDictionary *d=zh_dictionary_load(argv[1],&count,error,sizeof error);assert(d&&count==1045);
 FILE *f=fopen(argv[1],"rb");assert(f);char line[2048];unsigned checked=0;
 while(fgets(line,sizeof line,f)){
  if(line[0]=='#'||line[0]=='\r'||line[0]=='\n')continue;
  char *tab=strchr(line,'\t');assert(tab);*tab++=0;tab[strcspn(tab,"\r\n")]=0;
  char out[1024];size_t rule=0;assert(zh_translate_exact(d,line,strlen(line),out,sizeof out,&rule)==1);assert(!strcmp(out,tab));
  ZhPlan plan={0};assert(!zh_plan(out,strlen(out),(int)strlen(line),1,&plan));
  for(size_t i=0;i<plan.count;i++){ZhRun *r=&plan.runs[i];assert(r->y==0&&r->x>=0&&r->x+r->width<=(int)strlen(line));if(r->width==2)assert(r->token[1]==r->token[0]+1);}
  zh_plan_free(&plan);checked++;
 }
 fclose(f);assert(checked==count);char out[64];assert(zh_translate_exact(d,"Lrg. Matter Pod (105)",20,out,sizeof out,0)==0);
 zh_dictionary_free(d);printf("Item exact translations and native column plans: %u passed; parameterized names preserved.\n",checked);return 0;
}

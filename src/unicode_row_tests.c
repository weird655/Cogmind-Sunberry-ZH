#include "unicode_row.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
 assert(argc==2||argc==3);FILE *f=fopen(argv[1],"rb");assert(f);char line[32768];unsigned checked=0,rows=0;
 while(fgets(line,sizeof line,f)){
  if(line[0]=='#')continue;char *tab=strchr(line,'\t');if(!tab)continue;char *target=tab+1;target[strcspn(target,"\r\n")]=0;
  ZhPlan p={0};assert(!zh_plan(target,strlen(target),68,128,&p));
  for(int row=0;row<p.used_rows;row++){
   char encoded[8192];assert(unicode_row_text(&p,row,encoded,sizeof encoded));ZhPlan again={0};assert(!zh_plan(encoded,strlen(encoded),68,1,&again));size_t at=0;
   for(size_t i=0;i<p.count;i++)if(p.runs[i].y==row){assert(at<again.count);ZhRun *a=&p.runs[i],*b=&again.runs[at++];assert(a->scalar==b->scalar&&a->x==b->x&&a->width==b->width);assert(!strcmp(p.styles[a->style],again.styles[b->style]));}
   assert(at==again.count);zh_plan_free(&again);rows++;
  }
  zh_plan_free(&p);checked++;
 }
 fclose(f);assert(checked==(argc==3?(unsigned)strtoul(argv[2],NULL,10):16378u));printf("PASS Unicode scratch round trip: %u targets, %u rows; scalars, columns and style preserved.\n",checked,rows);return 0;
}

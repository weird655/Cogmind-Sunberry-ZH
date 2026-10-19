#include "text_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
 assert(argc==2);size_t count=0;char error[128],out[16384];size_t rule=0;
 ZhDictionary *d=zh_dictionary_load(argv[1],&count,error,sizeof error);
 if(!d){fprintf(stderr,"dictionary error: %s\n",error);return 1;}assert(count==16378&&zh_registry_count>0);
 const char *real_alert="Current Alert Level: Low Security";
 assert(zh_translate(d,real_alert,strlen(real_alert),out,sizeof out,&rule)==1);
 ZhPlan alert_plan={0};assert(zh_plan(out,strlen(out),46,1,&alert_plan)==0&&alert_plan.used_rows==1);zh_plan_free(&alert_plan);
 const char *literal="Connecting with Door Terminal...";
 assert(zh_translate(d,literal,strlen(literal),out,sizeof out,&rule)==1);
 ZhPlan p={0};assert(zh_plan(out,strlen(out),46,1,&p)==0&&p.count>5&&p.count<30);zh_plan_free(&p);
 const char *dynamic="Connecting with QA-17/TEST...";
 assert(zh_translate(d,dynamic,strlen(dynamic),out,sizeof out,&rule)==2&&strstr(out,"QA-17/TEST"));
 assert(!strstr(out,"{1}"));
 assert(zh_translate(d,"Deep Access #37",15,out,sizeof out,&rule)==2&&strstr(out,"37"));
 assert(zh_translate(d,"Deep Access #+",14,out,sizeof out,&rule)==0);
 assert(zh_translate(d,"not a real Cogmind string 9283",29,out,sizeof out,&rule)==0);
 assert(zh_translate_exact(d,literal,strlen(literal),out,2,NULL)==-1);
 const char *mixed="A\xe6\x97\xa5";
 assert(zh_plan(mixed,4,2,2,&p)==0&&p.count==2&&p.runs[0].width==1&&p.runs[1].width==2&&p.runs[1].x==0&&p.runs[1].y==1);zh_plan_free(&p);
 assert(zh_plan("AB CD",5,4,2,&p)==0&&p.runs[3].x==0&&p.runs[3].y==1);zh_plan_free(&p);
 const char *styled="`f0`A`b0`B`x`C";
 assert(zh_plan(styled,strlen(styled),6,1,&p)==0&&p.count==3&&!strcmp(p.styles[p.runs[1].style],"`f0``b0`")&&!strcmp(p.styles[p.runs[2].style],"`x`"));zh_plan_free(&p);
 const char *bad[]={"\xc0\x80","\xed\xa0\x80","\xf4\x90\x80\x80","\xe6\x97","\x80","`f0"};
 for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++){ZhPlan sentinel={0},saved;sentinel.rows=123;saved=sentinel;assert(zh_plan(bad[i],strlen(bad[i]),9,3,&sentinel)<0&&!memcmp(&sentinel,&saved,sizeof saved));}
 assert(zh_plan(mixed,4,1,1,&p)<0);
 assert(zh_plan("A\nB",3,1,2,&p)==0&&p.runs[1].y==1);zh_plan_free(&p);
 for(size_t i=0;i<zh_registry_count;i++)assert(zh_glyph(zh_registry[i])==(int)i);
 FILE *corpus=fopen(argv[1],"rb");assert(corpus);char line[65536];unsigned planned=0,max_rows=0;
 while(fgets(line,sizeof line,corpus)){
  if(line[0]=='#'||line[0]=='\r'||line[0]=='\n')continue;
  char *target=strchr(line,'\t');assert(target);target++;size_t n=strlen(target);while(n&&(target[n-1]=='\r'||target[n-1]=='\n'))n--;
  int result=zh_plan(target,n,80,512,&p);if(result){fprintf(stderr,"Corpus plan failed row=%u code=%d\n",planned+1,result);return 2;}
  int previous_end=0,previous_row=0;
  for(size_t j=0;j<p.count;j++){ZhRun *r=&p.runs[j];assert(r->x>=0&&r->x+r->width<=80&&r->y>=previous_row&&r->y<1024);if(r->y==previous_row)assert(r->x>=previous_end);previous_row=r->y;previous_end=r->x+r->width;assert(r->token[0]<(320+2*zh_registry_count+31)/32*32&&(r->width==1||r->token[1]==r->token[0]+1));}
  size_t copied_runs=0;for(int row=0;row<p.used_rows;row++){ZhPlan one={0};assert(zh_plan_row(&p,row,&one)==0&&one.rows==1&&one.columns>=1&&one.columns<=80);assert(one.runs!=p.runs&&!memcmp(one.styles,p.styles,sizeof p.styles));for(size_t j=0;j<one.count;j++)assert(one.runs[j].y==0&&one.runs[j].x+one.runs[j].width<=one.columns);copied_runs+=one.count;zh_plan_free(&one);}assert(copied_runs==p.count);
  if((unsigned)p.used_rows>max_rows)max_rows=p.used_rows;zh_plan_free(&p);planned++;
 }
 fclose(corpus);assert(planned==count);printf("PASS corpus: %u targets laid out at width 80, max rows %u, no split glyphs or overlaps.\n",planned,max_rows);
 char fixture[4096];assert(strlen(argv[1])+32<sizeof fixture);snprintf(fixture,sizeof fixture,"%s.core-test.tmp",argv[1]);
 FILE *f=fopen(fixture,"wb");assert(f);fputs("A{1}C\tfirst {1}\nA{2}C\tsecond {2}\nABC\texact\nPair {1} / {2}\t{2}:{1}\nSame {1} / {1}\t{1}\nNumber {n}\tN={n}\n",f);fclose(f);
 ZhDictionary *small=zh_dictionary_load(fixture,NULL,error,sizeof error);assert(small);remove(fixture);
 assert(zh_translate(small,"ABC",3,out,sizeof out,NULL)==1&&!strcmp(out,"exact"));
 strcpy(out,"unchanged");assert(zh_translate(small,"AXC",3,out,sizeof out,NULL)==-2&&!strcmp(out,"unchanged"));
 assert(zh_translate(small,"Pair AA / BB",12,out,sizeof out,NULL)==2&&!strcmp(out,"BB:AA"));
 assert(zh_translate(small,"Same AA / AA",12,out,sizeof out,NULL)==2&&!strcmp(out,"AA"));
 assert(zh_translate(small,"Same AA / BB",12,out,sizeof out,NULL)==0);
 assert(zh_translate(small,"Number -12.5",12,out,sizeof out,NULL)==2&&!strcmp(out,"N=-12.5"));
 assert(zh_translate(small,"Number -",8,out,sizeof out,NULL)==0);zh_dictionary_free(small);
 puts("PASS templates: exact precedence, ambiguity leaves output unchanged, reordered captures, repeated captures, signed decimals.");
 ZhPlan unchanged={0};unchanged.rows=789;ZhPlan saved=unchanged;assert(zh_plan_row(&p,-1,&unchanged)<0&&!memcmp(&saved,&unchanged,sizeof saved));
 puts("PASS row plans: all corpus rows deep-copied, styles retained, compact widths, atomic invalid-row failure.");
 zh_dictionary_free(d);printf("PASS: 16378 rules, %zu glyphs, exact/template lookup, strict UTF-8, mixed widths, wrapping, control runs, atomic failure.\n",zh_registry_count);return 0;
}

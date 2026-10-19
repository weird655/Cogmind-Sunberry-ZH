#include <stdio.h>
#include "linked_text.h"
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static int prepare(const char *r,const char *p,const char *t,size_t n,int cols,char *out,size_t cap){return linked_text_prepare(r,strlen(r),p,strlen(p),t,strlen(t),n,cols,out,cap);}
static void rejection(const char *raw,const char *processed,const char *target,size_t count,int cols,int result){
 char out[512],before[512];memset(out,0x42,sizeof out);memcpy(before,out,sizeof out);
 CHECK(prepare(raw,processed,target,count,cols,out,sizeof out)==result);CHECK(!memcmp(out,before,sizeof out));
}
int main(void){
 char out[512];CHECK(prepare("The [ISC] and [Human].","The [!ISC] and [!Human].","来自[ISC]的人类[人类]。",2,46,out,sizeof out)==1);
 CHECK(!strcmp(out,"来自[!ISC]的人类[!Human]。"));
 CHECK(prepare("[!ISC]","[ISC]","[机构]",1,46,out,sizeof out)==1);CHECK(!strcmp(out,"[ISC]"));
 rejection("[ISC] [Human]","[Human] [ISC]","[机构][人类]",2,46,0);
 rejection("[ISC]","[!Other]","[机构]",1,46,0);
 rejection("[ISC]","[ISC]","[机构][多余]",1,46,0);
 rejection("[ISC]","[ISC]","机构",1,46,0);
 rejection("[ISC]","[ISC]","[[机构]]",1,46,0);
 rejection("[ISC]","[ISC]","机构]",1,46,0);
 rejection("[ISC]","[ISC]","[机构",1,46,0);
 rejection("[]","[]","[]",1,46,0);
 rejection("[ISC]","[!]","[机构]",1,46,0);
 rejection("[ISC]","[!!ISC]","[机构]",1,46,0);
 rejection("[ISC]","[ISC>>1]","[机构]",1,46,0);
 rejection("[ISC]","[ISC]","`f0`[机构]",1,46,0);
 rejection("[ISC]","[ISC]","[机\n构]",1,46,0);
 rejection("[IS\nC]","[ISC]","[机构]",1,46,0);
 rejection("[ISC]","[ISC]","\xc0\xaf[机构]",1,46,0);
 rejection("[ISC]","[ISC]","[机构]",1,4,0);
 rejection("[ISC]","[ISC]","[机构]",0,46,0);
 char tiny[2]={1,2};CHECK(prepare("[ISC]","[ISC]","[机构]",1,46,tiny,sizeof tiny)==-1);CHECK(tiny[0]==1&&tiny[1]==2);
 ZhPlan plan={0};CHECK(linked_plan_atomic("中文[!ISC]文",strlen("中文[!ISC]文"),8,3,&plan)==1);
 CHECK(plan.used_rows==2);CHECK(plan.runs[0].width==2&&plan.runs[1].x==2);
 for(size_t i=2;i<8;i++)CHECK(plan.runs[i].y==1&&plan.runs[i].x==(int)i-2&&plan.runs[i].width==1);
 CHECK(plan.runs[8].y==1&&plan.runs[8].x==6&&plan.runs[8].width==2);zh_plan_free(&plan);
 CHECK(linked_plan_atomic("中[ISC]文",strlen("中[ISC]文"),7,3,&plan)==1);CHECK(plan.runs[1].x==2&&plan.runs[1].y==0);CHECK(plan.runs[6].y==1&&plan.runs[6].x==0);zh_plan_free(&plan);
 CHECK(linked_plan_atomic("[ISC]\n[!Human]",strlen("[ISC]\n[!Human]"),8,2,&plan)==1);CHECK(plan.used_rows==2);zh_plan_free(&plan);
 ZhPlan sentinel;memset(&sentinel,0x55,sizeof sentinel);ZhPlan before=sentinel;
 CHECK(linked_plan_atomic("中文[!ISC]文",strlen("中文[!ISC]文"),8,1,&sentinel)==-1);CHECK(!memcmp(&sentinel,&before,sizeof sentinel));
 CHECK(linked_plan_atomic("[!Human]",8,7,2,&sentinel)==-1);CHECK(!memcmp(&sentinel,&before,sizeof sentinel));
 CHECK(linked_plan_atomic("[人类]",strlen("[人类]"),46,2,&sentinel)==0);CHECK(!memcmp(&sentinel,&before,sizeof sentinel));
 CHECK(linked_plan_atomic("\xed\xa0\x80",3,46,2,&sentinel)==0);CHECK(!memcmp(&sentinel,&before,sizeof sentinel));
 CHECK(linked_plan_atomic("123456[ISC]",11,7,2,&plan)==1);CHECK(plan.runs[6].x==0&&plan.runs[6].y==1);zh_plan_free(&plan);
 /* Preparation commits only after all source reads, so target/output alias works. */
 strcpy(out,"中文[人类]");CHECK(prepare("[Human]","[!Human]",out,1,46,out,sizeof out)==1);CHECK(!strcmp(out,"中文[!Human]"));
 for(int cols=5;cols<=16;cols++)for(int prefix=0;prefix<=cols;prefix++){
  char sample[80];memset(sample,' ',(size_t)prefix);strcpy(sample+prefix,"[ISC]中");
  CHECK(linked_plan_atomic(sample,strlen(sample),cols,3,&plan)==1);
  int expected_row=prefix+5>cols?1:0,expected_x=expected_row?0:prefix;
  for(int i=0;i<5;i++)CHECK(plan.runs[prefix+i].y==expected_row&&plan.runs[prefix+i].x==expected_x+i&&plan.runs[prefix+i].scalar==(unsigned char)"[ISC]"[i]);
  for(size_t i=0;i<plan.count;i++){CHECK(plan.runs[i].x+plan.runs[i].width<=cols);if(plan.runs[i].width==2)CHECK(plan.runs[i].token[1]==plan.runs[i].token[0]+1);}
  zh_plan_free(&plan);
 }
 char many[200]="";for(unsigned i=0;i<33;i++)strcat(many,"[A]");rejection(many,many,many,33,46,0);
 rejection("[ISC]","[ISC]","[机构]\r",1,46,0);
 rejection("[ISC]","[ISC]","[机构] {1}",1,46,0);
 rejection("[ISC]","[I\tSC]","[机构]",1,46,0);
 CHECK(prepare("[ISC]","[!ISC]","[机构]",1,46,out,7)==1);CHECK(!strcmp(out,"[!ISC]"));
 char before_out[512];memcpy(before_out,out,sizeof out);CHECK(prepare("[ISC]","[!ISC]","[机构]",1,46,out,6)==-1);CHECK(!memcmp(out,before_out,sizeof out));
 printf("PASS %u checks; offline ASCII linked-span preservation and atomic Unicode layout\n",checks);return 0;
}

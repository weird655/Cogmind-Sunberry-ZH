#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define INPUT_NATIVE 128
typedef struct {uint64_t scope;int limit;} InputContext;
static uint32_t item_vec[3],robot_vec[3],item_learned_vec[3],robot_learned_vec[3];
static uint32_t items[3],robots[2],item_flags[3],robot_flags[2];
static unsigned char item_records[3][512],robot_records[2][512];
static int allow=1,scope_changes=0,context_calls=0,mutate_flags_after_reads=0,ns_calls=0;
static uintptr_t va(uintptr_t address){
 switch(address){
  case 0xd2d1c4:return (uintptr_t)item_vec;
  case 0xd25de0:return (uintptr_t)robot_vec;
  case 0xcf4844:return (uintptr_t)item_learned_vec;
  case 0xcf4888:return (uintptr_t)robot_learned_vec;
 }
 assert(0);return 0;
}
static int readable(const void *p,size_t n){return p!=NULL&&n<1024*1024;}
static int input_get_context(InputContext *c){
 c->scope=(scope_changes&&context_calls++>0)?2:1;c->limit=127;return 1;
}
static int input_allowed(const InputContext *c,unsigned code){(void)c;return allow&&code==66;}
static int input_command_name(unsigned code,char *out,size_t cap){
 if(code!=66||cap<8)return 0;strcpy(out,"Load(@)");return 1;
}
static int input_ns(uintptr_t address,char *out,size_t cap){
 uint32_t *s=(void*)address;unsigned length=s[4];
 if(mutate_flags_after_reads&&++ns_calls==mutate_flags_after_reads)item_flags[0]^=1;
 if(length>=cap||length>15||s[5]!=15)return 0;
 memcpy(out,s,length);out[length]=0;return 1;
}
static int picker_vector45(uint32_t address,uint32_t *begin,unsigned *count,unsigned limit){
 uint32_t *v=(void*)(uintptr_t)address;
 if(!readable(v,12)||v[1]<v[0]||(v[1]-v[0])%4||v[2]<v[1]||
    v[1]-v[0]>limit*4||!readable((void*)(uintptr_t)v[0],v[1]-v[0]))return 0;
 *begin=v[0];*count=(v[1]-v[0])/4;return 1;
}
#include "picker_load_candidates46.h"
static void setstr(void *p,const char *s){
 size_t n=strlen(s);assert(n<=15);memset(p,0,28);memcpy(p,s,n);
 ((uint32_t*)p)[4]=(uint32_t)n;((uint32_t*)p)[5]=15;
}
static void vector(uint32_t *v,void *b,unsigned count){
 v[0]=(uint32_t)(uintptr_t)b;v[1]=v[2]=v[0]+count*4;
}
static void reset(void){
 memset(item_records,0,sizeof item_records);memset(robot_records,0,sizeof robot_records);
 const char *ia[]={"Laser","Cannon","Wheel"},*id[]={"Sml. Laser","Hvy. Cannon","Wheel"};
 const char *ra[]={"Scout","Worker"},*rd[]={"G-34 Scout","W-25 Worker"};
 for(unsigned n=0;n<3;n++){
  *(uint32_t*)item_records[n]=n;setstr(item_records[n]+8,ia[n]);setstr(item_records[n]+0x24,id[n]);
  items[n]=(uint32_t)(uintptr_t)item_records[n];item_flags[n]=n==1?0:1;
 }
 for(unsigned n=0;n<2;n++){
  *(uint32_t*)robot_records[n]=n;setstr(robot_records[n]+4,ra[n]);setstr(robot_records[n]+0x1ac,rd[n]);
  robots[n]=(uint32_t)(uintptr_t)robot_records[n];robot_flags[n]=n==1?0:1;
 }
 vector(item_vec,items,3);vector(robot_vec,robots,2);
 vector(item_learned_vec,item_flags,3);vector(robot_learned_vec,robot_flags,2);
 allow=1;scope_changes=0;context_calls=0;mutate_flags_after_reads=0;ns_calls=0;
}
int main(void){
 InputContext c={1,127};PickerLoadCandidate out[8];unsigned total;
 reset();assert(picker_load_candidates46(&c,66,out,8,0,&total)==3&&total==3);
 assert(out[0].kind==1&&out[0].ordinal==0&&!strcmp(out[0].name,"Sml. Laser")&&!strcmp(out[0].native,"Load(Laser)"));
 assert(out[2].kind==2&&out[2].ordinal==0&&!strcmp(out[2].native,"Load(Scout)"));
 assert(picker_load_candidates46(&c,66,out,1,1,&total)==1&&total==3&&!strcmp(out[0].native,"Load(Wheel)"));
 assert(picker_load_candidates46(&c,66,out,1,99,&total)==0&&total==3);
 reset();setstr(item_records[1]+8,"lAsEr");
 assert(picker_load_candidates46(&c,66,out,8,0,&total)==2&&total==2); /* even unlearned aliases shadow */
 reset();setstr(robot_records[0]+4,"wHeEl");
 assert(picker_load_candidates46(&c,66,out,8,0,&total)==1&&total==1&&!strcmp(out[0].native,"Load(Laser)"));
 reset();setstr(robot_records[0]+4,"cAnNoN");
 assert(picker_load_candidates46(&c,66,out,8,0,&total)==2&&total==2); /* unlearned item shadows learned robot */
 reset();setstr(robot_records[1]+4,"sCoUt");
 assert(picker_load_candidates46(&c,66,out,8,0,&total)==2&&total==2); /* unlearned duplicate robot also rejects */
 reset();mutate_flags_after_reads=2;
 assert(picker_load_candidates46(&c,66,out,8,0,&total)==-1&&total==0);
 reset();item_learned_vec[1]-=4;
 assert(picker_load_candidates46(&c,66,out,8,0,&total)==-1&&total==0);
 reset();scope_changes=1;
 assert(picker_load_candidates46(&c,66,out,8,0,&total)==-1&&total==0);
 reset();allow=0;
 assert(picker_load_candidates46(&c,66,out,8,0,&total)==-1&&total==0);
 reset();c.limit=8;
 assert(picker_load_candidates46(&c,66,out,8,0,&total)==-1&&total==0);
 reset();c.limit=127;setstr(item_records[0]+8,"bad(alias");
 assert(picker_load_candidates46(&c,66,out,8,0,&total)==-1&&total==0);
 puts("load fixture: learned filtering, native aliases, pagination, collision rejection, bounds/context/allowlist checks passed");
 return 0;
}

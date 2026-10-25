#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define INPUT_NATIVE 512
static uint32_t lore_vector[3],item_vector[3],robot_vector[3];
static unsigned char lore[2][0x1d0],items[2][0x1d0],robots[2][0x1d0];
static uint32_t lore_ptrs[2],item_ptrs[2],robot_ptrs[2];
static uintptr_t va(uintptr_t address){
 if(address==0xd35b58)return (uintptr_t)lore_vector;
 if(address==0xd2d1c4)return (uintptr_t)item_vector;
 if(address==0xd25de0)return (uintptr_t)robot_vector;
 return 0;
}
static int readable(const void *p,size_t n){return p&&n<1000000;}
static int input_ns(uintptr_t address,char *out,size_t cap){
 const uint32_t *s=(const void*)address;
 if(s[4]>=cap||s[4]>s[5])return 0;
 const char *raw=s[5]<16?(const void*)s:(const void*)(uintptr_t)s[0];
 if(!raw)return 0;
 for(unsigned i=0;i<s[4];i++)if((unsigned char)raw[i]<32||(unsigned char)raw[i]>126)return 0;
 memcpy(out,raw,s[4]);out[s[4]]=0;return 1;
}
#include "picker_visible_parameters46.h"
static void put(unsigned char *record,unsigned field,const char *s){
 uint32_t *p=(void*)(record+field);memset(p,0,28);unsigned n=(unsigned)strlen(s);
 if(n<16){memcpy(p,s,n);p[5]=15;}else{p[0]=(uint32_t)(uintptr_t)s;p[5]=n;}
 p[4]=n;
}
static void vector(uint32_t *v,uint32_t *p){v[0]=(uint32_t)(uintptr_t)p;v[1]=v[0]+8;v[2]=v[1];}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
 _Static_assert(sizeof(void*)==4,"compile 32-bit to match game ABI");
 for(unsigned i=0;i<2;i++){
  *(uint32_t*)lore[i]=*(uint32_t*)items[i]=*(uint32_t*)robots[i]=i;
  lore_ptrs[i]=(uint32_t)(uintptr_t)lore[i];item_ptrs[i]=(uint32_t)(uintptr_t)items[i];robot_ptrs[i]=(uint32_t)(uintptr_t)robots[i];
 }
 vector(lore_vector,lore_ptrs);vector(item_vector,item_ptrs);vector(robot_vector,robot_ptrs);
 put(lore[0],4,"Storage");put(lore[1],4,"Lore Two");
 put(items[0],8,"Etherial Claws A");put(items[1],8,"Etherial Claws B");
 put(items[0],0x24,"Etherial Claws");put(items[1],0x24,"Etherial Claws");
 put(robots[0],4,"Target Bot");put(robots[1],4,"Other Bot");
 put(robots[0],0x1ac,"N-001 Target Bot");put(robots[1],0x1ac,"N-002 Other Bot");
 put(robots[0],0x170,"Analysis");put(robots[1],0x170,"Analysis");
 PickerVisibleParameter46 row;
 CHECK(picker_visible_parameter46(1,0,511,&row)&&!strcmp(row.native,"Schematic(Etherial Claws A)"));
 CHECK(picker_visible_parameter46(1,1,511,&row)&&!strcmp(row.native,"Schematic(Etherial Claws B)"));
 CHECK(!strcmp(row.display,"Etherial Claws"));
 put(items[1],8,"etherial claws a");CHECK(!picker_visible_parameter46(1,1,511,&row));
 put(items[1],8,"Etherial Claws B");
 CHECK(picker_visible_parameter46(2,0,511,&row)&&!strcmp(row.native,"Schematic(Target Bot)"));
 CHECK(picker_visible_parameter46(3,0,511,&row)&&!strcmp(row.native,"Analysis(Target Bot)"));
 put(items[1],8,"target bot");CHECK(!picker_visible_parameter46(2,0,511,&row));
 CHECK(picker_visible_parameter46(3,0,511,&row));
 *(uint32_t*)(robots[0]+0x28)=0x47;CHECK(!picker_visible_parameter46(3,0,511,&row));
 *(uint32_t*)(robots[0]+0x28)=0;put(robots[0],0x170,"");CHECK(!picker_visible_parameter46(3,0,511,&row));
 CHECK(picker_visible_parameter46(0,0,511,&row)&&!strcmp(row.native,"Query(Storage)"));
 CHECK(!picker_visible_parameter46(0,2,511,&row));CHECK(!picker_visible_parameter46(1,0,5,&row));
 puts("visible parameter identity tests passed");return 0;
}

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define INPUT_NATIVE 256
#define RECORD_IDENTITY_MANIFEST 1
#ifndef PICKER_ITEM_TEST_PRODUCTION46
#define PICKER_ITEM_PARAMETER_FIXTURE46 1
#endif
typedef void (__attribute__((thiscall)) *NativeVoid)(void*);
typedef struct {uint64_t scope;int limit;} InputContext;
typedef struct {uint32_t handle,prototype,ordinal;} RecordBinding;
static uint32_t world[0x700/4],entity[0x180/4],item[4][16],prototype[4][0x220/4];
static uint32_t world_global,entity_global,item_global;
static uint32_t entity_registry[8],item_registry[8],entity_slots[2],entity_gens[2],item_slots[5],item_gens[5],handles[4];
static const char *names[4];static unsigned char allowed[4];static uint64_t current_scope=7;static int mutate_name,changed_scope;
static uintptr_t va(uintptr_t a){return a==0xcefc4c?(uintptr_t)&world_global:a==0xd3c228?(uintptr_t)&entity_global:a==0xd3c224?(uintptr_t)&item_global:0;}
static int readable(const void *p,size_t n){
 MEMORY_BASIC_INFORMATION info;if(!p||!VirtualQuery(p,&info,sizeof info)||info.State!=MEM_COMMIT||info.Protect&(PAGE_NOACCESS|PAGE_GUARD))return 0;
 return (uintptr_t)p+n>=(uintptr_t)p&&(uintptr_t)p+n<=(uintptr_t)info.BaseAddress+info.RegionSize;
}
static int picker_vector45(uint32_t address,uint32_t *begin,unsigned *count,unsigned limit){
 uint32_t *v=(void*)(uintptr_t)address;
 if(!readable(v,12)||v[1]<v[0]||(v[1]-v[0])%4||v[2]<v[1]||v[1]-v[0]>limit*4||(v[1]>v[0]&&!readable((void*)(uintptr_t)v[0],v[1]-v[0])))return 0;
 *begin=v[0];*count=(v[1]-v[0])/4;return 1;
}
static int input_get_context(InputContext *c){c->scope=current_scope;c->limit=255;return 1;}
static int input_allowed(const InputContext *c,unsigned code){(void)c;return code==76||code==81||code==94||code==96;}
static int input_command_name(unsigned code,char *out,size_t cap){const char *s=code==76?"Scan(@)":code==81?"Recycle(@)":code==94?"Insert(@)":code==96?"Study(@)":"Load(@)";if(strlen(s)>=cap)return 0;strcpy(out,s);return 1;}
#ifdef PICKER_ITEM_TEST_PRODUCTION46
static int input_ns(uintptr_t address,char *out,size_t cap){uint32_t *s=(void*)address;if(!readable(s,28)||s[4]>=cap||s[4]>s[5])return 0;const char *p=s[5]<16?(void*)s:(void*)(uintptr_t)s[0];if(!readable(p,s[4]?s[4]:1))return 0;memcpy(out,p,s[4]);out[s[4]]=0;return 1;}
#endif
static int native_record_binding(uint32_t handle,RecordBinding *binding){unsigned index=(handle&65535)-1;if(index>=4)return 0;*binding=(RecordBinding){handle,(uint32_t)(uintptr_t)prototype[index],index};return 1;}
static int picker_item_fixture_eligible46(uint32_t ptr,unsigned code){(void)code;for(unsigned i=0;i<4;i++)if(ptr==(uint32_t)(uintptr_t)item[i])return allowed[i];return 0;}
static int picker_item_fixture_name46(uint32_t ptr,char *out,size_t cap){for(unsigned i=0;i<4;i++)if(ptr==(uint32_t)(uintptr_t)item[i]){if(strlen(names[i])>=cap)return 0;strcpy(out,names[i]);if(mutate_name){handles[3]^=1;mutate_name=0;}if(changed_scope)current_scope++;return 1;}return 0;}
#include "picker_item_candidates46.h"
static void vector(uint32_t *v,const void *p,unsigned count){v[0]=(uint32_t)(uintptr_t)p;v[1]=v[2]=v[0]+count*4;}
static void initialize(void){
 memset(entity_registry,0,sizeof entity_registry);memset(item_registry,0,sizeof item_registry);
 vector(entity_registry,entity_slots,2);vector(entity_registry+4,entity_gens,2);vector(item_registry,item_slots,5);vector(item_registry+4,item_gens,5);
 world_global=(uint32_t)(uintptr_t)world;entity_global=(uint32_t)(uintptr_t)entity_registry;item_global=(uint32_t)(uintptr_t)item_registry;
 world[0x66c/4]=0x10001;entity_slots[1]=(uint32_t)(uintptr_t)entity;entity_gens[1]=1;
 vector(entity+0x134/4,handles,4);
 for(unsigned i=0;i<4;i++){handles[i]=0x20001+i;item_slots[i+1]=(uint32_t)(uintptr_t)item[i];item_gens[i+1]=2;item[i][1]=handles[i];item[i][2]=(uint32_t)(uintptr_t)prototype[i];allowed[i]=i!=3;}
 names[0]="Laser";names[1]="Ion Engine";names[2]="Broken Rifle";names[3]="lAsEr";current_scope=7;mutate_name=changed_scope=0;
}
int main(void){
 initialize();InputContext c={7,255};PickerItemCandidate rows[4]={{0}};unsigned total=99;
 assert(picker_item_candidates46(&c,76,rows,1,1,&total)==1&&total==3);
 assert(rows[0].handle==0x20002&&rows[0].slot==1&&rows[0].ordinal==1&&!strcmp(rows[0].native,"Scan(Ion Engine)"));
 assert(rows[0].token==(((uint64_t)77<<32)|0x20002));
 assert(picker_item_candidates46(&c,94,rows,4,0,&total)==3&&total==3&&!strcmp(rows[0].native,"Insert(Laser)"));
 allowed[3]=1;
 assert(picker_item_candidates46(&c,81,rows,4,0,&total)==2&&total==2&&!strcmp(rows[0].native,"Recycle(Ion Engine)"));
 assert(picker_item_candidates46(&c,96,rows,4,2,&total)==0&&total==2);
 assert(picker_item_candidates46(&c,66,rows,4,0,&total)==-1&&total==0);
 item_gens[2]=1;assert(picker_item_candidates46(&c,76,rows,4,0,&total)==-1&&total==0);item_gens[2]=2;
 entity_gens[1]=0;assert(picker_item_candidates46(&c,76,rows,4,0,&total)==-1);entity_gens[1]=1;
 c.limit=8;assert(picker_item_candidates46(&c,76,rows,4,0,&total)==-1);c.limit=255;
 names[1]="Named (Mark II)";assert(picker_item_candidates46(&c,76,rows,4,0,&total)==2&&!strcmp(rows[0].native,"Scan(Named (Mark II))"));names[1]="Ion Engine";
 mutate_name=1;assert(picker_item_candidates46(&c,76,rows,4,0,&total)==-1);initialize();changed_scope=1;
 assert(picker_item_candidates46(&c,76,rows,4,0,&total)==-1);
 puts("PASS: actual-layout generation validation, all-carried source, exact native names, code bounds, pagination, case-insensitive duplicate rejection, stale scope and vector mutation rejection; no game loaded.");return 0;
}

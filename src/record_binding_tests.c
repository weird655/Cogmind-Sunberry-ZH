#include "record_binding.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 const char *name="Lgt. Ion Engine";RecordName r={14,1234,1238,15,0,"physical-record-14"};r.text_hash=record_text_hash(name,15);
 RecordBinding bound={0};assert(record_binding_verify(0x00120007,0x12,0x1000,0x1000,14,&r,name,15,&bound));assert(bound.handle==0x00120007&&bound.ordinal==14&&bound.payload==1238);
 RecordBinding before=bound;
 assert(!record_binding_verify(0x00130007,0x12,0x1000,0x1000,14,&r,name,15,&bound));assert(!memcmp(&before,&bound,sizeof bound));
 assert(!record_binding_verify(0x00120007,0x12,0x2000,0x1000,14,&r,name,15,&bound));
 assert(!record_binding_verify(0x00120007,0x12,0x1000,0x1000,15,&r,name,15,&bound));
 assert(!record_binding_verify(0x00120007,0x12,0x1000,0x1000,14,&r,"Lgt. Ion Enginx",15,&bound));
 assert(!record_binding_verify(0x00120007,0x12,0x1000,0x1000,14,&r,name,14,&bound));assert(!memcmp(&before,&bound,sizeof bound));
 puts("PASS record binding: native generations, prototype/ordinal/name checks, atomic rejection");return 0;
}

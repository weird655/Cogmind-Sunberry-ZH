#ifndef COGMIND_RECORD_BINDING_H
#define COGMIND_RECORD_BINDING_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef struct {uint32_t ordinal,prefix,payload,length;uint64_t text_hash;const char *physical_id;} RecordName;
typedef struct {uint32_t handle,prototype,ordinal,prefix,payload;const char *physical_id;} RecordBinding;
static inline uint64_t record_text_hash(const char *s,size_t n){uint64_t h=1469598103934665603ULL;for(size_t i=0;i<n;i++){h^=(unsigned char)s[i];h*=1099511628211ULL;}return h;}
/* Generation, prototype address and exact original name are independent checks.
   Never binds a textual candidate solely because its English happens to match. */
static inline int record_binding_verify(uint32_t handle,uint32_t generation,uint32_t current_prototype,uint32_t expected_prototype,uint32_t ordinal,const RecordName *record,const char *name,size_t bytes,RecordBinding *out){
 if(!handle||!record||!name||!out||generation!=(handle>>16)||!current_prototype||current_prototype!=expected_prototype||record->ordinal!=ordinal||record->length!=bytes||record_text_hash(name,bytes)!=record->text_hash)return 0;
 RecordBinding staged={handle,current_prototype,ordinal,record->prefix,record->payload,record->physical_id};*out=staged;return 1;
}
#endif

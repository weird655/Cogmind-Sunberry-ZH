#ifndef COGMIND_OWNER_INDEX_H
#define COGMIND_OWNER_INDEX_H
#include <stdint.h>
#include <string.h>
#define OWNER_INDEX_SIZE 2048u
typedef struct {const void *key;void *value;} OwnerIndexEntry;
typedef struct {OwnerIndexEntry entries[OWNER_INDEX_SIZE];} OwnerIndex;
static inline unsigned owner_index_slot(const void *key){return (unsigned)(((uintptr_t)key>>4)*2654435761u)&(OWNER_INDEX_SIZE-1);}
static inline void *owner_index_get(const OwnerIndex *index,const void *key){
 if(!key)return NULL;unsigned at=owner_index_slot(key);for(unsigned i=0;i<OWNER_INDEX_SIZE;i++,at=(at+1)&(OWNER_INDEX_SIZE-1)){const OwnerIndexEntry *e=&index->entries[at];if(!e->key)return NULL;if(e->key==key)return e->value;}return NULL;
}
static inline int owner_index_put(OwnerIndex *index,const void *key,void *value){
 if(!key||!value)return 0;unsigned at=owner_index_slot(key);for(unsigned i=0;i<OWNER_INDEX_SIZE;i++,at=(at+1)&(OWNER_INDEX_SIZE-1)){OwnerIndexEntry *e=&index->entries[at];if(!e->key||e->key==key){*e=(OwnerIndexEntry){key,value};return 1;}}return 0;
}
static inline void owner_index_remove(OwnerIndex *index,const void *key){
 if(!key)return;unsigned at=owner_index_slot(key);
 for(unsigned i=0;i<OWNER_INDEX_SIZE;i++,at=(at+1)&(OWNER_INDEX_SIZE-1)){
  OwnerIndexEntry *e=&index->entries[at];if(!e->key)return;if(e->key!=key)continue;
  memset(e,0,sizeof *e);at=(at+1)&(OWNER_INDEX_SIZE-1);
  while(index->entries[at].key){OwnerIndexEntry old=index->entries[at];memset(&index->entries[at],0,sizeof old);owner_index_put(index,old.key,old.value);at=(at+1)&(OWNER_INDEX_SIZE-1);}return;
 }
}
#endif

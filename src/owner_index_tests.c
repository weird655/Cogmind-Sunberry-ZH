#include "owner_index.h"
#include <stdio.h>
#include <stdlib.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
 OwnerIndex index={0};const void *keys[1024];
 /* Deliberately collide long runs; deletion must preserve later lookups. */
 for(unsigned i=0;i<1024;i++){keys[i]=(void*)(uintptr_t)(0x10000u+i*0x8000u);CHECK(owner_index_put(&index,keys[i],(void*)(uintptr_t)(i+1)));}
 for(unsigned i=0;i<1024;i++)CHECK(owner_index_get(&index,keys[i])==(void*)(uintptr_t)(i+1));
 for(unsigned i=0;i<1024;i+=2)owner_index_remove(&index,keys[i]);
 for(unsigned i=0;i<1024;i++)CHECK(owner_index_get(&index,keys[i])==(i%2?(void*)(uintptr_t)(i+1):NULL));
 for(unsigned i=0;i<1024;i+=2)CHECK(owner_index_put(&index,keys[i],(void*)(uintptr_t)(i+2049)));
 for(unsigned i=0;i<1024;i++)CHECK(owner_index_get(&index,keys[i])==(void*)(uintptr_t)(i%2?i+1:i+2049));
 for(unsigned i=0;i<1024;i++)owner_index_remove(&index,keys[i]);
 for(unsigned i=0;i<1024;i++)CHECK(owner_index_get(&index,keys[i])==NULL);
 CHECK(!owner_index_get(&index,NULL));CHECK(!owner_index_put(&index,NULL,(void*)1));CHECK(!owner_index_put(&index,(void*)1,NULL));
 printf("PASS %u owner-index collision, removal, reuse and 1024-owner checks\n",checks);return 0;
}

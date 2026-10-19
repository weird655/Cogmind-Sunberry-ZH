#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
typedef struct {void *owner;} OwnerRun;
static OwnerRun managed;static unsigned owner_active_count,queries;static uint32_t a[32],assoc[6],object[32];
static uintptr_t va(uintptr_t p){return p;}
static int readable(const void *p,size_t n){queries++;return (p==a&&n<=sizeof a)||(p==assoc&&n<=sizeof assoc)||(p==object&&n<=sizeof object);}
static OwnerRun *owner_find(void *p){return p==managed.owner?&managed:NULL;}
#include "animation_owner_lookup.h"
int main(void){
 for(unsigned i=0;i<1000000;i++)assert(!animation_owner((void*)(uintptr_t)(i+1)));assert(!queries);
 a[1]=(uint32_t)(uintptr_t)assoc;assoc[0]=(uint32_t)(uintptr_t)object;object[0]=0xc36328;managed.owner=object;owner_active_count=1;
 assert(animation_owner(a)==&managed&&queries==3);
 queries=0;a[1]=1;assert(!animation_owner(a)&&queries==2);a[1]=(uint32_t)(uintptr_t)assoc;
 queries=0;object[0]=0;assert(!animation_owner(a)&&queries==3);object[0]=0xc36328;
 queries=0;managed.owner=NULL;assert(!animation_owner(a)&&queries==3);
 queries=0;owner_active_count=0;assert(!animation_owner(a)&&queries==0);
 puts("PASS: 1M unrelated effects incur zero memory queries; managed owner, invalid association, vtable and stale owner guards retained.");return 0;
}

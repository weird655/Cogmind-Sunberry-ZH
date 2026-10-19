/* Unrelated native effects must not perform OS memory queries when no managed
   Unicode terminal owner exists. Positive lookups retain every old guard. */
static OwnerRun *animation_owner(void *p){
 if(!owner_active_count||!p)return NULL;
 uint32_t *a=p;if(!readable(a,0x80)||!readable((void*)a[1],24))return NULL;
 void *owner=*(void**)a[1];if(!readable(owner,4)||*(uint32_t*)owner!=va(0xc36328))return NULL;return owner_find(owner);
}

/* Beta 17.1, exact supported EXE profiles only. Main game UI thread.
   This adapter reads the real player's carried/attached handles and uses the
   same native item-name builder and eligibility predicates as the manual
   Scan/Recycle/Insert/Study parser. It never dispatches a command.

   Include after picker_schematic_candidates.h for picker_vector45(). New
   address mappings required in native_layout_steam.h are listed in REPORT.md.
   Duplicate qualifying handles whose names compare equal are omitted: the
   original text parser cannot identify a later same-name item separately. */
#ifndef SUNBERRY_PICKER_ITEM_CANDIDATES46_H
#define SUNBERRY_PICKER_ITEM_CANDIDATES46_H
#define PICKER_ITEM_LIMIT46 8192u
typedef struct {
 uint32_t ordinal,prototype,handle,slot;
 char name[INPUT_NATIVE],native[INPUT_NATIVE];
 uint64_t token;
} PickerItemCandidate;

#ifdef RECORD_IDENTITY_MANIFEST
static int picker_item_equal46(const char *a,const char *b){
 for(;*a&&*b;a++,b++){
  unsigned x=(unsigned char)*a,y=(unsigned char)*b;
  if(x>='A'&&x<='Z')x+='a'-'A';if(y>='A'&&y<='Z')y+='a'-'A';
  if(x!=y)return 0;
 }
 return *a==*b;
}
/* The native generation resolver uses slot=low16, generation=high16, pointer
   vector at registry+0 and generation vector at registry+0x10. */
static uint32_t picker_resolve_handle46(uintptr_t global,uint32_t handle){
 uint32_t *g=(void*)va(global);if(!handle||!readable(g,4))return 0;
 uint32_t registry=*g,items=0,generations=0;unsigned count=0,gen_count=0,index=handle&65535u;
 if(!registry||!picker_vector45(registry,&items,&count,65536)||
    !picker_vector45(registry+0x10,&generations,&gen_count,65536)||index>=count||index>=gen_count||
    *(uint32_t*)(uintptr_t)(generations+index*4)!=(handle>>16))return 0;
 return *(uint32_t*)(uintptr_t)(items+index*4);
}
static int picker_player_items46(uint32_t *world,uint32_t *player_handle,
 uint32_t *entity,uint32_t *begin,unsigned *count){
 uint32_t *g=(void*)va(0xcefc4c);if(!readable(g,4)||!*g||
    !readable((void*)(uintptr_t)(*g+0x66c),4))return 0;
 *world=*g;*player_handle=*(uint32_t*)(uintptr_t)(*world+0x66c);
 *entity=picker_resolve_handle46(0xd3c228,*player_handle);
 return *entity&&readable((void*)(uintptr_t)*entity,0x140)&&
        picker_vector45(*entity+0x134,begin,count,PICKER_ITEM_LIMIT46);
}
static int picker_item_native_name46(uint32_t item,char *out,size_t capacity){
#ifdef PICKER_ITEM_PARAMETER_FIXTURE46
 return picker_item_fixture_name46(item,out,capacity);
#else
 unsigned char str[28]={0};
 typedef void *(__attribute__((thiscall)) *ItemName)(void*,void*,int,int);
 ((ItemName)va(0x571db0))((void*)(uintptr_t)item,str,0,0);
 int ok=input_ns((uintptr_t)str,out,capacity);
 ((NativeVoid)va(0x9af350))(str);return ok;
#endif
}
static int picker_item_eligible46(uint32_t item,unsigned code){
#ifdef PICKER_ITEM_PARAMETER_FIXTURE46
 return picker_item_fixture_eligible46(item,code);
#else
 typedef unsigned char (__attribute__((thiscall)) *ItemPredicate)(void*);
 typedef unsigned char (__attribute__((thiscall)) *ScanPredicate)(void*,int,int);
 if(code==76)return ((ScanPredicate)va(0x5773d0))((void*)(uintptr_t)item,1,0)!=0;
 uintptr_t address=code==81?0x5775a0:code==94?0x5776c0:code==96?0x577700:0;
 return address&&((ItemPredicate)va(address))((void*)(uintptr_t)item)!=0;
#endif
}
#endif
/* -1 on changed context/invalid source; otherwise rows copied. total is the
   complete unambiguous eligible count, so callers can page without truncation.
   Requery on leaf selection and compare handle, slot, prototype, ordinal,
   token, native command and complete count before the original Enter path. */
static int picker_item_candidates46(const InputContext *c,unsigned code,
 PickerItemCandidate *out,unsigned capacity,unsigned offset,unsigned *total){
 if(total)*total=0;
#ifndef RECORD_IDENTITY_MANIFEST
 (void)c;(void)code;(void)out;(void)capacity;(void)offset;return -1;
#else
 const char *expected=code==76?"Scan(@)":code==81?"Recycle(@)":code==94?"Insert(@)":code==96?"Study(@)":NULL;
 InputContext current;char command[INPUT_NATIVE];
 if(!c||!out||!capacity||!expected||!input_get_context(&current)||current.scope!=c->scope||
    !input_allowed(c,code)||!input_command_name(code,command,sizeof command)||strcmp(command,expected))return -1;
 uint32_t world=0,player=0,entity=0,begin=0;unsigned count=0;
 if(!picker_player_items46(&world,&player,&entity,&begin,&count))return -1;
 uint32_t *handles=calloc(count?count:1,sizeof *handles);
 PickerItemCandidate *rows=calloc(count?count:1,sizeof *rows);
 unsigned char *ambiguous=calloc(count?count:1,1);
 if(!handles||!rows||!ambiguous){free(handles);free(rows);free(ambiguous);return -1;}
 if(count)memcpy(handles,(void*)(uintptr_t)begin,count*4);
 int ok=1;unsigned found=0,copied=0;
 for(unsigned n=0;n<count;n++){
  uint32_t item=picker_resolve_handle46(0xd3c224,handles[n]);RecordBinding binding={0};
  if(!item||!readable((void*)(uintptr_t)item,0x20)||
     *(uint32_t*)(uintptr_t)(item+4)!=handles[n]||!native_record_binding(handles[n],&binding)||
     *(uint32_t*)(uintptr_t)(item+8)!=binding.prototype||
     !readable((void*)(uintptr_t)binding.prototype,0x210)){ok=0;break;}
  if(!picker_item_eligible46(item,code))continue;
  PickerItemCandidate row={0};
  if(!picker_item_native_name46(item,row.name,sizeof row.name)||!row.name[0]){ok=0;break;}
  size_t prefix=strchr(expected,'(')-expected;
  int length=snprintf(row.native,sizeof row.native,"%.*s(%s)",(int)prefix,expected,row.name);
  if(length<=0||length>=(int)sizeof row.native||length>c->limit){ok=0;break;}
  row.handle=handles[n];row.slot=n;row.ordinal=binding.ordinal;row.prototype=binding.prototype;
  row.token=(((uint64_t)code+1)<<32)|row.handle;
  for(unsigned j=0;j<found;j++)if(picker_item_equal46(rows[j].name,row.name))ambiguous[j]=ambiguous[found]=1;
  rows[found++]=row;
 }
 uint32_t after_world=0,after_player=0,after_entity=0,after_begin=0;unsigned after_count=0;
 if(!input_get_context(&current)||current.scope!=c->scope||!input_allowed(c,code)||
    !picker_player_items46(&after_world,&after_player,&after_entity,&after_begin,&after_count)||
    after_world!=world||after_player!=player||after_entity!=entity||after_begin!=begin||after_count!=count||
    (count&&memcmp(handles,(void*)(uintptr_t)begin,count*4)))ok=0;
 unsigned eligible=0;
 if(ok)for(unsigned n=0;n<found;n++)if(!ambiguous[n]){
  if(eligible>=offset&&copied<capacity)out[copied++]=rows[n];eligible++;
 }
 free(handles);free(rows);free(ambiguous);
 if(!ok)return -1;if(total)*total=eligible;return (int)copied;
#endif
}
#endif

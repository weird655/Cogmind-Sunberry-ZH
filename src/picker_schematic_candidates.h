/* Profile-pinned autocomplete adapter, statically verified against both Beta
   17.1 executables. Include after terminal_input_native.h's
   native helpers, before its UI tick. Main game thread only. No Enter event,
   hack dispatcher, success roll, game object creation, or knowledge writes.

   The source addresses below are mapped in native_layout_steam.h. The isolated
   fixture exercises empty native candidates and save/read/restore without a
   command submission; nonempty gameplay candidates still require live QA.

   This source intentionally implements Schematic only. The native completion
   function has no equivalent Analysis/Load/Scan/Recycle/Insert/Study branch. */
#ifndef SUNBERRY_PICKER_SCHEMATIC_CANDIDATES45_H
#define SUNBERRY_PICKER_SCHEMATIC_CANDIDATES45_H

typedef struct {
 uint32_t candidate_index,ordinal,prototype;
 char name[INPUT_NATIVE],native[INPUT_NATIVE];
 uint64_t token;
} PickerSchematicCandidate;

static int picker_schematic_row45(unsigned code,uint32_t ordinal,uint32_t prototype,
 unsigned candidate,int limit,PickerSchematicCandidate *out){
 if(code!=1||!out)return 0;
 PickerSchematicCandidate row={0};char canonical[INPUT_NATIVE];
 if(!readable((void*)(uintptr_t)prototype,0x40)||*(uint32_t*)(uintptr_t)prototype!=ordinal||
    !input_ns(prototype+8,canonical,sizeof canonical)||!canonical[0]||strpbrk(canonical,"()")||
    !input_ns(prototype+0x24,row.name,sizeof row.name)||!row.name[0])return 0;
 /* Canonical name +8 is parser identity. Display name +0x24 may repeat. */
#ifdef RECORD_IDENTITY_MANIFEST
 if(ordinal>=RECORD_NAME_COUNT||strlen(canonical)!=record_names[ordinal].length||
    record_text_hash(canonical,strlen(canonical))!=record_names[ordinal].text_hash)return 0;
#endif
 int length=snprintf(row.native,sizeof row.native,"Schematic(%s)",canonical);
 if(length<=0||length>=(int)sizeof row.native||length>limit)return 0;
 row.candidate_index=candidate;row.ordinal=ordinal;row.prototype=prototype;
 row.token=(((uint64_t)code+1)<<32)|((uint64_t)ordinal+1);*out=row;return 1;
}

static int picker_vector45(uint32_t address,uint32_t *begin,unsigned *count,unsigned limit){
 uint32_t *v=(void*)(uintptr_t)address;
 if(!readable(v,12)||v[1]<v[0]||(v[1]-v[0])%4||v[2]<v[1]||
    v[1]-v[0]>limit*4||(v[1]>v[0]&&!readable((void*)(uintptr_t)v[0],v[1]-v[0])))return 0;
 *begin=v[0];*count=(v[1]-v[0])/4;return 1;
}
static void picker_set_native45(const InputContext *c,const char *text){
 unsigned char str[28]={0};
 ((StringCtor)va(0x9af0f0))(str,text);
 typedef void (__attribute__((thiscall)) *SetInput)(void*,void*);
 ((SetInput)va(0x48d300))((void*)(uintptr_t)c->input,str);
 ((NativeVoid)va(0x9af350))(str);
}
/* Returns -1 if context, bounds or restoration failed, otherwise number copied.
   total is the complete native candidate count; caller can paginate. Requery on
   leaf selection and require matching scope, ordinal, prototype and native text
   before passing through the existing original-input Enter path. Never retain
   record pointers beyond this context. The translated label is presentation. */
static int picker_schematic_candidates45(const InputContext *c,unsigned code,
 PickerSchematicCandidate *out,unsigned capacity,unsigned offset,unsigned *total){
 if(total)*total=0;
 /* Native key 1 is item schematic; key 2 is robot schematic and uses a
    different prototype bank. This helper supplies item candidates only. */
 if(!c||!out||!capacity||code!=1||!input_allowed(c,code))return -1;
 char command[INPUT_NATIVE],previous[INPUT_NATIVE];InputContext current;
 if(!input_command_name(code,command,sizeof command)||strcmp(command,"Schematic(*)")||
    !input_get_context(&current)||current.scope!=c->scope||
    !input_ns(c->input+0x6c,previous,sizeof previous))return -1;
 uint32_t *m=(void*)(uintptr_t)c->manual;
 /* The native refresh indexes the global map without checking its bounds.
    A detached/closing UI must not pass an uninitialized or stale map to it. */
 uint32_t mapping=0,allowed=0;unsigned mappings=0,allow_count=0;
 if(!picker_vector45((uint32_t)va(0xd3860c),&mapping,&mappings,256)||!mappings||
    !picker_vector45(c->manual+0x70,&allowed,&allow_count,INPUT_ROWS))return -1;
 for(unsigned k=0;k<allow_count;k++){
  unsigned logical=*(uint32_t*)(uintptr_t)(allowed+4*k);
  if(logical>=mappings)return -1;
  unsigned key=*(uint32_t*)(uintptr_t)(mapping+4*logical);char label[INPUT_NATIVE];
  if(key>=128||!input_command_name(key,label,sizeof label))return -1;
 }
 unsigned selected_command=m[0xac/4],selected_parameter=m[0xc0/4];
 typedef void (__attribute__((thiscall)) *RefreshManual)(void*);
 RefreshManual refresh=(RefreshManual)va(0x909c70);
 picker_set_native45(c,"Schematic(");refresh(m);
 unsigned count=0,known_count=0,bank_count=0;uint32_t begin=0,known=0,bank=0;
 int ok=input_get_context(&current)&&current.scope==c->scope&&
  picker_vector45(c->manual+0xb0,&begin,&count,8192)&&
  picker_vector45((uint32_t)va(0xcfd1cc),&known,&known_count,8192)&&
  picker_vector45((uint32_t)va(0xd2d1c4),&bank,&bank_count,8192);
 unsigned copied=0;
 if(ok){
  if(total)*total=count;
  for(unsigned n=offset;n<count&&copied<capacity;n++){
   uint32_t candidate=*(uint32_t*)(uintptr_t)(begin+4*n);
   if(candidate>=known_count){ok=0;break;}
   uint32_t ordinal=*(uint32_t*)(uintptr_t)(known+4*candidate);
   if(ordinal>=bank_count){ok=0;break;}
   uint32_t prototype=*(uint32_t*)(uintptr_t)(bank+4*ordinal);
   PickerSchematicCandidate row={0};
   if(!picker_schematic_row45(code,ordinal,prototype,candidate,c->limit,&row)){ok=0;break;}
   out[copied++]=row;
  }
 }
 /* Restore input through native refresh even when reading fails. Native blank
    refresh may retain its previous candidate vectors; do not claim a bytewise
    cache rollback. Restore original cursor values only while in bounds. */
 picker_set_native45(c,previous);refresh(m);
 uint32_t restore_begin;unsigned restore_count;
 if(picker_vector45(c->manual+0x9c,&restore_begin,&restore_count,256)&&selected_command<restore_count)m[0xac/4]=selected_command;
 if(picker_vector45(c->manual+0xb0,&restore_begin,&restore_count,8192)&&selected_parameter<restore_count)m[0xc0/4]=selected_parameter;
 char restored[INPUT_NATIVE];
 if(!input_get_context(&current)||current.scope!=c->scope||
    !input_ns(c->input+0x6c,restored,sizeof restored)||strcmp(previous,restored))ok=0;
 if(!ok){if(total)*total=0;return -1;}return (int)copied;
}
#endif

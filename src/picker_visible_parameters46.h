/* Verified native-row adapter: include after input_ns/readable/va.
   Caller must first validate a CURRENT, non-disabled native menu row, its
   definition pointer, and input_allowed(c, definition[0]). No UI mutation.
   No database-wide choice enumeration. Whole banks are read only to prove that
   the native parser's FIRST case-insensitive exact name match is this index.

   Add Steam mappings if absent:
     d25de0 -> d26ed8; // robot prototype bank
   Item/lore banks already mapped by the current project.
*/
#ifndef SUNBERRY_PICKER_VISIBLE_PARAMETERS46_H
#define SUNBERRY_PICKER_VISIBLE_PARAMETERS46_H

typedef struct {
 unsigned code,ordinal;uint32_t prototype;
 char canonical[INPUT_NATIVE],display[INPUT_NATIVE],native[INPUT_NATIVE];
} PickerVisibleParameter46;

static int picker_visible_vector46(uint32_t address,uint32_t *begin,unsigned *count){
 uint32_t *v=(void*)(uintptr_t)address;
 if(!readable(v,12)||v[1]<v[0]||(v[1]-v[0])%4||v[2]<v[1]||
    v[1]-v[0]>8192*4||(v[1]>v[0]&&!readable((void*)(uintptr_t)v[0],v[1]-v[0])))return 0;
 *begin=v[0];*count=(v[1]-v[0])/4;return 1;
}
static int picker_ascii_equal46(const char *a,const char *b){
 for(;;a++,b++){
  unsigned x=(unsigned char)*a,y=(unsigned char)*b;
  if(x>='A'&&x<='Z')x+='a'-'A';
  if(y>='A'&&y<='Z')y+='a'-'A';
  if(x!=y)return 0;
  if(!x)return 1;
 }
}
/* Same first-match semantics as old 9e2830 (field+8) and 9e2890
   (field+4). -2 is incomplete/invalid storage; -1 is a complete absence. */
static int picker_exact_parameter46(uint32_t bank,unsigned field,const char *name){
 uint32_t begin=0;unsigned count=0;
 if(!picker_visible_vector46(bank,&begin,&count))return -2;
 for(unsigned n=0;n<count;n++){
  uint32_t record=*(uint32_t*)(uintptr_t)(begin+4*n);char current[INPUT_NATIVE];
  if(!readable((void*)(uintptr_t)record,field+28)||
     *(uint32_t*)(uintptr_t)record!=n||
     !input_ns(record+field,current,sizeof current))return -2;
  if(picker_ascii_equal46(current,name))return (int)n;
 }
 return -1;
}
static int picker_visible_parameter46(unsigned code,unsigned ordinal,int limit,
 PickerVisibleParameter46 *out){
 if(!out||code>3||limit<1||limit>=INPUT_NATIVE)return 0;
 uint32_t address=(uint32_t)va(code==0?0xd35b58:code==1?0xd2d1c4:0xd25de0);
 unsigned field=code==1?8:4,display=code==0?4:code==1?0x24:0x1ac;
 uint32_t begin=0;unsigned count=0;
 if(!picker_visible_vector46(address,&begin,&count)||ordinal>=count)return 0;
 uint32_t record=*(uint32_t*)(uintptr_t)(begin+4*ordinal);
 PickerVisibleParameter46 row={0};
 if(!readable((void*)(uintptr_t)record,display+28)||
    *(uint32_t*)(uintptr_t)record!=ordinal||
    !input_ns(record+field,row.canonical,sizeof row.canonical)||
    !row.canonical[0]||strpbrk(row.canonical,"()")||
    picker_exact_parameter46(address,field,row.canonical)!=(int)ordinal)return 0;
 /* Schematic resolves item names before robot names. Reject a robot leaf if
    its spelling would be routed to an item. */
 if(code==2&&picker_exact_parameter46((uint32_t)va(0xd2d1c4),8,row.canonical)!=-1)return 0;
 /* Analysis explicitly rejects classes 0x47/0x48 and empty analysis text. */
 if(code==3){
  if(*(uint32_t*)(uintptr_t)(record+0x28)==0x47||
     *(uint32_t*)(uintptr_t)(record+0x28)==0x48)return 0;
  /* Check native string length, not text contents: analyses can exceed the
     command string buffer, and are not picker labels. */
  uint32_t *s=(void*)(uintptr_t)(record+0x170);
  if(!readable(s,28)||!s[4]||s[4]>s[5]||s[5]>65536)return 0;
 }
 if(!input_ns(record+display,row.display,sizeof row.display)||!row.display[0])
  strcpy(row.display,row.canonical);
 const char *verb=code==0?"Query":code==3?"Analysis":"Schematic";
 int length=snprintf(row.native,sizeof row.native,"%s(%s)",verb,row.canonical);
 if(length<=0||length>=(int)sizeof row.native||length>limit)return 0;
 row.code=code;row.ordinal=ordinal;row.prototype=record;*out=row;return 1;
}
#endif

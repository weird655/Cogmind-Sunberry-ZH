/* Exact supported Beta 17.1 profiles; main game UI thread only.
   Load(@) uses learned item/robot schematics. It does not name an inventory
   slot. Labels use the native display string; commands use the exact alias
   consumed by the native parser and must round-trip to the same bank record.
   Include after picker_schematic_candidates.h for picker_vector45(). */
#ifndef SUNBERRY_PICKER_LOAD_CANDIDATES46_H
#define SUNBERRY_PICKER_LOAD_CANDIDATES46_H
#define PICKER_LOAD_LIMIT46 8192u
typedef struct {
 uint32_t ordinal,prototype,kind; /* kind 1=item, 2=robot */
 char name[INPUT_NATIVE],native[INPUT_NATIVE];
 uint64_t token;
} PickerLoadCandidate;

static int picker_load_ascii46(const char *s){
 for(;*s;s++)if((unsigned char)*s<32||(unsigned char)*s>=127)return 0;
 return 1;
}
static int picker_load_equal46(const char *a,const char *b){
 for(;*a&&*b;a++,b++){
  unsigned x=(unsigned char)*a,y=(unsigned char)*b;
  if(x>='a'&&x<='z')x-='a'-'A';
  if(y>='a'&&y<='z')y-='a'-'A';
  if(x!=y)return 0;
 }
 return *a==*b;
}
/* Mirrors native 9e2830/9e2890 exact alias equality on validated ASCII.
   Native lookup scans items first, then robots only when no item alias matches.
   -1=absent, -2=invalid/incomplete source, -3=duplicate alias. Rejecting an
   ambiguous alias is stricter than the native parser's first-match behavior. */
static int picker_load_lookup46(uint32_t bank,unsigned kind,const char *alias){
 uint32_t begin=0;unsigned count=0,field=kind==1?8:4;int selected=-1;
 if(!picker_vector45(bank,&begin,&count,PICKER_LOAD_LIMIT46))return -2;
 for(unsigned n=0;n<count;n++){
  uint32_t record=*(uint32_t*)(uintptr_t)(begin+4*n);char current[INPUT_NATIVE];
  if(!record||!readable((void*)(uintptr_t)record,field+28)||
     *(uint32_t*)(uintptr_t)record!=n||
     !input_ns(record+field,current,sizeof current)||!picker_load_ascii46(current))return -2;
  if(picker_load_equal46(current,alias)){
   if(selected!=-1)return -3;selected=(int)n;
  }
 }
 return selected;
}
/* -1 = changed context or invalid source; otherwise rows copied. total is the
   full safe count. Requery on click and compare scope/kind/ordinal/prototype/
   token/native text before the original Enter path. Duplicate learned aliases
   compare case-insensitively and all such leaves are omitted. */
static int picker_load_candidates46(const InputContext *c,unsigned code,
 PickerLoadCandidate *out,unsigned capacity,unsigned offset,unsigned *total){
 if(total)*total=0;
 InputContext current;char command[INPUT_NATIVE];
 if(!c||code!=66||!out||!capacity||!input_get_context(&current)||
    current.scope!=c->scope||!input_allowed(c,code)||
    !input_command_name(code,command,sizeof command)||strcmp(command,"Load(@)"))return -1;
 uint32_t bank_address[2]={(uint32_t)va(0xd2d1c4),(uint32_t)va(0xd25de0)};
 uint32_t learned_address[2]={(uint32_t)va(0xcf4844),(uint32_t)va(0xcf4888)};
 uint32_t bank_begin[2]={0},learned_begin[2]={0};
 unsigned bank_count[2]={0},learned_count[2]={0},maximum=0;
 for(unsigned k=0;k<2;k++){
  if(!picker_vector45(bank_address[k],&bank_begin[k],&bank_count[k],PICKER_LOAD_LIMIT46)||
     !picker_vector45(learned_address[k],&learned_begin[k],&learned_count[k],PICKER_LOAD_LIMIT46)||
     bank_count[k]!=learned_count[k])return -1;
  maximum+=bank_count[k];
 }
 uint32_t *bank_snapshot[2]={0},*learned_snapshot[2]={0};
 PickerLoadCandidate *rows=calloc(maximum?maximum:1,sizeof *rows);
 unsigned char *ambiguous=calloc(maximum?maximum:1,1);
 int ok=rows&&ambiguous;unsigned found=0,copied=0,eligible=0;
 for(unsigned k=0;k<2&&ok;k++){
  bank_snapshot[k]=calloc(bank_count[k]?bank_count[k]:1,4);
  learned_snapshot[k]=calloc(learned_count[k]?learned_count[k]:1,4);
  if(!bank_snapshot[k]||!learned_snapshot[k]){ok=0;break;}
  if(bank_count[k]){
   memcpy(bank_snapshot[k],(void*)(uintptr_t)bank_begin[k],bank_count[k]*4);
   memcpy(learned_snapshot[k],(void*)(uintptr_t)learned_begin[k],learned_count[k]*4);
  }
 }
 for(unsigned k=0;k<2&&ok;k++)for(unsigned n=0;n<bank_count[k];n++){
  if(!learned_snapshot[k][n])continue;
  uint32_t prototype=bank_snapshot[k][n];unsigned kind=k+1;
  unsigned name_offset=kind==1?0x24:0x1ac,alias_offset=kind==1?8:4;
  PickerLoadCandidate row={0};char alias[INPUT_NATIVE];
  if(!prototype||!readable((void*)(uintptr_t)prototype,name_offset+28)||
     *(uint32_t*)(uintptr_t)prototype!=n||
     !input_ns(prototype+name_offset,row.name,sizeof row.name)||!row.name[0]||
     !input_ns(prototype+alias_offset,alias,sizeof alias)||!alias[0]||
     !picker_load_ascii46(alias)||strpbrk(alias,"()")){ok=0;break;}
  int length=snprintf(row.native,sizeof row.native,"Load(%s)",alias);
  if(length<=0||length>=(int)sizeof row.native||length>c->limit){ok=0;break;}
  row.ordinal=n;row.prototype=prototype;row.kind=kind;
  row.token=(((uint64_t)code+1)<<32)|((uint64_t)kind<<24)|((uint64_t)n+1);
  for(unsigned j=0;j<found;j++)if(picker_load_equal46(rows[j].native,row.native))
   ambiguous[j]=ambiguous[found]=1;
  rows[found++]=row;
 }
 /* Do not use display-name substring matching. Native exact lookup must select
    this record; an unlearned item alias also takes precedence over a robot. */
 for(unsigned n=0;n<found&&ok;n++)if(!ambiguous[n]){
  PickerLoadCandidate *row=&rows[n];char alias[INPUT_NATIVE];
  if(!input_ns(row->prototype+(row->kind==1?8:4),alias,sizeof alias)) {ok=0;break;}
  int item=picker_load_lookup46(bank_address[0],1,alias),robot=-1;
  if(item==-2){ok=0;break;}
  if(row->kind==2&&item==-1){
   robot=picker_load_lookup46(bank_address[1],2,alias);
   if(robot==-2){ok=0;break;}
  }
  int same=row->kind==1?item==(int)row->ordinal:item==-1&&robot==(int)row->ordinal;
  if(!same){ambiguous[n]=1;continue;}
 }
 for(unsigned k=0;k<2&&ok;k++){
  uint32_t b=0,l=0;unsigned bc=0,lc=0;
  if(!picker_vector45(bank_address[k],&b,&bc,PICKER_LOAD_LIMIT46)||
     !picker_vector45(learned_address[k],&l,&lc,PICKER_LOAD_LIMIT46)||
     b!=bank_begin[k]||l!=learned_begin[k]||bc!=bank_count[k]||lc!=learned_count[k]||
     (bc&&memcmp(bank_snapshot[k],(void*)(uintptr_t)b,bc*4))||
     (lc&&memcmp(learned_snapshot[k],(void*)(uintptr_t)l,lc*4)))ok=0;
 }
 if(!input_get_context(&current)||current.scope!=c->scope||!input_allowed(c,code))ok=0;
 if(ok)for(unsigned n=0;n<found;n++)if(!ambiguous[n]){
  if(eligible>=offset&&copied<capacity)out[copied++]=rows[n];eligible++;
 }
 for(unsigned k=0;k<2;k++){free(bank_snapshot[k]);free(learned_snapshot[k]);}
 free(rows);free(ambiguous);
 if(!ok)return -1;if(total)*total=eligible;return (int)copied;
}
#endif

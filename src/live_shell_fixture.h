#ifdef LIVE_RESOURCE_PROBE
#ifdef CONTENT_NATIVE_PROBE
#include "content_fixture_sources.h"
#include "log_content_fixture.h"
#endif
/* Sacrificial native UI fixture. Calls display output only, never hack handlers. */
static void *live_fixture_parent;
static uintptr_t live_fixture_vtable[32];
static unsigned live_fixture_stage,live_fixture_errors,live_fixture_owner_baseline,live_fixture_child_baseline,live_fixture_cn_owners,live_fixture_links;

#ifdef RECORD_IDENTITY_PROBE
static void record_identity_fixture(void *parent){
 uint32_t *bank=(void*)va(0xd2d1c4);if(!readable(bank,12)||bank[1]-bank[0]<15*4){live_fixture_errors++;return;}
 uint32_t *prototypes=(void*)(uintptr_t)bank[0];if(!readable(prototypes,15*4)){live_fixture_errors++;return;}
 uint32_t handle=0;typedef void* (__attribute__((thiscall)) *Create)(void*,uint32_t*,void*);
 ((Create)va(0x7932b0))(*(void**)va(0xcefc4c),&handle,(void*)(uintptr_t)prototypes[14]);
 RecordBinding binding={0};int bound=native_record_binding(handle,&binding);char output[512];
 TextObject object={TEXT_ENTRY_ITEM_LABEL,"Lgt. Ion Engine",15,bound?&binding:NULL};
 int translated=message_resolve_object(&object,output,sizeof output,NULL);
 RecordBinding unchanged=binding;int stale=native_record_binding(handle^0x10000,&binding);
 unsigned errors=!bound||binding.ordinal!=14||translated!=1||stale||memcmp(&unchanged,&binding,sizeof binding);
 void *widget=(*(NativeNew*)va(0xb6132c))(0xa0);
 if(!widget)errors++;
 else{
  memset(widget,0,0xa0);((ConsoleCtor)va(0x48c060))(widget,parent,60,1,0,0,0,1,-1);uint32_t *w=widget;w[0x6c/4]=handle;
  NativeStringValue label={{0}};((StringCtor)va(0x9af0f0))(&label,"Lgt. Ion Engine               ");
  w[0]=va(0xc3599c);int inventory=inventory_name(widget,7,0,&label,va(0x4aa4e8));
  w[0]=va(0xc35794);int equipped=inventory_name(widget,5,0,&label,va(0x4a90eb));
  errors+=inventory!=1||equipped!=1;((NativeVoid)va(0x9af350))(&label);live_cells_remove(widget);
  uint32_t *children=(void*)((char*)parent+0x44);for(unsigned i=0;i<vector_count(children);i++)if(((void**)children[0])[i]==widget){w[0]=va(0xc2ef14);typedef void (__cdecl *Remove)(void*,unsigned);((Remove)va(0x9cea50))(children,i);break;}
  fprintf(logfile,"RECORD-IDENTITY-ADAPTERS inventory=%d equipped=%d same-native-handle=1 errors=%u\n",inventory,equipped,errors);
 }
 live_fixture_errors+=errors;
 fprintf(logfile,"RECORD-IDENTITY-FIXTURE created-native-item=1 ordinal=%lu bound=%d stale-rejected=%d errors=%u isolated-process-only=1\n",(unsigned long)binding.ordinal,bound,!stale,errors);fflush(logfile);
}
#endif
static DWORD live_fixture_born;
static uint32_t live_fixture_machine[64],live_fixture_ref[2],live_fixture_state[8];
static void* __attribute__((fastcall)) live_fixture_resolve(void *self,void *unused){
 (void)unused;if(live_shell_fixture_owner&&(self==(char*)live_shell_fixture_owner+0x70||live_replay))return live_fixture_ref;
 typedef void* (__attribute__((thiscall)) *Resolve)(void*);return ((Resolve)va(0x9b64f0))(self);
}
static void* __attribute__((fastcall)) live_fixture_get_state(void *self,void *unused){
 (void)unused;if(self==live_fixture_ref)return live_fixture_state;
 typedef void* (__attribute__((thiscall)) *State)(void*);return ((State)va(0x45cb30))(self);
}
static int live_fixture_context_install(void){
 memset(live_fixture_machine,0,sizeof live_fixture_machine);memset(live_fixture_state,0,sizeof live_fixture_state);live_fixture_ref[0]=0;live_fixture_ref[1]=(uint32_t)(uintptr_t)live_fixture_machine;
 return live_shell_patch_call(0x90d9e4,0x9b64f0,live_fixture_resolve)&&live_shell_patch_call(0x90da0d,0x9b64f0,live_fixture_resolve)&&live_shell_patch_call(0x90da14,0x45cb30,live_fixture_get_state)&&live_shell_patch_call(0x90e38d,0x9b64f0,live_fixture_resolve)&&live_shell_patch_call(0x90e3af,0x9b64f0,live_fixture_resolve)&&live_shell_patch_call(0x90e3b6,0x45cb30,live_fixture_get_state);
}
static const char *live_fixture_lore="Derelicts have been working to develop longer-range cannons for use by larger robots, likely to facilitate guerilla warfare tactics to reduce their generally enormous losses whenever they decide to enter [Complex 0b10]. The first known type, a version of the gauss cannon, is known as the slug cannon and certainly does significant damage for its weight, but at a reduced accuracy. The newer hypervelocity variant, the bore cannon, is even less accurate, but capable of penetrating multiple targets with a high chance of disabling whatever it penetrates.";
static unsigned live_fixture_lore_index=255;
#ifdef DSF_RESOURCE_PROBE
static int live_fixture_select_dsf(void){
 const char *dsf="Distributed [Storage] Facility. An area designated for storage of materials and components. Distribution serves two purposes, making local access to resources easier while also minimizing losses in the event that one is compromised. DSF entrances are temporarily sealed in the event that potential threats appear in the areas they serve.";
 uint32_t *records=(void*)va(0xd35b58);unsigned count=safe_native_vector(records,8192);
 for(unsigned i=0;i<count;i++){
  void *record=((void**)records[0])[i];if(!readable(record,0xc0))continue;
  for(unsigned off=0;off+24<=0xb0;off+=4){const char *raw;size_t n;
   if(!plain_source((char*)record+off,&raw,&n))continue;
   char decoded[16385];for(size_t k=0;k<n;k++){unsigned char c=raw[k];decoded[k]=c>='A'&&c<='Z'?'A'+(c-'A'+13)%26:c>='a'&&c<='z'?'a'+(c-'a'+13)%26:c;}decoded[n]=0;
   if((n==3&&(!memcmp(raw,"DSF",3)||!memcmp(decoded,"DSF",3)))||
      (n>=28&&(!memcmp(raw,"Distributed [Storage] Facili",28)||!memcmp(decoded,"Distributed [Storage] Facili",28)))){
    live_fixture_lore=dsf;live_fixture_lore_index=i;fprintf(logfile,"DSF-NATIVE-RECORD index=%u field=%u\n",i,off);return 1;
   }
  }
 }
 fprintf(logfile,"DSF-NATIVE-RECORD unavailable=1 records=%u\n",count);return 0;
}
#endif
static DWORD live_fixture_clock(void){typedef DWORD (__cdecl *Clock)(void);return (*(Clock*)va(0xb6143c))();}
static void __attribute__((fastcall)) live_fixture_update(void *self,void *unused){(void)self;(void)unused;}
/* A title-screen fixture has no gameplay selection panel. Do not run its
   terminal animation's optional map/selection callbacks; reveal cells here.
   The production game continues to use its original animation path. */
static void live_fixture_settle(void){
 void *shell=live_shell_fixture_owner;if(!shell)return;uint32_t *rows=(void*)((char*)shell+0x74);
 for(unsigned i=0;i<vector_count(rows);i++){
  uint32_t *row=((void**)rows[0])[i];if(!row)continue;row[24]=3;OwnerRun *owned=owner_find(row);
  for(unsigned x=0;x<row[2];x++){unsigned char *cell=(void*)(row[4]+x*20);if(owned&&x<(unsigned)owned->plan.columns)memcpy(cell+12,owned->colors[x],6);else{cell[12]=0;cell[13]=255;cell[14]=0;}}
 }
 ((NativeVoid)va(0x9bac20))((char*)shell+0x8c);((NativeVoid)va(0x9b8e80))((char*)shell+0x8c);
}
static void live_fixture_send(const char *text,int link){
 NativeStringValue a={{0}},b={{0}};((StringCtor)va(0x9af0f0))(&a,"");((StringCtor)va(0x9af0f0))(&b,text);
 live_shell_capture_command(live_shell_fixture_owner,&a,&b,1,link,0);
 live_replay++;hooked_command(live_shell_fixture_owner,NULL,&a,&b,1,link,0);live_replay--;
 ((NativeVoid)va(0x9af350))(&a);((NativeVoid)va(0x9af350))(&b);
}
static void live_fixture_check(const char *phase,int chinese){
 void *self=live_shell_fixture_owner;LiveShell *s=live_shell_get(self,0);unsigned errors=0,cjk=0,links=0,rows=vector_count((char*)self+0x74),pending=vector_count((char*)self+0x8c);uint32_t *v=(void*)((char*)self+0x74);
 #ifdef CONTENT_NATIVE_PROBE
 errors+=!s||!s->complete||s->count!=2+sizeof content_fixture_sources/sizeof content_fixture_sources[0];
 #else
 errors+=!s||!s->complete||s->count!=2;
 #endif
 errors+=pending!=0;
 uint32_t *lore_vector=(void*)va(0xd35b58),*lore=((void**)lore_vector[0])[live_fixture_lore_index],*expected=(void*)((char*)lore+0xb0);unsigned expected_count=safe_native_vector(expected,32);
 for(unsigned i=0;i<rows;i++){uint32_t *row=((void**)v[0])[i];if(!row)continue;
  unsigned width=row[2];for(unsigned x=0;x<width;x++){uint32_t token=*(uint32_t*)(row[4]+x*20+4);cjk+=token>=320;}
  uint32_t *children=(void*)((char*)row+0x70);unsigned count=safe_native_vector(children,32);
  for(unsigned j=0;j<count;j++){uint32_t *child=((void**)children[0])[j];errors+=links>=expected_count;if(links<expected_count){uint32_t *entry=((void**)expected[0])[links];errors+=child[0x6c/4]!=entry[1];}links++;}
 }
 errors+=links!=expected_count||!links;if(chinese)errors+=!cjk;else errors+=cjk!=0;
 if(chinese&&live_fixture_stage==1)live_fixture_cn_owners=owner_count();
 if(chinese&&live_fixture_stage==3)errors+=owner_count()!=live_fixture_cn_owners;
 live_fixture_links=links;live_fixture_errors+=errors;
 fprintf(logfile,"LIVE-FIXTURE-CHECK phase=%s chinese=%d rows=%u pending=%u cjk=%u links=%u expected-links=%u owners=%u events=%u errors=%u\n",phase,chinese,rows,pending,cjk,links,expected_count,owner_count(),s?s->count:0,errors);fflush(logfile);
}
static void live_fixture_close(void){
 void *self=live_shell_fixture_owner;if(!self)return;LiveShell *s=live_shell_get(self,0);if(s&&live_shell_valid_rows(s))live_shell_clear(s);else live_fixture_errors++;
 live_shell_remove(self);uint32_t *children=(void*)((char*)live_fixture_parent+0x44);unsigned n=vector_count(children),index=n;for(unsigned i=0;i<n;i++)if(((void**)children[0])[i]==self){index=i;break;}
 if(index<n){*(uint32_t*)self=va(0xc2ef14);typedef void (__cdecl *Remove)(void*,unsigned);((Remove)va(0x9cea50))(children,index);}else live_fixture_errors++;
 live_shell_fixture_owner=NULL;queue_fixture=NULL;
 live_fixture_errors+=live_queued_frame!=NULL||live_replacement_old!=NULL||live_replacement_target!=NULL;
 live_fixture_errors+=owner_count()!=live_fixture_owner_baseline;live_fixture_errors+=vector_count(children)!=live_fixture_child_baseline;
 fprintf(logfile,"LIVE-FIXTURE-SUMMARY switches=%u recorded-display-events=2 business-commands-issued=0 links=%u owners=%u baseline-owners=%u children=%u baseline-children=%u bytes=%u errors=%u\n",live_switches,live_fixture_links,owner_count(),live_fixture_owner_baseline,vector_count(children),live_fixture_child_baseline,(unsigned)live_shell_bytes,live_fixture_errors);fflush(logfile);live_fixture_stage=4;
 #ifdef CONTENT_NATIVE_PROBE
 #ifdef LIVE_RAPID_PROBE
 live_fixture_errors+=live_rapid_capture_errors;
 #endif
 fprintf(logfile,"CONTENT-NATIVE-FINAL terminal-errors=%u log-errors=%u\n",live_fixture_errors,log_fixture_errors);fflush(logfile);message_route_dump();ExitProcess(live_fixture_errors||log_fixture_errors?87:0);
 #endif
}
static void live_blit_fixture(void *parent){
 unsigned baseline=vector_count((char*)parent+0x44);void *source=(*(NativeNew*)va(0xb6132c))(0x6c),*target=(*(NativeNew*)va(0xb6132c))(0x6c);
 ((ConsoleCtor)va(0x48c060))(source,parent,20,1,0,0,0,1,-1);((ConsoleCtor)va(0x48c060))(target,parent,20,1,0,0,0,1,-1);
 NativeStringValue text={{0}};((StringCtor)va(0x9af0f0))(&text,"Overview");((TextFn)va(0x4181d0))(source,0,0,&text);((NativeVoid)va(0x9af350))(&text);
 uint32_t *src=source,*dst=target;int point[2]={0,0},prior=live_chinese;uint32_t cn=*(uint32_t*)(src[4]+4);
 live_chinese=0;((LiveBlitFn)va(0x429fe0))(source,target,point,NULL);unsigned errors=cn<320;const char *word="Overview";
 for(unsigned x=0;x<8;x++)errors+=*(uint32_t*)(dst[4]+x*20+4)!=(unsigned char)word[x];errors+=*(uint32_t*)(src[4]+4)!=cn;
 live_chinese=1;((NativeVoid)va(0x417bc0))(target);point[0]=point[1]=0;((LiveBlitFn)va(0x429fe0))(source,target,point,NULL);errors+=*(uint32_t*)(dst[4]+4)!=cn;live_chinese=prior;
 uint32_t *children=(void*)((char*)parent+0x44);void *objects[2]={source,target};for(unsigned k=0;k<2;k++)for(unsigned j=0;j<vector_count(children);j++)if(((void**)children[0])[j]==objects[k]){live_cells_remove(objects[k]);typedef void (__cdecl *Remove)(void*,unsigned);((Remove)va(0x9cea50))(children,j);break;}
 errors+=vector_count(children)!=baseline;live_fixture_errors+=errors;fprintf(logfile,"LIVE-BLIT-FIXTURE english-Overview=1 chinese-restored=1 source-unchanged=1 errors=%u\n",errors);fflush(logfile);
}
static void live_fixture(void *parent){
 if(live_fixture_stage==4||(live_fixture_parent&&parent!=live_fixture_parent))return;
 if(!live_fixture_stage){
  if(GetTickCount()-probe_start<6000||!live_shell_idle())return;
  uint32_t *records=(void*)va(0xd35b58);if(safe_native_vector(records,8192)<=255){fprintf(logfile,"LIVE-FIXTURE-SKIP lore-table-unavailable\n");live_fixture_stage=4;return;}
  #ifdef DSF_RESOURCE_PROBE
  if(!live_fixture_select_dsf()){live_fixture_errors++;live_fixture_stage=4;fflush(logfile);ExitProcess(88);return;}
  #endif
  if(!live_fixture_context_install()){fprintf(logfile,"LIVE-FIXTURE-ERROR mock-context-callsite-validation\n");live_fixture_errors++;live_fixture_stage=4;return;}
  #ifdef CONTENT_NATIVE_PROBE
  log_content_fixture(parent);
  #endif
  live_blit_fixture(parent);live_fixture_parent=parent;live_fixture_born=GetTickCount();live_fixture_owner_baseline=owner_count();live_fixture_child_baseline=vector_count((char*)parent+0x44);
  void *shell=(*(NativeNew*)va(0xb6132c))(0x128);if(!shell){live_fixture_stage=4;return;}memset(shell,0,0x128);
  ((ConsoleCtor)va(0x48c060))(shell,parent,60,10,2,12,0,0,20);live_shell_fixture_owner=shell;queue_fixture=shell;
  memcpy(live_fixture_vtable,(void*)va(0xc36224),sizeof live_fixture_vtable);live_fixture_vtable[6]=(uintptr_t)live_fixture_update;*(void**)shell=live_fixture_vtable;
  ((NativeVoid)va(0x9b8e80))((char*)shell+0x74);((NativeVoid)va(0x9b8e80))((char*)shell+0x8c);((uint32_t*)shell)[0x70/4]=0;((uint32_t*)shell)[24]=3;((uint32_t*)shell)[0x9c/4]=live_fixture_clock();
  void *scratch=(*(NativeNew*)va(0xb6132c))(0x128);if(!scratch){live_fixture_errors++;live_fixture_close();return;}memset(scratch,0,0x128);((ConsoleCtor)va(0x48c060))(scratch,shell,46,200,0,0,0,1,-1);*(void**)((char*)shell+0x88)=scratch;
  #ifdef RECORD_IDENTITY_PROBE
 record_identity_fixture(parent);
 #endif
 live_chinese=1;live_native_english=0;live_fixture_stage=1;live_fixture_send("Current Alert Level: Low",-1);live_fixture_send(live_fixture_lore,(int)live_fixture_lore_index);
  #ifdef CONTENT_NATIVE_PROBE
  for(unsigned i=0;i<sizeof content_fixture_sources/sizeof content_fixture_sources[0];i++)live_fixture_send(content_fixture_sources[i],-1);
  #endif
  fprintf(logfile,"LIVE-FIXTURE-CREATE shell=%p source-events=2\n",shell);fflush(logfile);return;
 }
 live_fixture_settle();DWORD age=GetTickCount()-live_fixture_born;if(age>20000){live_fixture_errors++;fprintf(logfile,"LIVE-FIXTURE-TIMEOUT stage=%u\n",live_fixture_stage);live_fixture_close();return;}
 #ifdef LIVE_RAPID_PROBE
 static int queued_reverse;
 if(live_fixture_stage==2&&live_phase&&!queued_reverse&&GetTickCount()-live_phase_start>=400){queued_reverse=1;live_request(1);fprintf(logfile,"RAPID-REQUEST queued-during-animation=1\n");fflush(logfile);}
 #endif
 if(!live_shell_idle()||live_phase)return;
 if(live_fixture_stage==1&&age>=1000){live_fixture_check("initial-zh",1);live_fixture_stage=2;live_request(0);}
 #ifdef LIVE_RAPID_PROBE
 else if(live_fixture_stage==2&&live_chinese&&live_pending<0&&queued_reverse){live_fixture_check("queued-return-zh",1);live_fixture_errors+=owner_count()!=live_fixture_cn_owners;live_fixture_stage=3;live_request(0);}
 else if(live_fixture_stage==3&&!live_chinese&&live_pending<0){live_fixture_check("rapid-then-en",0);live_fixture_stage=5;live_request(1);}
 else if(live_fixture_stage==5&&live_chinese&&live_pending<0){live_fixture_check("rapid-final-zh",1);live_fixture_close();}
 #else
 else if(live_fixture_stage==2&&!live_chinese&&live_pending<0){live_fixture_check("switched-en",0);live_fixture_stage=3;live_request(1);}
 else if(live_fixture_stage==3&&live_chinese&&live_pending<0){live_fixture_check("returned-zh",1);live_fixture_close();}
 #endif
}
#endif

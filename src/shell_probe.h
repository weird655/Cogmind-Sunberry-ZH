/* Unicode run maps are owned per actual CShellText, never stored in cell
   padding. Kind-1 text and preplanned kind-2 failures use native animation paths. */
typedef struct {
 void *owner,*main_anim;ZhPlan plan;unsigned generation,id;int synthetic,armed,shown,mode;
 int col_run[512],heads[512],widths[512];uint32_t tokens[512];unsigned char colors[512][6];
 uint32_t fallback_glyph[512];unsigned char fallback_colors[512][6];
 unsigned logical,sounds;DWORD born;
} OwnerRun;
#include "owner_index.h"
#define OWNER_LIMIT 1024u
static OwnerIndex owner_index;
static OwnerRun owners[OWNER_LIMIT];static unsigned owner_generation,owner_serial,owner_active_count;
static void *constructing_owner,*test_owner;
static int constructing_kind,test_width,test_creating;
static DWORD probe_start;
static _Thread_local void *advancing_animation,*sound_animation;
static _Thread_local unsigned command_scope;
static _Thread_local ZhPlan *queue_pending_row;
static _Thread_local void *queue_pending_parent;
static _Thread_local size_t queue_pending_rule;
static void *queue_fixture;
static unsigned owner_destructors;
#ifdef QUEUE_EXPERIMENT
static unsigned queue_cycle;
#endif
typedef void* (__cdecl *NativeNew)(size_t);
typedef void* (__attribute__((thiscall)) *StringCtor)(void*,const char*);
typedef void (__attribute__((thiscall)) *NativeVoid)(void*);
typedef void (__attribute__((thiscall)) *ShellAnimate)(void*,int);
typedef unsigned char (__attribute__((thiscall)) *AdvanceFn)(void*);
typedef void* (__attribute__((thiscall)) *ConsoleCtor)(void*,void*,int,int,int,int,int,int,int);
typedef void (__attribute__((thiscall)) *FinalizeFn)(void*,const int*);
typedef unsigned char (__attribute__((thiscall)) *InitFn)(void*,void*,void*,const int*,const int*,const int*,const int*,int);
typedef unsigned char (__attribute__((thiscall)) *PathStepFn)(void*,int*,int*);
typedef void (__attribute__((thiscall)) *SoundActionFn)(void*,void*);
typedef int (__cdecl *PlaySoundFn)(void*,int,int,int,int);
typedef unsigned char (__attribute__((thiscall)) *EventsFn)(void*,int,const int*);
typedef void* (__attribute__((thiscall)) *DeleteFn)(void*,unsigned);
static AdvanceFn original_advance;static ConsoleCtor original_console_ctor;static NativeVoid original_compose;
static FinalizeFn original_finalize;static InitFn original_init;static PathStepFn original_path_step;
static SoundActionFn original_sound_action;static PlaySoundFn original_play_sound;static EventsFn original_events;
static ShellAnimate original_animate;static DeleteFn original_delete;
static NativeVoid original_owner_compose;
static OwnerRun *owner_find(void *p){
 return (OwnerRun*)owner_index_get(&owner_index,p);
}
#include "animation_owner_lookup.h"
static unsigned char *owner_cell(OwnerRun *o,int x){return (void*)(((uint32_t*)o->owner)[4]+x*20);}
static void owner_remove(void *p){
 OwnerRun *o=owner_find(p);if(!o)return;owner_index_remove(&owner_index,p);fprintf(logfile,"OWNER-REMOVE id=%u generation=%u\n",o->id,o->generation);zh_plan_free(&o->plan);memset(o,0,sizeof *o);owner_active_count--;
}
static void* __attribute__((fastcall)) hooked_delete(void *self,void *unused,unsigned flags){(void)unused;live_cells_remove(self);if(owner_find(self))owner_destructors++;owner_remove(self);return original_delete(self,flags);}
static void* __attribute__((fastcall)) hooked_console_ctor(void *self,void *unused,void *parent,int w,int h,int x,int y,int font,int hidden,int depth){
 (void)unused;live_cells_remove(self);if(test_creating&&self==test_owner){depth=20;w=test_width;}
 if(queue_pending_row&&(constructing_kind==1||constructing_kind==2||constructing_kind==4)&&parent==queue_pending_parent&&font==0){w=queue_pending_row->columns;if(parent==queue_fixture)depth=21;}
 return original_console_ctor(self,parent,w,h,x,y,font,hidden,depth);
}
static int plain_source(void *s,const char **out,size_t *length){
 if(!readable(s,24))return 0;uint32_t *v=s;size_t n=v[4];const char *p=v[5]<16?s:*(void**)s;
 if(n>16384||!readable(p,n))return 0;
 while(n&&*p=='`'){const char *end=memchr(p+1,'`',n-1);if(!end||end-p>32)return 0;size_t used=end-p+1;p+=used;n-=used;}
 while(n>=3&&!memcmp(p+n-3,"`x`",3))n-=3;
 if(memchr(p,'`',n)){char validated[16385];if(terminal_normalize(p,n,validated,sizeof validated)!=1)return 0;}*out=p;*length=n;return n>0;
}
/* Preserve native formatting and embedded link markers in diagnostics. */
static void audit_native_string(const char *label,void *s){
 if(!readable(s,24)){fprintf(logfile,"%s unreadable=1\n",label);return;}
 uint32_t *v=s;size_t n=v[4];const char *raw=v[5]<16?s:*(void**)s;
 if(n>16384||!readable(raw,n)){fprintf(logfile,"%s invalid=1\n",label);return;}
 fprintf(logfile,"%s bytes=%u raw=",label,(unsigned)n);
 for(size_t i=0;i<n;i++){unsigned char c=raw[i];if(c=='\n')fputs("\\n",logfile);else if(c=='\r')fputs("\\r",logfile);else if(c=='\\')fputs("\\\\",logfile);else if(c<32)fprintf(logfile,"\\x%02x",c);else fputc(c,logfile);}fputc('\n',logfile);
}
static int write_owned_translation(void *self,int x,int y,void *s){
 if(live_native_english||!live_chinese)return 0;
 if(constructing_owner!=self||(constructing_kind!=1&&!((constructing_kind==2||constructing_kind==4)&&(queue_pending_row||test_creating)))||x||y||!readable(self,24))return 0;
 uint32_t *native=self;int cols=native[2];if(native[5]||native[3]!=1||cols<1||cols>512)return 0;
 const char *source;size_t bytes,rule=0;char translated[16384]="[preplanned native queue row]";int match;ZhPlan plan={0};
 if(queue_pending_row&&native[1]==(uint32_t)queue_pending_parent){plan=*queue_pending_row;memset(queue_pending_row,0,sizeof *queue_pending_row);rule=queue_pending_rule;match=3;}
 else {
  if(command_scope)return 0;
  if(!plain_source(s,&source,&bytes))return 0;
  match=message_resolve_entry(TEXT_ENTRY_TERMINAL_SHELL,source,bytes,translated,sizeof translated,&rule);if(match<=0)return 0;
  if(message_plan_entry(TEXT_ENTRY_TERMINAL_SHELL,translated,strlen(translated),cols,1,&plan)){fprintf(logfile,"TRANSLATION-NOFIT bytes=%u columns=%d\n",(unsigned)strlen(translated),cols);return 0;}
 }
 if(!extend_demo_fonts()){zh_plan_free(&plan);return 0;}
 OwnerRun *o=NULL;for(unsigned i=0;i<OWNER_LIMIT;i++)if(!owners[i].owner){o=&owners[i];break;}if(!o){zh_plan_free(&plan);return 0;}
 original_text(self,0,0,s);unsigned char base_colors[6];memcpy(base_colors,(void*)(native[4]+12),6);
 o->owner=self;if(!owner_index_put(&owner_index,self,o)){o->owner=NULL;zh_plan_free(&plan);return 0;}owner_active_count++;o->plan=plan;o->generation=++owner_generation;o->id=++owner_serial;o->synthetic=test_creating||native[1]==(uint32_t)queue_fixture;o->born=GetTickCount();o->mode=1;
 for(int col=0;col<cols;col++){o->fallback_glyph[col]=*(uint32_t*)(owner_cell(o,col)+4);memcpy(o->fallback_colors[col],owner_cell(o,col)+12,6);}
 for(int col=0;col<cols;col++){((PutCell)va(0x417f50))(self,col,0,32);o->col_run[col]=-1;o->tokens[col]=32;memcpy(owner_cell(o,col)+12,base_colors,6);}
 for(size_t i=0;i<plan.count;i++){
  ZhRun *run=&plan.runs[i];unsigned index=o->logical++;o->heads[index]=run->x;o->widths[index]=run->width;
  if(run->style){
   char styled[160];snprintf(styled,sizeof styled,"%s@`x`",plan.styles[run->style]);unsigned char ns[28]={0};
   ((StringCtor)va(0x9af0f0))(ns,styled);original_text(self,run->x,0,ns);((NativeVoid)va(0x9af350))(ns);
  }
  for(int half=0;half<run->width;half++){
   int col=run->x+half;o->col_run[col]=index;o->tokens[col]=run->token[half];((PutCell)va(0x417f50))(self,col,0,run->token[half]);
   if(half)memcpy(owner_cell(o,col)+12,owner_cell(o,run->x)+12,6);
  }
 }
 for(int col=0;col<cols;col++)if(o->col_run[col]<0){unsigned i=o->logical++;o->heads[i]=col;o->widths[i]=1;o->col_run[col]=i;}
 for(int col=0;col<cols;col++)memcpy(o->colors[col],owner_cell(o,col)+12,6);
 fprintf(logfile,"TRANSLATION owner=%p id=%u kind=%s rule=%u logical=%u columns=%d text=%s\n",self,o->id,match==3?"queue":match==1?"exact":"template",(unsigned)rule,o->logical,cols,translated);fflush(logfile);return 1;
}
static void mirror_run(void *anim,int x,int y){
 OwnerRun *o=animation_owner(anim);if(!o||y||x<0||x>=(int)o->plan.columns)return;int run=o->col_run[x],head=o->heads[run];
 if(o->widths[run]==2)memcpy(owner_cell(o,head+1)+12,owner_cell(o,head)+12,6);
 for(int half=0;half<o->widths[run];half++)if(*(uint32_t*)(owner_cell(o,head+half)+4)!=o->tokens[head+half]){
  fprintf(logfile,"OWNER-GLYPH-MISMATCH id=%u x=%d\n",o->id,head+half);((PutCell)va(0x417f50))(o->owner,head+half,0,o->tokens[head+half]);
 }
}
static void __attribute__((fastcall)) hooked_animate(void *self,void *unused,int flag){
 (void)unused;OwnerRun *o=owner_find(self);if(o)o->mode=((uint32_t*)self)[0x6c/4]==4?0:!!flag;
 original_animate(self,flag);if(!o)return;
 if(!o->mode){fprintf(logfile,"OWNER-MULTI id=%u elapsed=%lu columns=%d logical=%u\n",o->id,GetTickCount()-probe_start,o->plan.columns,o->logical);fflush(logfile);return;}
 if(!o->main_anim){
  fprintf(logfile,"OWNER-FALLBACK-PATH id=%u\n",o->id);
  for(int col=0;col<o->plan.columns;col++){((PutCell)va(0x417f50))(self,col,0,o->fallback_glyph[col]);memcpy(owner_cell(o,col)+12,o->fallback_colors[col],6);}owner_remove(self);return;
 }
 for(int col=0;col<o->plan.columns;col++)memset(owner_cell(o,col)+12,0,6);o->armed=1;
 fprintf(logfile,"OWNER-ARM id=%u logical=%u columns=%d\n",o->id,o->logical,o->plan.columns);
}
static unsigned char __attribute__((fastcall)) hooked_init(void *self,void *unused,void *assoc,void *config,const int *start,const int *frac_start,const int *end,const int *frac_end,int step){
 (void)unused;unsigned char r=original_init(self,assoc,config,start,frac_start,end,frac_end,step);OwnerRun *o=animation_owner(self);
 uint32_t *audit_anim=self;if(readable(audit_anim,0x80)&&readable((void*)audit_anim[1],24)&&readable(config,0xb8)){
  void *raw_owner=*(void**)audit_anim[1];if(readable(raw_owner,0x80)&&*(uint32_t*)raw_owner==va(0xc36328)){
   uint32_t *cfg=config;fprintf(logfile,"REAL-ANIM owner=%p kind=%u config=%p glyph=%u final-glyph=%u fg=%u bg=%u start=%d,%d end=%d,%d interval=%g\n",raw_owner,((uint32_t*)raw_owner)[0x6c/4],config,cfg[0x48/4],cfg[0xb4/4],cfg[0x60/4],cfg[0x74/4],start?start[0]:-999,start?start[1]:-999,end?end[0]:-999,end?end[1]:-999,*(float*)((char*)self+0x30));fflush(logfile);
  }
 }

 if(o&&!o->mode){uint32_t *cfg=config;fprintf(logfile,"MULTI-INIT id=%u start=%d,%d end=%d,%d interval=%g glyph=%u final-glyph=%u fg=%u bg=%u\n",o->id,start?start[0]:-999,start?start[1]:-999,end?end[0]:-999,end?end[1]:-999,*(float*)((char*)self+0x30),cfg[0x48/4],cfg[0xb4/4],cfg[0x60/4],cfg[0x74/4]);if(cfg[0x48/4]||cfg[0xb4/4]){fprintf(logfile,"MULTI-UNSUPPORTED-GLYPH-CONFIG\n");fflush(logfile);ExitProcess(89);}}
 if(o&&o->mode&&start&&end&&frac_start&&frac_end&&!start[1]&&!end[1]&&start[0]==0&&end[0]==o->plan.columns-1&&*(float*)((char*)self+0x30)>0){
  uint32_t *a=self,*cfg=config;int ls[2]={o->col_run[start[0]],0},le[2]={o->col_run[end[0]],0};
  typedef void (__attribute__((thiscall)) *PathInit)(void*,const int*,const int*,const int*,const int*,int);
  ((PathInit)va(0x4103b0))((char*)self+0x38,ls,frac_start,le,frac_end,9);a[26]=o->heads[le[0]];a[27]=0;
  if(cfg[10]==3)a[3]=a[2]+(uint32_t)((le[0]-ls[0])*(*(float*)(a+12)));o->main_anim=self;
  fprintf(logfile,"OWNER-PATH id=%u logical=%d->%d physical-end=%u interval=%g duration=%u\n",o->id,ls[0],le[0],a[26],*(float*)(a+12),a[3]-a[2]);
 }
 return r;
}
static unsigned char __attribute__((fastcall)) hooked_path_step(void *self,void *unused,int *whole,int *fraction){
 (void)unused;unsigned char r=original_path_step(self,whole,fraction);OwnerRun *o=animation_owner(advancing_animation);
 if(o&&o->main_anim==advancing_animation&&self==(char*)advancing_animation+0x38){
  int logical=whole[0];whole[0]=logical<0?logical:logical<(int)o->logical?o->heads[logical]:o->plan.columns+logical-o->logical;
  if(o->armed&&logical>=o->shown&&logical<(int)o->logical){
   for(int i=o->shown;i<=logical;i++)for(int half=0;half<o->widths[i];half++){int col=o->heads[i]+half;memcpy(owner_cell(o,col)+12,o->colors[col],6);}
   o->shown=logical+1;fprintf(logfile,"OWNER-REVEAL id=%u t=%lu logical=%d head=%d shown=%d\n",o->id,GetTickCount()-o->born,logical,whole[0],o->shown);
  }
 }
 return r;
}
static unsigned char __attribute__((fastcall)) hooked_advance(void *self,void *unused){
 (void)unused;void *saved=advancing_animation;OwnerRun *o=animation_owner(self);advancing_animation=o?self:NULL;
 #ifdef INJECT_LAG
 static int stalled;if(o&&o->synthetic&&!stalled&&((o->mode&&self==o->main_anim&&o->shown>=2&&o->shown<(int)o->logical)||(!o->mode&&o->id>=4&&GetTickCount()-o->born>=100))){stalled=1;fputs("INJECTED-STALL milliseconds=100\n",logfile);Sleep(100);}
 #endif
 unsigned char r=original_advance(self);advancing_animation=saved;return r;
}
static void __attribute__((fastcall)) hooked_compose(void *self,void *unused){(void)unused;original_compose(self);OwnerRun *o=animation_owner(self);if(o){uint32_t *a=self,*assoc=(void*)a[1];mirror_run(self,a[4]+assoc[3],a[5]+assoc[4]);}}
static void __attribute__((fastcall)) hooked_finalize(void *self,void *unused,const int *xy){(void)unused;original_finalize(self,xy);OwnerRun *o=animation_owner(self);if(o){uint32_t *assoc=(void*)((uint32_t*)self)[1];mirror_run(self,xy[0]+assoc[3],xy[1]+assoc[4]);}}
static void __attribute__((fastcall)) hooked_owner_compose(void *self,void *unused){
 (void)unused;original_owner_compose(self);OwnerRun *o=owner_find(self);if(!o||o->mode)return;
 for(unsigned i=0;i<o->logical;i++)if(o->widths[i]==2)memcpy(owner_cell(o,o->heads[i]+1)+12,owner_cell(o,o->heads[i])+12,6);
 for(int col=0;col<o->plan.columns;col++)if(*(uint32_t*)(owner_cell(o,col)+4)!=o->tokens[col])fprintf(logfile,"MULTI-GLYPH-MISMATCH id=%u column=%d\n",o->id,col);
}
static void __attribute__((fastcall)) hooked_sound_action(void *self,void *unused,void *action){(void)unused;void *saved=sound_animation;sound_animation=animation_owner(self)?self:NULL;original_sound_action(self,action);sound_animation=saved;}
static int __cdecl hooked_play_sound(void *def,int channel,int volume,int p4,int p5){int r=original_play_sound(def,channel,volume,p4,p5);OwnerRun *o=animation_owner(sound_animation);if(o){o->sounds++;fprintf(logfile,"OWNER-SOUND id=%u result=%d count=%u\n",o->id,r,o->sounds);}return r;}
static unsigned char __attribute__((fastcall)) hooked_events(void *self,void *unused,int phase,const int *xy){(void)unused;OwnerRun *o=animation_owner(self);if(o&&o->main_anim==self&&(phase==3||phase==4))fprintf(logfile,"OWNER-EVENT id=%u phase=%d x=%d\n",o->id,phase,xy[0]);return original_events(self,phase,xy);}
#ifdef EXPERIMENT_INJECT
static void create_paragraph_probe(void *parent){
 const char *source="Aside from the options menu, there are many more interface features and behavior you can tweak in Cogmind's advanced.cfg file, the functionality of which is described in the manual under Advanced UI: Advanced Options.";
 char target[16384];size_t rule;ZhPlan plan={0};if(message_resolve_entry(TEXT_ENTRY_TERMINAL_SHELL,source,strlen(source),target,sizeof target,&rule)!=1||message_plan_entry(TEXT_ENTRY_TERMINAL_SHELL,target,strlen(target),60,8,&plan))return;
 void *object=(*(NativeNew*)va(0xb6132c))(0x80);if(!object){zh_plan_free(&plan);return;}
 unsigned char carrier[28]={0};test_owner=object;test_creating=1;test_width=60;
 ((StringCtor)va(0x9af0f0))(carrier," ");((ShellFn)va(0x4b0b30))(object,(int)(uintptr_t)parent,carrier,1);((NativeVoid)va(0x9af350))(carrier);test_creating=0;test_owner=NULL;
 typedef void (__attribute__((thiscall)) *Resize)(void*,int,int);((Resize)va(0x4289a0))(object,60,plan.used_rows);
 typedef void (__attribute__((thiscall)) *Position)(void*,const int*);int point[2]={2,22};((Position)va(0x4289e0))(object,point);
 typedef void (__attribute__((thiscall)) *SetColor)(void*,unsigned);((SetColor)va(0x417b00))(object,0x00ff00);
 unsigned pair_errors=0,glyph_errors=0;
 for(size_t i=0;i<plan.count;i++){
  ZhRun *r=&plan.runs[i];char styled[160];snprintf(styled,sizeof styled,"%s@`x`",plan.styles[r->style]);unsigned char ns[28]={0},color[6];
  ((StringCtor)va(0x9af0f0))(ns,styled);original_text(object,r->x,r->y,ns);((NativeVoid)va(0x9af350))(ns);
  unsigned char *cell=(void*)(((uint32_t*)object)[4]+(r->x*plan.used_rows+r->y)*20);memcpy(color,cell+12,6);
  for(int half=0;half<r->width;half++){unsigned char *part=cell+half*plan.used_rows*20;((PutCell)va(0x417f50))(object,r->x+half,r->y,r->token[half]);memcpy(part+12,color,6);glyph_errors+=*(uint32_t*)(part+4)!=r->token[half];}
  if(r->width==2)pair_errors+=memcmp(cell+12,cell+plan.used_rows*20+12,6)!=0;
 }
 fprintf(logfile,"PARAGRAPH-NATIVE rule=%u runs=%u columns=60 rows=%d pair=%u glyph=%u\n",(unsigned)rule,(unsigned)plan.count,plan.used_rows,pair_errors,glyph_errors);zh_plan_free(&plan);
}
#endif
#ifdef META_RESOURCE_PROBE
static void meta_fixture_tick(void *parent);
#endif
static void create_shell_probe(void *parent){
 #ifdef META_RESOURCE_PROBE
 meta_fixture_tick(parent);
 #endif
 #ifdef FAILURE_RESOURCE_PROBE
 static int failure_created;if(failure_created||GetTickCount()-probe_start<5000)return;failure_created=1;
 for(unsigned i=0;i<2;i++){
  const char *query="Failed 40% (near success, encountered trap node).";char target[16384];size_t rule;
  if(message_resolve_entry(TEXT_ENTRY_TERMINAL_SHELL,query,strlen(query),target,sizeof target,&rule)<=0)continue;
  ZhPlan plan={0};if(message_plan_entry(TEXT_ENTRY_TERMINAL_SHELL,target,strlen(target),46,1,&plan))continue;test_width=0;
  for(size_t j=0;j<plan.count;j++)if(plan.runs[j].x+plan.runs[j].width>test_width)test_width=plan.runs[j].x+plan.runs[j].width;zh_plan_free(&plan);
  void *object=(*(NativeNew*)va(0xb6132c))(0x80);test_owner=object;test_creating=1;
  unsigned char s[28]={0};((StringCtor)va(0x9af0f0))(s,query);((ShellFn)va(0x4b0b30))(object,(int)(uintptr_t)parent,s,2);((NativeVoid)va(0x9af350))(s);
  int point[2]={2,12+(int)i*2};typedef void (__attribute__((thiscall)) *Position)(void*,const int*);((Position)va(0x4289e0))(object,point);
  ((ShellAnimate)va(0x90b850))(object,i?0:1);test_creating=0;test_owner=NULL;
 }
 #endif

 #ifdef EXPERIMENT_INJECT
 static int created;if(created||GetTickCount()-probe_start<5000)return;created=1;
 const char *samples[]={"Connecting with Door Terminal...","Connecting with QA-17/TEST...","Deep Access #37","`f0`Terminal is limited access only.`x`","Connecting with Door Terminal..."};
 for(unsigned i=0;i<5;i++){
  const char *query=samples[i];size_t len=strlen(query);if(i==3){query+=4;len-=7;}
  char target[16384];size_t rule;if(message_resolve_entry(TEXT_ENTRY_TERMINAL_SHELL,query,len,target,sizeof target,&rule)<=0)continue;
  ZhPlan plan={0};if(message_plan_entry(TEXT_ENTRY_TERMINAL_SHELL,target,strlen(target),44,1,&plan))continue;test_width=0;for(size_t j=0;j<plan.count;j++)if(plan.runs[j].x+plan.runs[j].width>test_width)test_width=plan.runs[j].x+plan.runs[j].width;zh_plan_free(&plan);
  if(i==4)test_width=44;
  void *object=(*(NativeNew*)va(0xb6132c))(0x80);if(!object)continue;test_owner=object;test_creating=1;
  unsigned char s[28]={0};((StringCtor)va(0x9af0f0))(s,samples[i]);((ShellFn)va(0x4b0b30))(object,(int)(uintptr_t)parent,s,1);((NativeVoid)va(0x9af350))(s);
  int point[2]={2,12+(int)i*2};typedef void (__attribute__((thiscall)) *Position)(void*,const int*);((Position)va(0x4289e0))(object,point);((ShellAnimate)va(0x90b850))(object,i==4?0:1);test_creating=0;test_owner=NULL;
 }
 create_paragraph_probe(parent);
 #else
 (void)parent;
 #endif
}
static void check_shell_completion(void){
 static unsigned summarized[OWNER_LIMIT];for(unsigned i=0;i<OWNER_LIMIT;i++)if(owners[i].owner&&summarized[i]!=owners[i].generation&&GetTickCount()-owners[i].born>3000){
  OwnerRun *o=&owners[i];uint32_t *native=o->owner,*manager=*(void**)((char*)native+0x64);
  unsigned glyph_errors=0,pair_errors=0;
  for(int col=0;col<o->plan.columns;col++)glyph_errors+=*(uint32_t*)(owner_cell(o,col)+4)!=o->tokens[col];
  for(unsigned j=0;j<o->logical;j++)if(o->widths[j]==2)pair_errors+=memcmp(owner_cell(o,o->heads[j])+12,owner_cell(o,o->heads[j]+1)+12,6)!=0;
  fprintf(logfile,"OWNER-SUMMARY id=%u shown=%d logical=%u state=%u active=%u sounds=%u kind=%u mode=%d synthetic=%d glyph=%u pair=%u\n",o->id,o->shown,o->logical,native[24],(manager[6]-manager[5])/4,o->sounds,native[0x6c/4],o->mode,o->synthetic,glyph_errors,pair_errors);summarized[i]=o->generation;fflush(logfile);
 }
}
#ifdef CAPTURE_DIAGNOSTICS
static void record_shell_frame(void *surface){
 static unsigned frame;
 #ifdef SERIAL_EXPERIMENT
 if(frame>=150||owner_serial<6)return;
 #else
 if(frame>=60)return;
 #endif
 int any=0;
 #ifdef QUEUE_EXPERIMENT
 if(queue_cycle!=4)return;
 #endif
 for(unsigned i=0;i<OWNER_LIMIT;i++){OwnerRun *o=&owners[i];
 #ifdef SERIAL_EXPERIMENT
 const DWORD frame_age_limit=3000;
 #else
 const DWORD frame_age_limit=1800;
 #endif
 if(!o->owner||GetTickCount()-o->born>frame_age_limit)continue;any=1;unsigned pair=0,glyph=0;
  for(unsigned r=0;r<o->logical;r++)if(o->widths[r]==2)pair+=memcmp(owner_cell(o,o->heads[r])+12,owner_cell(o,o->heads[r]+1)+12,6)!=0;
  for(int col=0;col<o->plan.columns;col++)glyph+=*(uint32_t*)(owner_cell(o,col)+4)!=o->tokens[col];
  fprintf(logfile,"OWNER-FRAME frame=%u id=%u t=%lu shown=%d logical=%u pair=%u glyph=%u\n",frame,o->id,GetTickCount()-o->born,o->shown,o->logical,pair,glyph);
 }
 if(!any)return;
 typedef void* (__cdecl *RW)(const char*,const char*);typedef int (__cdecl *Save)(void*,void*,int);HMODULE sdl=GetModuleHandleW(L"SDL.dll");RW rw=(RW)GetProcAddress(sdl,"SDL_RWFromFile");Save save=(Save)GetProcAddress(sdl,"SDL_SaveBMP_RW");
 if(frame%10==0&&rw&&save){char name[64];snprintf(name,sizeof name,"words-frame-%03u.bmp",frame);void *f=rw(name,"wb");if(f)save(surface,f,1);}frame++;fflush(logfile);
}
#endif

/* General AddShellLine only. Command output 90D550 has a separate serial queue. */
typedef void (__attribute__((thiscall)) *AddLineFn)(void*,void*,int,int,int);
static AddLineFn original_add_line;
static unsigned queue_calls,queue_rows,queue_fallbacks;
static unsigned owner_count(void){return owner_active_count;}
static unsigned vector_count(void *v){uint32_t *p=v;return (p[1]-p[0])/4;}
typedef struct {unsigned char raw[28];} NativeStringValue;
typedef void (__attribute__((thiscall)) *MetaFn)(void*,NativeStringValue);
static MetaFn original_meta;
typedef struct {void *shell;ZhPlan *rows;int count,emitted;size_t rule;} MetaContext;
static _Thread_local MetaContext *meta_context;
static void __attribute__((fastcall)) hooked_add_line(void *self,void *unused,void *s,int kind,int insert,int sound){
 (void)unused;live_shell_capture_add(self,s,kind,insert,sound);if(live_native_english||!live_chinese){original_add_line(self,s,kind,insert,live_replay?0:sound);return;}
 fprintf(logfile,"REAL-ADD kind=%d insert=%d sound=%d scope=%u\n",kind,insert,sound,command_scope);audit_native_string("REAL-ADD-TEXT",s);
 MetaContext *meta=meta_context;
 if(meta&&self==meta->shell&&kind==4){
  if(!meta->emitted++){
   ZhPlan *prior=queue_pending_row;void *parent=queue_pending_parent;size_t rule=queue_pending_rule;
   for(int row=0;row<meta->count;row++){
    queue_pending_row=&meta->rows[row];queue_pending_parent=self;queue_pending_rule=meta->rule;
    original_add_line(self,s,4,insert,sound);
    if(meta->rows[row].runs)fprintf(logfile,"META-UNCONSUMED row=%d\n",row);
   }
   queue_pending_row=prior;queue_pending_parent=parent;queue_pending_rule=rule;
  }
  return;
 }

 if(command_scope&&!queue_pending_row){original_add_line(self,s,kind,insert,sound);return;}
 if(kind!=1||queue_pending_row||!readable(self,0x88)||(*(uint32_t*)self!=va(0xc36224)&&self!=queue_fixture)){original_add_line(self,s,kind,insert,sound);return;}
 const char *source;size_t bytes,rule;char target[16384];ZhPlan full={0};
 int columns=self==queue_fixture?(int)((uint32_t*)self)[2]-4:*(int*)va(0xd316bc)-4;
 if(!plain_source(s,&source,&bytes)||columns<2||columns>512||message_resolve_entry(TEXT_ENTRY_TERMINAL_QUEUE,source,bytes,target,sizeof target,&rule)<=0||message_plan_entry(TEXT_ENTRY_TERMINAL_QUEUE,target,strlen(target),columns,128,&full)){
  queue_fallbacks++;original_add_line(self,s,kind,insert,sound);return;
 }
 int rows=full.used_rows;ZhPlan *plans=calloc(rows?rows:1,sizeof *plans);int ready=plans&&rows>0&&rows<=(int)(OWNER_LIMIT-owner_count());
 for(int i=0;ready&&i<rows;i++)if(zh_plan_row(&full,i,&plans[i]))ready=0;
 if(ready&&!extend_demo_fonts())ready=0;
 if(!ready){if(plans){for(int i=0;i<rows;i++)zh_plan_free(&plans[i]);free(plans);}zh_plan_free(&full);queue_fallbacks++;original_add_line(self,s,kind,insert,sound);return;}
 unsigned before=vector_count((char*)self+0x74);int effective_insert=insert&&before>0;
 ZhPlan *saved_plan=queue_pending_row;void *saved_parent=queue_pending_parent;size_t saved_rule=queue_pending_rule;
 for(int row=0;row<rows;row++){
  queue_pending_row=&plans[row];queue_pending_parent=self;queue_pending_rule=rule;
  original_add_line(self,s,kind,effective_insert,row==0?sound:0);
  if(plans[row].runs)fprintf(logfile,"QUEUE-ROW-UNCONSUMED row=%d\n",row);
 }
 queue_pending_row=saved_plan;queue_pending_parent=saved_parent;queue_pending_rule=saved_rule;
 unsigned after=vector_count((char*)self+0x74);queue_calls++;queue_rows+=rows;
 fprintf(logfile,"QUEUE-TRANSLATE call=%u scope=%s rule=%u rows=%d columns=%d before=%u after=%u insert=%d effective-insert=%d sound=%d\n",queue_calls,self==queue_fixture?"synthetic-native-algorithm":"real-CShell",(unsigned)rule,rows,columns,before,after,insert,effective_insert,sound);fflush(logfile);
 for(int i=0;i<rows;i++)zh_plan_free(&plans[i]);free(plans);zh_plan_free(&full);
}
static void __attribute__((fastcall)) hooked_meta(void *self,void *unused,NativeStringValue text){
 (void)unused;live_shell_capture_meta(self,&text);if(live_native_english||!live_chinese){original_meta(self,text);return;}fprintf(logfile,"META-BEGIN shell=%p\n",self);audit_native_string("META-RAW",&text);fflush(logfile);
 MetaContext ctx={0};ctx.shell=self;ZhPlan full={0};const char *source;size_t bytes;char translated[16384],wrapped[16384];
 int eligible=!meta_context&&readable(self,0x88)&&(*(uint32_t*)self==va(0xc36224)||self==queue_fixture)&&vector_count((char*)self+0x74)>0;
 if(eligible&&plain_source(&text,&source,&bytes)&&message_resolve_entry(TEXT_ENTRY_TERMINAL_QUEUE,source,bytes,translated,sizeof translated,&ctx.rule)>0){
  int n=snprintf(wrapped,sizeof wrapped,"[%s]",translated);int columns=*(int*)va(0xd316bc)-4;if(columns>46)columns=46;
  if(n>0&&(size_t)n<sizeof wrapped&&columns>=2&&message_plan_entry(TEXT_ENTRY_TERMINAL_QUEUE,wrapped,(size_t)n,columns,128,&full)==0){
   ctx.count=full.used_rows;ctx.rows=calloc(ctx.count?ctx.count:1,sizeof *ctx.rows);int ready=ctx.rows&&ctx.count>0&&ctx.count<=(int)(OWNER_LIMIT-owner_count());
   for(int i=0;ready&&i<ctx.count;i++)if(zh_plan_row(&full,i,&ctx.rows[i]))ready=0;
   if(ready&&!extend_demo_fonts())ready=0;
   if(!ready){if(ctx.rows){for(int i=0;i<ctx.count;i++)zh_plan_free(&ctx.rows[i]);free(ctx.rows);}ctx.rows=NULL;ctx.count=0;}
  }
 }
 zh_plan_free(&full);MetaContext *saved=meta_context;ZhPlan *prior=queue_pending_row;void *parent=queue_pending_parent;size_t rule=queue_pending_rule;
 meta_context=ctx.rows?&ctx:NULL;queue_pending_row=NULL;queue_pending_parent=NULL;queue_pending_rule=0;command_scope++;
 /* C's trivial 28-byte shallow packet forwards the original ownership once.
    original_meta performs the only native dtor; text is never read afterward. */
 original_meta(self,text);
 command_scope--;meta_context=saved;queue_pending_row=prior;queue_pending_parent=parent;queue_pending_rule=rule;
 unsigned unconsumed=0;if(ctx.rows){for(int i=0;i<ctx.count;i++){unconsumed+=ctx.rows[i].runs!=NULL;zh_plan_free(&ctx.rows[i]);}free(ctx.rows);}
 fprintf(logfile,"META-END rows=%d original-lines=%d unconsumed=%u owners=%u\n",ctx.count,ctx.emitted,unconsumed,owner_count());fflush(logfile);
}
#ifdef META_RESOURCE_PROBE
static void meta_fixture_tick(void *parent){
 static void *root;static DWORD born;static int created,deleted;
 if(root&&root!=parent)return;
 if(!created&&GetTickCount()-probe_start>5000){
  created=1;root=parent;born=GetTickCount();queue_fixture=(*(NativeNew*)va(0xb6132c))(0x128);memset(queue_fixture,0,0x128);
  ((ConsoleCtor)va(0x48c060))(queue_fixture,parent,60,8,2,12,0,0,20);
  ((NativeVoid)va(0x9b8e80))((char*)queue_fixture+0x74);((NativeVoid)va(0x9b8e80))((char*)queue_fixture+0x8c);
  unsigned char seed[28]={0};((StringCtor)va(0x9af0f0))(seed,"QA-SEED");original_add_line(queue_fixture,seed,1,0,0);((NativeVoid)va(0x9af350))(seed);
  const char *texts[]={"ALERT: Suspicious activity at Repair Station F005. Dispatching Investigation squad.","QA","Unrecognized QA metadata deliberately longer than the native small string capacity."};
  for(unsigned i=0;i<3;i++){
   NativeStringValue ns={{0}};((StringCtor)va(0x9af0f0))(&ns,texts[i]);uintptr_t before,after;
   __asm__ volatile("movl %%esp,%0":"=r"(before));((MetaFn)va(0x90ec30))(queue_fixture,ns);__asm__ volatile("movl %%esp,%0":"=r"(after));
   fprintf(logfile,"META-ABI case=%u stack-delta=%d rows=%u owners=%u\n",i,(int)(after-before),vector_count((char*)queue_fixture+0x74),owner_count());
  }
  fflush(logfile);
 }
 if(created&&!deleted&&GetTickCount()-born>4000){
  deleted=1;unsigned before=owner_destructors;uint32_t *pv=(void*)((char*)root+0x44);unsigned n=vector_count(pv),index=n;
  for(unsigned i=0;i<n;i++)if(((void**)pv[0])[i]==queue_fixture){index=i;break;}
  if(index==n){fprintf(logfile,"META-DELETE-ERROR\n");return;}
  ((NativeVoid)va(0x9bac20))((char*)queue_fixture+0x74);((NativeVoid)va(0x9bac20))((char*)queue_fixture+0x8c);
  typedef void (__cdecl *Remove)(void*,unsigned);((Remove)va(0x9cea50))(pv,index);queue_fixture=NULL;
  fprintf(logfile,"META-FIXTURE-CLOSED cleanups=%u owners=%u\n",owner_destructors-before,owner_count());fflush(logfile);
 }
}
#endif
#ifdef QUEUE_EXPERIMENT
static void *queue_root;static unsigned queue_stage,queue_created_rows,queue_baseline,queue_phase;
static char queue_snapshot_name[96];
static DWORD queue_born;static void *old_addresses[128];static unsigned old_count;
static const char *queue_paragraph="Aside from the options menu, there are many more interface features and behavior you can tweak in Cogmind's advanced.cfg file, the functionality of which is described in the manual under Advanced UI: Advanced Options.";
static void queue_send(const char *source,int insert){unsigned char s[28]={0};((StringCtor)va(0x9af0f0))(s,source);((AddLineFn)va(0x90ed30))(queue_fixture,s,1,insert,0);((NativeVoid)va(0x9af350))(s);}
static void queue_check_layout(const char *phase){
 uint32_t *q=queue_fixture,*v=(void*)((char*)q+0x74);unsigned n=vector_count(v),errors=0,visible=0;int scroll=q[0x84/4],height=q[3];
 for(unsigned i=0;i<n;i++){
  uint32_t *child=((void**)v[0])[i];int shown=(int)i>=scroll&&(int)i<=scroll+height-3;
  errors+=!!*((unsigned char*)child+0x54)==shown;
  if(shown){visible++;errors+=(int)child[8]!=(int)i-scroll+1;}
  OwnerRun *o=owner_find(child);if(!o){errors++;continue;}
  for(unsigned run=0;run<o->logical;run++)if(o->widths[run]==2)errors+=memcmp(owner_cell(o,o->heads[run])+12,owner_cell(o,o->heads[run]+1)+12,6)!=0;
  for(int col=0;col<o->plan.columns;col++)errors+=*(uint32_t*)(owner_cell(o,col)+4)!=o->tokens[col];
 }
 fprintf(logfile,"QUEUE-LAYOUT cycle=%u phase=%s rows=%u scroll=%d visible=%u errors=%u\n",queue_cycle,phase,n,scroll,visible,errors);fflush(logfile);
 snprintf(queue_snapshot_name,sizeof queue_snapshot_name,"queue-%u-%s.bmp",queue_cycle,phase);
}
static void queue_check_completion(const char *phase){
 uint32_t *q=queue_fixture,*v=(void*)((char*)q+0x74);unsigned incomplete=0,visible=0,hidden_pending=0;
 for(unsigned i=0;i<vector_count(v);i++){
  void *child=((void**)v[0])[i];OwnerRun *o=owner_find(child);int done=o&&o->shown==(int)o->logical&&((uint32_t*)child)[24]==3;
  if(!*((unsigned char*)child+0x54)){visible++;incomplete+=!done;}else hidden_pending+=!done;
 }
 fprintf(logfile,"QUEUE-COMPLETION cycle=%u phase=%s visible=%u incomplete=%u hidden-pending=%u\n",queue_cycle,phase,visible,incomplete,hidden_pending);fflush(logfile);
}
static void queue_create(void *parent){
 queue_root=parent;queue_phase=0;queue_baseline=vector_count((char*)parent+0x44);queue_fixture=(*(NativeNew*)va(0xb6132c))(0x128);if(!queue_fixture)return;
 ((ConsoleCtor)va(0x48c060))(queue_fixture,parent,60,6,2,12,0,0,20);((NativeVoid)va(0x9b8e80))((char*)queue_fixture+0x74);*(unsigned*)((char*)queue_fixture+0x84)=0;queue_cycle++;queue_born=GetTickCount();
 queue_send(queue_paragraph,1);unsigned initial=vector_count((char*)queue_fixture+0x74),order_errors=0;uint32_t *v=(void*)((char*)queue_fixture+0x74);
 unsigned prior_id=0;for(unsigned i=0;i<initial;i++){OwnerRun *o=owner_find(((void**)v[0])[i]);if(!o||o->id<=prior_id)order_errors++;if(o)prior_id=o->id;}
 for(unsigned i=1;i<=8;i++){char source[64];snprintf(source,sizeof source,"Deep Access #%u",i);queue_send(source,0);}
 v=(void*)((char*)queue_fixture+0x74);unsigned before=vector_count(v);void *last=((void**)v[0])[before-1];queue_send(queue_paragraph,1);
 v=(void*)((char*)queue_fixture+0x74);unsigned after=vector_count(v);order_errors+=((void**)v[0])[after-1]!=last;
 prior_id=0;for(unsigned i=before-1;i<after-1;i++){OwnerRun *o=owner_find(((void**)v[0])[i]);if(!o||o->id<=prior_id)order_errors++;if(o)prior_id=o->id;}
 unsigned reused=0;for(unsigned i=0;i<after;i++)for(unsigned j=0;j<old_count;j++)reused+=((void**)v[0])[i]==old_addresses[j];
 queue_created_rows=after;fprintf(logfile,"QUEUE-CREATE cycle=%u initial-rows=%u rows=%u owners=%u order-errors=%u reused-addresses=%u\n",queue_cycle,initial,after,owner_count(),order_errors,reused);queue_check_layout("bottom-after-insert");
}
static void queue_delete(void){
 uint32_t *v=(void*)((char*)queue_fixture+0x74);old_count=vector_count(v);memcpy(old_addresses,(void*)v[0],old_count*sizeof(void*));
 unsigned before_owners=owner_count(),before_dtors=owner_destructors;uint32_t *parentv=(void*)((char*)queue_root+0x44);unsigned parent_count=vector_count(parentv),index=parent_count;
 for(unsigned i=0;i<parent_count;i++)if(((void**)parentv[0])[i]==queue_fixture){index=i;break;}
 if(index==parent_count){fprintf(logfile,"QUEUE-DELETE-ERROR missing-parent\n");return;}
 ((NativeVoid)va(0x9bac20))((char*)queue_fixture+0x74);typedef void (__cdecl *RemoveChild)(void*,unsigned);((RemoveChild)va(0x9cea50))(parentv,index);queue_fixture=NULL;
 unsigned stale=0;for(unsigned i=0;i<old_count;i++)stale+=owner_find(old_addresses[i])!=NULL;
 fprintf(logfile,"QUEUE-DELETE cycle=%u rows=%u removed=%u destructor-cleanups=%u remaining=%u stale=%u parent-before=%u parent-after=%u baseline=%u\n",queue_cycle,queue_created_rows,before_owners-owner_count(),owner_destructors-before_dtors,owner_count(),stale,parent_count,vector_count(parentv),queue_baseline);fflush(logfile);
}
static void queue_probe_tick(void *parent){
 if(queue_root&&parent!=queue_root)return;
 DWORD now=GetTickCount();if(!queue_fixture){if(queue_stage<4&&now-probe_start>5000+queue_stage*1800){queue_create(parent);queue_stage++;}return;}
 if(queue_stage==1&&now-queue_born>100){queue_delete();return;}
 if(queue_stage>=2&&queue_stage<=3){
  typedef void (__attribute__((thiscall)) *Scroll)(void*,int);
  if(queue_phase==0&&now-queue_born>2500){queue_check_completion("initial-bottom");((Scroll)va(0x90d0a0))(queue_fixture,-1000000);queue_check_layout("top");queue_phase=1;}
  else if(queue_phase==1&&now-queue_born>5000){queue_check_completion("top");((Scroll)va(0x90d0a0))(queue_fixture,1000000);queue_check_layout("bottom");queue_phase=2;}
  else if(queue_phase==2&&now-queue_born>7500){queue_check_completion("bottom");queue_delete();}return;
 }
 if(queue_stage==4&&queue_phase==0&&now-queue_born>2500){queue_check_completion("initial-bottom");typedef void (__attribute__((thiscall)) *Scroll)(void*,int);((Scroll)va(0x90d0a0))(queue_fixture,-1000000);queue_check_layout("final-top");queue_phase=1;}
 if(queue_stage==4&&queue_phase==1&&now-queue_born>5000){queue_check_completion("final-top");queue_check_layout("final-top-complete");queue_phase=2;fprintf(logfile,"QUEUE-SUMMARY cycles=4 destroyed=3 calls=%u translated-rows=%u fallbacks=%u owners-live=%u\n",queue_calls,queue_rows,queue_fallbacks,owner_count());fflush(logfile);}
}
#else
static void queue_probe_tick(void *parent){(void)parent;}
#endif


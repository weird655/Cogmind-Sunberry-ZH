/* Restricted command-output interception. Original business method stays intact. */
typedef int (__attribute__((thiscall)) *ParagraphFn)(void*,int,int,int,int,void*);
typedef void* (__attribute__((thiscall)) *ExtractFn)(void*,void*,const int*,unsigned);
typedef void (__attribute__((thiscall)) *CommandFn)(void*,void*,void*,int,int,int);
static ParagraphFn original_paragraph;static ExtractFn original_extract;static CommandFn original_command;
typedef struct {void *shell,*console;ZhPlan *rows;int count,kind,link;unsigned expected,before;uint32_t ids[32];size_t rule,raw_length;char raw[16385],target[16385],prefix[128];const char *fallback;} SerialContext;
static _Thread_local SerialContext *serial_context;
static unsigned serial_layouts,serial_extracts;
static void serial_context_free(SerialContext *ctx){if(ctx->rows){for(int i=0;i<ctx->count;i++)zh_plan_free(&ctx->rows[i]);free(ctx->rows);ctx->rows=NULL;}}
static int __attribute__((fastcall)) hooked_paragraph(void *self,void *unused,int x,int y,int width,int maxrows,void *s){
 (void)unused;if(live_native_english||!live_chinese)return original_paragraph(self,x,y,width,maxrows,s);uintptr_t caller=(uintptr_t)__builtin_return_address(0);
 if(caller==va(0x790a33)||caller==va(0x8089d1)){int value;if(scratch_paragraph(self,x,y,width,maxrows,s,caller,&value))return value;return original_paragraph(self,x,y,width,maxrows,s);}
 SerialContext *ctx=serial_context;
 if(!ctx){int result;if(display_bridge(self,x,y,width,maxrows,-1,2,s,&result))return result;}
 if(!ctx||self!=ctx->console||x||y||width!=46||maxrows!=9999||ctx->rows){if(ctx&&!ctx->rows)ctx->fallback="unsupported-layout-scope";return original_paragraph(self,x,y,width,maxrows,s);}
 const char *source;size_t bytes,rule;char translated[16384];ZhPlan full={0};
 if(!plain_source(s,&source,&bytes)){ctx->fallback="unsupported-inline-format";return original_paragraph(self,x,y,width,maxrows,s);}
 while(bytes&&source[bytes-1]==' ')bytes--;
 int cols=*(int*)va(0xd316bc)-4;if(cols>width)cols=width;if(cols<2){ctx->fallback="invalid-columns";return original_paragraph(self,x,y,width,maxrows,s);}
 if(ctx->link>=0){
  audit_native_string("LINK-PROCESSED",s);
  if(linked_text_prepare(ctx->raw,ctx->raw_length,source,bytes,ctx->target,strlen(ctx->target),ctx->expected,cols,translated,sizeof translated)!=1||linked_plan_atomic(translated,strlen(translated),cols,128,&full)!=1){ctx->fallback="link-validation-or-layout";return original_paragraph(self,x,y,width,maxrows,s);}
  rule=(size_t)-2;fprintf(logfile,"LINK-LAYOUT expected=%u rows=%d\n",ctx->expected,full.used_rows);
 }else{
  int match=message_resolve_profile(ctx->kind==1?MESSAGE_REPAIR:MESSAGE_TERMINAL,source,bytes,translated,sizeof translated,&rule);
  if(match<=0){ctx->fallback=match==0?"no-rule":match==-2?"ambiguous-rule":"invalid-or-capacity";return original_paragraph(self,x,y,width,maxrows,s);}
  if(zh_plan(translated,strlen(translated),cols,128,&full)){ctx->fallback="unicode-layout-limit";return original_paragraph(self,x,y,width,maxrows,s);}
 }
 int count=full.used_rows;ZhPlan *rows=calloc(count?count:1,sizeof *rows);int ready=rows&&count>0&&count<=(int)(OWNER_LIMIT-owner_count());
 for(int i=0;ready&&i<count;i++)if(zh_plan_row(&full,i,&rows[i]))ready=0;
 if(ready&&!extend_demo_fonts())ready=0;
 if(!ready){ctx->fallback="owner-font-or-allocation-limit";if(rows){for(int i=0;i<count;i++)zh_plan_free(&rows[i]);free(rows);}zh_plan_free(&full);return original_paragraph(self,x,y,width,maxrows,s);}
 uint32_t *v=s;const char *raw=v[5]<16?s:*(void**)s;size_t prefix=(size_t)(source-raw);if(prefix<sizeof ctx->prefix){memcpy(ctx->prefix,raw,prefix);ctx->prefix[prefix]=0;}
 ctx->rows=rows;ctx->count=count;ctx->rule=rule;zh_plan_free(&full);serial_layouts++;
 fprintf(logfile,"SERIAL-LAYOUT rows=%d width=%d rule=%u scope=%s\n",count,cols,(unsigned)rule,ctx->shell==queue_fixture?"synthetic-pipeline":"real-command");fflush(logfile);return count;
}
static void* __attribute__((fastcall)) hooked_extract(void *self,void *unused,void *out,const int *point,unsigned maximum){
 (void)unused;void *scratch=scratch_extract(self,out,point,maximum,(uintptr_t)__builtin_return_address(0));if(scratch)return scratch;SerialContext *ctx=serial_context;
 if(!ctx||self!=ctx->console||!ctx->rows||!point||point[0]||point[1]<0||point[1]>=ctx->count||maximum!=46)return original_extract(self,out,point,maximum);
 ZhPlan *row=&ctx->rows[point[1]];if(!row->runs)return original_extract(self,out,point,maximum);
 char carrier[768];size_t prefix=strlen(ctx->prefix);memcpy(carrier,ctx->prefix,prefix);memset(carrier+prefix,'@',row->columns);memcpy(carrier+prefix+row->columns,"`x`",4);
 ((StringCtor)va(0x9af0f0))(out,carrier);queue_pending_row=row;queue_pending_parent=ctx->shell;queue_pending_rule=ctx->rule;serial_extracts++;
 fprintf(logfile,"SERIAL-EXTRACT row=%d columns=%d\n",point[1],row->columns);return out;
}
static unsigned safe_native_vector(void *v,unsigned limit){
 if(!readable(v,16))return 0;uint32_t *p=v;if(p[1]<p[0]||(p[1]-p[0])%4)return 0;unsigned n=(p[1]-p[0])/4;
 return n<=limit&&(!n||readable((void*)p[0],n*4))?n:0;
}
static int linked_context_prepare(SerialContext *ctx,void *output,int link){
 if(link<0||*(unsigned char*)va(0xd28c8a)||*(uint32_t*)va(0xcf4738))return 0;
 const char *marker;size_t n;
 if(!plain_source((void*)va(0xd2ed44),&marker,&n)||n!=1||marker[0]!='['||!plain_source((void*)va(0xd2f26c),&marker,&n)||n!=1||marker[0]!=']')return 0;
 uint32_t *v=(void*)va(0xd35b58);unsigned total=safe_native_vector(v,8192);if((unsigned)link>=total)return 0;
 void *lore=((void**)v[0])[link];if(!readable(lore,0xc0))return 0;unsigned count=safe_native_vector((char*)lore+0xb0,32);if(!count)return 0;
 uint32_t *links=(void*)((char*)lore+0xb0);
 for(unsigned i=0;i<count;i++){uint32_t *item=((void**)links[0])[i];if(!readable(item,8))return 0;ctx->ids[i]=item[1];}
 const char *source;size_t bytes,rule;if(!plain_source(output,&source,&bytes)||bytes>=sizeof ctx->raw||message_resolve_terminal(source,bytes,ctx->target,sizeof ctx->target,&rule)<=0)return 0;
 memcpy(ctx->raw,source,bytes);ctx->raw[bytes]=0;ctx->raw_length=bytes;ctx->expected=count;ctx->link=link;return 1;
}
static void linked_audit(SerialContext *ctx){
 if(ctx->link<0||!ctx->count)return;
 uint32_t *v=(void*)((char*)ctx->shell+0x74);unsigned total=safe_native_vector(v,4096),ordinal=0,errors=0;
 for(unsigned i=ctx->before;i<total;i++){
  void *row=((void**)v[0])[i];if(!readable(row,0x80)||*(uint32_t*)row!=va(0xc36328))continue;
  unsigned count=safe_native_vector((char*)row+0x70,32);uint32_t *lv=(void*)((char*)row+0x70);
  for(unsigned j=0;j<count;j++){
   uint32_t *child=((void**)lv[0])[j];if(!readable(child,0x78)||child[0]!=va(0xc362f4)){errors++;continue;}
   errors+=ordinal>=ctx->expected||child[0x6c/4]!=(ordinal<ctx->expected?ctx->ids[ordinal]:0)||child[1]!=(uint32_t)row||child[3]!=1||child[7]+child[2]>((uint32_t*)row)[2];
   fprintf(logfile,"LINK-CHILD ordinal=%u target=%u x=%u width=%u row-width=%u first=%u last=%u shortcut=%d\n",ordinal,child[0x6c/4],child[7],child[2],((uint32_t*)row)[2],((unsigned char*)child)[0x71],((unsigned char*)child)[0x72],(int)child[0x74/4]);ordinal++;
  }
 }
 errors+=ordinal!=ctx->expected;fprintf(logfile,"LINK-CHECK expected=%u actual=%u errors=%u\n",ctx->expected,ordinal,errors);fflush(logfile);
}
static void __attribute__((fastcall)) hooked_command(void *self,void *unused,void *command,void *output,int kind,int link,int spoken){
 (void)unused;live_shell_capture_command(self,command,output,kind,link,spoken);fprintf(logfile,"REAL-COMMAND-BEGIN shell=%p kind=%d link=%d spoken=%d owners=%u\n",self,kind,link,spoken,owner_count());
 if(kind==1&&link>=0){fprintf(logfile,"LINK-CANDIDATE index=%d numeric=%u disabled=%u\n",link,*(unsigned char*)va(0xd28c8a),*(uint32_t*)va(0xcf4738));audit_native_string("LINK-OPEN-MARKER",(void*)va(0xd2ed44));audit_native_string("LINK-CLOSE-MARKER",(void*)va(0xd2f26c));}
 audit_native_string("REAL-COMMAND-INPUT",command);audit_native_string("REAL-COMMAND-OUTPUT",output);fflush(logfile);
 SerialContext ctx={0};ctx.link=-1;SerialContext *saved_ctx=serial_context;ZhPlan *saved_plan=queue_pending_row;void *saved_parent=queue_pending_parent;size_t saved_rule=queue_pending_rule;
 int base_eligible=live_chinese&&!live_native_english&&!saved_ctx&&(kind==1||kind==2)&&spoken==0&&live_shell_is_valid(self);
 int eligible=base_eligible&&(link==-1||(kind==1&&linked_context_prepare(&ctx,output,link)));
 ctx.fallback=eligible?"paragraph-path-not-used":base_eligible?"link-precheck":"scope-kind-or-spoken";
 queue_pending_row=NULL;queue_pending_parent=NULL;queue_pending_rule=0;serial_context=NULL;command_scope++;
 if(eligible){ctx.shell=self;ctx.kind=kind;ctx.before=vector_count((char*)self+0x74);ctx.console=*(void**)((char*)self+0x88);serial_context=&ctx;}
 original_command(self,command,output,kind,link,spoken);
 linked_audit(&ctx);
 unsigned unconsumed=0;for(int i=0;i<ctx.count;i++)unconsumed+=ctx.rows[i].runs!=NULL;
 if(ctx.count)fprintf(logfile,"SERIAL-COMMAND rows=%d unconsumed=%u layouts=%u extracts=%u\n",ctx.count,unconsumed,serial_layouts,serial_extracts);
 serial_context=saved_ctx;queue_pending_row=saved_plan;queue_pending_parent=saved_parent;queue_pending_rule=saved_rule;serial_context_free(&ctx);command_scope--;
 fprintf(logfile,"REAL-COMMAND-END eligible=%d translated-rows=%d unconsumed=%u owners=%u reason=%s\n",eligible,ctx.count,unconsumed,owner_count(),ctx.count?"translated":ctx.fallback);fflush(logfile);
}
#ifdef SERIAL_EXPERIMENT
static void *serial_root,*serial_scratch;static DWORD serial_born;static unsigned serial_cycle,serial_steps,serial_done;
static unsigned serial_old_count;static void *serial_old[128];static unsigned serial_baseline;
static uintptr_t serial_vtable[8];static unsigned serial_blocked;
typedef DWORD (__cdecl *NativeClock)(void);
static DWORD serial_clock(void){return (*(NativeClock*)va(0xb6143c))();}
static void __attribute__((fastcall)) serial_native_update(void *self,void *unused){
 (void)unused;uint32_t *globals=(void*)va(0xd35870),*q=self;
 if(globals[1]>=globals[0]+15*4&&readable((void*)globals[0],15*4)){
  uint32_t *lock=((void**)globals[0])[14];if(readable(lock,0x2c)&&lock[10]==0&&q[24]==3&&q[0x124/4]==0){
   unsigned before=vector_count((char*)q+0x8c);((NativeVoid)va(0x90c3a0))(self);unsigned after=vector_count((char*)q+0x8c);
   if(after<before){serial_steps++;fprintf(logfile,"SERIAL-STEP cycle=%u before=%u after=%u native-clock=%lu lock-before=0 lock-after=%u\n",serial_cycle,before,after,serial_clock(),lock[10]);fflush(logfile);}
   if(lock[10]||q[24]!=3||q[0x124/4]){fprintf(logfile,"SERIAL-GUARD-TRANSITION state=%u close=%u lock=%u pending=%u\n",q[24],q[0x124/4],lock[10],after);fflush(logfile);}return;
  }
 }
 serial_blocked++;((NativeVoid)va(0x429e30))(self);
}
static void serial_create(void *parent){
 serial_root=parent;serial_baseline=vector_count((char*)parent+0x44);queue_fixture=(*(NativeNew*)va(0xb6132c))(0x128);if(!queue_fixture)return;memset(queue_fixture,0,0x128);
 ((ConsoleCtor)va(0x48c060))(queue_fixture,parent,60,8,2,12,0,0,20);memcpy(serial_vtable,*(void**)queue_fixture,sizeof serial_vtable);serial_vtable[6]=(uintptr_t)serial_native_update;*(void**)queue_fixture=serial_vtable;
 ((NativeVoid)va(0x9b8e80))((char*)queue_fixture+0x74);((NativeVoid)va(0x9b8e80))((char*)queue_fixture+0x8c);((uint32_t*)queue_fixture)[24]=3;((uint32_t*)queue_fixture)[0x9c/4]=serial_clock();
 serial_scratch=(*(NativeNew*)va(0xb6132c))(0x128);((ConsoleCtor)va(0x48c060))(serial_scratch,queue_fixture,46,1,0,0,0,1,-1);*(void**)((char*)queue_fixture+0x88)=serial_scratch;
 serial_cycle++;serial_born=GetTickCount();SerialContext ctx={0};ctx.link=-1;ctx.shell=queue_fixture;ctx.console=serial_scratch;ctx.kind=1;serial_context=&ctx;
 const char *source="Scanning Imp. Spread Laser...\nReady to Repair:\n  Imp. Spread Laser\n  Rating: 4p\n  Time: 23";
 #ifdef TERMINAL_CORPUS_PROBE
 const char *corpus_sources[]={"NOTICE: Protoforge disruption unresolved, advanced fortification protocols active.","ALERT: LRC-V3 rejoined 0b10. Threat protocols amended."};
 if(serial_cycle<=2)source=corpus_sources[serial_cycle-1];
 fprintf(logfile,"TERMINAL-CORPUS-FIXTURE cycle=%u source=%s\n",serial_cycle,source);
 #endif
 unsigned char original[28]={0};((StringCtor)va(0x9af0f0))(original,source);int rows=((ParagraphFn)va(0x418260))(serial_scratch,0,0,46,9999,original);((NativeVoid)va(0x9af350))(original);
 unsigned reused=0;
 for(int row=0;row<rows&&ctx.rows;row++){
  int point[2]={0,row};unsigned char carrier[28]={0};((ExtractFn)va(0x4177a0))(serial_scratch,carrier,point,46);
  void *child=(*(NativeNew*)va(0xb6132c))(0x80);((ShellFn)va(0x4b0b30))(child,(int)(uintptr_t)queue_fixture,carrier,1);((NativeVoid)va(0x9af350))(carrier);
  for(unsigned j=0;j<serial_old_count;j++)reused+=child==serial_old[j];
  typedef void (__attribute__((thiscall)) *Push)(void*,void*);((Push)va(0x9b9d30))((char*)queue_fixture+0x74,&child);((Push)va(0x9b9d30))((char*)queue_fixture+0x8c,&child);
  OwnerRun *o=owner_find(child);if(o)for(int col=0;col<o->plan.columns;col++)memset(owner_cell(o,col)+12,0,6);
 }
 serial_context=NULL;queue_pending_row=NULL;queue_pending_parent=NULL;serial_context_free(&ctx);typedef void (__attribute__((thiscall)) *Scroll)(void*,int);((Scroll)va(0x90d0a0))(queue_fixture,0);
 fprintf(logfile,"SERIAL-CREATE cycle=%u rows=%d owners=%u reused=%u parent-baseline=%u\n",serial_cycle,rows,owner_count(),reused,serial_baseline);fflush(logfile);
}
static void serial_delete(void){
 uint32_t *v=(void*)((char*)queue_fixture+0x74);serial_old_count=vector_count(v);memcpy(serial_old,(void*)v[0],serial_old_count*4);unsigned before=owner_destructors;uint32_t *pv=(void*)((char*)serial_root+0x44);unsigned n=vector_count(pv),index=n;
 for(unsigned i=0;i<n;i++)if(((void**)pv[0])[i]==queue_fixture){index=i;break;}if(index==n){fprintf(logfile,"SERIAL-DELETE-ERROR parent\n");return;}
 ((NativeVoid)va(0x9bac20))((char*)queue_fixture+0x74);((NativeVoid)va(0x9bac20))((char*)queue_fixture+0x8c);typedef void (__cdecl *Remove)(void*,unsigned);((Remove)va(0x9cea50))(pv,index);queue_fixture=NULL;serial_scratch=NULL;
 unsigned stale=0;for(unsigned i=0;i<serial_old_count;i++)stale+=owner_find(serial_old[i])!=NULL;
 fprintf(logfile,"SERIAL-DELETE cycle=%u rows=%u destructor-cleanups=%u remaining=%u stale=%u parent=%u baseline=%u\n",serial_cycle,serial_old_count,owner_destructors-before,owner_count(),stale,vector_count(pv),serial_baseline);fflush(logfile);
}
static void serial_probe_tick(void *parent){
 if(serial_root&&parent!=serial_root)return;DWORD age=GetTickCount()-probe_start;
 if(!queue_fixture){if(serial_cycle<3&&age>5000+serial_cycle*2000)serial_create(parent);return;}
 if(serial_cycle==1&&GetTickCount()-serial_born>100){serial_delete();return;}
 if(GetTickCount()-serial_born>3000&&!serial_done){
  uint32_t *v=(void*)((char*)queue_fixture+0x74);unsigned incomplete=0,errors=0;for(unsigned i=0;i<vector_count(v);i++){
   OwnerRun *o=owner_find(((void**)v[0])[i]);if(!o){errors++;continue;}uint32_t *native=o->owner,*manager=*(void**)((char*)native+0x64);incomplete+=native[24]!=3||(manager[6]-manager[5])!=0;
   for(unsigned j=0;j<o->logical;j++)if(o->widths[j]==2)errors+=memcmp(owner_cell(o,o->heads[j])+12,owner_cell(o,o->heads[j]+1)+12,6)!=0;
   for(int col=0;col<o->plan.columns;col++)errors+=*(uint32_t*)(owner_cell(o,col)+4)!=o->tokens[col];
  }
  uint32_t *locks=(void*)va(0xd35870),*lock=((void**)locks[0])[14];
  fprintf(logfile,"SERIAL-COMPLETE cycle=%u pending=%u incomplete=%u errors=%u blocked=%u final-lock=%u\n",serial_cycle,vector_count((char*)queue_fixture+0x8c),incomplete,errors,serial_blocked,lock[10]);fflush(logfile);
  if(serial_cycle==2)serial_delete();else{serial_done=1;fprintf(logfile,"SERIAL-SUMMARY cycles=3 destroyed=2 native-steps=%u layouts=%u extracts=%u owners=%u\n",serial_steps,serial_layouts,serial_extracts,owner_count());fflush(logfile);}
 }
}
#else
static void serial_probe_tick(void *parent){(void)parent;}
#endif

/* Pinned native log path: capture complete text before native wrapping, keep
   English native records intact, and attach display-only Unicode rows. */
#define LOG_CONTENT_SLOTS 1024u
#define LOG_CONTENT_BYTES (8u*1024u*1024u)
typedef struct {void *record;char *english,*chinese;size_t bytes;unsigned row,count;void *manager;} LogContentRow;
static LogContentRow log_content_rows[LOG_CONTENT_SLOTS];
static unsigned log_content_next,log_content_count;static size_t log_content_bytes;
typedef struct {void *manager;char source[16385];size_t length;int copied;} LogContentContext;
static _Thread_local LogContentContext *log_content_context;
typedef int (__attribute__((thiscall)) *LogAppendFn)(void*,void*);
static LogAppendFn log_original_append;
static int log_native_source(void *ns,const char **out,size_t *length){
 if(!readable(ns,24))return 0;uint32_t *v=ns;size_t n=v[4];const char *p=v[5]<16?ns:*(void**)ns;
 if(!n||n>16384||!readable(p,n)||memchr(p,0,n))return 0;*out=p;*length=n;return 1;
}
static void * __attribute__((fastcall)) log_content_copy(void *self,void *unused,void *from){
 (void)unused;LogContentContext *ctx=log_content_context;const char *p;size_t n;
 if(ctx&&log_native_source(from,&p,&n)){memcpy(ctx->source,p,n);ctx->source[n]=0;ctx->length=n;ctx->copied=1;}
 typedef void* (__attribute__((thiscall)) *Copy)(void*,void*);return ((Copy)va(0x9aefb0))(self,from);
}
static unsigned log_bank_count(void *manager){return safe_native_vector(manager,65536);}
static void log_content_release(LogContentRow *r){if(r->record){log_content_bytes-=r->bytes;log_content_count--;}free(r->english);free(r->chinese);memset(r,0,sizeof *r);}
static void log_content_put(void *record,const char *english,const char *chinese,unsigned row,unsigned count,void *manager){
 size_t en=strlen(english)+1,cn=strlen(chinese)+1;if(en+cn>LOG_CONTENT_BYTES)return;
 char *a=malloc(en),*b=malloc(cn);if(!a||!b){free(a);free(b);return;}memcpy(a,english,en);memcpy(b,chinese,cn);
 for(unsigned i=0;i<LOG_CONTENT_SLOTS;i++)if(log_content_rows[i].record==record)log_content_release(&log_content_rows[i]);
 while(log_content_bytes+en+cn>LOG_CONTENT_BYTES){LogContentRow *old=&log_content_rows[log_content_next++%LOG_CONTENT_SLOTS];log_content_release(old);}
 LogContentRow *slot=&log_content_rows[log_content_next++%LOG_CONTENT_SLOTS];log_content_release(slot);*slot=(LogContentRow){record,a,b,en+cn,row,count,manager};log_content_bytes+=en+cn;log_content_count++;
}
static int __attribute__((fastcall)) log_content_append(void *self,void *unused,void *record){
 (void)unused;int known=self==(void*)va(0xcf1080)||self==(void*)va(0xd2f75c);const char *p;size_t n;
 if(!known||log_content_context||!readable(record,0x28)||!log_native_source((char*)record+4,&p,&n))return log_original_append(self,record);
 LogContentContext ctx={0};ctx.manager=self;ctx.length=n;memcpy(ctx.source,p,n);ctx.source[n]=0;
 unsigned before=log_bank_count(self);LogContentContext *prior=log_content_context;log_content_context=&ctx;
 int result=log_original_append(self,record);log_content_context=prior;
 unsigned after=log_bank_count(self);int repeat=0;unsigned first=before,count=after>before?after-before:0;
 if(after==before&&after){void *last=((void**)((uint32_t*)self)[0])[after-1];
  for(unsigned i=0;i<LOG_CONTENT_SLOTS;i++){LogContentRow *r=&log_content_rows[i];if(r->record==last&&r->manager==self&&r->count&&r->count<=after&&r->row+1==r->count){count=r->count;first=after-count;repeat=1;break;}}
 }
 if(!count||count>128)return result;
 uint32_t *bank=self;char joined[16385];size_t at=0;void *records[128];const char *english[128];size_t sizes[128];
 for(unsigned i=0;i<count;i++){
  records[i]=((void**)bank[0])[first+i];if(!readable(records[i],0x28)||!log_native_source((char*)records[i]+4,&english[i],&sizes[i])||at+sizes[i]+1>=sizeof joined)return result;
  if(i)joined[at++]='\n';memcpy(joined+at,english[i],sizes[i]);at+=sizes[i];
 }joined[at]=0;
 char a[16385],b[16385],target[32768];
 if(terminal_normalize(ctx.source,ctx.length,a,sizeof a)!=1||terminal_normalize(joined,at,b,sizeof b)!=1)return result;
 if(repeat){size_t length=strlen(b),open=length;while(open&&b[open-1]!='<')open--;
  if(!open||length-open<3||b[open]!='x'||b[length-1]!='>')return result;size_t digit=open+1;while(digit<length-1&&b[digit]>='0'&&b[digit]<='9')digit++;if(digit!=length-1||digit==open+1)return result;
  size_t base=open-1;while(base&&b[base-1]==' ')base--;char saved=b[base];b[base]=0;int equal=!strcmp(a,b);b[base]=saved;if(!equal)return result;
  ctx.length=strlen(b);memcpy(ctx.source,b,ctx.length+1);
 }else if(strcmp(a,b))return result;
 int match=message_resolve_entry(TEXT_ENTRY_LOG,ctx.source,ctx.length,target,sizeof target,NULL);
 if(match<=0){fprintf(logfile,"GAMELOG-MISS rows=%u bytes=%u raw=%s\n",count,(unsigned)ctx.length,ctx.source);return result;}
 int width=*(int*)va(0xcebd64);ZhPlan plan={0};if(width<2||width>512||message_plan_entry(TEXT_ENTRY_LOG,target,strlen(target),width,(int)count,&plan)){fprintf(logfile,"GAMELOG-NOFIT rows=%u width=%d raw=%s\n",count,width,ctx.source);zh_plan_free(&plan);return result;}
 char *prepared[128]={0};int ready=1;
 for(unsigned i=0;i<count;i++){
  char row[8192]={0};if((int)i<plan.used_rows&&!unicode_row_text(&plan,(int)i,row,sizeof row)){ready=0;break;}
  prepared[i]=malloc(strlen(row)+1);if(!prepared[i]){ready=0;break;}strcpy(prepared[i],row);
 }
 if(ready){for(unsigned i=0;i<count;i++)log_content_put(records[i],english[i],prepared[i],i,count,self);fprintf(logfile,"GAMELOG-CONNECT rows=%u cn-rows=%d width=%d bytes=%u cache=%u repeat=%d raw=%s target=%s\n",count,plan.used_rows,width,(unsigned)ctx.length,log_content_count,repeat,ctx.source,target);fflush(logfile);}
 for(unsigned i=0;i<count;i++)free(prepared[i]);zh_plan_free(&plan);return result;
}
/* Saved logs already contain native wrapped records. Reconstruct only a
   non-indented header plus its native two-space continuations, with identical
   descriptor/repetition/turn fields. Never rewrite the save or log bank. */
static int log_content_same_group(void *a,void *b){
 if(!readable(a,0x28)||!readable(b,0x28))return 0;uint32_t *x=a,*y=b;return x[0]==y[0]&&x[8]==y[8]&&x[9]==y[9];
}
static int log_content_continuation(void *record){const char *s;size_t n;return readable(record,0x28)&&log_native_source((char*)record+4,&s,&n)&&n>2&&s[0]==' '&&s[1]==' ';}
static int log_content_recover(void *wanted){
 void *managers[2]={(void*)va(0xcf1080),(void*)va(0xd2f75c)};
 for(unsigned bank_id=0;bank_id<2;bank_id++){
  void *manager=managers[bank_id];unsigned total=log_bank_count(manager);uint32_t *bank=manager;unsigned index=total;
  for(unsigned i=0;i<total;i++)if(((void**)bank[0])[i]==wanted){index=i;break;}if(index==total)continue;
  unsigned first=index;while(first&&index-first<127&&log_content_continuation(((void**)bank[0])[first])&&log_content_same_group(((void**)bank[0])[first-1],wanted))first--;
  const char *header;size_t hn;if(!log_native_source((char*)((void**)bank[0])[first]+4,&header,&hn)||header[0]==' '||header[0]=='\t')return 0;
  unsigned end=first+1;while(end<total&&end-first<128&&log_content_continuation(((void**)bank[0])[end])&&log_content_same_group(((void**)bank[0])[end],wanted))end++;
  unsigned count=end-first;int width=*(int*)va(0xcebd64);if(count<2||index>=end||width<2||width>512||hn<(size_t)width/2)return 0;
  void *records[128];const char *english[128];char joined[16385],canonical[16385],target[32768];size_t at=0;
  for(unsigned i=0;i<count;i++){size_t length;records[i]=((void**)bank[0])[first+i];if(!log_native_source((char*)records[i]+4,&english[i],&length)||at+length+1>=sizeof joined)return 0;if(i)joined[at++]='\n';memcpy(joined+at,english[i],length);at+=length;}joined[at]=0;
  if(terminal_normalize(joined,at,canonical,sizeof canonical)!=1)return 0;
  int match=message_resolve_entry(TEXT_ENTRY_LOG_CANONICAL,canonical,strlen(canonical),target,sizeof target,NULL);if(match<=0)return 0;
  ZhPlan plan={0};if(message_plan_entry(TEXT_ENTRY_LOG_CANONICAL,target,strlen(target),width,(int)count,&plan)){zh_plan_free(&plan);return 0;}
  char *prepared[128]={0};int ready=1;
  for(unsigned i=0;i<count;i++){char row[8192]={0};if((int)i<plan.used_rows&&!unicode_row_text(&plan,(int)i,row,sizeof row)){ready=0;break;}prepared[i]=malloc(strlen(row)+1);if(!prepared[i]){ready=0;break;}strcpy(prepared[i],row);}
  if(ready){for(unsigned i=0;i<count;i++)log_content_put(records[i],english[i],prepared[i],i,count,manager);fprintf(logfile,"GAMELOG-HISTORY rows=%u cn-rows=%d width=%d raw=%s target=%s\n",count,plan.used_rows,width,canonical,target);fflush(logfile);}
  for(unsigned i=0;i<count;i++)free(prepared[i]);zh_plan_free(&plan);return ready;
 }
 return 0;
}
static int log_content_print(void *self,int x,int y,void *ns,uintptr_t caller){
 if((caller!=va(0x48e3bc)&&caller!=va(0x7b3d0f))||x||y||!readable(self,0x70)||*(uint32_t*)self!=va(0xc344b0))return 0;
 void *record=*(void**)((char*)self+0x6c);const char *source;size_t n;if(!log_native_source(ns,&source,&n))return 0;
 int present=0;for(unsigned i=0;i<LOG_CONTENT_SLOTS;i++)if(log_content_rows[i].record==record){if(strlen(log_content_rows[i].english)==n&&!memcmp(log_content_rows[i].english,source,n))present=1;else log_content_release(&log_content_rows[i]);break;}if(!present)log_content_recover(record);
 for(unsigned i=0;i<LOG_CONTENT_SLOTS;i++){
  LogContentRow *r=&log_content_rows[i];if(r->record!=record)continue;
  if(strlen(r->english)!=n||memcmp(r->english,source,n)){log_content_release(r);return 0;}
  const char *saved=display_override_target;unsigned saved_scope=display_owned_literal_scope;display_override_target=r->chinese;display_owned_literal_scope++;int value;int ok=display_bridge(self,x,y,0,0,-1,0,ns,&value);display_override_target=saved;display_owned_literal_scope=saved_scope;return ok;
 }return 0;
}
static int log_content_install(void){
 if(!live_shell_patch_call(0x512979,0x9aefb0,log_content_copy))return 0;
 log_original_append=hook(0x5121f0,log_content_append);return log_original_append!=NULL;
}

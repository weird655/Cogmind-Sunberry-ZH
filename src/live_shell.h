/* Own original display events, never game commands. Included after shell hooks. */
#define LIVE_SHELL_COUNT 8
#define LIVE_SHELL_EVENTS 512
#define LIVE_SHELL_BYTES (8u*1024u*1024u)
typedef struct {int type,kind,link,spoken,insert,sound;char *command,*output;size_t command_length,output_length;unsigned first,last;} LiveShellEvent;
typedef struct {void *shell;uint32_t cells;unsigned count;int complete;LiveShellEvent events[LIVE_SHELL_EVENTS];} LiveShell;
static LiveShell live_shells[LIVE_SHELL_COUNT];
static size_t live_shell_bytes;
static NativeVoid live_original_shell_dtor;
#ifdef LIVE_RESOURCE_PROBE
static void *live_shell_fixture_owner;
#endif
static int live_shell_is_valid(void *self){
 if(!readable(self,0x128))return 0;
#ifdef LIVE_RESOURCE_PROBE
 if(self==live_shell_fixture_owner)return 1;
#endif
 return *(uint32_t*)self==va(0xc36224);
}
static void live_shell_remove(void *self){
 for(unsigned i=0;i<LIVE_SHELL_COUNT;i++)if(live_shells[i].shell==self){
  LiveShell *s=&live_shells[i];for(unsigned j=0;j<s->count;j++){LiveShellEvent *e=&s->events[j];if(e->command){live_shell_bytes-=e->command_length+1;free(e->command);}if(e->output){live_shell_bytes-=e->output_length+1;free(e->output);}}
  memset(s,0,sizeof *s);return;
 }
}
static void __attribute__((fastcall)) live_shell_dtor(void *self,void *unused){(void)unused;live_shell_remove(self);live_original_shell_dtor(self);}
static LiveShell *live_shell_get(void *self,int create){
 if(!live_shell_is_valid(self))return NULL;
 for(unsigned i=0;i<LIVE_SHELL_COUNT;i++)if(live_shells[i].shell==self){
  if(live_shells[i].cells!=((uint32_t*)self)[4]){live_shells[i].complete=0;return NULL;}return &live_shells[i];
 }
 if(create)for(unsigned i=0;i<LIVE_SHELL_COUNT;i++)if(!live_shells[i].shell){LiveShell *s=&live_shells[i];s->shell=self;s->cells=((uint32_t*)self)[4];s->complete=vector_count((char*)self+0x74)==0;return s;}return NULL;
}
static char *live_shell_copy(void *ns,size_t *length){
 if(!readable(ns,24))return NULL;uint32_t *v=ns;size_t n=v[4];const char *p=v[5]<16?ns:*(void**)ns;
 if(n>16384||!readable(p,n)||memchr(p,0,n)||n+1>LIVE_SHELL_BYTES-live_shell_bytes)return NULL;
 char *r=malloc(n+1);if(!r)return NULL;memcpy(r,p,n);r[n]=0;*length=n;live_shell_bytes+=n+1;return r;
}
static void live_shell_capture(int type,void *self,void *command,void *output,int kind,int link,int spoken,int insert,int sound){
 if(live_replay||command_scope||serial_context||meta_context)return;LiveShell *s=live_shell_get(self,1);if(!s||!s->complete)return;
 if(s->count==LIVE_SHELL_EVENTS){s->complete=0;fprintf(logfile,"LIVE-SHELL-CAPACITY shell=%p events=%u\n",self,s->count);return;}
 LiveShellEvent e={0};e.type=type;e.kind=kind;e.link=link;e.spoken=spoken;e.insert=insert;e.sound=sound;e.first=vector_count((char*)self+0x74);
 if(command)e.command=live_shell_copy(command,&e.command_length);if(output)e.output=live_shell_copy(output,&e.output_length);
 if((command&&!e.command)||(output&&!e.output)){if(e.command){live_shell_bytes-=e.command_length+1;free(e.command);}if(e.output){live_shell_bytes-=e.output_length+1;free(e.output);}s->complete=0;return;}
 if(s->count)s->events[s->count-1].last=e.first;s->events[s->count++]=e;
}
static void live_shell_capture_command(void *self,void *command,void *output,int kind,int link,int spoken){live_shell_capture(1,self,command,output,kind,link,spoken,0,0);}
static void live_shell_capture_add(void *self,void *text,int kind,int insert,int sound){live_shell_capture(2,self,NULL,text,kind,-1,0,insert,sound);}
static void live_shell_capture_meta(void *self,void *text){live_shell_capture(3,self,NULL,text,4,-1,0,1,0);}
static int live_shell_valid_rows(LiveShell *s){
 void *self=s->shell;if(!live_shell_is_valid(self)||s->cells!=((uint32_t*)self)[4])return 0;
 unsigned n=safe_native_vector((char*)self+0x74,4096),children=safe_native_vector((char*)self+0x44,8192);uint32_t *rows=(void*)((char*)self+0x74),*cv=(void*)((char*)self+0x44);
 if(vector_count(rows)!=n||vector_count(cv)!=children)return 0;
 for(unsigned i=0;i<n;i++){void *row=((void**)rows[0])[i];if(!row)continue;if(!readable(row,0x80)||*(uint32_t*)row!=va(0xc36328))return 0;unsigned found=0;for(unsigned j=0;j<children;j++)found+=((void**)cv[0])[j]==row;if(found!=1)return 0;}return 1;
}
static void live_shell_clear(LiveShell *s){
 void *self=s->shell;uint32_t *rows=(void*)((char*)self+0x74),*cv=(void*)((char*)self+0x44);unsigned n=vector_count(rows);
 /* Pending queue aliases the owning children; drop aliases before deleting. */
 ((NativeVoid)va(0x9bac20))((char*)self+0x8c);((NativeVoid)va(0x9b8e80))((char*)self+0x8c);
 for(unsigned i=0;i<n;i++){void *row=((void**)rows[0])[i];if(!row)continue;for(unsigned j=0;j<vector_count(cv);j++)if(((void**)cv[0])[j]==row){typedef void (__cdecl *Remove)(void*,unsigned);((Remove)va(0x9cea50))(cv,j);break;}}
 ((NativeVoid)va(0x9bac20))(rows);((NativeVoid)va(0x9b8e80))(rows);((uint32_t*)self)[0x84/4]=0;((uint32_t*)self)[0xa0/4]=0;
}
/* Return 0 when an unrecorded pre-existing/overflowed shell cannot be rebuilt. */
static int live_shell_rebuild_all(int chinese){
 int result=1;unsigned rebuilt=0;int prior_english=live_native_english;live_native_english=!chinese;live_replay++;
 for(unsigned i=0;i<LIVE_SHELL_COUNT;i++){
  LiveShell *s=&live_shells[i];if(!s->shell||!s->count)continue;if(!s->complete||!live_shell_valid_rows(s)){result=0;continue;}
  unsigned old_count=vector_count((char*)s->shell+0x74),old_scroll=((uint32_t*)s->shell)[0x84/4],anchor=0;int old_height=((uint32_t*)s->shell)[3];int at_bottom=(int)old_scroll>=((int)old_count-old_height+1);
  s->events[s->count-1].last=old_count;for(unsigned j=0;j<s->count;j++)if(s->events[j].first<=old_scroll)anchor=j;
  live_shell_clear(s);
  for(unsigned j=0;j<s->count;j++){
   LiveShellEvent *e=&s->events[j];NativeStringValue a={{0}},b={{0}};e->first=vector_count((char*)s->shell+0x74);
   if(e->command)((StringCtor)va(0x9af0f0))(&a,e->command);if(e->output)((StringCtor)va(0x9af0f0))(&b,e->output);
   if(e->type==1)hooked_command(s->shell,NULL,&a,&b,e->kind,e->link,e->spoken);
   else if(e->type==2)hooked_add_line(s->shell,NULL,&b,e->kind,e->insert,0);
   else if(e->type==3)hooked_meta(s->shell,NULL,b); /* native by-value callee consumes b */
   if(e->command)((NativeVoid)va(0x9af350))(&a);if(e->output&&e->type!=3)((NativeVoid)va(0x9af350))(&b);e->last=vector_count((char*)s->shell+0x74);
  }
  unsigned n=vector_count((char*)s->shell+0x74);int max=(int)n-old_height+1;if(max<0)max=0;int target=at_bottom?max:(int)s->events[anchor].first;if(target>max)target=max;((uint32_t*)s->shell)[0x84/4]=target;
  typedef void (__attribute__((thiscall)) *Scroll)(void*,int);((Scroll)va(0x90d0a0))(s->shell,0);rebuilt++;
 }
 live_replay--;live_native_english=prior_english;fprintf(logfile,"LIVE-SHELL-REBUILD chinese=%d shells=%u complete=%d bytes=%u\n",chinese,rebuilt,result,(unsigned)live_shell_bytes);fflush(logfile);return result;
}
typedef unsigned char (__cdecl *LiveShellSoundFn)(int,void*,int,int,int,int,int,int);
static unsigned char __cdecl live_shell_sound(int a,void *b,int c,int d,int e,int f,int g,int h){if(live_replay)return 0;return ((LiveShellSoundFn)va(0x5111e0))(a,b,c,d,e,f,g,h);}
static void __attribute__((fastcall)) live_shell_log_scroll(void *self,void *unused){(void)unused;if(!live_replay)((NativeVoid)va(0x7b4f10))(self);}
static int live_shell_patch_call(uintptr_t address,uintptr_t expected,void *replacement){
 unsigned char *p=(void*)va(address);if(p[0]!=0xe8||p+5+*(int32_t*)(p+1)!=(unsigned char*)va(expected))return 0;
 DWORD old;if(!VirtualProtect(p,5,PAGE_EXECUTE_READWRITE,&old))return 0;*(int32_t*)(p+1)=(int32_t)((unsigned char*)replacement-(p+5));FlushInstructionCache(GetCurrentProcess(),p,5);VirtualProtect(p,5,old,&old);return 1;
}
static int live_shell_install(void){
 const uintptr_t sounds[]={0x90d738,0x90d79f,0x90d94f},scrolls[]={0x90d75a,0x90d7c1,0x90d971};
 for(unsigned i=0;i<3;i++)if(!live_shell_patch_call(sounds[i],0x5111e0,live_shell_sound)||!live_shell_patch_call(scrolls[i],0x7b4f10,live_shell_log_scroll))return 0;
 live_original_shell_dtor=hook(0x4b0cd0,live_shell_dtor);return live_original_shell_dtor!=NULL;
}

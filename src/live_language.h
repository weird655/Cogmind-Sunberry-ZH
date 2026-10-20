#ifndef COGMIND_LIVE_LANGUAGE_H
#define COGMIND_LIVE_LANGUAGE_H
#include "replacement_animation.h"
/* SDL events and all text tree changes stay on the game's UI thread. */
static int live_pending=-1,live_phase;
static DWORD live_phase_start;
static unsigned live_switches,live_view_logs;
#ifdef LIVE_RAPID_PROBE
static unsigned live_rapid_capture_errors;
#endif
typedef int (__cdecl *LivePollFn)(void*);
static LivePollFn live_original_poll;
static int live_shortcut_held;
static int live_shell_idle(void){
 if(command_scope||serial_context||meta_context||queue_pending_row||display_busy||constructing_owner||live_replay)return 0;
 for(unsigned i=0;i<LIVE_SHELL_COUNT;i++)if(live_shells[i].shell){void *s=live_shells[i].shell;if(!readable(s,0x128))return 0;if(vector_count((char*)s+0x8c))return 0;}
 return 1;
}
static void live_request(int chinese){
 live_pending=!!chinese;
 fprintf(logfile,"LIVE-LANGUAGE-REQUEST target=%s\n",chinese?"zh":"en");fflush(logfile);
}
static int __cdecl live_poll(void *event){
 perf_flush();message_route_flush();
 live_tick();terminal_input_tick();
 #ifdef TERMINAL_INPUT_FIXTURE
 terminal_input_fixture(NULL);
 #endif
 int r;
 while((r=live_original_poll(event))!=0){
  if(!event)return r;
  unsigned char *e=event;int symbol=0,mod=0;
  if(e[0]==2||e[0]==3){memcpy(&symbol,e+8,4);memcpy(&mod,e+12,4);
   #ifdef LIVE_RESOURCE_PROBE
   if(symbol==290)continue;
   #endif
   if(symbol==290){
    if(e[0]==2&&(mod&0x0200)){if(!live_shortcut_held){live_request(!(live_pending>=0?live_pending:live_chinese));live_shortcut_held=1;}continue;}
    if(e[0]==3&&live_shortcut_held){live_shortcut_held=0;continue;}
   }
  }
  /* A text transition never moves native buttons or link hitboxes. Avoid
     accepting clicks while their text is crossing the viewport edge. */
  if(live_phase&&(e[0]==5||e[0]==6))continue;
  return r;
 }
 live_tick();terminal_input_tick();
 #ifdef TERMINAL_INPUT_FIXTURE
 terminal_input_fixture(NULL);
 #endif
 return r;
}
#define LIVE_REPLACEMENT_MS 1600u
static void *live_last_canvas;
static unsigned char *live_replacement_old,*live_replacement_target;
static unsigned live_replacement_columns,live_replacement_rows;
static void *live_replacement_owner;
static uint32_t live_replacement_vt,live_replacement_grid;
static int live_replacement_target_valid;
/* A queued reversal starts from the frame actually composed at completion.
   Native canvas storage can be repopulated before the next input poll. */
static unsigned char *live_queued_frame;
static void *live_queued_owner;
static uint32_t live_queued_vt,live_queued_grid;
static unsigned live_queued_columns,live_queued_rows;
static DWORD live_queued_at;
static void live_queued_release(void){free(live_queued_frame);live_queued_frame=NULL;live_queued_owner=NULL;}
static void live_replacement_release(void){
 free(live_replacement_old);free(live_replacement_target);live_replacement_old=live_replacement_target=NULL;live_replacement_owner=NULL;live_replacement_target_valid=0;
}
static void live_replacement_capture(void){
 live_replacement_release();uint32_t *o=live_last_canvas;
 if(!readable(o,20)||!o[2]||!o[3]||o[2]>512||o[3]>512||!readable((void*)o[4],(size_t)o[2]*o[3]*20)){live_queued_release();return;}
 size_t bytes=(size_t)o[2]*o[3]*20;live_replacement_old=malloc(bytes);live_replacement_target=malloc(bytes);
 if(!live_replacement_old||!live_replacement_target){live_replacement_release();live_queued_release();return;}
 int queued=live_queued_frame&&live_queued_owner==o&&live_queued_vt==o[0]&&live_queued_grid==o[4]&&live_queued_columns==o[2]&&live_queued_rows==o[3]&&GetTickCount()-live_queued_at<250u;
 memcpy(live_replacement_old,queued?live_queued_frame:(void*)o[4],bytes);
 #ifdef LIVE_RAPID_PROBE
 if(live_switches==1&&!live_chinese){live_rapid_capture_errors+=!queued;fprintf(logfile,"RAPID-CAPTURE queued-reverse=1 completed-frame-used=%d errors=%u\n",queued,live_rapid_capture_errors);fflush(logfile);}
 #endif
 live_queued_release();live_replacement_columns=o[2];live_replacement_rows=o[3];live_replacement_owner=o;live_replacement_vt=o[0];live_replacement_grid=o[4];
}
static void live_replacement_apply(void *self){
 if(!live_phase||!live_replacement_old||self!=live_replacement_owner)return;uint32_t *o=self;
 if(!readable(o,20)||o[0]!=live_replacement_vt||o[4]!=live_replacement_grid||o[2]!=live_replacement_columns||o[3]!=live_replacement_rows||!readable((void*)o[4],(size_t)o[2]*o[3]*20)){live_phase=0;live_replacement_release();return;}
 size_t bytes=(size_t)o[2]*o[3]*20;memcpy(live_replacement_target,(void*)o[4],bytes);live_replacement_target_valid=1;
 DWORD elapsed=GetTickCount()-live_phase_start;
 replacement_frame(live_replacement_old,live_replacement_target,(void*)o[4],o[2],o[3],elapsed,LIVE_REPLACEMENT_MS,320);
 /* Retire only after the target frame was actually composed. A queued switch
    then captures a complete language frame, not the previous mixed suffix. */
 if(elapsed>=LIVE_REPLACEMENT_MS){
  if(live_pending>=0&&live_pending!=live_chinese){
   live_queued_release();live_queued_frame=live_replacement_target;live_replacement_target=NULL;live_queued_owner=o;live_queued_vt=o[0];live_queued_grid=o[4];live_queued_columns=o[2];live_queued_rows=o[3];live_queued_at=GetTickCount();
  }
  live_phase=0;live_replacement_release();
 }
}
static void live_tick(void){
 /* No drawable canvas / a closed panel must not trap input indefinitely. */
 if(live_phase&&GetTickCount()-live_phase_start>=LIVE_REPLACEMENT_MS+250u){
  uint32_t *o=live_replacement_owner;
  if(live_replacement_target_valid&&readable(o,20)&&o[0]==live_replacement_vt&&o[4]==live_replacement_grid&&o[2]==live_replacement_columns&&o[3]==live_replacement_rows&&readable((void*)o[4],(size_t)o[2]*o[3]*20))memcpy((void*)o[4],live_replacement_target,(size_t)o[2]*o[3]*20);
  live_phase=0;live_replacement_release();
 }
 if(!live_phase&&live_pending>=0&&live_pending!=live_chinese&&live_shell_idle()){
  live_replacement_capture();live_chinese=live_pending;live_pending=-1;
  int complete=live_shell_rebuild_all(live_chinese);
  FILE *f=fopen("native-language.txt","wb");if(f){fputs(live_chinese?"zh":"en",f);fclose(f);}
  live_phase=live_replacement_old!=NULL;live_phase_start=GetTickCount();live_switches++;live_view_logs=0;
  typedef void (__cdecl *Caption)(const char*,const char*);Caption caption=(Caption)GetProcAddress(GetModuleHandleW(L"SDL.dll"),"SDL_WM_SetCaption");if(caption)caption(live_chinese?"Cogmind - Beta 17.1 [Chinese | RAlt+F9]":"Cogmind - Beta 17.1 [English | RAlt+F9]",NULL);
  fprintf(logfile,"LIVE-LANGUAGE-COMMIT mode=%s switches=%u shell-complete=%d animation=in-place-glyph-replacement old-frame=%d\n",live_chinese?"zh":"en",live_switches,complete,live_replacement_old!=NULL);fflush(logfile);
 }
 if(!live_phase&&live_pending==live_chinese)live_pending=-1;
 if(!live_phase&&live_pending<0&&live_queued_frame)live_queued_release();
}
static void *live_draw_view(void *owner){
 if(live_native_english||!readable(owner,24))return NULL;
 uint32_t *o=owner;unsigned w=o[2],h=o[3];if(!w||!h||w>512||h>512||!readable((void*)o[4],(size_t)w*h*20))return NULL;
 unsigned char *view=live_chinese?NULL:live_cells_view(owner);
 if(view&&live_view_logs<4){unsigned changed=0;for(unsigned x=0;x<w*h;x++)changed+=*(uint32_t*)(view+x*20+4)!=*(uint32_t*)(o[4]+x*20+4);fprintf(logfile,"LIVE-ENGLISH-VIEW owner=%p vt=%08lx changed-cells=%u\n",owner,(unsigned long)(o[0]-base+0x400000),changed);fflush(logfile);live_view_logs++;}
 return view;
}
#ifdef ANIMATION_VISUAL_PROBE
/* Isolated-test captures only. Never compiled into the production adapter. */
static void animation_visual_frame(void *surface){
 static unsigned last,mask;if(!live_switches)return;if(last!=live_switches){last=live_switches;mask=0;}
 const unsigned times[]={0,400,800,1200,1500,1600};DWORD elapsed=GetTickCount()-live_phase_start;
 for(unsigned i=0;i<6;i++)if(!(mask&(1u<<i))&&elapsed>=times[i]){
  typedef void* (__cdecl *RW)(const char*,const char*);typedef int (__cdecl *Save)(void*,void*,int);
  HMODULE sdl=GetModuleHandleW(L"SDL.dll");RW rw=(RW)GetProcAddress(sdl,"SDL_RWFromFile");Save save=(Save)GetProcAddress(sdl,"SDL_SaveBMP_RW");
  char name[80];snprintf(name,sizeof name,"switch-%u-%04u.bmp",last,times[i]);
  if(rw&&save){void *file=rw(name,"wb");if(file){fprintf(logfile,"ANIMATION-VISUAL switch=%u elapsed=%lu requested=%u result=%d\n",last,elapsed,times[i],save(surface,file,1));fflush(logfile);}}
  mask|=1u<<i;break;
 }
}
#endif
static NativeVoid live_original_composite;
static unsigned live_composite_active,live_composite_logs;
static int live_composite_last_mode=-1;
typedef struct {uint32_t *owner;uint32_t saved;void *view;} LiveViewSwap;
static void __attribute__((fastcall)) live_composite(void *self,void *unused){
 (void)unused;
 if(live_composite_active||live_native_english){live_original_composite(self);return;}
 if(readable(self,20)){uint32_t *root=self;if(root[2]>=80&&root[3]>=25)live_last_canvas=self;}
 if(live_composite_last_mode!=live_chinese){live_composite_last_mode=live_chinese;live_composite_logs=0;}
 if(live_chinese&&!live_phase){live_composite_active++;live_original_composite(self);live_composite_active--;return;}
 LiveViewSwap *swaps=calloc(LIVE_CELLS_MAX_OWNERS,sizeof *swaps);if(!swaps){live_original_composite(self);return;}unsigned count=0;live_composite_active++;
 for(unsigned i=0;i<LIVE_CELLS_MAX_OWNERS;i++){
  void *owner=live_cells_entries[i].owner;if(!owner||owner==self)continue;
  void *view=live_draw_view(owner);if(!view)continue;uint32_t *o=owner;swaps[count].owner=o;swaps[count].saved=o[4];swaps[count].view=view;o[4]=(uint32_t)view;count++;
 }
 live_original_composite(self);live_replacement_apply(self);
 unsigned cjk=0;uint32_t *canvas=self;if(readable(canvas,20)&&canvas[2]<=512&&canvas[3]<=512&&readable((void*)canvas[4],(size_t)canvas[2]*canvas[3]*20))for(unsigned i=0;i<canvas[2]*canvas[3];i++)cjk+=*(uint32_t*)(canvas[4]+i*20+4)>=320;
 for(unsigned i=0;i<count;i++){LiveViewSwap *s=&swaps[i];if(readable(s->owner,20)&&s->owner[4]==(uint32_t)s->view)s->owner[4]=s->saved;free(s->view);}free(swaps);live_composite_active--;
 if(count&&live_composite_logs<3){fprintf(logfile,"LIVE-COMPOSITE mode=%s source-views=%u canvas-cjk=%u native-sources-restored=1\n",live_chinese?"zh":"en",count,cjk);fflush(logfile);live_composite_logs++;}
}
typedef void (__attribute__((thiscall)) *LiveBlitFn)(void*,void*,void*,void*);
static LiveBlitFn live_original_blit;
static void __attribute__((fastcall)) live_blit(void *self,void *unused,void *target,void *point,void *crop){
 (void)unused;if(live_composite_active){live_original_blit(self,target,point,crop);return;}uint32_t *o=self;void *view=live_draw_view(self);uint32_t saved=o[4];if(view)o[4]=(uint32_t)view;
 live_original_blit(self,target,point,crop);if(view){o[4]=saved;free(view);}
}
static int live_install(void){
 live_original_blit=hook(0x429fe0,live_blit);if(!live_original_blit)return 0;
 live_original_composite=hook(0x42ded0,live_composite);if(!live_original_composite)return 0;
 IMAGE_DOS_HEADER *dos=(void*)base;IMAGE_NT_HEADERS32 *nt=(void*)(base+dos->e_lfanew);
 DWORD directory=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
 IMAGE_IMPORT_DESCRIPTOR *imports=(void*)(base+directory);
 for(;imports->Name;imports++){
  if(_stricmp((char*)(base+imports->Name),"SDL.dll"))continue;
  IMAGE_THUNK_DATA32 *names=(void*)(base+imports->OriginalFirstThunk),*iat=(void*)(base+imports->FirstThunk);
  for(;names->u1.AddressOfData;names++,iat++){
   if(IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))continue;
   IMAGE_IMPORT_BY_NAME *name=(void*)(base+names->u1.AddressOfData);if(strcmp((char*)name->Name,"SDL_PollEvent"))continue;
   DWORD old;if(!VirtualProtect(iat,4,PAGE_READWRITE,&old))return 0;live_original_poll=(void*)iat->u1.Function;iat->u1.Function=(uintptr_t)live_poll;VirtualProtect(iat,4,old,&old);
   fprintf(logfile,"LIVE-LANGUAGE-READY mode=%s shortcut=RAlt+F9 transition=fixed-position-glyph-replacement-1600ms\n",live_chinese?"zh":"en");fflush(logfile);return 1;
  }
 }
 return 0;
}
#ifdef LIVE_RESOURCE_PROBE
static void live_fixture(void *parent);
#endif
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "performance.h"
#include "text_core.h"
#include "translation_overlay.h"
#include "terminal_text.h"
#include "repair_text.h"
#include "linked_text.h"
#include "ui_text.h"
#include "canonical_item_names.h"

static int live_chinese=1,live_native_english;
static _Thread_local unsigned live_replay;
static void live_shell_capture_command(void*,void*,void*,int,int,int);
static void live_shell_capture_add(void*,void*,int,int,int);
static void live_shell_capture_meta(void*,void*);
static void live_tick(void);
static void terminal_input_tick(void);
static int terminal_input_event(void*);
static void *terminal_input_draw_view(void*);
static void terminal_input_draw_overlay(void*,void*,void*);
static int terminal_input_draw_background_full(void*,void*);
static void terminal_input_note_print(void*,void*,const char*);
#ifdef TERMINAL_INPUT_FIXTURE
static void terminal_input_fixture(void*);
#endif
static void *live_draw_view(void*);
#ifdef ANIMATION_VISUAL_PROBE
static void animation_visual_frame(void*);
#endif
static int live_install(void);
static int live_shell_is_valid(void*);
#ifdef LIVE_RESOURCE_PROBE
static void live_fixture(void*);
#endif
static FILE *logfile;
static uintptr_t base;
static unsigned calls;
static ZhDictionary *dictionary;
static ZhDictionary *item_dictionary;
static ZhDictionary *short_dictionary;
static void message_observe(int,int,const char*,size_t);
static void message_layout_observe(int,int,int,int);
#include "text_context.h"
static void message_binding_observe(const TextObject*,int);
#define MESSAGE_OBSERVE(profile,result,source,bytes) message_observe(profile,result,source,bytes)
#define TEXT_OBJECT_OBSERVE(object,result) message_binding_observe(object,result)
#define TEXT_LAYOUT_OBSERVE(entry,result,columns,rows) message_layout_observe(entry,result,columns,rows)
#include "message_resolver.h"
#include "message_diagnostics.h"
static HMODULE patch_module;
typedef void (__attribute__((thiscall)) *TextFn)(void*,int,int,void*);
static TextFn original_text;
static int display_bridge(void*,int,int,int,int,int,int,void*,int*);
static void *hook(uintptr_t,void*);
static void diagnostic_memory_tick(void);
#ifdef REPAIR_POPUP_PROBE
static void repair_popup_fixture(void*);
#endif
#ifdef ZOOM_RESOURCE_PROBE
static void zoom_fixture(void*);
#endif
#ifdef SCRATCH_RESOURCE_PROBE
static void scratch_fixture(void*);
#endif
static _Thread_local unsigned display_owned_literal_scope;
static _Thread_local const char *display_override_target;
static _Thread_local const char *display_override_english;
static int scratch_paragraph(void*,int,int,int,int,void*,uintptr_t,int*);
static void *scratch_extract(void*,void*,const int*,unsigned,uintptr_t);
static int scratch_row_print(void*,int,int,void*,uintptr_t);
#ifdef MANUAL_RESOURCE_PROBE
static void manual_fixture(void*);
#endif
static int menu_compact_note_print(void*,int,int,void*,const char*);
static _Thread_local uintptr_t display_origin_site;
#ifdef DISPLAY_RESOURCE_PROBE
static void display_fixture(void*);
#ifdef INTRO_RESOURCE_PROBE
static void intro_fixture(void*);
#endif
#endif
typedef void* (__attribute__((thiscall)) *ShellFn)(void*,int,void*,int);
static ShellFn original_shell;
#include "native_layout_steam.h"
static uintptr_t va(uintptr_t a) { return base+native_layout_address(a)-0x400000; }
static int readable(const void *p,size_t n) {
 MEMORY_BASIC_INFORMATION m;
 if(!p || !VirtualQuery(p,&m,sizeof m) || m.State!=MEM_COMMIT || (m.Protect&(PAGE_NOACCESS|PAGE_GUARD))) return 0;
 return (uintptr_t)p+n>=(uintptr_t)p && (uintptr_t)p+n<=(uintptr_t)m.BaseAddress+m.RegionSize;
}
#include "native_record_binding.h"
#include "live_cells.h"
#ifdef CHINESE_DEMO
#include "native_demo.h"
#include "intro_font.h"
#include "zoom_font.h"
#ifdef SHELL_PROBE
#include "shell_probe.h"
#include "queue_probe.h"
#include "serial_probe.h"
#endif
typedef void (__attribute__((thiscall)) *DrawFn)(void*,void*,void*);
static DrawFn original_draw;
static unsigned rendered;
#ifdef CAPTURE_DIAGNOSTICS
static DWORD capture_start;
static int captured;
#ifdef FONT_SWITCH_SMOKE
static int switch_stage;
static int saved_font;
static int alternate_font=-1;
static unsigned released_font;
static void switch_font_smoke(DWORD elapsed) {
 typedef void (__attribute__((thiscall)) *Activate)(void*,int,int);
 if(demo_state!=1)return;
 uint32_t *rex=(void*)va(0xd223f0),*v=(void*)va(0xd224ac);
 if((!switch_stage&&elapsed>30000)||(switch_stage==1&&elapsed>32000)){
  if(!switch_stage)saved_font=rex[0xcc/4];uint32_t *active=(void*)rex[0xd0/4];unsigned count=(v[1]-v[0])/4;
  for(unsigned i=0;i<count;i++){
   uint32_t *set=((void**)v[0])[i];
   if((int)i!=saved_font&&(int)i!=alternate_font&&set[11]==active[11]&&set[12]==active[12]){
    ((Activate)va(0x423f40))(rex,i,0);switch_stage++;alternate_font=i;
    fprintf(logfile,"FONT-SWITCH from=%d to=%u size=%ux%u active=%u\n",saved_font,i,set[11],set[12],rex[0xcc/4]);fflush(logfile);break;
   }
  }
 }else if(switch_stage==2&&elapsed>33000){
  unsigned count=(v[1]-v[0])/4;released_font=~0u;uint32_t *active=(void*)rex[0xd0/4];
  for(unsigned j=0;j<count;j++){uint32_t *set=((void**)v[0])[j];if((int)j!=saved_font&&(int)j!=alternate_font&&set[11]==active[11]&&set[12]==active[12]){released_font=j;break;}}
  font_release_fixture();switch_stage=3;
 }else if(switch_stage==3&&elapsed>34000){
  ((Activate)va(0x423f40))(rex,released_font,0);switch_stage=4;
  fprintf(logfile,"FONT-CACHE-REBUILD index=%u active=%u surface=%p\n",released_font,rex[0xcc/4],(void*)role0_atlas(*(void**)va(0xd224c0))[10]);fflush(logfile);
  #ifdef QUEUE_EXPERIMENT
  queue_check_layout("font-cache-rebuilt");
  #endif
 }else if(switch_stage==4&&elapsed>36000){
  ((Activate)va(0x423f40))(rex,saved_font,0);switch_stage=5;
  fprintf(logfile,"FONT-RESTORE active=%u\n",rex[0xcc/4]);fflush(logfile);
  #ifdef QUEUE_EXPERIMENT
  queue_check_layout("font-restored");
  #endif
 }
}
#endif
static int (__cdecl *original_flip)(void*);
static int __cdecl capture_flip(void *surface) {
 int r=original_flip(surface);
 #ifdef ANIMATION_VISUAL_PROBE
 animation_visual_frame(surface);
 #endif
 #ifdef REAL_SHELL_OBSERVER
 static void *real_previous;static unsigned last_rows=~0u,last_pending=~0u,last_lock=~0u;static int last_scroll=-999;
 uint32_t *real_shell=*(void**)va(0xcec100);
 if(real_shell&&readable(real_shell,0x128)&&*real_shell==va(0xc36224)){
  uint32_t *locks=(void*)va(0xd35870);unsigned lock=~0u;if(locks[1]>=locks[0]+60&&readable((void*)locks[0],60)){uint32_t *l=((void**)locks[0])[14];if(readable(l,44))lock=l[10];}
  unsigned rows=vector_count((char*)real_shell+0x74),pending=vector_count((char*)real_shell+0x8c);int scroll=(int)real_shell[0x84/4];
  if(real_shell!=real_previous||rows!=last_rows||pending!=last_pending||lock!=last_lock||scroll!=last_scroll){fprintf(logfile,"REAL-STATE shell=%p rows=%u pending=%u scroll=%d state=%u lock=%u owners=%u\n",(void*)real_shell,rows,pending,scroll,real_shell[24],lock,owner_count());fflush(logfile);last_rows=rows;last_pending=pending;last_lock=lock;last_scroll=scroll;}
  real_previous=real_shell;
 }else if(real_previous){fprintf(logfile,"REAL-SHELL-CLOSED shell=%p owners=%u\n",real_previous,owner_count());fflush(logfile);real_previous=NULL;last_rows=~0u;last_pending=~0u;last_lock=~0u;last_scroll=-999;}
 #endif
 #ifdef QUEUE_EXPERIMENT
 if(queue_snapshot_name[0]){
  typedef void* (__cdecl *RW)(const char*,const char*);typedef int (__cdecl *Save)(void*,void*,int);HMODULE sdl=GetModuleHandleW(L"SDL.dll");RW rw=(RW)GetProcAddress(sdl,"SDL_RWFromFile");Save save=(Save)GetProcAddress(sdl,"SDL_SaveBMP_RW");
  if(rw&&save){void *f=rw(queue_snapshot_name,"wb");if(f)fprintf(logfile,"QUEUE-SNAPSHOT file=%s result=%d\n",queue_snapshot_name,save(surface,f,1));}queue_snapshot_name[0]=0;fflush(logfile);
 }
 #endif
 #ifdef SHELL_PROBE
 record_shell_frame(surface);
 #endif
 #ifdef FONT_SWITCH_SMOKE
 switch_font_smoke(GetTickCount()-capture_start);
 #endif
 DWORD elapsed=GetTickCount()-capture_start;
 #ifdef SERIAL_EXPERIMENT
 const char *frame_names[]={"serial-render.bmp","serial-complete.bmp","serial-final.bmp"};
 const DWORD thresholds[]={14500,15500,16500};
 #else
 const char *frame_names[]={"native-render.bmp","native-font-switched.bmp","native-font-restored.bmp"};
 const DWORD thresholds[]={28000,35000,38000};
 #endif
 for(unsigned frame=0;frame<3;frame++)if(!(captured&(1<<frame)) && demo_writes && elapsed>thresholds[frame]) {
  typedef void* (__cdecl *RWFile)(const char*,const char*);
  typedef int (__cdecl *SaveBMP)(void*,void*,int);
  HMODULE sdl=GetModuleHandleW(L"SDL.dll");
  RWFile rw=(RWFile)GetProcAddress(sdl,"SDL_RWFromFile");SaveBMP save=(SaveBMP)GetProcAddress(sdl,"SDL_SaveBMP_RW");
  if(rw&&save){void *file=rw(frame_names[frame],"wb");if(file){fprintf(logfile,"FINAL-FRAME-BMP file=%s result=%d elapsed=%lu\n",frame_names[frame],save(surface,file,1),elapsed);captured|=1<<frame;fflush(logfile);}}
 }
 return r;
}
static int capture_import(void) {
 IMAGE_DOS_HEADER *dos=(void*)base;IMAGE_NT_HEADERS *pe=(void*)(base+dos->e_lfanew);
 IMAGE_IMPORT_DESCRIPTOR *d=(void*)(base+pe->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
 for(;d->Name;d++) {
  if(!d->OriginalFirstThunk)continue;
  IMAGE_THUNK_DATA *names=(void*)(base+d->OriginalFirstThunk),*iat=(void*)(base+d->FirstThunk);
  for(;names->u1.AddressOfData;names++,iat++) {
   if(IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))continue;
   IMAGE_IMPORT_BY_NAME *name=(void*)(base+names->u1.AddressOfData);
   if(strcmp((char*)name->Name,"SDL_Flip"))continue;
   DWORD old;if(!VirtualProtect(iat,4,PAGE_READWRITE,&old))return 0;
   original_flip=(void*)iat->u1.Function;iat->u1.Function=(uintptr_t)capture_flip;VirtualProtect(iat,4,old,&old);return 1;
  }
 }
 return 0;
}
#endif
static void __attribute__((fastcall)) hooked_draw(void *self,void *unused,void *destination,void *options) {
 unsigned long long perf_start=perf_now();
 (void)unused;unsigned matches=0;uint32_t *o=self;
 diagnostic_memory_tick();
 #ifdef TERMINAL_INPUT_FIXTURE
 if(readable(o,24)&&o[2]>=100&&o[2]<=300&&o[3]>=35&&o[3]<=150)terminal_input_fixture(self);
 #endif
 #ifdef REPAIR_POPUP_PROBE
 if(readable(o,24)&&o[2]>=100&&o[2]<=300&&o[3]>=35&&o[3]<=150)repair_popup_fixture(self);
 #endif
 #ifdef LIVE_RESOURCE_PROBE
 if(readable(o,24)&&o[2]>=100&&o[2]<=300&&o[3]>=35&&o[3]<=150)live_fixture(self);
 #endif
 #ifdef ZOOM_RESOURCE_PROBE
 if(readable(o,24)&&o[2]>=100&&o[2]<=300&&o[3]>=35&&o[3]<=150)zoom_fixture(self);
 #endif
 #ifdef SCRATCH_RESOURCE_PROBE
 if(readable(o,24)&&o[2]>=100&&o[2]<=200&&o[3]>=35&&o[3]<=70)scratch_fixture(self);
 #endif
 #ifdef MANUAL_RESOURCE_PROBE
 if(readable(o,24)&&o[2]>=100&&o[2]<=200&&o[3]>=35&&o[3]<=70)manual_fixture(self);
 #endif
 #ifdef DISPLAY_RESOURCE_PROBE
 if(readable(o,24)&&o[2]>=100&&o[2]<=200&&o[3]>=35&&o[3]<=70)display_fixture(self);
#ifdef INTRO_RESOURCE_PROBE
 if(readable(o,24)&&o[2]>=100&&o[2]<=200&&o[3]>=35&&o[3]<=70)intro_fixture(self);
#endif
 #endif
 #ifdef SHELL_PROBE
 if(readable(o,24)&&o[2]>=100&&o[2]<=200&&o[3]>=35&&o[3]<=70){create_shell_probe(self);queue_probe_tick(self);serial_probe_tick(self);check_shell_completion();}
 #endif
 if(readable(o,24)&&o[2]<=512&&o[3]<=512&&readable((void*)o[4],o[2]*o[3]*20)) {
  uint32_t *cells=(void*)o[4];
  for(unsigned i=0;i<o[2]*o[3];i++)if(cells[i*5+1]>=320&&cells[i*5+1]<=323&&cells[i*5]==0&&cells[i*5+2]==cells[i*5+1])matches++;
 }
 void *picker_view=terminal_input_draw_view(self);
 /* Native argument two is the previous-cell comparison canvas. A custom
    view must repaint blanks too, including the first frame after closing. */
 original_draw(self,destination,terminal_input_draw_background_full(self,picker_view)?NULL:options);
 if(picker_view)terminal_input_draw_overlay(self,destination,picker_view);
 perf_add(2,perf_start);if(readable(o,20)&&o[2]>=80&&o[3]>=25)perf_add(3,perf_start);
 if(matches && rendered++<16){
  fprintf(logfile,"NATIVE-DRAW self=%p grid=%ux%u CJK-half-cells=%u returned=1\n",self,o[2],o[3],matches);
  fflush(logfile);
 }
}
#endif
static void dump(const char *name,const void *p,unsigned n) {
 fprintf(logfile,"%s @%p",name,p);
 if(readable(p,n)) for(unsigned i=0;i<n;i+=4) fprintf(logfile," %08lx",(unsigned long)*(const uint32_t*)((const char*)p+i));
 else fputs(" unreadable",logfile);
 fputc('\n',logfile);
}
static void text_record(const char *kind,void *self,int x,int y,void *s) {
 if(!readable(s,24)) return;
 uint32_t len=*(uint32_t*)((char*)s+16),cap=*(uint32_t*)((char*)s+20);
 const unsigned char *data=cap<16?s:*(void**)s;
 fprintf(logfile,"%s #%u self=%p xy=%d,%d len=%lu cap=%lu text=",kind,calls,self,x,y,(unsigned long)len,(unsigned long)cap);
 if(len<=16384 && readable(data,len)) for(unsigned i=0;i<len;i++) {
  unsigned c=data[i]; if(c>=32&&c<127&&c!='\\') fputc(c,logfile); else fprintf(logfile,"\\x%02x",c);
 } else fputs("<invalid>",logfile);
 fputc('\n',logfile);
}
static void font_record(void) {
 dump("roles",(void*)va(0xd2248c),16);
 uint32_t *v=(void*)va(0xd2248c);
 if(readable((void*)v[0],v[1]-v[0]) && v[1]>=v[0] && v[1]-v[0]<=4096) dump("roles-data",(void*)v[0],v[1]-v[0]);
 dump("map-outer",(void*)va(0xd20ae8),16);
 dump("active",(void*)va(0xd224c0),4);
 void *active=*(void**)va(0xd224c0);
 dump("active-data",active,64);
 if(readable((char*)active+28,12)) {
  uint32_t *a=(void*)((char*)active+28);
  if(a[1]>=a[0] && a[1]-a[0]<=256 && readable((void*)a[0],a[1]-a[0])) {
   dump("atlas-vector",(void*)a[0],a[1]-a[0]);
   for(unsigned i=0;i<(a[1]-a[0])/4;i++) dump("atlas",((void**)a[0])[i],64);
  }
 }
}
/* Read-only inventory discovery. Retain distinct native prints after startup,
   without retaining game pointers or changing the strings/cells. */
static uint64_t print_seen[32768];static unsigned print_seen_count;
static void print_trace(void *self,int x,int y,void *s,uintptr_t caller){
 if(!readable(s,24)||!readable(self,24))return;
 uint32_t *v=s,*o=self;size_t n=v[4];const unsigned char *p=v[5]<16?s:*(void**)s;
 if(n<2||n>240||!readable(p,n)||print_seen_count>=24000)return;
 uint64_t h=1469598103934665603ULL;
 uintptr_t fields[]={(uintptr_t)self,caller,(uintptr_t)x,(uintptr_t)y};
 for(unsigned i=0;i<sizeof fields;i++){h^=((unsigned char*)fields)[i];h*=1099511628211ULL;}
 for(size_t i=0;i<n;i++){h^=p[i];h*=1099511628211ULL;}if(!h)h=1;
 unsigned slot=(unsigned)h&32767;while(print_seen[slot]&&print_seen[slot]!=h)slot=(slot+1)&32767;
 if(print_seen[slot])return;print_seen[slot]=h;print_seen_count++;
 fprintf(logfile,"PRINT-TRACE unique=%u ms=%lu caller=%08lx self=%p vt=%08lx grid=%ux%u font=%u xy=%d,%d raw=",print_seen_count,GetTickCount()-probe_start,(unsigned long)(caller-base+0x400000),self,(unsigned long)(o[0]-base+0x400000),o[2],o[3],o[5],x,y);
 for(size_t i=0;i<n;i++){unsigned c=p[i];if(c>=32&&c<127&&c!='\\')fputc(c,logfile);else fprintf(logfile,"\\x%02x",c);}fputc('\n',logfile);fflush(logfile);
}
static int inventory_name(void *self,int x,int y,void *s,uintptr_t caller){
 if(!item_dictionary||!readable(self,24)||!readable(s,24))return 0;
 uint32_t *o=self,*v=s;size_t capacity=v[4],n=capacity;const char *p=v[5]<16?s:*(void**)s;
 int list=caller==va(0x4aa4e8)&&o[0]==va(0xc3599c)&&(x==7||x==8);
 int equipped=caller==va(0x4a90eb)&&o[0]==va(0xc35794);
 int title=caller==va(0x4ae24e)&&o[0]==va(0xc35fe8)&&!x;
 if((!list&&!equipped&&!title)||o[5]||o[3]!=1||y||x<0||capacity>128||!capacity||x+(int)capacity>(int)o[2]||!readable(p,n))return 0;
 while(n&&p[n-1]==' ')n--;
 /* Title's native icon child is positioned from the English name width.
    Preserve its entire suffix and replace only the original name prefix. */
 if(title){
  for(size_t i=1;i+2<n;i++)if(p[i]==' '&&p[i+1]=='['&&p[n-1]==']'){
   int safe=1;for(size_t j=i+2;j<n-1;j++)if((unsigned char)p[j]<32||(unsigned char)p[j]>126||p[j]=='['||p[j]==']'||p[j]=='`')safe=0;
   if(!safe)return 0;capacity=n=i;break;
  }
 }
 char translated[512];
 RecordBinding binding={0};int bound=0;
 if((list||equipped)&&readable(self,0x70))bound=native_record_binding(*(uint32_t*)((char*)self+0x6c),&binding);
 TextEntry item_entry=list?TEXT_ENTRY_INVENTORY:equipped?TEXT_ENTRY_EQUIPPED:TEXT_ENTRY_ITEM_LABEL;
 TextObject object={item_entry,p,n,bound?&binding:NULL};
 if(message_resolve_object(&object,translated,sizeof translated,NULL)!=1)return 0;
 ZhPlan plan={0};if(message_plan_entry(item_entry,translated,strlen(translated),(int)capacity,1,&plan)){
  fprintf(logfile,"ITEM-NOFIT columns=%u source=%.*s\n",(unsigned)capacity,(int)n,p);fflush(logfile);return 0;
 }
 if(!extend_demo_fonts()){zh_plan_free(&plan);return 0;}
 original_text(self,x,y,s);LiveCellsSnapshot *live_item_snapshot=live_cells_begin(self,x,y,(int)capacity,1,0);
 /* Native printer clears the full padded region and applies its own colors.
    Replace only glyphs; neither inventory data nor state animations change. */
 for(unsigned col=0;col<capacity;col++)((PutCell)va(0x417f50))(self,x+(int)col,0,32);
 for(size_t i=0;i<plan.count;i++)for(int half=0;half<plan.runs[i].width;half++)
  ((PutCell)va(0x417f50))(self,x+plan.runs[i].x+half,0,plan.runs[i].token[half]);
 fprintf(logfile,"ITEM-NATIVE self=%p scope=%s columns=%u source=%.*s translated=%s\n",self,title?"title":equipped?"equipped":"inventory",(unsigned)capacity,(int)n,p,translated);fflush(logfile);
 if(live_item_snapshot)live_cells_commit(live_item_snapshot);
 zh_plan_free(&plan);return 1;
}
#include "display_bridge.h"
#ifdef REPAIR_POPUP_PROBE
#include "repair_popup_fixture.h"
#endif
#include "menu_compact.h"
#include "scratch_pipeline.h"
#include "memory_diagnostics.h"
#include "live_shell.h"
#include "live_language.h"
#ifdef TERMINAL_INPUT_IME
#include "terminal_input_native.h"
#include "terminal_input_fixture.h"
#else
static void terminal_input_tick(void){}
static int terminal_input_event(void *event){(void)event;return 0;}
static void *terminal_input_draw_view(void *owner){(void)owner;return NULL;}
static void terminal_input_draw_overlay(void *owner,void *destination,void *view){(void)owner;(void)destination;(void)view;}
static int terminal_input_draw_background_full(void *owner,void *view){(void)owner;(void)view;return 0;}
static void terminal_input_note_print(void *owner,void *text,const char *target){(void)owner;(void)text;(void)target;}
#endif
#include "log_content.h"
#include "live_shell_fixture.h"
static void __attribute__((fastcall)) hooked_text(void *self,void *unused,int x,int y,void *s) {
 (void)unused; if(live_native_english||(!live_chinese&&(constructing_owner||command_scope))){original_text(self,x,y,s);return;}unsigned id=++calls;
 uintptr_t caller=(uintptr_t)__builtin_return_address(0);print_trace(self,x,y,s,caller);
 if(log_content_print(self,x,y,s,caller))return;
 #ifndef MANUAL_BYTE_FIX_OFF
 if(scratch_row_print(self,x,y,s,caller))return;
 if(notification_literal_print(self,x,y,s,caller))return;
 if(caller==va(0x4931af)&&readable(self,0x70)&&*(uintptr_t*)self==va(0xc349c4)&&readable(s,24)){
  uint32_t *ns=s,*o=self;const char *raw=ns[5]<16?s:*(void**)s;size_t n=ns[4];int legacy=0;
  if(n==68&&o[2]==68&&o[3]==1&&o[5]==0&&x==0&&y==0&&readable(raw,n))for(size_t i=0;i<n;i++)if((unsigned char)raw[i]>=128||(unsigned char)raw[i]<32){legacy=1;break;}
  /* These are raw art tiles, including NUL, newline and backtick glyphs.
     They must never enter the native prose/format parser. */
  if(legacy){for(size_t i=0;i<n;i++)original_display_cell(self,(int)i,0,(unsigned char)raw[i]);
   fprintf(logfile,"MANUAL-ART bytes=%u unsigned-glyphs=1 parser-bypassed=1\n",(unsigned)n);fflush(logfile);return;
  }
 }
 #endif
 if(inventory_name(self,x,y,s,caller))return;
 int bridge_result;uintptr_t saved_site=display_origin_site;display_origin_site=caller;
 int handled=display_bridge(self,x,y,0,0,-1,0,s,&bridge_result);display_origin_site=saved_site;if(handled)return;
 if(id<=1000) text_record("TEXT",self,x,y,s);
 #ifdef SHELL_PROBE
 if(test_creating&&self==test_owner&&constructing_kind==1){typedef void (__attribute__((thiscall)) *SetColor)(void*,uint32_t);((SetColor)va(0x417b00))(self,0x0000ff00);}
 if(write_owned_translation(self,x,y,s))return;
 #endif
 #ifdef CHINESE_DEMO
 if(!try_demo(self,x,y,s))
 #endif
 original_text(self,x,y,s);
 if(id<=8) { dump("console",self,96);font_record(); }
 if(id<=1000) fflush(logfile);
}
static void* __attribute__((fastcall)) hooked_shell(void *self,void *unused,int a,void *s,int t) {
 (void)unused; text_record("SHELL-BEGIN",self,a,t,s);
 void *saved_owner=constructing_owner;int saved_kind=constructing_kind;constructing_owner=self;constructing_kind=t;
 void *r=original_shell(self,a,s,t);
 constructing_owner=saved_owner;constructing_kind=saved_kind;
 dump("SHELL-AFTER",self,128); fflush(logfile);return r;
}
#ifdef META_RESOURCE_PROBE
static LONG WINAPI fixture_exception(EXCEPTION_POINTERS *p){
 if(logfile){CONTEXT *c=p->ContextRecord;fprintf(logfile,"FIXTURE-EXCEPTION code=%lx eip=%lx eax=%lx ecx=%lx edx=%lx esp=%lx\n",p->ExceptionRecord->ExceptionCode,c->Eip,c->Eax,c->Ecx,c->Edx,c->Esp);dump("exception-stack",(void*)c->Esp,64);fflush(logfile);}return EXCEPTION_CONTINUE_SEARCH;
}
#endif
typedef void (__attribute__((thiscall)) *RepairFinishFn)(void*,int);
static RepairFinishFn original_repair_finish;static NativeVoid original_part_restore,original_part_clear;
static _Thread_local unsigned repair_finishing;
static void __attribute__((fastcall)) hooked_repair_finish(void *self,void *unused,int completed){
 (void)unused;unsigned type=readable(self,8)?((uint32_t*)self)[1]:0,prior=repair_finishing;
 if(type==0x4d){fprintf(logfile,"REPAIR-JOB-BEGIN job=%p completed=%d\n",self,completed);fflush(logfile);repair_finishing=!!completed;}
 original_repair_finish(self,completed);repair_finishing=prior;
 if(type==0x4d){fprintf(logfile,"REPAIR-JOB-END completed=%d\n",completed);fflush(logfile);}
}
static void __attribute__((fastcall)) hooked_part_restore(void *self,void *unused){
 (void)unused;unsigned before=readable(self,0x28)?((uint32_t*)self)[7]:0;original_part_restore(self);
 if(repair_finishing&&readable(self,0x28)){void *type=((void**)self)[2];unsigned maximum=readable(type,0xac)?((uint32_t*)type)[0xa8/4]:0;fprintf(logfile,"REPAIR-PART-INTEGRITY part=%p before=%u after=%u maximum=%u\n",self,before,((uint32_t*)self)[7],maximum);fflush(logfile);}
}
static void __attribute__((fastcall)) hooked_part_clear(void *self,void *unused){
 (void)unused;unsigned before=readable(self,0x28)?((uint32_t*)self)[9]:0;original_part_clear(self);
 if(repair_finishing&&readable(self,0x28)){fprintf(logfile,"REPAIR-PART-STATE part=%p before=%u after=%u\n",self,before,((uint32_t*)self)[9]);fflush(logfile);}
}
#ifdef DEBUG_REPAIR_SUCCESS
static int (__attribute__((thiscall)) *original_hack_chance)(void*,uint32_t,uint32_t,void*,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
static int __attribute__((fastcall)) hooked_hack_chance(void *self,void *unused,uint32_t a,uint32_t b,void *descriptor,uint32_t code,uint32_t e,uint32_t f,uint32_t g,uint32_t h,uint32_t i){
 (void)unused;uintptr_t caller=(uintptr_t)__builtin_return_address(0);uint32_t effective=descriptor&&readable(descriptor,4)?*(uint32_t*)descriptor:code;
 int chance=original_hack_chance(self,a,b,descriptor,code,e,f,g,h,i);
 if((effective==0x4c||effective==0x4d)&&(caller==va(0x91d409)||caller==va(0x8fb8cf)||caller==va(0x8fba8f))){
  if(caller==va(0x91d409)){fprintf(logfile,"DEBUG-REPAIR-SUCCESS-OVERRIDE command=%x original=%d forced=100 caller=%lx\n",effective,chance,(unsigned long)caller);fflush(logfile);}return 100;
 }
 return chance;
}
#endif
static void *hook(uintptr_t address,void *replacement) {
 unsigned char *p=(void*)va(address);
 const unsigned char seh[5]={0x55,0x8b,0xec,0x6a,0xff};
 unsigned char frame[6]={0x55,0x8b,0xec,0x83,0xec,0x0c};
 if(address==0x50e890)frame[5]=0x78;
 if(address==0x50f530)frame[5]=0x7c;
 if(address==0x50da50)frame[5]=0x3c;
 if(address==0x4ff050)frame[5]=0x14;
 if(address==0x431de0)frame[5]=8;
 if(address==0x90b850)frame[5]=0x78;
 if(address==0x418260||address==0x4182b0)frame[5]=0x10;if(address==0x429fe0)frame[5]=0x64;
 unsigned n=address==0x4181d0||address==0x50e890||address==0x50f530||address==0x50da50||address==0x50d500||address==0x4ff050||address==0x431de0||address==0x90b850||address==0x418260?6:5;
 if(address==0x4180b0||address==0x418220||address==0x4182b0||address==0x429fe0)n=6;
 unsigned char init_frame[9]={0x55,0x8b,0xec,0x81,0xec,0x80,0,0,0};
 if(address==0x50ced0)init_frame[5]=0x8c;if(address==0x42ded0)init_frame[5]=0xa0;
 const unsigned char short_frame[7]={0x55,0x8b,0xec,0x51,0x89,0x4d,0xfc};
 if(address==0x50de10||address==0x50ced0||address==0x42ded0)n=9;if(address==0x410420||address==0x4b0c00||address==0x416b40||address==0x48c5b0||address==0x458340||address==0x458460||address==0x44af50)n=7;
 const unsigned char *expected=n==9?init_frame:n==7?short_frame:n==6?frame:seh;
 if(memcmp(p,expected,n)) { fprintf(logfile,"hook refused %08lx\n",(unsigned long)address);fflush(logfile);return NULL; }
 unsigned char *tr=VirtualAlloc(NULL,16,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
 if(!tr) return NULL;
 memcpy(tr,p,n);tr[n]=0xe9;*(int32_t*)(tr+n+1)=(int32_t)((p+n)-(tr+n+5));
 DWORD old; if(!VirtualProtect(p,n,PAGE_EXECUTE_READWRITE,&old)) {VirtualFree(tr,0,MEM_RELEASE);return NULL;}
 p[0]=0xe9;*(int32_t*)(p+1)=(int32_t)((unsigned char*)replacement-(p+5));
 for(unsigned i=5;i<n;i++)p[i]=0x90;
 FlushInstructionCache(GetCurrentProcess(),p,n);VirtualProtect(p,n,old,&old);return tr;
}
__declspec(dllexport) DWORD WINAPI ProbeStart(void *unused) {
 (void)unused;base=(uintptr_t)GetModuleHandleW(NULL);
#ifdef TERMINAL_INPUT_IME
 /* Verified Beta 17.1 startup: push -1; call [ImmDisableIME].
    Preserve stack balance by replacing the entire eight-byte instruction pair. */
 {unsigned char expected[8]={0x6a,0xff,0xff,0x15,0x00,0x20,0xb6,0x00};
 unsigned char *site=(unsigned char*)va(0x422835);DWORD protection;
 if(memcmp(site,expected,8)==0&&VirtualProtect(site,8,PAGE_EXECUTE_READWRITE,&protection)){
  memset(site,0x90,8);FlushInstructionCache(GetCurrentProcess(),site,8);VirtualProtect(site,8,protection,&protection);
 }}
#endif

 logfile=fopen("native-probe.log","wb"); if(!logfile)return 1;
 fprintf(logfile,"TRANSLATION-UNIFIED-CONTRACT 20261019 success=1 exact-and-template=1\n");
 diagnostic_memory_start();live_cells_clear();live_cells_set_readable(readable);
 FILE *language_file=fopen("native-language.txt","rb");if(language_file){char language[8]={0};fread(language,1,7,language_file);fclose(language_file);live_chinese=memcmp(language,"en",2)!=0;}
 #ifdef META_RESOURCE_PROBE
 AddVectoredExceptionHandler(1,fixture_exception);
 #endif
 fprintf(logfile,"probe-v1 base=%p pid=%lu\n",(void*)base,GetCurrentProcessId());
 #ifdef DEBUG_REPAIR_SUCCESS
 fprintf(logfile,"DEBUG-MODE scan-repair-forced-success=1 production-acceptance=conditional\n");
 original_hack_chance=hook(0x71adc0,hooked_hack_chance);if(!original_hack_chance)return 1;
 #endif
 char path[MAX_PATH];DWORD path_length=GetModuleFileNameA(patch_module,path,sizeof path);if(!path_length||path_length>=sizeof path)return 1;
 char *slash=strrchr(path,'\\');if(!slash||(size_t)(slash-path)+sizeof "\\..\\zh_para.tsv">sizeof path)return 1;strcpy(slash,"\\..\\zh_para.tsv");
 size_t rules=0;char error[128];dictionary=zh_dictionary_load(path,&rules,error,sizeof error);
 fprintf(logfile,"DICTIONARY rules=%u glyphs=%u result=%s\n",(unsigned)rules,(unsigned)zh_registry_count,error);fflush(logfile);if(!dictionary||rules!=16378)return 1;
 strcpy(slash,"\\..\\item_names.tsv");size_t item_rules=0;item_dictionary=zh_dictionary_load(path,&item_rules,error,sizeof error);terminal_item_dictionary=item_dictionary;
 fprintf(logfile,"ITEM-DICTIONARY rules=%u result=%s exact-only=1\n",(unsigned)item_rules,error);fflush(logfile);if(!item_dictionary||item_rules<1045)return 1;
 strcpy(slash,"\\..\\zh_trans.tsv");size_t short_rules=0;short_dictionary=zh_dictionary_load(path,&short_rules,error,sizeof error);
 fprintf(logfile,"SHORT-DICTIONARY rules=%u result=%s exact-only-ui=1\n",(unsigned)short_rules,error);fflush(logfile);if(!short_dictionary||short_rules!=56249)return 1;
 strcpy(slash,"\\..\\zh_extra.tsv");size_t extra_rules=0;display_extra_dictionary=zh_dictionary_load(path,&extra_rules,error,sizeof error);
 repair_extra_dictionary=display_extra_dictionary;repair_short_dictionary=short_dictionary;repair_paragraph_dictionary=dictionary;
 fprintf(logfile,"EXTRA-DICTIONARY rules=%u result=%s\n",(unsigned)extra_rules,error);fflush(logfile);if(!display_extra_dictionary)return 1;
 strcpy(slash,"\\..\\runtime_patches.tsv");size_t runtime_rules=0;terminal_runtime_dictionary=zh_dictionary_load(path,&runtime_rules,error,sizeof error);
 fprintf(logfile,"RUNTIME-DICTIONARY rules=%u result=%s\n",(unsigned)runtime_rules,error);if(!terminal_runtime_dictionary)return 1;
 if(!menu_compact_install())return 1;
 if(!log_content_install())return 1;
 #ifdef CHINESE_DEMO
 original_prepare_set=hook(0x431de0,hooked_prepare_set);if(!original_prepare_set)return 1;
 original_release_atlas=hook(0x416b40,hooked_release_atlas);if(!original_release_atlas)return 1;
 #endif
 original_text=hook(0x4181d0,hooked_text);
 original_display_cell=hook(0x4180b0,hooked_display_cell);original_align_text=hook(0x418220,hooked_align_text);original_align_paragraph=hook(0x4182b0,hooked_align_paragraph);
 if(!original_display_cell||!original_align_text||!original_align_paragraph)return 1;
 original_shell=hook(0x4b0b30,hooked_shell);
 #ifdef SHELL_PROBE
 probe_start=GetTickCount();original_advance=hook(0x50e890,hooked_advance);if(!original_advance)return 1;
 original_compose=hook(0x50f530,hooked_compose);original_finalize=hook(0x50da50,hooked_finalize);if(!original_compose||!original_finalize)return 1;
 original_console_ctor=hook(0x4282e0,hooked_console_ctor);if(!original_console_ctor)return 1;
 original_animate=hook(0x90b850,hooked_animate);original_delete=hook(0x4b0c00,hooked_delete);if(!original_animate||!original_delete)return 1;
 original_add_line=hook(0x90ed30,hooked_add_line);if(!original_add_line)return 1;
 original_meta=hook(0x90ec30,hooked_meta);if(!original_meta)return 1;
 original_repair_finish=hook(0x65a260,hooked_repair_finish);original_part_restore=hook(0x458340,hooked_part_restore);original_part_clear=hook(0x458460,hooked_part_clear);
 if(!original_repair_finish||!original_part_restore||!original_part_clear)return 1;
 original_owner_compose=hook(0x48c5b0,hooked_owner_compose);if(!original_owner_compose)return 1;
 original_paragraph=hook(0x418260,hooked_paragraph);original_extract=hook(0x4177a0,hooked_extract);original_command=hook(0x90d550,hooked_command);if(!original_paragraph||!original_extract||!original_command)return 1;
 original_init=hook(0x50de10,hooked_init);original_path_step=hook(0x410420,hooked_path_step);if(!original_init||!original_path_step)return 1;
 original_sound_action=hook(0x50d500,hooked_sound_action);original_play_sound=hook(0x4ff050,hooked_play_sound);original_events=hook(0x50ced0,hooked_events);
 if(!original_sound_action||!original_play_sound||!original_events)return 1;
 #endif
 #ifdef CHINESE_DEMO
 original_draw=hook(0x42a5b0,hooked_draw);
 if(!original_draw)return 1;
 #ifdef CAPTURE_DIAGNOSTICS
 capture_start=GetTickCount();if(!capture_import())return 1;
 #endif
 #endif
 if(!live_shell_install()||!live_install())return 1;
 fprintf(logfile,"STEAM-LAYOUT-READY profile=260906a mapped=155 all-hooks-installed=1\n");fflush(logfile);return !original_text || !original_shell;
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID reserved) {
 (void)reserved; if(reason==DLL_PROCESS_ATTACH){patch_module=h;DisableThreadLibraryCalls(h);}return TRUE;
}

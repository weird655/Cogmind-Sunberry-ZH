/* Shared display translation for all four native text wrappers. Input game
   strings remain untouched; each wrapper owns only its new carrier string. */
#include "display_text.h"
typedef void (__attribute__((thiscall)) *AlignTextFn)(void*,int,int,int,void*);
typedef int (__attribute__((thiscall)) *AlignParagraphFn)(void*,int,int,int,int,int,void*);
static AlignTextFn original_align_text;
static AlignParagraphFn original_align_paragraph;
static PutCell original_display_cell;
typedef struct {void *owner;int x,y,columns,rows,align;int *tokens,*widths;unsigned writes,errors;} DisplayContext;
static _Thread_local DisplayContext *display_context;
static _Thread_local int display_busy;
static unsigned display_translations,display_fallbacks;
static uint64_t display_logged[8192];static unsigned display_log_count;
static int display_log_once(void *owner,int mode,const char *text){
 if(display_log_count>=6000)return 0;uint64_t h=1469598103934665603ULL;uintptr_t fields[]={(uintptr_t)owner,(uintptr_t)mode};
 for(unsigned i=0;i<sizeof fields;i++){h^=((unsigned char*)fields)[i];h*=1099511628211ULL;}
 for(const unsigned char *p=(const void*)text;*p;p++){h^=*p;h*=1099511628211ULL;}if(!h)h=1;
 unsigned slot=(unsigned)h&8191;while(display_logged[slot]&&display_logged[slot]!=h)slot=(slot+1)&8191;if(display_logged[slot])return 0;
 display_logged[slot]=h;display_log_count++;return 1;
}
static void __attribute__((fastcall)) hooked_display_cell(void *self,void *unused,int x,int y,int glyph){
 (void)unused;DisplayContext *c=display_context;
 if(c&&self==c->owner&&(uintptr_t)__builtin_return_address(0)==va(0x42970a)){
  int row=y-c->y;if(row<0||row>=c->rows){c->errors++;return;}
  int start=c->x-(c->align==1?c->widths[row]/2:c->align==2?c->widths[row]-1:0),col=x-start;
  if(col<0||col>=c->widths[row]||glyph!='@'){c->errors++;return;}
  glyph=c->tokens[row*c->columns+col];c->writes++;
 }
 original_display_cell(self,x,y,glyph);
}
static int display_original(void *self,int x,int y,int width,int maxrows,int align,int mode,void *s){
 if(mode==0){original_text(self,x,y,s);return 0;}
 if(mode==1){original_align_text(self,x,y,align,s);return 0;}
 if(mode==2)return original_paragraph(self,x,y,width,maxrows,s);
 return original_align_paragraph(self,x,y,width,maxrows,align,s);
}
static int display_bridge_impl(void *self,int x,int y,int width,int maxrows,int align,int mode,void *s,int *result){
 if(live_native_english||display_busy||display_context||!readable(self,0x3c)||!readable(s,24))return 0;
 /* Owned final log rows / final notification widgets are display-only. Their
    validated sidecars may render while a terminal command owns its scratch. */
 uint32_t *owned=self;int owned_literal=display_owned_literal_scope&&mode==0&&owned[5]==0&&(owned[0]==va(0xc344b0)||owned[0]==va(0xc2ef14));
 if((constructing_owner||command_scope||serial_context)&&!owned_literal)return 0;
 uint32_t *o=self,*ns=s;size_t n=ns[4];const char *raw=ns[5]<16?s:*(void**)s;
 if(n>=20&&n<500&&readable(raw,n)&&(!memcmp(raw,"See the manual (in ",19)||!memcmp(raw,"You can change your ",20))){static unsigned footer_trace;if(footer_trace++<4){fprintf(logfile,"DIFFICULTY-FOOTER mode=%d vt=%08lx font=%u grid=%ux%u xy=%d,%d width=%d rows=%d raw=%.*s\n",mode,(unsigned long)(o[0]-base+0x400000),o[5],o[2],o[3],x,y,width,maxrows,(int)n,raw);fflush(logfile);}}
 int intro=o[5]==2&&o[0]==va(0xc342dc)&&mode==0&&display_origin_site==va(0x48cfc2);
 int zoom=o[5]==1&&o[0]==va(0xc2ef14)&&mode==0&&display_origin_site==va(0x8118a6);
 if((o[5]&&!intro&&!zoom)||!n||n>16384||o[2]>512||o[3]>512||x<0||y<0||x>=(int)o[2]||y>=(int)o[3]||!readable(raw,n))return 0;
 if(align<0)align=(int)o[0x38/4];if(align<0||align>2)return 0;
 int available=align==2?x+1:align==1?(int)o[2]:(int)o[2]-x;
 int columns=width>0&&width<available?width:available;
 int rows=mode<2?1:(maxrows>0&&maxrows<(int)o[3]-y?maxrows:(int)o[3]-y);if(rows>128)rows=128;
 int credits=mode==1&&o[0]==va(0xc34bcc)&&n>=55&&!memcmp(raw,"(c) All Rights Reserved by Josh Ge and Grid Sage Games ",55);
 int full_log_line=mode==0&&o[0]==va(0xc344b0)&&!x&&!y&&o[3]==1;
 int footprint=ui_visible_width(raw,n);if(mode<2&&!credits&&!full_log_line&&!display_override_target&&footprint>0&&footprint<columns)columns=footprint;
 if(columns<1||rows<1)return 0;
 /* Unscoped base consoles may be byte readback buffers. Keep them English
    until their readback boundary has an explicit Unicode owner. */
 if(mode>=2&&o[0]==va(0xc2ef14)&&!display_override_target)return 0;
 display_busy=1;char translated[32768];int match;unsigned long long lookup_start=perf_now();
 if(display_override_target){size_t length=strlen(display_override_target);match=length<sizeof translated?1:-1;if(match==1)memcpy(translated,display_override_target,length+1);}
 else match=message_resolve(raw,n,translated,sizeof translated);
 perf_add(0,lookup_start);if(match!=1){display_busy=0;return 0;}
 if(credits){const char *credit="  汉化组：太阳莓";size_t current=strlen(translated),extra=strlen(credit);if(current+extra<sizeof translated)memcpy(translated+current,credit,extra+1);}
 if(display_override_target&&!translated[0])strcpy(translated," ");
 ZhPlan plan={0};if(zh_plan(translated,strlen(translated),columns,rows,&plan)||!(intro?intro_font_extend():zoom?zoom_font_extend():extend_demo_fonts())){zh_plan_free(&plan);display_busy=0;return 0;}
 if(!plan.used_rows){zh_plan_free(&plan);display_busy=0;return 0;}
 DisplayContext ctx={0};ctx.owner=self;ctx.x=x;ctx.y=y;ctx.align=align;ctx.columns=columns;ctx.rows=plan.used_rows;
 ctx.tokens=malloc((size_t)ctx.rows*columns*sizeof(int));ctx.widths=calloc(ctx.rows,sizeof(int));char *carrier=malloc(65536);
 if(!ctx.tokens||!ctx.widths||!carrier){free(ctx.tokens);free(ctx.widths);free(carrier);zh_plan_free(&plan);display_busy=0;return 0;}
 for(int i=0;i<ctx.rows*columns;i++)ctx.tokens[i]=32;
 for(size_t i=0;i<plan.count;i++){ZhRun *r=&plan.runs[i];int end=r->x+r->width;if(end>ctx.widths[r->y])ctx.widths[r->y]=end;for(int half=0;half<r->width;half++)ctx.tokens[r->y*columns+r->x+half]=(int)r->token[half]+(intro&&r->token[half]>=320?352:0);}
 /* Keep single-line widgets' original footprint, so old labels are cleared
    and native positioning/adjacent columns stay exactly bounded. */
 if(mode<2){int used=ctx.widths[0],shift=align==2?columns-used:align==1?(columns-used)/2:0;
  if(shift){memmove(ctx.tokens+shift,ctx.tokens,used*sizeof(int));for(int i=0;i<shift;i++)ctx.tokens[i]=32;for(size_t i=0;i<plan.count;i++)plan.runs[i].x+=shift;}
  ctx.widths[0]=columns;
 }
 size_t at=0,run=0;int ready=1;
 for(int row=0;ready&&row<ctx.rows;row++){
  int start=x-(align==1?ctx.widths[row]/2:align==2?ctx.widths[row]-1:0);
  if(start<0||start+ctx.widths[row]>(int)o[2]){ready=0;break;}
  unsigned last_style=~0u;
  for(int col=0;ready&&col<ctx.widths[row];col++){
   while(run<plan.count&&(plan.runs[run].y<row||(plan.runs[run].y==row&&plan.runs[run].x+plan.runs[run].width<=col)))run++;
   unsigned style=run<plan.count&&plan.runs[run].y==row&&plan.runs[run].x<=col?plan.runs[run].style:0;
   if(style!=last_style){const char *fmt=plan.styles[style];if(at+strlen(fmt)+4>=65536){ready=0;break;}memcpy(carrier+at,"`x`",3);at+=3;memcpy(carrier+at,fmt,strlen(fmt));at+=strlen(fmt);last_style=style;}
   if(at+2>=65536){ready=0;break;}carrier[at++]='@';
  }
  if(row+1<ctx.rows&&ready)carrier[at++]='\n';
 }
 carrier[at]=0;
 if(ready){
  size_t live_grid_size=(size_t)o[2]*o[3]*20;void *live_old_grid=mode>=2?malloc(live_grid_size):NULL;
  if(live_old_grid)memcpy(live_old_grid,(void*)o[4],live_grid_size);
  unsigned char live_english_string[28]={0};void *live_english_source=s;
  if(display_override_english){((StringCtor)va(0x9af0f0))(live_english_string,display_override_english);live_english_source=live_english_string;}
  int live_english_rows=display_original(self,x,y,width,maxrows,align,mode,live_english_source);
  if(display_override_english)((NativeVoid)va(0x9af350))(live_english_string);
  int live_height=mode>=2&&live_english_rows>ctx.rows?live_english_rows:ctx.rows;
  LiveCellsSnapshot *live_snapshot=live_cells_begin(self,x,y,columns,live_height,align);
  if(live_old_grid){memcpy((void*)o[4],live_old_grid,live_grid_size);free(live_old_grid);}
  if(!live_snapshot){*result=display_original(self,x,y,width,maxrows,align,mode,s);free(ctx.tokens);free(ctx.widths);free(carrier);zh_plan_free(&plan);display_busy=0;return 1;}
  unsigned char native[28]={0};((StringCtor)va(0x9af0f0))(native,carrier);DisplayContext *saved=display_context;display_context=&ctx;
  *result=display_original(self,x,y,width,maxrows,align,mode,native);display_context=saved;((NativeVoid)va(0x9af350))(native);
  unsigned expected=0;for(int i=0;i<ctx.rows;i++)expected+=(unsigned)ctx.widths[i];
  if(ctx.errors||ctx.writes!=expected){
   display_fallbacks++;fprintf(logfile,"DISPLAY-MAP-FALLBACK mode=%d rows=%d expected=%u actual=%u errors=%u\n",mode,ctx.rows,expected,ctx.writes,ctx.errors);
   for(int row=0;row<ctx.rows;row++){int start=x-(align==1?ctx.widths[row]/2:align==2?ctx.widths[row]-1:0);for(int col=0;col<ctx.widths[row];col++)original_display_cell(self,start+col,y+row,32);}
   *result=display_original(self,x,y,width,maxrows,align,mode,s);
  }else{
   if(mode==0&&display_origin_site==va(0x8fc91d))menu_compact_note_print(self,x,y,s,translated);
   display_translations++;if(display_log_once(self,mode,translated))fprintf(logfile,"DISPLAY-NATIVE count=%u mode=%d vt=%08lx xy=%d,%d align=%d rows=%d writes=%u text=%s\n",display_translations,mode,(unsigned long)(o[0]-base+0x400000),x,y,align,ctx.rows,ctx.writes,translated);
  }
  if(live_snapshot){if(ctx.errors||ctx.writes!=expected)live_cells_cancel(live_snapshot);else live_cells_commit(live_snapshot);}
  fflush(logfile);
 }
 free(ctx.tokens);free(ctx.widths);free(carrier);zh_plan_free(&plan);display_busy=0;return ready;
}
static int display_bridge(void *self,int x,int y,int width,int maxrows,int align,int mode,void *s,int *result){unsigned long long start=perf_now();int r=display_bridge_impl(self,x,y,width,maxrows,align,mode,s,result);perf_add(1,start);return r;}
static void __attribute__((fastcall)) hooked_align_text(void *self,void *unused,int x,int y,int align,void *s){
 (void)unused;int result;if(!display_bridge(self,x,y,0,0,align,1,s,&result))original_align_text(self,x,y,align,s);
}
static int __attribute__((fastcall)) hooked_align_paragraph(void *self,void *unused,int x,int y,int width,int maxrows,int align,void *s){
 (void)unused;int result;if(display_bridge(self,x,y,width,maxrows,align,3,s,&result))return result;return original_align_paragraph(self,x,y,width,maxrows,align,s);
}
#ifdef DISPLAY_RESOURCE_PROBE
static void display_fixture(void *parent){
 static int done;if(done||GetTickCount()-probe_start<6000)return;done=1;
 typedef void (__cdecl *Caption)(const char*,const char*);Caption caption=(Caption)GetProcAddress(GetModuleHandleW(L"SDL.dll"),"SDL_WM_SetCaption");if(caption)caption("Cogmind translation component test - closes automatically",NULL);
 const char *sources[]={"Type","Overview","Active Upkeep","Chain reaction triggered in Lgt. Nuclear Core.","Increases inventory capacity by 20.\n<no_stack>"," CORE   272/700"," TEMP `f0` Warm `x` Heat 130","Increases inventory capacity by 20.\n<no_stack>","34 MATTER","Record! \"Derelict Rail Weapons\"","Schematic \"Imp. Coolant Injector\""," MOVEMENT: Rolling `f0` SLOW `x` (62%)"};
 unsigned before=vector_count((char*)parent+0x44),passed=0;
 for(unsigned i=0;i<sizeof sources/sizeof sources[0];i++){
  void *object=(*(NativeNew*)va(0xb6132c))(0x128);memset(object,0,0x128);((ConsoleCtor)va(0x48c060))(object,parent,80,12,0,0,0,1,20);
  unsigned char s[28]={0};((StringCtor)va(0x9af0f0))(s,sources[i]);unsigned prior=display_translations;int result=0;
  if(i<3)((AlignTextFn)va(0x418220))(object,i==2?30:21,0,i==2?1:2,s);
  else if(i==4)result=((ParagraphFn)va(0x418260))(object,0,0,46,8,s);
  else if(i==7)result=((AlignParagraphFn)va(0x4182b0))(object,39,0,46,8,1,s);
  else ((TextFn)va(0x4181d0))(object,0,0,s);
  ((NativeVoid)va(0x9af350))(s);uint32_t *o=object;unsigned chinese=0,pair=0;
  for(unsigned row=0;row<o[3];row++)for(unsigned col=0;col<o[2];col++){
   unsigned char *cell=(void*)(o[4]+(col*o[3]+row)*20);unsigned glyph=*(uint32_t*)(cell+4);unsigned stride=o[3]*20;
   if(glyph>=320&&glyph<320+2*zh_registry_count&&!(glyph&1)){
    if(chinese<3)fprintf(logfile,"DISPLAY-FIXTURE-CELL case=%u xy=%u,%u head=%u next=%u color-diff=%d\n",i,col,row,glyph,col+1<o[2]?*(uint32_t*)(cell+stride+4):0,col+1<o[2]?memcmp(cell+12,cell+stride+12,6):-1);
    chinese++;if(col+1>=o[2]||*(uint32_t*)(cell+stride+4)!=glyph+1||memcmp(cell+12,cell+stride+12,6))pair++;
   }
  }
  int ok=display_translations>prior&&chinese>0&&!pair;
  if(i==4||i==7)ok=display_translations==prior&&chinese==0&&result>0;
  passed+=ok;
  fprintf(logfile,"DISPLAY-FIXTURE case=%u passed=%d chinese=%u pair-errors=%u result=%d source=%s\n",i,ok,chinese,pair,result,sources[i]);fflush(logfile);
  uint32_t *pv=(void*)((char*)parent+0x44);unsigned count=vector_count(pv);for(unsigned j=0;j<count;j++)if(((void**)pv[0])[j]==object){memmove((void*)(pv[0]+j*4),(void*)(pv[0]+(j+1)*4),(count-j-1)*4);pv[1]-=4;break;}
  ((DeleteFn)(*(uintptr_t**)object)[0])(object,1);
 }
 unsigned after=vector_count((char*)parent+0x44);fprintf(logfile,"DISPLAY-FIXTURE-SUMMARY passed=%u expected=12 child-before=%u child-after=%u fallback=%u\n",passed,before,after,display_fallbacks);fflush(logfile);
}
#endif

#ifdef INTRO_RESOURCE_PROBE
static void intro_fixture(void *parent){
 static void *objects[3];static int state;static DWORD born;DWORD now=GetTickCount();
 if(!state&&now-probe_start>=6500){
  const char *sources[]={"SEQUENCE INITIATED","REGENERATING NEURAL FIBERS","PREPARING CONTROL OVERLAY"};
  typedef void* (__attribute__((thiscall)) *IntroTextCtor)(void*,void*,const int*,void*,int,int,int);
  typedef void (__attribute__((thiscall)) *NamedAnimation)(void*,NativeStringValue);
  for(unsigned i=0;i<3;i++){
   unsigned char source[28]={0};((StringCtor)va(0x9af0f0))(source,sources[i]);int point[2]={2,2+(int)i};
   void *o=(*(NativeNew*)va(0xb6132c))(0x88);objects[i]=o;((IntroTextCtor)va(0x48cf10))(o,parent,point,source,2,0,10);((NativeVoid)va(0x9af350))(source);
   NativeStringValue config={0};((StringCtor)va(0x9af0f0))(&config,"A_CIntro_Text");((NamedAnimation)va(0x48c3f0))(o,config);((NativeVoid)va(0x9af350))(&config);
  }
  state=1;born=now;fprintf(logfile,"INTRO-FIXTURE-CREATED count=3\n");fflush(logfile);
 }else if(state==1&&now-born>=3000){
  unsigned passed=0;
  for(unsigned i=0;i<3;i++){
   uint32_t *o=objects[i];unsigned chinese=0,bad=0;for(unsigned x=0;x<o[2];x++){
    unsigned char *cell=(void*)(o[4]+x*20);unsigned g=*(uint32_t*)(cell+4);
    if(g>=672&&g<672+2*zh_registry_count&&!(g&1)){chinese++;if(x+1>=o[2]||*(uint32_t*)(cell+24)!=g+1||memcmp(cell+12,cell+32,6))bad++;}
   }
   int ok=chinese>0&&!bad;passed+=ok;fprintf(logfile,"INTRO-FIXTURE case=%u passed=%d chinese=%u pair-errors=%u font=%u\n",i,ok,chinese,bad,o[5]);
   uint32_t *pv=(void*)((char*)parent+0x44);unsigned count=vector_count(pv);for(unsigned j=0;j<count;j++)if(((void**)pv[0])[j]==o){memmove((void*)(pv[0]+j*4),(void*)(pv[0]+(j+1)*4),(count-j-1)*4);pv[1]-=4;break;}
   ((DeleteFn)(*(uintptr_t**)o)[0])(o,1);
  }
  state=2;fprintf(logfile,"INTRO-FIXTURE-SUMMARY passed=%u expected=3\n",passed);fflush(logfile);
 }
}
#endif

#ifdef MANUAL_RESOURCE_PROBE
static void manual_fixture(void *parent){
 static int done;if(done||GetTickCount()-probe_start<6500)return;done=1;
 typedef void* (__attribute__((thiscall)) *ManualCtor)(void*,void*,int,void*,unsigned char);
 const unsigned char art[]={102,103,142,143,2,3,66,67,204,205,154,155,24,25,112,113,150,151,138,139,84,85,210,211,34,35,130,131,48,49,194,195,120,121,96,97,88,89,86,87,250,251,96,97,204,205,26,27,224,225,10,11,156,157,138,139,184,185,130,131,196,197,194,195,212,213,76,77,0};
 unsigned before=vector_count((char*)parent+0x44);unsigned char source[28]={0};((StringCtor)va(0x9af0f0))(source,(const char*)art);
 /* Match the actual manual row's embedded NUL without truncating std::string. */
 ((unsigned char*)*(void**)source)[10]=0;
 void *o=(*(NativeNew*)va(0xb6132c))(0x70);fprintf(logfile,"MANUAL-FIXTURE-BEGIN bytes=68\n");fflush(logfile);
 ((ManualCtor)va(0x493010))(o,parent,0,source,0);((NativeVoid)va(0x9af350))(source);
 uint32_t *v=o;unsigned a=*(uint32_t*)(v[4]+2*20+4),b=*(uint32_t*)(v[4]+3*20+4);int ok=1;for(unsigned j=0;j<68;j++)if(*(uint32_t*)(v[4]+j*20+4)!=(j==10?0:art[j]))ok=0;
 fprintf(logfile,"MANUAL-FIXTURE passed=%d glyph-2=%u glyph-3=%u\n",ok,a,b);
 uint32_t *pv=(void*)((char*)parent+0x44);unsigned count=vector_count(pv);for(unsigned j=0;j<count;j++)if(((void**)pv[0])[j]==o){memmove((void*)(pv[0]+j*4),(void*)(pv[0]+(j+1)*4),(count-j-1)*4);pv[1]-=4;break;}
 ((DeleteFn)(*(uintptr_t**)o)[0])(o,1);
 fprintf(logfile,"MANUAL-FIXTURE-SUMMARY passed=%d child-before=%u child-after=%u\n",ok,before,vector_count((char*)parent+0x44));fflush(logfile);
}
#endif

#ifdef ZOOM_RESOURCE_PROBE
static void zoom_fixture(void *parent){
 static int done;if(done||GetTickCount()-probe_start<7000)return;done=1;unsigned before=vector_count((char*)parent+0x44),passed=0;
 const char *sources[]={"40 MATTER","MIC. NUCLEAR CORE [4]"};
 for(unsigned i=0;i<2;i++){
  void *o=(*(NativeNew*)va(0xb6132c))(0x6c);((ConsoleCtor)va(0x48c060))(o,parent,40,1,0,0,1,1,-1);
  unsigned char source[28]={0};((StringCtor)va(0x9af0f0))(source,sources[i]);uintptr_t saved=display_origin_site;display_origin_site=va(0x8118a6);int result;int ok=display_bridge(o,4,0,0,0,-1,0,source,&result);display_origin_site=saved;((NativeVoid)va(0x9af350))(source);
  uint32_t *v=o;unsigned chinese=0,bad=0;for(unsigned x=0;x<v[2];x++){unsigned char *cell=(void*)(v[4]+x*20);unsigned g=*(uint32_t*)(cell+4);if(g>=320&&g<320+2*zh_registry_count&&!(g&1)){chinese++;if(x+1>=v[2]||*(uint32_t*)(cell+24)!=g+1||memcmp(cell+12,cell+32,6))bad++;}}
  ok=ok&&chinese>0&&!bad;passed+=ok;fprintf(logfile,"ZOOM-FIXTURE case=%u passed=%d font=%u chinese=%u pair-errors=%u\n",i,ok,v[5],chinese,bad);
  uint32_t *pv=(void*)((char*)parent+0x44);unsigned count=vector_count(pv);for(unsigned j=0;j<count;j++)if(((void**)pv[0])[j]==o){memmove((void*)(pv[0]+j*4),(void*)(pv[0]+(j+1)*4),(count-j-1)*4);pv[1]-=4;break;}((DeleteFn)(*(uintptr_t**)o)[0])(o,1);
 }
 fprintf(logfile,"ZOOM-FIXTURE-SUMMARY passed=%u expected=2 child-before=%u child-after=%u\n",passed,before,vector_count((char*)parent+0x44));fflush(logfile);
}
#endif

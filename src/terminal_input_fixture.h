#ifdef TERMINAL_INPUT_FIXTURE
/* Own, detached native controls. Gameplay globals and hack callbacks are never
   dispatched; the native-key boundary is counted by the fixture event sink. */
static void terminal_input_fixture(void *parent){
 static int stage;static DWORD born;static void *shell,*manual,*widget,*target;static unsigned errors;
 if(stage==3)return;
 if(!stage){
  if(!parent||GetTickCount()-probe_start<6000)return;
  shell=(*(NativeNew*)va(0xb6132c))(0x128);manual=(*(NativeNew*)va(0xb6132c))(0xc4);target=(*(NativeNew*)va(0xb6132c))(0x128);widget=(*(NativeNew*)va(0xb6132c))(0xe4);
  if(!shell||!manual||!target||!widget)ExitProcess(90);
  memset(shell,0,0x128);memset(manual,0,0xc4);memset(target,0,0x128);memset(widget,0,0xe4);
  ((ConsoleCtor)va(0x48c060))(shell,parent,50,10,2,2,0,0,20);*(uint32_t*)shell=va(0xc36224);
  /* Detach without deletion: no native update method can consume mock state. */
  uint32_t *v=(void*)((char*)parent+0x44);unsigned n=vector_count(v);for(unsigned j=0;j<n;j++)if(((void**)v[0])[j]==shell){memmove((void*)(v[0]+j*4),(void*)(v[0]+(j+1)*4),(n-j-1)*4);v[1]-=4;break;}
  ((ConsoleCtor)va(0x48c060))(manual,shell,46,1,2,1,0,0,20);*(uint32_t*)manual=va(0xc362c0);((uint32_t*)shell)[0xa4/4]=(uint32_t)(uintptr_t)manual;
  ((ConsoleCtor)va(0x48c060))(target,shell,50,10,0,0,0,0,20);*(uint32_t*)target=va(0xc361f0);((uint32_t*)target)[0x70/4]=0x10001;
  typedef void* (__attribute__((thiscall)) *Ctor)(void*,void*,int,int,int,int,int,int,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
  ((Ctor)va(0x48d130))(widget,manual,0,0,44,0,0,0,va(0x900920),va(0x8fec50),va(0x4b1c30),0,0,va(0x909990));
  typedef void (__attribute__((thiscall)) *Limit)(void*,int);((Limit)va(0x4544c0))(widget,44);((uint32_t*)manual)[0x6c/4]=(uint32_t)(uintptr_t)widget;
  uint32_t *ids=(*(NativeNew*)va(0xb6132c))(4);*ids=31;uint32_t *list=(void*)((char*)manual+0x70);list[0]=(uint32_t)(uintptr_t)ids;list[1]=list[2]=list[0]+4;
  /* Detached native render owners: repaint translated rows repeatedly, then
     compare every reconstructed English glyph with a pristine native owner. */
  {void *actual=(*(NativeNew*)va(0xb6132c))(0xc4),*reference=(*(NativeNew*)va(0xb6132c))(0xc4);
   ((ConsoleCtor)va(0x48c060))(actual,shell,50,4,0,0,0,0,20);((ConsoleCtor)va(0x48c060))(reference,shell,50,4,0,0,0,0,20);
   const char *samples[]={"Scanning nodes...","Network defenses at 100%...","Terminal M005 - Restricted Access","/ R E S U L T S /","Assault Status"};unsigned repaint_errors=0,handled=0;
   for(unsigned sample=0;sample<5;sample++){
    live_cells_remove(actual);for(unsigned x=0;x<50;x++)for(unsigned y=0;y<4;y++){original_display_cell(actual,x,y,32);original_display_cell(reference,x,y,32);}
    NativeStringValue text={{0}};((StringCtor)va(0x9af0f0))(&text,samples[sample]);int mode=sample==3?1:0,x=mode?25:0;
    display_original(reference,x,0,0,0,mode,mode,&text);
    for(unsigned pass=0;pass<12;pass++){
     int result;int ok=display_bridge_impl(actual,x,0,0,0,mode,mode,&text,&result);handled+=ok;
     unsigned char *english=live_cells_view(actual);if(!english){repaint_errors++;continue;}
     for(unsigned cell=0;cell<200;cell++)repaint_errors+=*(uint32_t*)(english+cell*20+4)!=*(uint32_t*)(((uint32_t*)reference)[4]+cell*20+4);
     free(english);
    }
    ((NativeVoid)va(0x9af350))(&text);
   }
   fprintf(logfile,"IME-FIXTURE-REPAINT passes=60 handled=%u glyph-errors=%u\n",handled,repaint_errors);errors+=repaint_errors;fflush(logfile);
  }
  input_fixture_shell=(uint32_t)(uintptr_t)shell;input_fixture_target=(uint32_t)(uintptr_t)target;
  terminal_input_tick();errors+=!input_edit||!IsWindowUnicode(input_edit)||input_registry.count==0;
  SetWindowTextW(input_edit,L"\u4e0d\u5728\u5f53\u524d\u547d\u4ee4\u96c6\u5408");input_pending=1;terminal_input_tick();char original[INPUT_NATIVE];errors+=!input_ns((uintptr_t)widget+0x6c,original,sizeof original)||original[0]||input_fixture_events;
  wchar_t known[256];int converted=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,input_registry.rows[0].alias,-1,known,256);errors+=converted<=0;
  for(unsigned j=0;known[j];j++){if(known[j]==40)known[j]=0xff08;else if(known[j]==41)known[j]=0xff09;}SetWindowTextW(input_edit,known);input_composing=1;SendMessageW(input_edit,WM_KEYDOWN,VK_RETURN,0);errors+=input_pending!=0||input_fixture_events;
  input_composing=0;input_committed_at=GetTickCount();SendMessageW(input_edit,WM_KEYDOWN,VK_RETURN,0);errors+=input_pending!=0;
  born=GetTickCount();stage=1;return;
 }
 if(stage==1&&GetTickCount()-born>160){
  SendMessageW(input_edit,WM_KEYDOWN,VK_RETURN,0);fprintf(logfile,"IME-FIXTURE-ENTER pending=%d composing=%d elapsed=%lu alias=%s\n",input_pending,input_composing,GetTickCount()-input_committed_at,input_registry.rows[0].alias);terminal_input_tick();char native[INPUT_NATIVE];
  errors+=!input_ns((uintptr_t)widget+0x6c,native,sizeof native)||strcmp(native,"Enumerate(Transport)")||input_fixture_events!=1;
  SendMessageW(input_edit,WM_KEYDOWN,VK_RETURN,1L<<30);terminal_input_tick();errors+=input_fixture_events!=1;
  fprintf(logfile,"TERMINAL-INPUT-FIXTURE unicode-window=1 unknown-rejected=1 composition-not-submitted=1 separate-enter=1 native-setter=%s enter-events=%u native-business-dispatch=0 errors=%u\n",native,input_fixture_events,errors);fflush(logfile);
  input_fixture_shell=input_fixture_target=0;input_hide();SetFocus(input_game);stage=3;ExitProcess(errors?91:0);
 }
}
#endif

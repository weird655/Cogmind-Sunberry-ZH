/* Unicode EDIT owns IME composition. Only committed text is resolved on a
   separate Enter. No RNG, hack chance, result or target object is changed. */
#include "terminal_input.h"
#include "terminal_command_aliases.h"
#include "terminal_picker.h"

typedef struct {uint32_t shell,manual,input,target,machine,grid,canvas;uint64_t scope;int x,y,width,height,columns,rows,limit;} InputContext;
static InputRegistry input_registry;
static InputContext input_context;
static HWND input_game,input_edit,input_status;
static WNDPROC input_game_proc,input_edit_proc;
static HFONT input_font;
static int input_pending,input_composing;
static DWORD input_committed_at;
static uint32_t input_retiring;
static void input_picker_hide(void);
static int input_picker_feedback(const wchar_t *text);
/* SDL 1.2 may detach the default IME context. Own one only for our EDIT. */
static HANDLE input_ime_context;
typedef HANDLE (WINAPI *InputImeCreate)(void);
typedef HANDLE (WINAPI *InputImeAssociate)(HWND,HANDLE);
typedef BOOL (WINAPI *InputImeOpen)(HANDLE,BOOL);
static InputImeAssociate input_ime_associate;
static InputImeOpen input_ime_open;
static void input_enable_ime(void){
 if(!input_ime_context){
  HMODULE imm=LoadLibraryW(L"imm32.dll");if(!imm)return;
  InputImeCreate create=(InputImeCreate)GetProcAddress(imm,"ImmCreateContext");
  input_ime_associate=(InputImeAssociate)GetProcAddress(imm,"ImmAssociateContext");
  input_ime_open=(InputImeOpen)GetProcAddress(imm,"ImmSetOpenStatus");
  if(create&&input_ime_associate&&input_ime_open){
   HKL layouts[32];int count=GetKeyboardLayoutList(32,layouts);
   for(int i=0;i<count;i++)if(PRIMARYLANGID(LOWORD((uintptr_t)layouts[i]))==LANG_CHINESE){ActivateKeyboardLayout(layouts[i],0);break;}
   input_ime_context=create();
   fprintf(logfile,"TERMINAL-INPUT-IME layout=%p context=%p error=%lu\n",(void*)GetKeyboardLayout(0),(void*)input_ime_context,(unsigned long)GetLastError());fflush(logfile);
  }
 }
 if(input_ime_context){input_ime_associate(input_edit,input_ime_context);input_ime_open(input_ime_context,TRUE);}
}


static int input_ns(uintptr_t address,char *out,size_t cap){
 uint32_t *s=(void*)address;if(!readable(s,28)||s[4]>=cap||s[4]>s[5]||s[5]>65536)return 0;
 const char *raw=s[5]<16?(void*)s:(void*)(uintptr_t)s[0];if(!readable(raw,s[4]?s[4]:1)||memchr(raw,0,s[4]))return 0;
 for(unsigned i=0;i<s[4];i++)if((unsigned char)raw[i]<32||(unsigned char)raw[i]>126)return 0;
 memcpy(out,raw,s[4]);out[s[4]]=0;return 1;
}
typedef struct {uint32_t owner,grid;char source[INPUT_NATIVE],target[INPUT_TEXT];} InputPrintedRow;
static InputPrintedRow input_printed[128];static unsigned input_printed_next;
static void terminal_input_note_print(void *owner,void *text,const char *target){
 if(!readable(owner,0x98)||*(uint32_t*)owner!=va(0xc361bc)||!target||strlen(target)>=INPUT_TEXT)return;
 char source[INPUT_NATIVE];if(!input_ns((uintptr_t)text,source,sizeof source))return;
 InputPrintedRow *slot=NULL;for(unsigned i=0;i<128;i++)if(input_printed[i].owner==(uint32_t)(uintptr_t)owner){slot=&input_printed[i];break;}
 if(!slot)slot=&input_printed[input_printed_next++%128];slot->owner=(uint32_t)(uintptr_t)owner;slot->grid=((uint32_t*)owner)[4];strcpy(slot->source,source);strcpy(slot->target,target);
}
static void input_register_printed(InputRegistry *registry,uint32_t *owner,const char *source,const char *native,uint64_t token){
 for(unsigned i=0;i<128;i++){InputPrintedRow *p=&input_printed[i];if(p->owner==(uint32_t)(uintptr_t)owner&&p->grid==owner[4]&&!strcmp(p->source,source))input_register(registry,p->target,native,token);}
}
static uint64_t input_mix(uint64_t h,uint32_t v){for(unsigned i=0;i<4;i++){h^=(v>>(i*8))&255;h*=1099511628211ULL;}return h;}
#ifdef TERMINAL_INPUT_FIXTURE
static uint32_t input_fixture_shell,input_fixture_target;
static unsigned input_fixture_events;
#endif
static uint32_t input_global(uintptr_t address){
#ifdef TERMINAL_INPUT_FIXTURE
 if(address==0xcec100&&input_fixture_shell)return input_fixture_shell;
 if(address==0xcec0fc&&input_fixture_target)return input_fixture_target;
#endif
 return *(uint32_t*)va(address);
}
static uint64_t input_menu_signature(uint32_t *owner,unsigned depth,uint64_t hash){
 if(depth>4||!readable(owner,0x98))return input_mix(hash,0xffffffff);
 if(owner[0]==va(0xc361bc)){
  hash=input_mix(hash,(uint32_t)(uintptr_t)owner);hash=input_mix(hash,owner[0x7c/4]);hash=input_mix(hash,owner[0x74/4]);
  uint32_t *def=(void*)(uintptr_t)owner[0x70/4];if(readable(def,8)){hash=input_mix(hash,def[0]);hash=input_mix(hash,def[1]);}
  char label[INPUT_NATIVE];if(input_ns((uintptr_t)owner+0x80,label,sizeof label))for(const unsigned char *p=(const void*)label;*p;p++){hash^=*p;hash*=1099511628211ULL;}
 }
 uint32_t *v=(void*)((char*)owner+0x44);if(v[1]<v[0]||(v[1]-v[0])%4||v[1]-v[0]>INPUT_ROWS*4||!readable((void*)(uintptr_t)v[0],v[1]-v[0]))return input_mix(hash,0xfffffffe);
 for(uint32_t p=v[0];p<v[1];p+=4){uint32_t *child=*(void**)(uintptr_t)p;if(readable(child,8)&&child[1]==(uint32_t)(uintptr_t)owner)hash=input_menu_signature(child,depth+1,hash);}return hash;
}
static int input_get_context(InputContext *out){
 uint32_t *global=(void*)va(0xcec100);if(!readable(global,4)||input_global(0xcec104))return 0;
 uint32_t *s=(void*)(uintptr_t)input_global(0xcec100);if(!readable(s,0x128)||s[0]!=va(0xc36224))return 0;
 uint32_t *m=(void*)(uintptr_t)s[0xa4/4];if(!readable(m,0xc4)||m[0]!=va(0xc362c0)||m[1]!=(uint32_t)(uintptr_t)s||*((unsigned char*)m+0x54))return 0;
 uint32_t *i=(void*)(uintptr_t)m[0x6c/4];if(!readable(i,0xe4)||i[0]!=va(0xc34344)||i[1]!=(uint32_t)(uintptr_t)m||*((unsigned char*)i+0x54)||*((unsigned char*)i+0x8c)||i[0xac/4]!=va(0x900920)||i[0xb8/4]!=va(0x4b1c30)||i[0xdc/4]!=va(0x909990))return 0;
 uint32_t *t=(void*)(uintptr_t)input_global(0xcec0fc);if(!readable(t,0x98)||t[0]!=va(0xc361f0)||!t[0x70/4])return 0;
 uint32_t *v=(void*)((char*)m+0x70);if(v[1]<v[0]||(v[1]-v[0])%4||v[1]-v[0]>INPUT_ROWS*4||!readable((void*)(uintptr_t)v[0],v[1]-v[0]))return 0;
 InputContext c={0};c.shell=(uint32_t)(uintptr_t)s;c.manual=(uint32_t)(uintptr_t)m;c.input=(uint32_t)(uintptr_t)i;c.target=(uint32_t)(uintptr_t)t;c.machine=t[0x70/4];c.grid=i[4];c.width=i[2];c.height=i[3];c.limit=i[0x90/4];
 if(c.width<1||c.width>256||c.height!=1||c.limit<1||c.limit>=INPUT_NATIVE)return 0;
 uint32_t *o=i;
 for(unsigned depth=0;depth<12;depth++){
  if(!readable(o,0x24)||(int)o[7]<0||(int)o[8]<0||o[7]>1024||o[8]>1024)return 0;
  c.x+=(int)o[7];c.y+=(int)o[8];if(!o[1]){c.columns=o[2];c.rows=o[3];c.canvas=(uint32_t)(uintptr_t)o;break;}if(o[1]==(uint32_t)(uintptr_t)o)return 0;o=(void*)(uintptr_t)o[1];
 }
 if(c.columns<1||c.columns>512||c.rows<1||c.rows>512||c.x+c.width>c.columns||c.y>=c.rows)return 0;
 uint64_t h=1469598103934665603ULL;h=input_mix(h,c.shell);h=input_mix(h,c.manual);h=input_mix(h,c.input);h=input_mix(h,c.machine);h=input_mix(h,c.grid);h=input_mix(h,c.canvas);
 for(uint32_t p=v[0];p<v[1];p+=4)h=input_mix(h,*(uint32_t*)(uintptr_t)p);
 h=input_menu_signature(t,0,h);c.scope=h?h:1;*out=c;return 1;
}
static int input_command_code(unsigned logical,unsigned *code){
#ifdef TERMINAL_INPUT_FIXTURE
 if(input_fixture_shell){if(logical==1){*code=1;return 1;}if(logical!=31)return 0;*code=27;return 1;}
#endif
 uint32_t *v=(void*)va(0xd3860c);if(!readable(v,12)||v[1]<v[0]||(v[1]-v[0])%4||logical>=(v[1]-v[0])/4||(v[1]-v[0])>1024||!readable((void*)(uintptr_t)(v[0]+logical*4),4))return 0;
 unsigned key=*(uint32_t*)(uintptr_t)(v[0]+logical*4);if(key>=128)return 0;*code=key;return 1;
}
static int input_command_name(unsigned code,char *out,size_t cap){
#ifdef TERMINAL_INPUT_FIXTURE
 if(input_fixture_shell){if(code==1&&cap>=13){strcpy(out,"Schematic(*)");return 1;}if(code!=27||cap<21)return 0;strcpy(out,"Enumerate(Transport)");return 1;}
#endif
 return input_ns(va(0xd2d508)+28*code,out,cap)&&out[0];
}
static int input_allowed(const InputContext *c,unsigned code){
 uint32_t *v=(void*)(uintptr_t)(c->manual+0x70);
 for(uint32_t p=v[0];p<v[1];p+=4){unsigned key;if(input_command_code(*(uint32_t*)(uintptr_t)p,&key)&&key==code)return 1;}return 0;
}
static void input_add_translation(InputRegistry *registry,const char *source,const char *native,uint64_t token){
 if(!strcmp(source,native))for(unsigned i=0;i<sizeof input_command_aliases/sizeof input_command_aliases[0];i++){
  const InputCommandAlias *a=&input_command_aliases[i];if(!strcmp(a->native,native)&&!strchr(native,'*')&&!strchr(native,'@'))input_register(registry,a->chinese,native,token);
 }
 char translated[INPUT_TEXT];int result=message_resolve_entry(TEXT_ENTRY_TERMINAL_SHELL,source,strlen(source),translated,sizeof translated,NULL);
 if(result==1){input_register(registry,translated,native,token);return;}
 /* The reviewed log heading is another exact spelling of this registered
    command, not a reverse search of the entire translation dictionary. */
 char heading[INPUT_NATIVE+3];if(snprintf(heading,sizeof heading,">>%s",source)>=(int)sizeof heading)return;
 if(message_resolve_entry(TEXT_ENTRY_TERMINAL_SHELL,heading,strlen(heading),translated,sizeof translated,NULL)==1&&!strncmp(translated,">>",2))input_register(registry,translated+2,native,token);
}
#include "picker_visible_parameters46.h"
static int input_visible_target_valid(const InputContext *c,uint64_t token,const char *native){
 unsigned code=(unsigned)(token>>32)-1,argument=(unsigned)token;
 if(code>3||!argument)return 1;
 PickerVisibleParameter46 row;
 return input_allowed(c,code)&&picker_visible_parameter46(code,argument-1,c->limit,&row)&&!strcmp(row.native,native);
}
static void input_menu_rows(const InputContext *c,InputRegistry *registry,uint32_t *owner,unsigned depth){
 if(depth>4||!readable(owner,0x98))return;
 if(owner[0]==va(0xc361bc)&&!owner[0x7c/4]&&owner[0x70/4]){
  uint32_t *definition=(void*)(uintptr_t)owner[0x70/4];char native[INPUT_NATIVE],label[INPUT_NATIVE];
  if(readable(definition,8)&&input_allowed(c,definition[0])&&input_command_name(definition[0],native,sizeof native)&&input_ns((uintptr_t)owner+0x80,label,sizeof label)){
   uint64_t token=((uint64_t)definition[0]+1)<<32;
   if(strchr(native,'*')){
    /* Concrete visible Query / item or robot Schematic / Analysis records.
       The canonical argument must round-trip through native first-match rules. */
    PickerVisibleParameter46 row;
    if(!picker_visible_parameter46(definition[0],definition[1],c->limit,&row))goto children;
    strcpy(native,row.native);token|=row.ordinal+1;
   }
   if(strlen(native)<=(size_t)c->limit){input_register_printed(registry,owner,label,native,token);input_add_translation(registry,label,native,token);input_add_translation(registry,native,native,token);}
  }
 }
 children:;
 uint32_t *v=(void*)((char*)owner+0x44);if(v[1]<v[0]||(v[1]-v[0])%4||v[1]-v[0]>INPUT_ROWS*4||!readable((void*)(uintptr_t)v[0],v[1]-v[0]))return;
 for(uint32_t p=v[0];p<v[1];p+=4){uint32_t *child=*(void**)(uintptr_t)p;if(readable(child,8)&&child[1]==(uint32_t)(uintptr_t)owner)input_menu_rows(c,registry,child,depth+1);}
}
static void input_registry_build(const InputContext *c,InputRegistry *registry){
 memset(registry,0,sizeof *registry);registry->scope=c->scope;uint32_t *v=(void*)(uintptr_t)(c->manual+0x70);
 for(uint32_t p=v[0];p<v[1];p+=4){unsigned code;char native[INPUT_NATIVE];if(!input_command_code(*(uint32_t*)(uintptr_t)p,&code)||!input_command_name(code,native,sizeof native)||strchr(native,'*')||strlen(native)>(size_t)c->limit)continue;input_add_translation(registry,native,native,((uint64_t)code+1)<<32);}
 input_menu_rows(c,registry,(void*)(uintptr_t)c->target,0);
 fprintf(logfile,"TERMINAL-INPUT-REGISTRY machine=%08lx manual=%08lx rows=%u scope=%llx\n",(unsigned long)c->machine,(unsigned long)c->manual,registry->count,(unsigned long long)c->scope);fflush(logfile);
}
static void input_build_registry(const InputContext *c){input_registry_build(c,&input_registry);}
static void input_hide(void){if(input_edit)ShowWindow(input_edit,SW_HIDE);if(input_status)ShowWindow(input_status,SW_HIDE);input_picker_hide();input_pending=0;input_composing=0;memset(&input_context,0,sizeof input_context);}
static void input_feedback(const wchar_t *text){fprintf(logfile,"TERMINAL-INPUT-REJECT pending=%d composing=%d\n",input_pending,input_composing);fflush(logfile);if(!input_picker_feedback(text)){SetWindowTextW(input_status,text);ShowWindow(input_status,SW_SHOW);}MessageBeep(MB_OK);}
static int input_push_key(int symbol,int mod){
#ifdef TERMINAL_INPUT_FIXTURE
 if(input_fixture_shell){input_fixture_events++;fprintf(logfile,"IME-FIXTURE-KEY symbol=%d mod=%d native-business-dispatch=0\n",symbol,mod);return 1;}
#endif
 typedef int (__cdecl *Push)(void*);Push push=(Push)GetProcAddress(GetModuleHandleW(L"SDL.dll"),"SDL_PushEvent");if(!push)return 0;
 unsigned char down[24]={2,0,1},up[24]={3,0,0};memcpy(down+8,&symbol,4);memcpy(up+8,&symbol,4);memcpy(down+12,&mod,4);memcpy(up+12,&mod,4);uint16_t unicode=symbol<128?(uint16_t)symbol:0;memcpy(down+16,&unicode,2);
 if(push(down)<0)return 0;(void)push(up);return 1;
}
#include "terminal_picker_parameters.h"
#include "terminal_picker_ui.h"
static unsigned input_candidates[6],input_candidate_count,input_candidate_selected;
static wchar_t input_candidate_query[256];
static void input_candidate_draw(void){
 wchar_t display[2048]=L"";
 for(unsigned i=0;i<input_candidate_count;i++){
  wchar_t alias[INPUT_TEXT],line[INPUT_TEXT+32];
  MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,input_registry.rows[input_candidates[i]].alias,-1,alias,INPUT_TEXT);
  swprintf(line,INPUT_TEXT+32,L"%ls %ls\r\n",i==input_candidate_selected?L">":L" ",alias);
  if(wcslen(display)+wcslen(line)<2000)wcscat(display,line);
 }
 if(input_candidate_count)wcscat(display,L"↑↓ 选择 · Tab 补全 · Enter 提交");
 else wcscpy(display,L"未找到当前终端的候选命令。");
 SetWindowTextW(input_status,display);
}
static void input_candidate_update(void){
 if(input_composing)return;wchar_t wide[256];GetWindowTextW(input_edit,wide,256);
 if(!wcscmp(wide,input_candidate_query))return;wcscpy(input_candidate_query,wide);
 char query[INPUT_TEXT];int bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide,-1,query,sizeof query,NULL,NULL);
 input_candidate_count=bytes?input_suggest(&input_registry,input_context.scope,query,input_candidates,6):0;input_candidate_selected=0;
 if(wide[0])input_candidate_draw();else SetWindowTextW(input_status,L"输入中文查找命令；↑↓ 选择，Tab 补全，Enter 提交。");
}
static LRESULT CALLBACK input_child_proc(HWND window,UINT message,WPARAM wp,LPARAM lp){
 if(message==WM_SETFOCUS)input_enable_ime();
 if(message==WM_IME_STARTCOMPOSITION)input_composing=1;
 if(message==WM_IME_ENDCOMPOSITION){input_composing=0;input_committed_at=GetTickCount();}
 if(message==WM_KEYDOWN&&!(lp&(1L<<30))){
  if(!input_composing&&input_candidate_count&&(wp==VK_UP||wp==VK_DOWN)){
   input_candidate_selected=(input_candidate_selected+input_candidate_count+(wp==VK_UP?-1:1))%input_candidate_count;input_candidate_draw();return 0;
  }
  if(!input_composing&&wp==VK_TAB&&input_candidate_count){
   wchar_t alias[INPUT_TEXT];MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,input_registry.rows[input_candidates[input_candidate_selected]].alias,-1,alias,INPUT_TEXT);
   SetWindowTextW(input_edit,alias);SendMessageW(input_edit,EM_SETSEL,(WPARAM)-1,(LPARAM)-1);input_candidate_update();return 0;
  }
  if(wp==VK_RETURN){if(!input_composing&&GetTickCount()-input_committed_at>120)input_pending=1;return 0;}
  if(wp==VK_ESCAPE&&!input_composing){input_pending=2;return 0;}
  if(wp==VK_F9&&(GetKeyState(VK_RMENU)&0x8000)){SetFocus(input_game);input_push_key(290,0x0200);return 0;}
 }
 if(message==WM_CHAR&&(wp==13||wp==27||wp==9))return 0;
 return CallWindowProcW(input_edit_proc,window,message,wp,lp);
}
static LRESULT CALLBACK input_parent_proc(HWND window,UINT message,WPARAM wp,LPARAM lp){
 if(message==WM_CTLCOLORSTATIC&&input_picker_owns((HWND)lp)){SetTextColor((HDC)wp,RGB(0,220,0));SetBkColor((HDC)wp,RGB(0,0,0));return (LRESULT)GetStockObject(BLACK_BRUSH);}
 if(input_picker_message(window,message,wp,lp))return message==WM_DRAWITEM?TRUE:0;
 if((message==WM_CTLCOLOREDIT&&(HWND)lp==input_edit)||(message==WM_CTLCOLORSTATIC&&(HWND)lp==input_status)){
  SetTextColor((HDC)wp,(HWND)lp==input_edit?RGB(0,255,0):RGB(255,210,0));SetBkColor((HDC)wp,RGB(0,0,0));return (LRESULT)GetStockObject(BLACK_BRUSH);
 }
 return CallWindowProcW(input_game_proc,window,message,wp,lp);
}
static BOOL CALLBACK input_find_window(HWND window,LPARAM unused){
 (void)unused;wchar_t name[32];DWORD pid=0;GetWindowThreadProcessId(window,&pid);
 if(pid==GetCurrentProcessId()&&GetClassNameW(window,name,32)&&!wcscmp(name,L"SDL_app")){input_game=window;return FALSE;}return TRUE;
}
static int input_create(void){
 if(input_edit){if(IsWindow(input_edit))return 1;input_edit=input_status=NULL;input_game=NULL;}EnumThreadWindows(GetCurrentThreadId(),input_find_window,0);if(!input_game)return 0;
 input_game_proc=(WNDPROC)GetWindowLongPtrW(input_game,GWLP_WNDPROC);if(!input_game_proc)return 0;
 SetLastError(0);if(!SetWindowLongPtrW(input_game,GWLP_WNDPROC,(LONG_PTR)input_parent_proc)&&GetLastError())return 0;
 input_game_style=GetWindowLongPtrW(input_game,GWL_STYLE);
 input_edit=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|ES_AUTOHSCROLL,0,0,20,20,input_game,NULL,patch_module,NULL);
 input_status=CreateWindowExW(0,L"STATIC",L"",WS_CHILD|SS_LEFT,0,0,20,20,input_game,NULL,patch_module,NULL);
 if(!input_edit||!input_status||!IsWindowUnicode(input_edit)){if(input_edit)DestroyWindow(input_edit);if(input_status)DestroyWindow(input_status);input_edit=input_status=NULL;SetWindowLongPtrW(input_game,GWLP_WNDPROC,(LONG_PTR)input_game_proc);return 0;}
 input_edit_proc=(WNDPROC)SetWindowLongPtrW(input_edit,GWLP_WNDPROC,(LONG_PTR)input_child_proc);SendMessageW(input_edit,EM_SETLIMITTEXT,255,0);
 if(!input_edit_proc||!input_picker_create()){input_picker_destroy();DestroyWindow(input_edit);DestroyWindow(input_status);input_edit=input_status=NULL;SetWindowLongPtrW(input_game,GWLP_WNDPROC,(LONG_PTR)input_game_proc);return 0;}
 fprintf(logfile,"TERMINAL-INPUT-READY unicode-edit=1 native-judge=unchanged\n");fflush(logfile);return input_edit_proc!=NULL;
}
static void terminal_input_tick(void){
 InputContext c;if(!live_chinese||live_phase||live_replay||!input_get_context(&c)){
  if(input_context.scope){if(GetFocus()==input_edit||input_picker_owns(GetFocus()))SetFocus(input_game);input_hide();}input_retiring=0;return;
 }
 if(input_retiring==c.manual)return;
 if(!input_create())return;
 if(c.scope!=input_context.scope){
  input_picker_new_context();input_build_registry(&c);input_context=c;input_candidate_query[0]=0;input_candidate_count=0;SetWindowTextW(input_edit,L"");SetWindowTextW(input_status,L"输入中文查找命令；↑↓ 选择，Tab 补全，Enter 提交。");input_pending=0;
  input_picker_render();input_picker_layout_dirty=1;input_picker_update_layout();
  SetFocus(input_picker_mode?(IsWindowVisible(input_tree)?input_tree:input_game):input_edit);if(!input_picker_mode)input_enable_ime();
  fprintf(logfile,"TERMINAL-INPUT-FOCUS picker=%d focus=%p\n",input_picker_mode,(void*)GetFocus());fflush(logfile);
 }
 /* SDL can reclaim focus after the opening key event. Recover only while
    this game is foreground and its original terminal input is still active. */
 if(c.canvas!=input_context.canvas||c.x!=input_context.x||c.y!=input_context.y||c.width!=input_context.width||c.columns!=input_context.columns||c.rows!=input_context.rows){input_context=c;input_picker_layout_dirty=1;}
 if(input_picker_layout_dirty)input_picker_update_layout();
 if(GetForegroundWindow()==input_game&&GetFocus()==input_game){SetFocus(input_picker_mode?(IsWindowVisible(input_tree)?input_tree:input_game):input_edit);}
 if(!input_picker_mode)input_candidate_update();
 if(input_picker_mode&&input_picker_action&&!input_pending)input_picker_action_tick(&c);
 if(!input_pending)return;int pending=input_pending;input_pending=0;
 if(input_composing&&pending!=3)return;
 if(pending==2){input_retiring=c.manual;input_hide();SetFocus(input_game);input_push_key(27,0);return;}
 char committed[INPUT_TEXT]={0},native[INPUT_NATIVE];int bytes=0,ascii=0,result=0;
 if(pending==3){
  result=input_picker_pending_resolve(&c,native,sizeof native);
  if(result!=1){input_feedback(L"可选指令已变化，请重新选择。");return;}
 }else{
  wchar_t wide[256];int length=GetWindowTextW(input_edit,wide,256);if(length<=0)return;
  bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide,length,committed,sizeof committed-1,NULL,NULL);if(bytes<=0)return;committed[bytes]=0;
  ascii=1;for(int i=0;i<bytes;i++){unsigned char ch=(unsigned char)committed[i];if(ch<32||ch==127){input_feedback(L"不支持控制字符，未执行。");return;}if(ch>=128)ascii=0;}
  if(ascii){result=bytes<INPUT_NATIVE?1:-1;if(result==1)strcpy(native,committed);}
  else result=input_resolve(&input_registry,c.scope,committed,native,sizeof native);
 }
 fprintf(logfile,"TERMINAL-INPUT-RESOLVE ascii=%d result=%d bytes=%d\n",ascii,result,bytes);
#ifdef TERMINAL_INPUT_FIXTURE
 fprintf(logfile,"IME-FIXTURE-TEXT utf8=%s\n",committed);
#endif
 fflush(logfile);
 if(result!=1){input_feedback(result==-2?L"当前中文对应多个目标，未执行；请使用更完整的名称。":L"未匹配当前终端的完整中文命令，未执行；可使用原版英文。");return;}
 if(strlen(native)>(size_t)c.limit){input_feedback(L"命令超过原版输入长度，未执行。");return;}
 InputContext current;if(!input_get_context(&current)||current.scope!=c.scope){input_feedback(L"终端状态已改变，未执行。");return;}
 char previous[INPUT_NATIVE];if(!input_ns(c.input+0x6c,previous,sizeof previous))return;
 unsigned char value[28]={0};((StringCtor)va(0x9af0f0))(value,native);typedef void (__attribute__((thiscall)) *Set)(void*,void*);((Set)va(0x48d300))((void*)(uintptr_t)c.input,value);((NativeVoid)va(0x9af350))(value);
 SetFocus(input_game);
 if(!input_push_key(13,0)){((StringCtor)va(0x9af0f0))(value,previous);((Set)va(0x48d300))((void*)(uintptr_t)c.input,value);((NativeVoid)va(0x9af350))(value);SetFocus(input_picker_mode?(IsWindowVisible(input_tree)?input_tree:input_game):input_edit);input_feedback(L"未能提交，已恢复输入，请重试。");if(pending==3)input_picker_allow_retry();return;}
 fprintf(logfile,"TERMINAL-INPUT-SUBMIT machine=%08lx ascii=%d native=%s native-enter=queued-once\n",(unsigned long)c.machine,ascii,native);fflush(logfile);
 input_picker_record_submit(native);input_retiring=c.manual;input_hide();
}

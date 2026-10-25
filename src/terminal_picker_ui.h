#ifndef COGMIND_TERMINAL_PICKER_NATIVE_UI_H
#define COGMIND_TERMINAL_PICKER_NATIVE_UI_H
#include "terminal_picker_parameter_types.h"
#include "terminal_picker_memory.h"
#include "terminal_picker_native_layout.h"
#define PICKER_PARAMETER_PAGE 64
#define PICKER_UI_ROWS (PICKER_NODES+32)
enum {PUI_GROUP=1,PUI_FAMILY,PUI_COMMAND,PUI_PARAMETER};
typedef struct {unsigned kind,node,code,depth;uint64_t key;char label[INPUT_TEXT];} PickerUiRow;
typedef struct {int valid;unsigned code;uint64_t target;char native[INPUT_NATIVE],label[INPUT_TEXT];int bound;PickerParameterCandidate parameter;} PickerLastCommand;
static PickerModel input_picker_model;
static PickerUiRow input_ui_rows[PICKER_UI_ROWS];static unsigned input_ui_count,input_ui_scroll;
static PickerMemory input_picker_memory;static PickerLastCommand input_last_command;
static PickerParameterCandidate input_parameter_rows[PICKER_PARAMETER_PAGE];
static unsigned input_parameter_count,input_parameter_total,input_parameter_offset,input_parameter_code;
static int input_parameter_active,input_picker_action,input_picker_opening;
static unsigned input_picker_selected=PICKER_ROOT,input_picker_pending_node=PICKER_ROOT;
static uint64_t input_picker_pending_scope,input_picker_pending_stamp,input_picker_pending_target;
static char input_picker_pending_native[INPUT_NATIVE],input_picker_preview[INPUT_TEXT];
static int input_picker_mode=1,input_picker_layout_dirty=1,input_picker_building;
/* No HWND picker controls: the optional IME edit is the only Windows input. */
static HWND input_tree;static LONG_PTR input_game_style;
static unsigned char *input_picker_pixels;static size_t input_picker_pixel_capacity;
static PickerNativeLayout input_picker_box;static int input_picker_drawn;
static uint32_t input_picker_draw_grid;static uint64_t input_picker_draw_scope;
static int input_picker_cw,input_picker_ch;
static unsigned input_picker_capture;static int input_picker_pressed;
static uint64_t input_picker_pressed_scope,input_picker_pressed_key;
static void input_picker_render(void);
static void input_picker_update_layout(void);
static void input_picker_set_mode(int);
static int input_picker_repeat(void);
static void input_picker_message_text(const char *s){snprintf(input_picker_preview,sizeof input_picker_preview,"%s",s);}
static unsigned input_picker_view_code(void){return input_parameter_active?input_parameter_code:0;}
static uint64_t input_picker_group_key(unsigned group){char s[16];snprintf(s,sizeof s,"%u",group);return picker_memory_key(PUI_GROUP,s);}
static uint64_t input_picker_family_key(unsigned node){
 for(unsigned i=0;i<input_picker_model.count;i++)if(input_picker_model.nodes[i].parent==node){int route=picker_route(input_picker_model.nodes[i].native);if(route>=0)return picker_memory_key(PUI_FAMILY,picker_routes[route].verb);}return 0;
}
static void input_picker_remember(void){
 PickerRememberedView *v=picker_memory_view(&input_picker_memory,input_picker_view_code());if(!v)return;
 v->scroll=input_ui_scroll;v->page=input_parameter_offset/64;
 if(input_picker_selected<input_picker_model.count){PickerNode *n=input_picker_model.nodes+input_picker_selected;if(n->kind==PICKER_COMMAND){v->selected=n->target;strcpy(v->native,n->native);}}
 input_picker_memory.last_view=input_picker_view_code();
}
static void input_picker_add_row(unsigned kind,unsigned node,unsigned code,unsigned depth,uint64_t key,const char *label){
 if(input_ui_count>=PICKER_UI_ROWS)return;PickerUiRow *r=input_ui_rows+input_ui_count++;r->kind=kind;r->node=node;r->code=code;r->depth=depth;r->key=key;snprintf(r->label,sizeof r->label,"%s",label);
}
static void input_picker_flatten(void){
 input_ui_count=0;
 if(input_parameter_active){for(unsigned n=0;n<input_picker_model.count;n++)if(input_picker_model.nodes[n].kind==PICKER_COMMAND)input_picker_add_row(PUI_COMMAND,n,0,0,input_picker_model.nodes[n].target,input_picker_model.nodes[n].label);}
 else for(unsigned g=0;g<5;g++){
  unsigned gn=PICKER_ROOT;int parameter=0;
  for(unsigned n=0;n<input_picker_model.count;n++)if(input_picker_model.nodes[n].kind==PICKER_GROUP&&!strcmp(input_picker_model.nodes[n].label,picker_group_labels[g])){gn=n;break;}
  for(unsigned n=0;n<sizeof picker_parameter_branches/sizeof picker_parameter_branches[0];n++)if(picker_parameter_branches[n].group==g&&input_allowed(&input_context,picker_parameter_branches[n].code))parameter=1;
  if(gn==PICKER_ROOT&&!parameter)continue;uint64_t key=input_picker_group_key(g);input_picker_add_row(PUI_GROUP,gn,0,0,key,picker_group_labels[g]);if(!picker_memory_expanded(&input_picker_memory,key))continue;
  if(gn!=PICKER_ROOT)for(unsigned n=0;n<input_picker_model.count;n++)if(input_picker_model.nodes[n].kind==PICKER_FAMILY&&input_picker_model.nodes[n].parent==gn){
   uint64_t family=input_picker_family_key(n);input_picker_add_row(PUI_FAMILY,n,0,1,family,input_picker_model.nodes[n].label);
   if(picker_memory_expanded(&input_picker_memory,family))for(unsigned j=0;j<input_picker_model.count;j++)if(input_picker_model.nodes[j].kind==PICKER_COMMAND&&input_picker_model.nodes[j].parent==n)input_picker_add_row(PUI_COMMAND,j,0,2,input_picker_model.nodes[j].target,input_picker_model.nodes[j].label);
  }
  for(unsigned n=0;n<sizeof picker_parameter_branches/sizeof picker_parameter_branches[0];n++){
   const PickerParameterBranch *b=picker_parameter_branches+n;if(b->group!=g||!input_allowed(&input_context,b->code))continue;char label[INPUT_TEXT];if(!WideCharToMultiByte(CP_UTF8,0,b->label,-1,label,sizeof label,NULL,NULL))continue;
   input_picker_add_row(PUI_PARAMETER,PICKER_ROOT,b->code,1,picker_memory_key(PUI_PARAMETER,label),label);
  }
 }
 if(input_ui_scroll>=input_ui_count)input_ui_scroll=input_ui_count?input_ui_count-1:0;
}
static void input_picker_reset_pending(void){input_picker_pending_scope=input_picker_pending_stamp=input_picker_pending_target=0;input_picker_pending_node=PICKER_ROOT;input_picker_pending_native[0]=0;}
static void input_picker_select(unsigned node){
 input_picker_selected=PICKER_ROOT;if(input_picker_building)return;
 if(node>=input_picker_model.count||input_picker_model.nodes[node].kind!=PICKER_COMMAND){input_picker_message_text("点击 + 展开分类；滚轮滚动列表，选择指令后执行。");return;}
 char native[INPUT_NATIVE];if(picker_resolve(&input_picker_model,node,&input_registry,input_context.scope,native,sizeof native)!=1){input_picker_message_text("可选指令已变化，请重新选择。");return;}
 input_picker_selected=node;snprintf(input_picker_preview,sizeof input_picker_preview,"已选择：%s",input_picker_model.nodes[node].label);input_picker_remember();
}
static void input_picker_render(void){
 input_picker_building=1;input_pending=0;input_picker_reset_pending();input_picker_selected=PICKER_ROOT;
 if(picker_build(&input_picker_model,&input_registry,input_context.scope)!=1)memset(&input_picker_model,0,sizeof input_picker_model);
 unsigned wanted=input_picker_memory.last_view;
 PickerRememberedView *v=picker_memory_view(&input_picker_memory,input_picker_view_code());input_ui_scroll=v?v->scroll:0;input_picker_flatten();input_picker_building=0;input_picker_message_text("点击 + 展开分类；滚轮滚动列表，选择指令后执行。");
 if(v&&v->selected)for(unsigned n=0;n<input_picker_model.count;n++)if(input_picker_model.nodes[n].kind==PICKER_COMMAND&&input_picker_model.nodes[n].target==v->selected&&!strcmp(input_picker_model.nodes[n].native,v->native)){input_picker_select(n);break;}
 if(input_picker_opening){input_picker_opening=0;unsigned code=wanted;if(input_picker_memory.enabled&&input_picker_parameter_supported(code)&&input_allowed(&input_context,code))input_picker_action=100+(int)code;}
}
static void input_picker_new_context(void){picker_memory_reopen(&input_picker_memory);input_parameter_active=input_picker_action=0;input_parameter_count=input_parameter_total=input_parameter_offset=input_parameter_code=0;input_picker_opening=1;input_picker_drawn=0;input_picker_capture=0;}
static int input_picker_load_page(const InputContext *c,unsigned code,unsigned offset){
 PickerParameterCandidate *rows=calloc(64,sizeof *rows);InputRegistry *registry=calloc(1,sizeof *registry);if(!rows||!registry){free(rows);free(registry);return 0;}
 unsigned total=0;int count=input_picker_parameter_candidates(c,code,rows,64,offset,&total);
 if(count>=0&&total&&offset>=total){offset=(total-1)/64*64;count=input_picker_parameter_candidates(c,code,rows,64,offset,&total);}
 int ok=count>=0;registry->scope=c->scope;
 for(int n=0;ok&&n<count;n++){char label[INPUT_TEXT];int result=message_resolve_entry(rows[n].kind==2?TEXT_ENTRY_TERMINAL_SHELL:TEXT_ENTRY_ITEM_LABEL,rows[n].name,strlen(rows[n].name),label,sizeof label,NULL);if(result<0){ok=0;break;}if(!result)strcpy(label,rows[n].name);if(!input_register(registry,label,rows[n].native,rows[n].token))ok=0;}
 if(ok){input_registry=*registry;memcpy(input_parameter_rows,rows,64*sizeof *rows);input_parameter_count=(unsigned)count;input_parameter_total=total;input_parameter_offset=offset;input_parameter_code=code;input_parameter_active=1;input_picker_render();input_picker_remember();if(!count)input_picker_message_text("没有符合条件且能唯一指定的目标。");}
 free(rows);free(registry);return ok;
}
static void input_picker_action_tick(const InputContext *c){
 int action=input_picker_action;input_picker_action=0;if(!action)return;input_picker_remember();
 if(action==4){input_parameter_active=0;input_parameter_offset=0;input_build_registry(c);input_picker_render();input_picker_remember();return;}
 unsigned code=action>=100?(unsigned)action-100:input_parameter_active?input_parameter_code:0;if(!input_picker_parameter_supported(code)||!input_allowed(c,code)){input_picker_message_text("当前终端没有这条指令。");return;}
 PickerRememberedView *v=picker_memory_view(&input_picker_memory,code);unsigned offset=action==2?input_parameter_offset+64:action==3?(input_parameter_offset>=64?input_parameter_offset-64:0):input_picker_memory.enabled&&v?v->page*64:0;
 if(!input_picker_load_page(c,code,offset))input_picker_message_text("暂时无法读取目标，请重新打开终端。");
}
static int input_picker_parameter_valid(const InputContext *c,const char *native,uint64_t token){
 if(!input_parameter_active)return 1;
 for(unsigned i=0;i<input_parameter_count;i++)if(input_parameter_rows[i].token==token&&!strcmp(input_parameter_rows[i].native,native)){
  PickerParameterCandidate r={0};unsigned total=0;int count=input_picker_parameter_candidates(c,input_parameter_code,&r,1,input_parameter_offset+i,&total);const PickerParameterCandidate *s=input_parameter_rows+i;
  return count==1&&total==input_parameter_total&&r.source_index==s->source_index&&r.ordinal==s->ordinal&&r.prototype==s->prototype&&r.handle==s->handle&&r.slot==s->slot&&r.kind==s->kind&&r.token==token&&!strcmp(r.native,native);
 }
 return 0;
}
static int input_picker_queue(void){
 if(!input_picker_mode||input_pending||!live_chinese||live_phase||live_replay||!input_context.scope||input_picker_selected>=input_picker_model.count)return 0;
 const PickerNode *n=input_picker_model.nodes+input_picker_selected;char native[INPUT_NATIVE];if(n->kind!=PICKER_COMMAND||picker_resolve(&input_picker_model,input_picker_selected,&input_registry,input_context.scope,native,sizeof native)!=1)return 0;
 input_picker_pending_scope=input_context.scope;input_picker_pending_stamp=input_picker_model.registry_stamp;input_picker_pending_target=n->target;input_picker_pending_node=input_picker_selected;strcpy(input_picker_pending_native,native);input_pending=3;input_picker_message_text("正在提交指令……");return 1;
}
static int input_picker_pending_resolve(const InputContext *c,char *out,size_t cap){
 if(!c||!input_picker_mode||c->scope!=input_picker_pending_scope||input_picker_model.registry_stamp!=input_picker_pending_stamp||input_picker_pending_node>=input_picker_model.count)return -3;
 const PickerNode *n=input_picker_model.nodes+input_picker_pending_node;char native[INPUT_NATIVE];
 if(n->kind!=PICKER_COMMAND||n->target!=input_picker_pending_target||picker_resolve(&input_picker_model,input_picker_pending_node,&input_registry,c->scope,native,sizeof native)!=1||strcmp(native,input_picker_pending_native)||!input_visible_target_valid(c,n->target,native)||!input_picker_parameter_valid(c,native,n->target))return -3;
 size_t bytes=strlen(native);if(bytes>=cap)return -1;memcpy(out,native,bytes+1);return 1;
}
static void input_picker_record_submit(const char *native){
 PickerLastCommand last={0};last.valid=1;snprintf(last.native,sizeof last.native,"%s",native);snprintf(last.label,sizeof last.label,"%s",native);
 if(input_picker_selected<input_picker_model.count){PickerNode *n=input_picker_model.nodes+input_picker_selected;if(n->kind==PICKER_COMMAND&&!strcmp(n->native,native)){last.target=n->target;strcpy(last.label,n->label);}}
 if(input_parameter_active)for(unsigned i=0;i<input_parameter_count;i++)if(!strcmp(input_parameter_rows[i].native,native)&&(!last.target||last.target==input_parameter_rows[i].token)){last.target=input_parameter_rows[i].token;last.code=input_parameter_code;last.parameter=input_parameter_rows[i];last.bound=1;break;}
 if(!last.code&&!last.target)for(unsigned i=0;i<sizeof picker_parameter_branches/sizeof picker_parameter_branches[0];i++){unsigned code=picker_parameter_branches[i].code;const char *verb=code==1?"Schematic(":code==66?"Load(":code==76?"Scan(":code==81?"Recycle(":code==94?"Insert(":"Study(";if(!strncmp(native,verb,strlen(verb))){last.code=code;break;}}
 input_last_command=last;input_picker_remember();
}
static int input_picker_history_match(const PickerLastCommand *last,const PickerParameterCandidate *r){
 if(strcmp(last->native,r->native)||(last->target&&last->target!=r->token))return 0;if(!last->bound)return 1;const PickerParameterCandidate *old=&last->parameter;return old->ordinal==r->ordinal&&old->prototype==r->prototype&&old->handle==r->handle&&old->kind==r->kind;
}
static int input_picker_repeat(void){
 if(input_pending||!input_picker_mode||!input_last_command.valid)return 0;InputContext c;if(!input_get_context(&c)||c.scope!=input_context.scope)return 0;const PickerLastCommand last=input_last_command;
 if(last.code){
  unsigned total=0;PickerParameterCandidate *page=calloc(64,sizeof *page);if(!page)goto unavailable;int count=input_picker_parameter_candidates(&c,last.code,page,64,0,&total);if(count<0||total>16384){free(page);goto unavailable;}
  if(total>64){free(page);page=calloc(total,sizeof *page);if(!page)goto unavailable;unsigned again=0;count=input_picker_parameter_candidates(&c,last.code,page,total,0,&again);if(count<0||again!=total||(unsigned)count!=total){free(page);goto unavailable;}}
  unsigned selected=PICKER_ROOT;for(int n=0;n<count;n++)if(input_picker_history_match(&last,page+n)){if(selected!=PICKER_ROOT){selected=PICKER_ROOT;break;}selected=(unsigned)n;}free(page);if(selected==PICKER_ROOT)goto unavailable;
  input_picker_remember();if(!input_picker_load_page(&c,last.code,selected/64*64))goto unavailable;
 }else{
  InputRegistry *registry=calloc(1,sizeof *registry);if(!registry)goto unavailable;input_registry_build(&c,registry);unsigned found=0;for(unsigned n=0;n<registry->count;n++)if(!strcmp(registry->rows[n].native,last.native)&&(!last.target||registry->rows[n].target==last.target))found++;
  if(!found){free(registry);goto unavailable;}input_picker_remember();input_registry=*registry;free(registry);input_parameter_active=0;input_parameter_offset=0;input_picker_render();
 }
 for(unsigned n=0;n<input_picker_model.count;n++)if(input_picker_model.nodes[n].kind==PICKER_COMMAND&&!strcmp(input_picker_model.nodes[n].native,last.native)&&(!last.target||input_picker_model.nodes[n].target==last.target)){input_picker_select(n);return input_picker_queue();}
unavailable:input_picker_message_text("当前终端或目标已变化，不能重复上次命令。");return 0;
}
static int input_picker_feedback(const wchar_t *text){if(!input_picker_mode||!input_context.scope)return 0;if(!WideCharToMultiByte(CP_UTF8,0,text,-1,input_picker_preview,sizeof input_picker_preview,NULL,NULL))input_picker_message_text("未能执行，请重新选择。");return 1;}
static void input_picker_allow_retry(void){}
static int input_picker_owns(HWND window){(void)window;return 0;}
static void input_picker_hide(void){input_picker_remember();input_picker_selected=PICKER_ROOT;input_picker_reset_pending();input_picker_layout_dirty=1;input_picker_drawn=0;input_picker_capture=0;}
static void input_picker_destroy(void){free(input_picker_pixels);input_picker_pixels=NULL;input_picker_pixel_capacity=0;input_picker_drawn=0;}
static int input_picker_create(void){picker_memory_init(&input_picker_memory);return 1;}
static int input_picker_message(HWND window,UINT message,WPARAM wp,LPARAM lp){(void)window;(void)wp;(void)lp;if(message==WM_SIZE)input_picker_layout_dirty=1;return 0;}
static void input_picker_update_layout(void){
 input_picker_layout_dirty=0;if(!input_game||!input_context.scope)return;if(input_picker_mode){ShowWindow(input_edit,SW_HIDE);ShowWindow(input_status,SW_HIDE);SetWindowLongPtrW(input_game,GWL_STYLE,input_game_style);return;}
 PickerNativeLayout b;if(!picker_native_layout(input_context.columns,input_context.rows,input_context.x,input_context.y,input_context.width,&b))return;RECT client;if(!GetClientRect(input_game,&client))return;int cw=client.right/input_context.columns,ch=client.bottom/input_context.rows;if(cw<1||ch<1)return;
 int x=(b.x+2)*cw,y=(b.y+4)*ch,w=(b.w-4)*cw;MoveWindow(input_edit,x,y,w,ch*2,TRUE);MoveWindow(input_status,x,y+ch*3,w,ch*(b.h-10),TRUE);
 HFONT font=CreateFontW(-ch,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");if(font){HFONT old=input_font;input_font=font;SendMessageW(input_edit,WM_SETFONT,(WPARAM)font,TRUE);SendMessageW(input_status,WM_SETFONT,(WPARAM)font,TRUE);if(old)DeleteObject(old);}
 SetWindowLongPtrW(input_game,GWL_STYLE,input_game_style|WS_CLIPCHILDREN);ShowWindow(input_edit,SW_SHOW);ShowWindow(input_status,SW_SHOW);
}
static void input_picker_set_mode(int picker){
 if(!input_context.scope)return;input_picker_remember();input_pending=input_composing=0;input_picker_reset_pending();input_picker_mode=picker!=0;if(!picker&&input_parameter_active){input_parameter_active=0;input_parameter_offset=0;input_build_registry(&input_context);input_picker_render();}input_picker_update_layout();SetFocus(input_picker_mode?input_game:input_edit);if(!input_picker_mode)input_enable_ime();
}
#include "terminal_picker_native_draw.h"
#endif

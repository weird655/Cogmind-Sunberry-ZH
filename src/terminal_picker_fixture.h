#ifdef TERMINAL_INPUT_FIXTURE
static unsigned picker46_checks,picker46_errors;
static void picker46_check(int ok,const char *name){picker46_checks++;if(!ok){picker46_errors++;fprintf(logfile,"PICKER46-FAIL %s\n",name);}}
static unsigned picker46_leaf(void){for(unsigned n=0;n<input_picker_model.count;n++)if(input_picker_model.nodes[n].kind==PICKER_COMMAND)return n;return PICKER_ROOT;}
static void picker46_reopen(void){input_retiring=0;input_hide();input_picker_mode=1;terminal_input_tick();}
static int picker46_mouse(unsigned type,unsigned button,int x,int y){unsigned char event[24]={0};uint16_t px=(uint16_t)(x*input_picker_cw+input_picker_cw/2),py=(uint16_t)(y*input_picker_ch+input_picker_ch/2);event[0]=(unsigned char)type;event[2]=(unsigned char)button;memcpy(event+4,&px,2);memcpy(event+6,&py,2);return terminal_input_event(event);}
static void picker46_click(int x,int y){picker46_check(picker46_mouse(5,1,x,y)&&picker46_mouse(6,1,x,y),"native-SDL-click-consumed");}
static unsigned terminal_picker_fixture45(void *widget,void *target){
 (void)target;picker46_checks=picker46_errors=0;unsigned events=input_fixture_events;
 picker46_reopen();unsigned leaf=picker46_leaf();picker46_check(leaf!=PICKER_ROOT,"leaf-registered");
 picker46_check(!IsWindowVisible(input_edit)&&!IsWindowVisible(input_status),"no-picker-HWND-visible");
 picker46_check(input_picker_memory.enabled,"memory-default-enabled");
 uint64_t group=input_picker_group_key(0);picker_memory_toggle(&input_picker_memory,group);input_picker_flatten();input_picker_remember();unsigned expanded=input_ui_count;
 picker46_reopen();picker46_check(picker_memory_expanded(&input_picker_memory,group)&&input_ui_count==expanded,"reopen-keeps-expanded-branches");
 input_picker_select(leaf);picker46_check(!input_pending&&input_fixture_events==events,"selection-does-not-submit");
 picker46_check(input_picker_queue(),"execute-queues");picker46_check(!input_picker_queue(),"double-click-deduplicated");terminal_input_tick();
 char native[INPUT_NATIVE];picker46_check(input_fixture_events==events+1&&input_ns((uintptr_t)widget+0x6c,native,sizeof native)&&!strcmp(native,"Enumerate(Transport)"),"one-native-enter");
 picker46_check(input_last_command.valid&&!strcmp(input_last_command.native,"Enumerate(Transport)"),"history-recorded-after-submit");
 picker46_reopen();picker46_check(input_picker_repeat(),"repeat-revalidates-current-registry");picker46_check(!input_picker_repeat(),"repeat-while-pending-ignored");terminal_input_tick();picker46_check(input_fixture_events==events+2,"repeat-enters-once");
 picker46_reopen();PickerLastCommand saved=input_last_command;strcpy(input_last_command.native,"Query(Not a current record)");picker46_check(!input_picker_repeat()&&!input_pending&&input_fixture_events==events+2,"unavailable-history-rejected");input_last_command=saved;
 input_picker_select(picker46_leaf());input_picker_queue();live_chinese=0;terminal_input_tick();picker46_check(!input_context.scope&&!input_pending,"language-switch-cancels-pending");live_chinese=1;
 picker46_reopen();input_picker_memory.enabled=0;picker46_reopen();picker46_check(!input_picker_memory.expanded_count&&input_ui_scroll==0,"memory-off-resets-reopen");input_picker_memory.enabled=1;
 picker_memory_toggle(&input_picker_memory,group);input_picker_flatten();
 for(unsigned n=0;n<input_picker_model.count;n++)if(input_picker_model.nodes[n].kind==PICKER_FAMILY){picker_memory_toggle(&input_picker_memory,input_picker_family_key(n));break;}
 input_picker_flatten();input_picker_select(picker46_leaf());input_picker_remember();
 input_picker_set_mode(0);picker46_check(IsWindowVisible(input_edit),"optional-ime-still-available");input_picker_set_mode(1);picker46_check(!IsWindowVisible(input_edit),"mouse-mode-hides-ime");
 {
  uint32_t *root=(void*)(uintptr_t)input_context.canvas;uint32_t source=root[4];unsigned char *view=terminal_input_draw_view(root);picker46_check(view&&input_picker_drawn&&root[4]==source,"native-canvas-is-detached-copy");
  if(view){PickerNativeLayout box=input_picker_box;unsigned char *border=view+((size_t)(box.x+box.w-3)*root[3]+box.y)*20;picker46_check(*(uint32_t*)(border+4)=='-'&&*(uint32_t*)(border+8)==((uint32_t*)(uintptr_t)font_meta[4])['-'],"native-font-mapping-preserved");
   (void)terminal_input_draw_background_full(root,view);picker46_check(!terminal_input_draw_background_full(root,view),"stable-frame-keeps-native-differential-refresh");input_picker_box.y++;picker46_check(terminal_input_draw_background_full(root,view),"moved-panel-refreshes-old-background");input_picker_box=box;(void)terminal_input_draw_background_full(root,view);picker46_check(terminal_input_draw_background_full(root,NULL)&&!terminal_input_draw_background_full(root,NULL),"closing-panel-refreshes-background-once");
   size_t untouched=((size_t)(root[2]-1)*root[3]+root[3]-1)*20;picker46_check(!memcmp(view+untouched,(void*)(uintptr_t)(source+untouched),20),"outside-frame-unchanged");
   unsigned row=0;while(row<input_ui_count&&input_ui_rows[row].kind!=PUI_COMMAND)row++;picker46_check(row<input_ui_count,"visible-command-row");
   if(row<input_ui_count){input_picker_selected=PICKER_ROOT;picker46_click(box.x+8,box.body+(int)row);picker46_check(input_picker_selected!=PICKER_ROOT&&!input_pending&&input_fixture_events==events+2,"SDL-row-select-is-preview-only");}
   picker46_check(!picker46_mouse(5,1,0,0)&&!picker46_mouse(6,1,0,0),"outside-frame-passes-through");
   picker46_mouse(5,1,box.x+3,box.y+box.h-2);picker46_mouse(6,1,box.x+15,box.y+box.h-2);picker46_check(!input_pending,"drag-to-different-button-does-not-execute");
   int enabled=input_picker_memory.enabled;picker46_click(box.x+3,box.y+1);picker46_check(input_picker_memory.enabled!=enabled,"memory-button-toggle");picker46_click(box.x+3,box.y+1);
   uint64_t scope=input_picker_draw_scope;input_picker_draw_scope++;picker46_check(!picker46_mouse(5,1,box.x+3,box.y+box.h-2)&&!input_pending,"stale-frame-cannot-execute");input_picker_draw_scope=scope;
   PickerUiRow *saved_rows=malloc(sizeof input_ui_rows);if(!saved_rows)ExitProcess(90);unsigned count=input_ui_count,scroll=input_ui_scroll;memcpy(saved_rows,input_ui_rows,sizeof input_ui_rows);
   for(unsigned i=0;i<40;i++){input_ui_rows[i]=saved_rows[0];input_ui_rows[i].key=1000+i;}input_ui_count=40;input_ui_scroll=0;
   for(unsigned i=0;i<30;i++){picker46_mouse(5,5,box.x+5,box.body+1);picker46_mouse(6,5,box.x+5,box.body+1);}terminal_input_draw_view(root);
   picker46_check(input_ui_scroll==40-(unsigned)box.body_rows&&!memcmp(&box,&input_picker_box,sizeof box),"wheel-scroll-clamped-with-fixed-footer");
   for(unsigned i=0;i<30;i++){picker46_mouse(5,4,box.x+5,box.body+1);picker46_mouse(6,4,box.x+5,box.body+1);}picker46_check(!input_ui_scroll,"wheel-scroll-returns-to-top");
   memcpy(input_ui_rows,saved_rows,sizeof input_ui_rows);free(saved_rows);input_ui_count=count;input_ui_scroll=scroll;terminal_input_draw_view(root);
   picker46_click(box.x+3,box.y+box.h-2);picker46_check(input_pending==3,"SDL-execute-queues-one-native-command");picker46_click(box.x+3,box.y+box.h-2);terminal_input_tick();picker46_check(input_fixture_events==events+3,"SDL-double-click-submits-once");picker46_reopen();
  }
 }
 {PickerLastCommand last={0};PickerParameterCandidate candidate={0};last.bound=last.valid=1;last.target=candidate.token=123;strcpy(last.native,"Scan(Lgt. Treads)");strcpy(candidate.native,last.native);candidate.ordinal=7;candidate.prototype=8;candidate.handle=9;candidate.kind=1;last.parameter=candidate;
  picker46_check(input_picker_history_match(&last,&candidate),"bound-history-matches");candidate.slot++;candidate.source_index++;picker46_check(input_picker_history_match(&last,&candidate),"moved-current-item-can-be-reselected");candidate.handle++;picker46_check(!input_picker_history_match(&last,&candidate),"replacement-item-not-repeated");candidate=last.parameter;candidate.token++;picker46_check(!input_picker_history_match(&last,&candidate),"new-generation-not-repeated");
 }
 fprintf(logfile,"TERMINAL-PICKER46-FIXTURE checks=%u errors=%u native-events-delta=%u native-business-dispatch=0\n",picker46_checks,picker46_errors,input_fixture_events-events);fflush(logfile);return picker46_errors;
}
#endif

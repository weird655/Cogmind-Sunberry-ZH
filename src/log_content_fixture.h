#ifdef CONTENT_NATIVE_PROBE
static unsigned log_fixture_errors;
static void log_content_fixture(void *parent){
 unsigned baseline=vector_count((char*)parent+0x44),passed=0;
 /* Exact screenshot alert, in the same nested command scope. */
 void *popup=(*(NativeNew*)va(0xb6132c))(0x6c);((ConsoleCtor)va(0x48c060))(popup,parent,81,1,0,0,0,1,-1);
 unsigned char alert[28]={0};((StringCtor)va(0x9af0f0))(alert,"  ALERT: Suspicious activity at Terminal M002. Dispatching Investigation squad.  ");
 SerialContext nested_popup={0},*saved_ctx_popup=serial_context;void *saved_owner_popup=constructing_owner;serial_context=&nested_popup;constructing_owner=parent;command_scope++;
 int popup_ok=notification_literal_print(popup,0,0,alert,va(0x808ce3));command_scope--;serial_context=saved_ctx_popup;constructing_owner=saved_owner_popup;
 ((NativeVoid)va(0x9af350))(alert);uint32_t *pv_popup=popup;unsigned popup_cjk=0,popup_bad=0;
 for(unsigned x=0;x<pv_popup[2];x++){unsigned char *cell=(void*)(pv_popup[4]+x*20);unsigned g=*(uint32_t*)(cell+4);if(g>=320&&g<320+2*zh_registry_count&&!(g&1)){popup_cjk++;if(x+1>=pv_popup[2]||*(uint32_t*)(cell+24)!=g+1||memcmp(cell+12,cell+32,6))popup_bad++;}}
 log_fixture_errors+=!popup_ok||!popup_cjk||popup_bad;
 fprintf(logfile,"NOTIFICATION-NESTED-FIXTURE passed=%d cjk=%u errors=%u\n",popup_ok&&popup_cjk&&!popup_bad,popup_cjk,popup_bad);fflush(logfile);
 uint32_t *popup_children=(void*)((char*)parent+0x44);for(unsigned j=0;j<vector_count(popup_children);j++)if(((void**)popup_children[0])[j]==popup){typedef void (__cdecl *Remove)(void*,unsigned);((Remove)va(0x9cea50))(popup_children,j);break;}

 const char *templates[]={"Found 6 haulers:","Found 2 active surveillance squads."};
 for(unsigned k=0;k<2;k++){
  unsigned char entry[0x28]={0};((StringCtor)va(0x9af0f0))(entry+4,templates[k]);
  void *widget=(*(NativeNew*)va(0xb6132c))(0x70);typedef void* (__attribute__((thiscall)) *Ctor)(void*,void*,int,void*,int);
  ((Ctor)va(0x48e340))(widget,parent,0,entry,0);int ok=1;
  uint32_t *v=widget;unsigned chinese=0,bad=0;
  for(unsigned x=0;x<v[2];x++){unsigned char *cell=(void*)(v[4]+x*20);unsigned g=*(uint32_t*)(cell+4);if(g>=320&&g<320+2*zh_registry_count&&!(g&1)){chinese++;if(x+1>=v[2]||*(uint32_t*)(cell+24)!=g+1||memcmp(cell+12,cell+32,6))bad++;}}
  log_fixture_errors+=!ok||!chinese||bad;
  fprintf(logfile,"TEMPLATE-DISPLAY-FIXTURE case=%u passed=%d cjk=%u errors=%u\n",k,ok&&chinese&&!bad,chinese,bad);fflush(logfile);
  uint32_t *children=(void*)((char*)parent+0x44);for(unsigned j=0;j<vector_count(children);j++)if(((void**)children[0])[j]==widget){typedef void (__cdecl *Remove)(void*,unsigned);((Remove)va(0x9cea50))(children,j);break;}
  ((NativeVoid)va(0x9af350))(entry+4);
 }
 const char *sources[]={"Found 6 haulers:","Found 2 active surveillance squads.","ALERT: Potential suspicious activity detected, Programmers report to Factory Zone 4Z.","Chain reaction triggered in Lgt. Nuclear Core.","Aside from the options menu, there are many more interface features and behavior you can tweak in Cogmind's advanced.cfg file, the functionality of which is described in the manual under Advanced UI: Advanced Options."};
 unsigned total=sizeof sources/sizeof sources[0];
 for(unsigned k=0;k<total;k++){
  void *record=(*(NativeNew*)va(0xb6132c))(0x28);memset(record,0,0x28);((StringCtor)va(0x9af0f0))((char*)record+4,sources[k]);
  unsigned before=log_bank_count((void*)va(0xcf1080));int result=((LogAppendFn)va(0x5121f0))((void*)va(0xcf1080),record);unsigned after=log_bank_count((void*)va(0xcf1080));
  unsigned chinese=0,bad=0,mapped=0;uint32_t *bank=(void*)va(0xcf1080);
  for(unsigned i=before;i<after;i++){
   void *entry=((void**)bank[0])[i];LogContentRow *expected=NULL;for(unsigned j=0;j<LOG_CONTENT_SLOTS;j++)if(log_content_rows[j].record==entry)expected=&log_content_rows[j];
   if(!expected){bad++;continue;}mapped++;
   void *object=(*(NativeNew*)va(0xb6132c))(0x70);typedef void* (__attribute__((thiscall)) *Ctor)(void*,void*,int,void*,int);
   void *saved_owner=constructing_owner;SerialContext nested={0},*saved_ctx=serial_context;constructing_owner=parent;serial_context=&nested;command_scope++;
   ((Ctor)va(0x48e340))(object,parent,0,entry,0);command_scope--;serial_context=saved_ctx;constructing_owner=saved_owner;uint32_t *v=object;
   ZhPlan p={0};if(zh_plan(expected->chinese,strlen(expected->chinese),v[2],1,&p))bad++;
   for(size_t z=0;z<p.count;z++){ZhRun *run=&p.runs[z];for(int half=0;half<run->width;half++)if(*(uint32_t*)(v[4]+(run->x+half)*20+4)!=run->token[half])bad++;}
   for(unsigned x=0;x<v[2];x++){unsigned char *cell=(void*)(v[4]+x*20);unsigned g=*(uint32_t*)(cell+4);if(g>=320&&g<320+2*zh_registry_count&&!(g&1)){chinese++;if(x+1>=v[2]||*(uint32_t*)(cell+24)!=g+1||memcmp(cell+12,cell+32,6))bad++;}}
   zh_plan_free(&p);uint32_t *children=(void*)((char*)parent+0x44);for(unsigned j=0;j<vector_count(children);j++)if(((void**)children[0])[j]==object){typedef void (__cdecl *Remove)(void*,unsigned);((Remove)va(0x9cea50))(children,j);break;}
  }
  int ok=after>before&&mapped==after-before&&chinese>0&&!bad;passed+=ok;
  fprintf(logfile,"CONTENT-LOG-FIXTURE case=%u passed=%d native-result=%d rows=%u mapped=%u cjk=%u errors=%u\n",k,ok,result,after-before,mapped,chinese,bad);fflush(logfile);
 }
 /* Simulate reopening an English saved history: drop only our display cache. */
 for(unsigned i=0;i<LOG_CONTENT_SLOTS;i++)log_content_release(&log_content_rows[i]);
 unsigned recovered=0,history_cjk=0,history_bad=0;uint32_t *history=(void*)va(0xcf1080);
 for(unsigned i=0;i<log_bank_count(history);i++){
  void *entry=((void**)history[0])[i];void *object=(*(NativeNew*)va(0xb6132c))(0x70);typedef void* (__attribute__((thiscall)) *Ctor)(void*,void*,int,void*,int);
  void *saved_owner=constructing_owner;SerialContext nested={0},*saved_ctx=serial_context;constructing_owner=parent;serial_context=&nested;command_scope++;
   ((Ctor)va(0x48e340))(object,parent,0,entry,0);command_scope--;serial_context=saved_ctx;constructing_owner=saved_owner;uint32_t *v=object;
  for(unsigned j=0;j<LOG_CONTENT_SLOTS;j++)if(log_content_rows[j].record==entry)recovered++;
  for(unsigned x=0;x<v[2];x++){unsigned char *cell=(void*)(v[4]+x*20);unsigned g=*(uint32_t*)(cell+4);if(g>=320&&g<320+2*zh_registry_count&&!(g&1)){history_cjk++;if(x+1>=v[2]||*(uint32_t*)(cell+24)!=g+1||memcmp(cell+12,cell+32,6))history_bad++;}}
  uint32_t *children=(void*)((char*)parent+0x44);for(unsigned j=0;j<vector_count(children);j++)if(((void**)children[0])[j]==object){typedef void (__cdecl *Remove)(void*,unsigned);((Remove)va(0x9cea50))(children,j);break;}
 }
 log_fixture_errors+=history_bad||!history_cjk||recovered<7;
 fprintf(logfile,"CONTENT-LOG-HISTORY-FIXTURE recovered=%u cjk=%u errors=%u\n",recovered,history_cjk,history_bad);fflush(logfile);
 log_fixture_errors+=passed!=total;log_fixture_errors+=vector_count((char*)parent+0x44)!=baseline;
 fprintf(logfile,"CONTENT-LOG-FIXTURE-SUMMARY passed=%u expected=%u children=%u baseline=%u errors=%u\n",passed,total,vector_count((char*)parent+0x44),baseline,log_fixture_errors);fflush(logfile);
}
#endif

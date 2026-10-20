/* Known display-only round trips: manual prelayout and popup notification.
   Native strings carry ASCII row IDs; UTF-8 rows remain owned by this module. */
#include "unicode_row.h"
#define SCRATCH_ROW_LIMIT 8192u
typedef struct {char *text,*english;unsigned columns;} ScratchRow;
static ScratchRow scratch_rows[SCRATCH_ROW_LIMIT];
static unsigned scratch_row_count;
typedef struct {void *owner;unsigned first,count,columns;int manual;} ScratchContext;
static _Thread_local ScratchContext scratch_context;
static void scratch_reject(const char *reason,void *self,void *s,uintptr_t caller,int width,int rows){
 static unsigned traced;if(caller!=va(0x8089d1)||traced>=32)return;traced++;
 fprintf(logfile,"SCRATCH-REJECT reason=%s owner=%p width=%d rows=%d\n",reason,self,width,rows);audit_native_string("SCRATCH-INPUT",s);fflush(logfile);
}
static int scratch_paragraph(void *self,int x,int y,int width,int maxrows,void *s,uintptr_t caller,int *result){
 memset(&scratch_context,0,sizeof scratch_context);
 if((caller!=va(0x790a33)&&caller!=va(0x8089d1))||x||y||width<9||width>512||maxrows<1||!readable(self,24))return 0;
 TextEntry entry=caller==va(0x790a33)?TEXT_ENTRY_MANUAL:TEXT_ENTRY_NOTIFICATION;
 const char *source;size_t bytes;if(!plain_source(s,&source,&bytes)){scratch_reject("source",self,s,caller,width,maxrows);return 0;}char target[32768];
 if(message_resolve_entry(entry,source,bytes,target,sizeof target,NULL)!=1){scratch_reject("lookup",self,s,caller,width,maxrows);return 0;}
 ZhPlan p={0},en={0};int limit=maxrows>200?200:maxrows;
 if(message_plan_entry(entry,target,strlen(target),width,limit,&p)||zh_plan(source,bytes,width,limit,&en)){
  scratch_reject("layout",self,s,caller,width,maxrows);zh_plan_free(&p);zh_plan_free(&en);return 0;
 }
 unsigned count=(unsigned)(p.used_rows>en.used_rows?p.used_rows:en.used_rows);
 if(!count||count>SCRATCH_ROW_LIMIT-scratch_row_count){zh_plan_free(&p);zh_plan_free(&en);return 0;}
 char *prepared[200]={0},*prepared_en[200]={0};unsigned columns[200]={0};int ready=1;
 for(unsigned row=0;ready&&row<count;row++){
  char text[8192]={0},english[8192]={0};
  if(((int)row<p.used_rows&&!unicode_row_text(&p,(int)row,text,sizeof text))||
     ((int)row<en.used_rows&&!unicode_row_text(&en,(int)row,english,sizeof english))){ready=0;break;}
  prepared[row]=malloc(strlen(text)+1);prepared_en[row]=malloc(strlen(english)+1);
  if(!prepared[row]||!prepared_en[row]){ready=0;break;}
  strcpy(prepared[row],text);strcpy(prepared_en[row],english);
  for(size_t i=0;i<p.count;i++)if(p.runs[i].y==(int)row&&(unsigned)(p.runs[i].x+p.runs[i].width)>columns[row])columns[row]=p.runs[i].x+p.runs[i].width;
  for(size_t i=0;i<en.count;i++)if(en.runs[i].y==(int)row&&(unsigned)(en.runs[i].x+en.runs[i].width)>columns[row])columns[row]=en.runs[i].x+en.runs[i].width;
 }
 if(!ready||!extend_demo_fonts()){
  for(unsigned i=0;i<count;i++){free(prepared[i]);free(prepared_en[i]);}
  zh_plan_free(&p);zh_plan_free(&en);return 0;
 }
 /* Stable row ownership is shared by both languages. The shorter layout keeps
    empty rows instead of reindexing existing native manual/notification objects. */
 ScratchContext c={self,scratch_row_count,count,(unsigned)width,caller==va(0x790a33)};
 for(unsigned i=0;i<count;i++){
  scratch_rows[scratch_row_count].text=prepared[i];scratch_rows[scratch_row_count].english=prepared_en[i];
  scratch_rows[scratch_row_count++].columns=columns[i];
 }
 scratch_context=c;*result=(int)count;
 fprintf(logfile,"SCRATCH-LAYOUT scope=%s owner=%p rows=%u cn-rows=%d en-rows=%d width=%u pool=%u\n",c.manual?"manual":"notification",self,c.count,p.used_rows,en.used_rows,c.columns,scratch_row_count);fflush(logfile);
 zh_plan_free(&p);zh_plan_free(&en);return 1;
}
static void *scratch_extract(void *self,void *out,const int *point,unsigned maximum,uintptr_t caller){
 ScratchContext *c=&scratch_context;
 if(!c->owner||self!=c->owner||!point||point[0]||point[1]<0||(unsigned)point[1]>=c->count||maximum!=c->columns||caller!=va(c->manual?0x790a92:0x808a12))return NULL;
 unsigned id=c->first+(unsigned)point[1],columns=c->manual?c->columns:scratch_rows[id].columns;if(columns<9)columns=9;
 char marker[513];int n=snprintf(marker,sizeof marker,"Z%06u|",id);if(n!=8||columns>512)return NULL;
 memset(marker+8,' ',columns-9);marker[columns-1]='#';marker[columns]=0;
 ((StringCtor)va(0x9af0f0))(out,marker);return out;
}
static int scratch_row_print(void *self,int x,int y,void *s,uintptr_t caller){
 if((caller!=va(0x4931af)&&caller!=va(0x808ce3))||!readable(s,24))return 0;
 uint32_t *ns=s;const char *raw=ns[5]<16?s:*(void**)s;size_t n=ns[4];if(n<9||n>1024||!readable(raw,n))return 0;
 size_t left=0,right=n;while(left<right&&raw[left]==' ')left++;while(right>left&&raw[right-1]==' ')right--;
 if(right-left<9||raw[left]!='Z'||raw[left+7]!='|'||raw[right-1]!='#')return 0;
 unsigned id=0;for(size_t i=left+1;i<left+7;i++){if(raw[i]<'0'||raw[i]>'9')return 0;id=id*10+(unsigned)(raw[i]-'0');}
 for(size_t i=left+8;i+1<right;i++)if(raw[i]!=' ')return 0;if(id>=scratch_row_count)return 0;
 char target[12288],english[12288];size_t body=strlen(scratch_rows[id].text),en_body=strlen(scratch_rows[id].english),tail=n-right;
 if(left+body+tail>=sizeof target||left+en_body+tail>=sizeof english)return 0;
 memset(target,' ',left);memcpy(target+left,scratch_rows[id].text,body);memset(target+left+body,' ',tail);target[left+body+tail]=0;
 memset(english,' ',left);memcpy(english+left,scratch_rows[id].english,en_body);memset(english+left+en_body,' ',tail);english[left+en_body+tail]=0;
 const char *saved=display_override_target,*saved_en=display_override_english;
 display_override_target=target;display_override_english=english;int result;
 int ok=display_bridge(self,x,y,0,0,-1,0,s,&result);
 display_override_target=saved;display_override_english=saved_en;
 if(ok){fprintf(logfile,"SCRATCH-ROW id=%u caller=%08lx chinese-restored=1\n",id,(unsigned long)(caller-base+0x400000));fflush(logfile);}else{fprintf(logfile,"SCRATCH-ROW-FAIL id=%u caller=%08lx\n",id,(unsigned long)(caller-base+0x400000));fflush(logfile);}return ok;
}

/* Short native notifications skip the paragraph scratch pipeline entirely.
   This is the verified final popup printer, not an arbitrary base console. */
static int notification_literal_print(void *self,int x,int y,void *ns,uintptr_t caller){
 if(caller!=va(0x808ce3)||x||y||!readable(self,0x3c)||!readable(ns,24))return 0;uint32_t *o=self,*v=ns;
 if(o[0]!=va(0xc2ef14)||o[5]||o[3]!=1||o[2]<9||o[2]>512)return 0;
 const char *source;size_t n;if(!plain_source(ns,&source,&n))return 0;char translated[32768];
 int match=message_resolve_entry(TEXT_ENTRY_NOTIFICATION,source,n,translated,sizeof translated,NULL);if(match<=0)return 0;
 (void)v;const char *saved=display_override_target;unsigned prior=display_owned_literal_scope;
 display_override_target=translated;display_owned_literal_scope++;int result;int ok=display_bridge(self,x,y,0,0,-1,0,ns,&result);display_owned_literal_scope=prior;display_override_target=saved;
 if(ok){fprintf(logfile,"NOTIFICATION-CONNECT command-scope=%u columns=%u translated=%s\n",command_scope,o[2],translated);fflush(logfile);}return ok;
}

#ifdef SCRATCH_RESOURCE_PROBE
static void scratch_fixture(void *parent){
 static int done;if(done||GetTickCount()-probe_start<6500)return;done=1;unsigned passed=0,before=vector_count((char*)parent+0x44);
 const char *sources[]={"This manual is not an exhaustive description of everything that may happen in the game. Rather it provides an overview of the core rules necessary for a fundamental understanding of how to play, while also explaining some features that may not be made explicitly known during normal play.","ALERT: Potential suspicious activity detected, Programmers report to Factory Zone 4Z.","A damaged part is partially visible under some rubble. There's a note attached: \"Everyone keeps telling us we should avoid high-traffic areas because that's where patrols are more likely to show up, but that doesn't do us much good if we can't spot those areas until they're on top of us... With this we can! Deliver it to Megawrench for repairs then we'll never be afraid of an ambush while sneaking around Materials again, ha.\""};
 for(unsigned kind=0;kind<3;kind++){
  int width=kind?94:68;void *temp=(*(NativeNew*)va(0xb6132c))(0x6c);((ConsoleCtor)va(0x48c060))(temp,parent,width,200,0,0,0,1,-1);
  unsigned char input[28]={0};((StringCtor)va(0x9af0f0))(input,sources[kind]);int rows=0;int accepted=scratch_paragraph(temp,0,0,width,kind==2?5:200,input,va(kind?0x8089d1:0x790a33),&rows);((NativeVoid)va(0x9af350))(input);int ok=accepted&&rows>0;
  for(int row=0;ok&&row<rows;row++){
   unsigned char line[28]={0};int point[2]={0,row};ok=scratch_extract(temp,line,point,width,va(kind?0x808a12:0x790a92))!=NULL;if(!ok)break;
   void *o=(*(NativeNew*)va(0xb6132c))(0x70);
   if(!kind){typedef void* (__attribute__((thiscall)) *Ctor)(void*,void*,int,void*,unsigned char);((Ctor)va(0x493010))(o,parent,0,line,0);}
   else{((ConsoleCtor)va(0x48c060))(o,parent,width,1,0,0,0,1,-1);ok=scratch_row_print(o,0,0,line,va(0x808ce3));}
   ((NativeVoid)va(0x9af350))(line);uint32_t *v=o;unsigned chinese=0,bad=0;
   for(unsigned x=0;x<v[2];x++){unsigned char *cell=(void*)(v[4]+x*20);unsigned g=*(uint32_t*)(cell+4);if(g>=320&&g<320+2*zh_registry_count&&!(g&1)){chinese++;if(x+1>=v[2]||*(uint32_t*)(cell+24)!=g+1||memcmp(cell+12,cell+32,6))bad++;}}
   unsigned id=scratch_context.first+(unsigned)row,en_bad=0,en_runs=0,marker_bad=0;
   unsigned char *english_view=live_cells_view(o),*grid=english_view?english_view:(void*)v[4];ZhPlan expected_en={0},expected_cn={0};
   int en_planned=!zh_plan(scratch_rows[id].english,strlen(scratch_rows[id].english),width,1,&expected_en);
   int cn_planned=!message_plan_entry(TEXT_ENTRY_SCRATCH,scratch_rows[id].text,strlen(scratch_rows[id].text),width,1,&expected_cn);
   if(!en_planned||!cn_planned)en_bad++;
   for(size_t i=0;i<expected_en.count;i++){
    ZhRun *r=&expected_en.runs[i];en_runs++;
    for(int half=0;half<r->width;half++){
     unsigned col=(unsigned)(r->x+half);if(col>=v[2]||*(uint32_t*)(grid+col*v[3]*20+4)!=r->token[half])en_bad++;
    }
   }
   /* A marker must never become visible in either language; checking its
      identifying prefix also catches an English-view cache of the ASCII ID. */
   for(unsigned col=0;col+7<v[2];col++){
    unsigned marker=*(uint32_t*)(grid+col*v[3]*20+4)=='Z';
    for(unsigned j=1;marker&&j<7;j++){unsigned g=*(uint32_t*)(grid+(col+j)*v[3]*20+4);marker=g>='0'&&g<='9';}
    if(marker&&*(uint32_t*)(grid+(col+7)*v[3]*20+4)=='|')marker_bad++;
   }
   ok=ok&&(!expected_cn.count||chinese>0)&&!bad&&!en_bad&&!marker_bad;
   fprintf(logfile,"SCRATCH-FIXTURE kind=%u row=%d chinese=%u pair-errors=%u english-runs=%u english-errors=%u marker-visible=%u passed=%d\n",kind,row,chinese,bad,en_runs,en_bad,marker_bad,ok);
   free(english_view);zh_plan_free(&expected_en);zh_plan_free(&expected_cn);
   uint32_t *pv=(void*)((char*)parent+0x44);unsigned count=vector_count(pv);for(unsigned j=0;j<count;j++)if(((void**)pv[0])[j]==o){memmove((void*)(pv[0]+j*4),(void*)(pv[0]+(j+1)*4),(count-j-1)*4);pv[1]-=4;break;}((DeleteFn)(*(uintptr_t**)o)[0])(o,1);
  }
  uint32_t *pv=(void*)((char*)parent+0x44);unsigned count=vector_count(pv);for(unsigned j=0;j<count;j++)if(((void**)pv[0])[j]==temp){memmove((void*)(pv[0]+j*4),(void*)(pv[0]+(j+1)*4),(count-j-1)*4);pv[1]-=4;break;}((DeleteFn)(*(uintptr_t**)temp)[0])(temp,1);passed+=ok;
 }
 /* Story-event bottom panel has a distinct centered final paragraph caller. */
 void *event=(*(NativeNew*)va(0xb6132c))(0x6c);((ConsoleCtor)va(0x48c060))(event,parent,94,5,0,0,0,1,-1);
 unsigned char input[28]={0};((StringCtor)va(0x9af0f0))(input,sources[2]);uintptr_t saved_site=display_origin_site;display_origin_site=va(0x875329);int result;
 int accepted=display_bridge(event,47,0,94,5,1,3,input,&result);display_origin_site=saved_site;((NativeVoid)va(0x9af350))(input);
 uint32_t *ev=event;unsigned cjk=0,bad=0;
 for(unsigned x=0;x<ev[2];x++)for(unsigned y=0;y<ev[3];y++){
  unsigned char *cell=(void*)(ev[4]+(x*ev[3]+y)*20);unsigned g=*(uint32_t*)(cell+4);
  if(g>=320&&g<320+2*zh_registry_count&&!(g&1)){cjk++;if(x+1>=ev[2]||*(uint32_t*)(cell+ev[3]*20+4)!=g+1||memcmp(cell+12,cell+ev[3]*20+12,6))bad++;}
 }
 unsigned char *english=live_cells_view(event);unsigned en=0;
 if(english)for(unsigned x=0;x<ev[2];x++)for(unsigned y=0;y<ev[3];y++){unsigned g=*(uint32_t*)(english+(x*ev[3]+y)*20+4);en+=g>=320;}
 int event_ok=accepted&&cjk&&!bad&&english&&!en;passed+=event_ok;free(english);
 fprintf(logfile,"STORY-POPUP-FIXTURE accepted=%d cjk=%u pair-errors=%u english-cjk=%u passed=%d\n",accepted,cjk,bad,en,event_ok);
 uint32_t *children=(void*)((char*)parent+0x44);for(unsigned j=0;j<vector_count(children);j++)if(((void**)children[0])[j]==event){live_cells_remove(event);typedef void (__cdecl *Remove)(void*,unsigned);((Remove)va(0x9cea50))(children,j);break;}
 memset(&scratch_context,0,sizeof scratch_context);fprintf(logfile,"SCRATCH-FIXTURE-SUMMARY passed=%u expected=4 child-before=%u child-after=%u\n",passed,before,vector_count((char*)parent+0x44));fflush(logfile);
 #ifdef SCRATCH_EXIT_PROBE
 ExitProcess(passed==4&&before==vector_count((char*)parent+0x44)?0:87);
 #endif
}
#endif

#ifndef COGMIND_MENU_COMPACT_H
#define COGMIND_MENU_COMPACT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Pure selection policy, also exercised by menu_compact_tests.c. */
static int menu_compact_width_policy(uintptr_t caller,uintptr_t base,int same_string,int original_columns,int target_columns){
 if(!same_string||original_columns<1||original_columns>35||target_columns<1||target_columns>original_columns)return 0;
 return caller==base+0x4fc943u||caller==base+0x4fca24u||caller==base+0x4fcad2u;
}
/* Map semantic boundaries rather than old English cell indices. */
static int menu_utf8_columns(const char *s,size_t n){int w=0;for(size_t i=0;i<n;i++)if(((unsigned char)s[i]&0xc0)!=0x80)w+=(unsigned char)s[i]<128?1:2;return w;}
static int menu_boundary(const char *source,const char *target,int at){
 size_t n=strlen(source),m=strlen(target);if(at<0||(size_t)at>n)return -1;
 if(!at)return 0;if((size_t)at==n)return menu_utf8_columns(target,m);
 const char *verbs[]={"Schematic","Record"},*labels[]={"图纸","记录"};
 for(unsigned i=0;i<2;i++){size_t k=strlen(verbs[i]),v=strlen(labels[i]);
  if(n<k||m<v||memcmp(source,verbs[i],k)||memcmp(target,labels[i],v))continue;
  if((size_t)at==k)return menu_utf8_columns(target,v);
  size_t a=k,b=v;while(a<n&&b<m&&source[a]==target[b]&&(source[a]==' '||source[a]=='!'||source[a]=='"')){
   a++;b++;if((size_t)at==a)return menu_utf8_columns(target,b);
  }
  if(source[n-1]=='"'&&target[m-1]=='"'&&(size_t)at==n-1)return menu_utf8_columns(target,m-1);
 }
 return -1;
}
static int menu_color_bounds(const char *source,const char *target,int start,int end,int *mapped_start,int *mapped_end){
 if(start<0||end<start)return 0;int a=menu_boundary(source,target,start),b=menu_boundary(source,target,end+1);
 if(a<0||b<=a)return 0;*mapped_start=a;*mapped_end=b-1;return 1;
}
#ifndef MENU_COMPACT_PURE
/* Include after display_bridge.h. No input/native strings or descriptors are
   changed. Only a committed successful menu print can authorize measurement. */
typedef void (__attribute__((thiscall)) *MenuCompactRedrawFn)(void*,uintptr_t,uintptr_t);
typedef int (__attribute__((thiscall)) *MenuCompactMeasureFn)(void*);
static MenuCompactRedrawFn original_menu_compact_redraw;
static MenuCompactMeasureFn original_menu_compact_measure;
typedef struct {void *owner,*string;int original_columns,target_columns,ready;unsigned overrides;char source[256],target[256];} MenuCompactContext;
static _Thread_local MenuCompactContext *menu_compact_context;
static unsigned menu_compact_commits,menu_compact_overrides;
/* Call ONLY after successful translated native rendering, with the unpadded
   translation's physical columns. Do not call for fallback or planning only. */
static int menu_compact_note_print(void *owner,int x,int y,void *string,const char *target){
 if(x==5&&!y)terminal_input_note_print(owner,string,target);
 int target_columns=display_columns(target);
 MenuCompactContext *c=menu_compact_context;
 if(!c||c->owner!=owner||x!=5||y||!original_menu_compact_measure||!readable(owner,0x9c)||!readable(string,24)||*(uintptr_t*)owner!=va(0xc361bc))return 0;
 int original_columns=original_menu_compact_measure(string);
 if(!menu_compact_width_policy(va(0x8fc943),va(0x400000),1,original_columns,target_columns))return 0;
 uint32_t *ns=string;const char *source=ns[5]<16?string:*(void**)string;
 if(ns[4]>=sizeof c->source||strlen(target)>=sizeof c->target||!readable(source,ns[4]))return 0;
 memcpy(c->source,source,ns[4]);c->source[ns[4]]=0;strcpy(c->target,target);
 c->string=string;c->original_columns=original_columns;c->target_columns=target_columns;c->ready=1;menu_compact_commits++;
 fprintf(logfile,"MENU-COMPACT-COMMIT owner=%p string=%p source-columns=%d target-columns=%d\n",owner,string,original_columns,target_columns);fflush(logfile);return 1;
}
static int __attribute__((fastcall)) hooked_menu_compact_measure(void *string,void *unused){
 (void)unused;uintptr_t caller=(uintptr_t)__builtin_return_address(0);int value=original_menu_compact_measure(string);MenuCompactContext *c=menu_compact_context;
 if(c&&c->ready&&value==c->original_columns&&menu_compact_width_policy(caller,va(0x400000),string==c->string,c->original_columns,c->target_columns)){
  c->overrides++;menu_compact_overrides++;return c->target_columns;
 }
 return value;
}
static void __attribute__((fastcall)) hooked_menu_compact_redraw(void *owner,void *unused,uintptr_t arg1,uintptr_t arg2){
 (void)unused;MenuCompactContext c={0};MenuCompactContext *saved=menu_compact_context;
 int prior_english=live_native_english;live_native_english=1;menu_compact_context=NULL;original_menu_compact_redraw(owner,arg1,arg2);live_native_english=prior_english;
 LiveCellsSnapshot *live_menu_snapshot=live_cells_capture_full(owner);
 c.owner=owner;menu_compact_context=&c;original_menu_compact_redraw(owner,arg1,arg2);menu_compact_context=saved;
 if(live_menu_snapshot)live_cells_commit(live_menu_snapshot);
 if(c.ready){fprintf(logfile,"MENU-COMPACT-END owner=%p overrides=%u source-columns=%d target-columns=%d\n",owner,c.overrides,c.original_columns,c.target_columns);fflush(logfile);}
}
static unsigned char __cdecl menu_color_range(int start,int index,int end){
 MenuCompactContext *c=menu_compact_context;int a=start,b=end;
 if(c&&c->ready&&menu_color_bounds(c->source,c->target,start,end,&a,&b)){
  if(!index){fprintf(logfile,"MENU-COLOR source=%d..%d target=%d..%d text=%s\n",start,end,a,b,c->target);fflush(logfile);}
 }
 return a<=index&&index<=b;
}
static int menu_color_install(void){
 unsigned char *p=(void*)va(0x8fc97e);if(p[0]!=0xe8||p+5+*(int32_t*)(p+1)!=(unsigned char*)va(0x9daf80))return 0;
 DWORD old;if(!VirtualProtect(p,5,PAGE_EXECUTE_READWRITE,&old))return 0;
 *(int32_t*)(p+1)=(int32_t)((unsigned char*)menu_color_range-(p+5));FlushInstructionCache(GetCurrentProcess(),p,5);VirtualProtect(p,5,old,&old);return 1;
}
/* hook() must use 7 stolen bytes at 44AF50, 5 at 8FBB70. */
static int menu_compact_install(void){
 original_menu_compact_measure=(MenuCompactMeasureFn)hook(0x44af50,hooked_menu_compact_measure);
 if(!original_menu_compact_measure)return 0;
 original_menu_compact_redraw=(MenuCompactRedrawFn)hook(0x8fbb70,hooked_menu_compact_redraw);
 return original_menu_compact_redraw!=NULL&&menu_color_install();
}
#endif
#endif

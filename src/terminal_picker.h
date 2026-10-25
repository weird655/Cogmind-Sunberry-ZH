#ifndef SUNBERRY_TERMINAL_PICKER_H
#define SUNBERRY_TERMINAL_PICKER_H
#include "terminal_input.h"
#include "terminal_command_aliases.h"
#include <stdlib.h>
#include <stdio.h>

#define PICKER_ROOT ((unsigned)-1)
#define PICKER_NODES (INPUT_ROWS * 3)
enum { PICKER_GROUP=1, PICKER_FAMILY=2, PICKER_COMMAND=3 };
typedef struct {
 unsigned parent,row,kind;
 char label[INPUT_TEXT],detail[INPUT_NATIVE];
 uint64_t target;
 char native[INPUT_NATIVE];
} PickerNode;
typedef struct {
 uint64_t scope,registry_stamp;
 unsigned count;
 PickerNode nodes[PICKER_NODES];
} PickerModel;
typedef struct {const char *verb,*label;unsigned group;} PickerRoute;
static const char *const picker_group_labels[]={
 "查询与情报","通道与地图","系统与安保","生产与物资","控制与干预"
};
/* Whole, reviewed native verbs only. A Chinese word never selects a route. */
static const PickerRoute picker_routes[]={
 {"Query","查询记录",0},{"Schematic","制造图纸",0},{"Analysis","结构分析",0},
 {"Prototypes","原型资料",0},{"Enumerate","小队与设施状态",0},
 {"Manifests","搬运工清单",0},{"Download","下载资料",0},
 {"Protocatalog","原型目录",0},{"Network","网络状态",0},
 {"Index","设施索引",1},{"Layout","区域布局",1},{"Open","开启通道",1},
 {"Access","访问通道",1},{"Alert","警报控制",2},{"Traps","陷阱操作",2},
 {"Inventory","库存查询",3},{"Load","装载物品",3},{"Build","制造物品",3},
 {"Scan","扫描物品",3},{"Repair","维修物品",3},{"Refit","改装物品",3},
 {"Recycle","回收物品",3},{"Recycling","回收状态",3},{"Retrieve","提取物资",3},
 {"Insert","放入物品",3},{"Scanalyze","扫描分析",3},{"Study","研究物品",3},
 {"Couplers","耦合器",3},{"Recall","召回队伍",4},{"Control","设施控制",4},
 {"Trojan","木马指令",4},{"Force","强制指令",4},{"Unlock","解锁",4},
 {"Seal","封锁",4}
};
#define PICKER_ROUTES (sizeof picker_routes/sizeof picker_routes[0])

static inline size_t picker_bounded_length(const char *s,size_t cap){
 const char *end=(const char*)memchr(s,0,cap);return end?(size_t)(end-s):cap;
}
static inline int picker_registry_valid(const InputRegistry *r){
 if(!r||r->count>INPUT_ROWS)return 0;
 for(unsigned i=0;i<r->count;i++){
  const InputRow *row=&r->rows[i];
  size_t a=picker_bounded_length(row->alias,sizeof row->alias),n=picker_bounded_length(row->native,sizeof row->native);
  if(!row->target||!a||a==sizeof row->alias||!n||n==sizeof row->native)return 0;
  char normalized[INPUT_TEXT];if(!input_normalize(row->alias,a,normalized,sizeof normalized))return 0;
  for(size_t j=0;j<n;j++)if((unsigned char)row->native[j]<32||(unsigned char)row->native[j]>126)return 0;
 }
 return 1;
}
static inline uint64_t picker_hash_bytes(uint64_t h,const void *bytes,size_t size){
 const unsigned char *p=(const unsigned char*)bytes;
 for(size_t i=0;i<size;i++){h^=p[i];h*=UINT64_C(1099511628211);}return h;
}
static inline uint64_t picker_registry_stamp(const InputRegistry *r){
 uint64_t h=UINT64_C(14695981039346656037);
 h=picker_hash_bytes(h,&r->scope,sizeof r->scope);h=picker_hash_bytes(h,&r->count,sizeof r->count);
 for(unsigned i=0;i<r->count;i++){
  h=picker_hash_bytes(h,r->rows[i].alias,strlen(r->rows[i].alias)+1);
  h=picker_hash_bytes(h,r->rows[i].native,strlen(r->rows[i].native)+1);
  h=picker_hash_bytes(h,&r->rows[i].target,sizeof r->rows[i].target);
 }return h;
}
/* Enter carries a command string, not our record token. Distinct targets with
   the same full native spelling cannot be selected unambiguously this way. */
static inline int picker_native_ambiguous(const InputRegistry *r,unsigned row){
 for(unsigned i=0;i<r->count;i++)if(r->rows[i].target!=r->rows[row].target){
  const unsigned char *a=(const void*)r->rows[i].native,*b=(const void*)r->rows[row].native;
  while(*a&&*b){unsigned ca=*a,cb=*b;if(ca>='a'&&ca<='z')ca-='a'-'A';if(cb>='a'&&cb<='z')cb-='a'-'A';if(ca!=cb)break;a++;b++;}
  if(!*a&&!*b)return 1;
 }
 return 0;
}
static inline int picker_has_chinese(const char *s){
 size_t left=strlen(s);while(left){size_t used;uint32_t cp;if(!zh_decode(s,left,&used,&cp))return 0;
  if((cp>=0x3400&&cp<=0x9fff)||(cp>=0x20000&&cp<=0x323af))return 1;s+=used;left-=used;
 }return 0;
}
/* Match a complete reviewed spelling, or a reviewed placeholder grammar with
   an actual nonempty parameter. Placeholder spellings themselves are hidden. */
static inline int picker_reviewed(const char *native){
 if(strchr(native,'*')||strchr(native,'@'))return -1;
 size_t n=strlen(native);
 for(unsigned i=0;i<sizeof input_command_aliases/sizeof input_command_aliases[0];i++){
  const char *pattern=input_command_aliases[i].native,*slot=strchr(pattern,'*');if(!slot)slot=strchr(pattern,'@');
  if(!slot){if(!strcmp(native,pattern))return (int)i;continue;}
  size_t before=(size_t)(slot-pattern),after=strlen(slot+1);
  if(n>before+after&&!strncmp(native,pattern,before)&&!strcmp(native+n-after,slot+1))return (int)i;
 }return -1;
}
static inline int picker_route(const char *native){
 const char *paren=strchr(native,'(');size_t verb=paren?(size_t)(paren-native):strlen(native);
 for(unsigned i=0;i<PICKER_ROUTES;i++)if(strlen(picker_routes[i].verb)==verb&&!memcmp(native,picker_routes[i].verb,verb))return (int)i;
 return -1;
}
static inline unsigned picker_add(PickerModel *m,unsigned parent,unsigned row,unsigned kind,const char *label){
 if(m->count>=PICKER_NODES)return PICKER_ROOT;unsigned index=m->count++;
 PickerNode *node=&m->nodes[index];node->parent=parent;node->row=row;node->kind=kind;
 snprintf(node->label,sizeof node->label,"%s",label);return index;
}
static inline void picker_command_label(const InputRow *row,int reviewed,const PickerRoute *route,char *out,size_t cap){
 const InputCommandAlias *known=&input_command_aliases[reviewed];
 if(!strchr(known->native,'*')&&!strchr(known->native,'@'))snprintf(out,cap,"%s",known->chinese);
 else if(picker_has_chinese(row->alias))snprintf(out,cap,"%s",row->alias);
 else {
  const char *param=strchr(row->native,'(');size_t bytes=param?strlen(param):0;
  if(bytes>2)snprintf(out,cap,"%s（%.*s）",route->label,(int)(bytes-2),param+1);
  else snprintf(out,cap,"%s",route->label);
 }
}
/* 1 built (including an empty tree), -3 stale scope, -1 invalid/allocation.
   On failure the old model is untouched. Heap staging avoids the 1 MB stack. */
static inline int picker_build(PickerModel *model,const InputRegistry *registry,uint64_t scope){
 if(!model||!registry)return -1;if(!scope||registry->scope!=scope)return -3;
 if(!picker_registry_valid(registry))return -1;
 PickerModel *next=(PickerModel*)calloc(1,sizeof *next);if(!next)return -1;
 next->scope=scope;next->registry_stamp=picker_registry_stamp(registry);
 unsigned group_nodes[5],family_nodes[PICKER_ROUTES];
 int reviewed_rows[INPUT_ROWS],route_rows[INPUT_ROWS];
 for(unsigned i=0;i<5;i++)group_nodes[i]=PICKER_ROOT;
 for(unsigned i=0;i<PICKER_ROUTES;i++)family_nodes[i]=PICKER_ROOT;
 for(unsigned i=0;i<registry->count;i++){
  reviewed_rows[i]=picker_native_ambiguous(registry,i)?-1:picker_reviewed(registry->rows[i].native);
  route_rows[i]=reviewed_rows[i]<0?-1:picker_route(registry->rows[i].native);
 }
 for(unsigned route_index=0;route_index<PICKER_ROUTES;route_index++)for(unsigned row_index=0;row_index<registry->count;row_index++){
  const InputRow *row=&registry->rows[row_index];int reviewed=reviewed_rows[row_index];
  if(reviewed<0||route_rows[row_index]!=(int)route_index)continue;
  unsigned duplicate=PICKER_ROOT;
  for(unsigned i=0;i<next->count;i++)if(next->nodes[i].kind==PICKER_COMMAND&&next->nodes[i].target==row->target&&!strcmp(next->nodes[i].native,row->native)){duplicate=i;break;}
  if(duplicate!=PICKER_ROOT){
   PickerNode *node=&next->nodes[duplicate];
   if(!picker_has_chinese(registry->rows[node->row].alias)&&picker_has_chinese(row->alias)){
    node->row=row_index;picker_command_label(row,reviewed,&picker_routes[route_index],node->label,sizeof node->label);
   }continue;
  }
  const PickerRoute *route=&picker_routes[route_index];
  if(group_nodes[route->group]==PICKER_ROOT)group_nodes[route->group]=picker_add(next,PICKER_ROOT,PICKER_ROOT,PICKER_GROUP,picker_group_labels[route->group]);
  if(family_nodes[route_index]==PICKER_ROOT)family_nodes[route_index]=picker_add(next,group_nodes[route->group],PICKER_ROOT,PICKER_FAMILY,route->label);
  unsigned node_index=picker_add(next,family_nodes[route_index],row_index,PICKER_COMMAND,"");
  if(group_nodes[route->group]==PICKER_ROOT||family_nodes[route_index]==PICKER_ROOT||node_index==PICKER_ROOT){free(next);return -1;}
  PickerNode *node=&next->nodes[node_index];node->target=row->target;
  strcpy(node->native,row->native);strcpy(node->detail,row->native);
  picker_command_label(row,reviewed,route,node->label,sizeof node->label);
 }
 /* Equal translated labels remain distinguishable when their actual native
    command strings differ. Equal native strings were rejected above. */
 unsigned char ambiguous[PICKER_NODES]={0};
 for(unsigned i=0;i<next->count;i++)if(next->nodes[i].kind==PICKER_COMMAND)
  for(unsigned j=0;j<next->count;j++)if(i!=j&&next->nodes[j].kind==PICKER_COMMAND&&
   (!strcmp(next->nodes[i].label,next->nodes[j].label)||!strcmp(next->nodes[i].native,next->nodes[j].native))){ambiguous[i]=1;break;}
 for(unsigned i=0;i<next->count;i++)if(ambiguous[i]){
   char base[INPUT_TEXT];strcpy(base,next->nodes[i].label);
   size_t prefix=strlen(base),limit=sizeof next->nodes[i].label-40;
   if(prefix>limit){prefix=limit;while(prefix&&((unsigned char)base[prefix]&0xc0)==0x80)prefix--;}
   snprintf(next->nodes[i].label,sizeof next->nodes[i].label,"%.*s [记录 %llu]",(int)prefix,base,(unsigned long long)next->nodes[i].target);
 }
 memcpy(model,next,sizeof *model);free(next);return 1;
}
/* Exact node identity, never a reverse lookup from a translated label.
   1 resolved, -3 stale snapshot/scope, -1 invalid/non-leaf/capacity.
   Failure never changes out. Caller still performs native context validation. */
static inline int picker_resolve(const PickerModel *model,unsigned selected,const InputRegistry *registry,uint64_t scope,char *out,size_t cap){
 if(!model||!registry||!out||!cap||model->count>PICKER_NODES||selected>=model->count)return -1;
 if(!scope||model->scope!=scope||registry->scope!=scope)return -3;
 if(!picker_registry_valid(registry))return -1;
 if(model->registry_stamp!=picker_registry_stamp(registry))return -3;
 const PickerNode *node=&model->nodes[selected];if(node->kind!=PICKER_COMMAND)return -1;
 if(node->row>=registry->count)return -3;
 const InputRow *row=&registry->rows[node->row];
 if(row->target!=node->target||strcmp(row->native,node->native)||picker_reviewed(row->native)<0||picker_native_ambiguous(registry,node->row))return -3;
 size_t bytes=strlen(row->native);if(bytes>=cap)return -1;memcpy(out,row->native,bytes+1);return 1;
}
#endif

#include <stdio.h>
#include "terminal_picker.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"picker FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static PickerModel model,snapshot;
static InputRegistry registry,changed;
static unsigned leaf_count(const PickerModel *m){unsigned count=0;for(unsigned i=0;i<m->count;i++)if(m->nodes[i].kind==PICKER_COMMAND)count++;return count;}
static unsigned leaf(const PickerModel *m,uint64_t target){for(unsigned i=0;i<m->count;i++)if(m->nodes[i].kind==PICKER_COMMAND&&m->nodes[i].target==target)return i;return PICKER_ROOT;}
int main(void){
 char out[INPUT_NATIVE]="unchanged";registry.scope=123;
 CHECK(picker_build(&model,&registry,123)==1);CHECK(model.count==0);
 CHECK(input_register(&registry,"运输状态","Enumerate(Transport)",1));
 CHECK(input_register(&registry,"枚举(运输)","Enumerate(Transport)",1));
 CHECK(input_register(&registry,"Query(Storage)","Query(Storage)",2));
 CHECK(input_register(&registry,"记录：仓库","Query(Storage)",2));
 CHECK(input_register(&registry,"制造","Build",3));
 CHECK(picker_build(&model,&registry,123)==1);CHECK(leaf_count(&model)==3);
 for(unsigned i=0;i<model.count;i++){
  const PickerNode *n=&model.nodes[i];
  CHECK(n->kind>=PICKER_GROUP&&n->kind<=PICKER_COMMAND);
  if(n->kind==PICKER_GROUP)CHECK(n->parent==PICKER_ROOT);
  else {CHECK(n->parent<i);CHECK(model.nodes[n->parent].kind==(n->kind==PICKER_FAMILY?PICKER_GROUP:PICKER_FAMILY));}
 }
 CHECK(!strcmp(model.nodes[leaf(&model,1)].label,"运输状态"));
 CHECK(!strcmp(model.nodes[leaf(&model,2)].label,"记录：仓库"));
 CHECK(model.nodes[leaf(&model,2)].row==3);
 CHECK(picker_resolve(&model,leaf(&model,1),&registry,123,out,sizeof out)==1);CHECK(!strcmp(out,"Enumerate(Transport)"));
 strcpy(out,"unchanged");CHECK(picker_resolve(&model,0,&registry,123,out,sizeof out)==-1);CHECK(!strcmp(out,"unchanged"));
 CHECK(picker_resolve(&model,leaf(&model,1),&registry,123,out,2)==-1);CHECK(!strcmp(out,"unchanged"));
 CHECK(picker_resolve(&model,PICKER_ROOT,&registry,123,out,sizeof out)==-1);
 CHECK(picker_resolve(&model,leaf(&model,1),&registry,124,out,sizeof out)==-3);CHECK(!strcmp(out,"unchanged"));
 snapshot=model;CHECK(picker_build(&model,&registry,124)==-3);CHECK(!memcmp(&snapshot,&model,sizeof model));
 changed=registry;changed.count=INPUT_ROWS+1;CHECK(picker_build(&model,&changed,123)==-1);CHECK(!memcmp(&snapshot,&model,sizeof model));
 changed=registry;memset(changed.rows[0].native,'x',INPUT_NATIVE);CHECK(picker_build(&model,&changed,123)==-1);CHECK(!memcmp(&snapshot,&model,sizeof model));
 changed=registry;strcpy(changed.rows[0].alias,"新别名");CHECK(picker_resolve(&model,leaf(&model,1),&changed,123,out,sizeof out)==-3);
 changed=registry;changed.rows[0].target=12;CHECK(picker_resolve(&model,leaf(&model,1),&changed,123,out,sizeof out)==-3);
 changed=registry;strcpy(changed.rows[0].native,"Enumerate(Guards)");CHECK(picker_resolve(&model,leaf(&model,1),&changed,123,out,sizeof out)==-3);
 changed=registry;InputRow tmp=changed.rows[0];changed.rows[0]=changed.rows[1];changed.rows[1]=tmp;
 CHECK(picker_resolve(&model,leaf(&model,1),&changed,123,out,sizeof out)==-3);
 CHECK(!strcmp(out,"unchanged"));

 memset(&registry,0,sizeof registry);registry.scope=456;
 CHECK(input_register(&registry,"同名记录","Query(Same)",41));
 CHECK(input_register(&registry,"同名记录","Query(Same)",42));
 CHECK(input_register(&registry,"同名记录","Query(same)",45));
 CHECK(input_register(&registry,"同名记录","Query(Different)",43));
 CHECK(picker_build(&model,&registry,456)==1);CHECK(leaf_count(&model)==1);
 CHECK(leaf(&model,41)==PICKER_ROOT);CHECK(leaf(&model,42)==PICKER_ROOT);CHECK(leaf(&model,45)==PICKER_ROOT);
 CHECK(picker_resolve(&model,leaf(&model,43),&registry,456,out,sizeof out)==1);
 CHECK(!strcmp(out,"Query(Different)"));
 /* A token cannot make a shared spelling choose a different native record. */
 snapshot=model;model.count=1;model.nodes[0].kind=PICKER_COMMAND;
 model.nodes[0].row=0;model.nodes[0].target=41;strcpy(model.nodes[0].native,"Query(Same)");
 strcpy(out,"unchanged");CHECK(picker_resolve(&model,0,&registry,456,out,sizeof out)==-3);
 CHECK(!strcmp(out,"unchanged"));model=snapshot;
 CHECK(input_register(&registry,"同名记录","Query(Another)",44));
 CHECK(picker_build(&model,&registry,456)==1);CHECK(leaf_count(&model)==2);
 for(unsigned i=0;i<model.count;i++)if(model.nodes[i].kind==PICKER_COMMAND){
  CHECK(strstr(model.nodes[i].label,"记录 ")!=NULL);
  CHECK(picker_resolve(&model,i,&registry,456,out,sizeof out)==1);
  CHECK(!strcmp(out,registry.rows[model.nodes[i].row].native));
 }
 CHECK(strcmp(model.nodes[leaf(&model,43)].label,model.nodes[leaf(&model,44)].label));

 memset(&registry,0,sizeof registry);registry.scope=789;
 CHECK(input_register(&registry,"未选记录","Query(*)",1));
 CHECK(input_register(&registry,"未选部件","Load(@)",2));
 CHECK(input_register(&registry,"未知命令","NewCommand(X)",3));
 CHECK(input_register(&registry,"空参数","Query()",4));
 CHECK(input_register(&registry,"伪造后缀","Build(X)",5));
 CHECK(input_register(&registry,"坏尾部","Query(X)tail",6));
 CHECK(input_register(&registry,"有效记录","Query(Unknown future name)",7));
 CHECK(picker_build(&model,&registry,789)==1);CHECK(leaf_count(&model)==1);
 CHECK(picker_resolve(&model,leaf(&model,7),&registry,789,out,sizeof out)==1);
 CHECK(!strcmp(out,"Query(Unknown future name)"));

 memset(&registry,0,sizeof registry);registry.scope=890;
 for(unsigned i=0;i<sizeof input_command_aliases/sizeof input_command_aliases[0];i++){
  const InputCommandAlias *a=&input_command_aliases[i];char native[INPUT_NATIVE],alias[INPUT_TEXT];
  const char *slot=strchr(a->native,'*');if(!slot)slot=strchr(a->native,'@');
  if(slot)snprintf(native,sizeof native,"%.*sVerified record %u%s",(int)(slot-a->native),a->native,i,slot+1);else strcpy(native,a->native);
  snprintf(alias,sizeof alias,"中文命令%u",i);
  CHECK(input_register(&registry,alias,native,(uint64_t)i+1));
 }
 CHECK(registry.count==111);CHECK(picker_build(&model,&registry,890)==1);CHECK(leaf_count(&model)==111);
 for(unsigned i=0;i<model.count;i++)if(model.nodes[i].kind==PICKER_COMMAND)CHECK(picker_resolve(&model,i,&registry,890,out,sizeof out)==1);

 memset(&registry,0,sizeof registry);registry.scope=999;
 for(unsigned i=0;i<INPUT_ROWS;i++){
  char alias[100],native[100];snprintf(alias,sizeof alias,"同名资料%u",i);snprintf(native,sizeof native,"Query(Record %u)",i);
  CHECK(input_register(&registry,alias,native,(uint64_t)i+1));
 }
 CHECK(picker_build(&model,&registry,999)==1);CHECK(leaf_count(&model)==INPUT_ROWS);CHECK(model.count==INPUT_ROWS+2);
 CHECK(picker_resolve(&model,leaf(&model,256),&registry,999,out,sizeof out)==1);CHECK(!strcmp(out,"Query(Record 255)"));
 /* A long Chinese alias remains valid UTF-8 after adding a record suffix. */
 memset(&registry,0,sizeof registry);registry.scope=1001;
 char long_alias[INPUT_TEXT];long_alias[0]=0;for(unsigned i=0;i<250;i++)strcat(long_alias,"中");
 CHECK(input_register(&registry,long_alias,"Query(Long)",UINT64_MAX));
 CHECK(input_register(&registry,long_alias,"Query(Longer)",UINT64_MAX-1));
 CHECK(picker_build(&model,&registry,1001)==1);
 for(unsigned i=0;i<model.count;i++)if(model.nodes[i].kind==PICKER_COMMAND){
  char normalized[INPUT_TEXT];CHECK(input_normalize(model.nodes[i].label,strlen(model.nodes[i].label),normalized,sizeof normalized));
  CHECK(strstr(model.nodes[i].label,"1844674407370955161")!=NULL);
 }
 puts("PASS terminal picker: hierarchy, 111 reviewed grammars, exact identity, duplicate aliases, ambiguous records, stale snapshots, atomic failure, 256 rows and UTF-8 labels");
 return 0;
}

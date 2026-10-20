#include <stdio.h>
#include "terminal_input.h"
#include "terminal_command_aliases.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"input FAIL line %d\n",__LINE__);return 1;}}while(0)
int main(void){
 static InputRegistry r;r.scope=123;char out[256]="unchanged";
 CHECK(input_register(&r,"枚举(运输)","Enumerate(Transport)",1));
 CHECK(input_register(&r,"运输状态","Enumerate(Transport)",1));
 CHECK(input_register(&r,"记录 \"回收单元\"","Query(Recycling Unit)",2));
 CHECK(input_resolve(&r,123," 枚举（运输） ",out,sizeof out)==1);CHECK(!strcmp(out,"Enumerate(Transport)"));
 CHECK(input_resolve(&r,123,"记录 “回收单元”",out,sizeof out)==1);CHECK(!strcmp(out,"Query(Recycling Unit)"));
 strcpy(out,"unchanged");CHECK(input_resolve(&r,124,"运输状态",out,sizeof out)==-3);CHECK(!strcmp(out,"unchanged"));
 CHECK(!input_resolve(&r,123,"运输",out,sizeof out));CHECK(!strcmp(out,"unchanged"));
 CHECK(!input_resolve(&r,123,"运输状态;警戒等级",out,sizeof out));CHECK(!strcmp(out,"unchanged"));
 CHECK(input_resolve(&r,123,"运输状态",out,2)==-1);CHECK(!strcmp(out,"unchanged"));
 CHECK(input_register(&r,"运输状态","Other(Transport)",3));
 CHECK(input_resolve(&r,123,"运输状态",out,sizeof out)==-2);CHECK(!strcmp(out,"unchanged"));
 CHECK(input_register(&r,"同名记录","Query(Same)",4));CHECK(input_register(&r,"同名记录","Query(Same)",5));
 CHECK(input_resolve(&r,123,"同名记录",out,sizeof out)==-2);
 CHECK(!input_register(&r,"未知","Query(中文)",6));CHECK(!input_register(&r,"\xc0\xaf","Bad",6));
 unsigned indices[6];CHECK(input_suggest(&r,123,"运",indices,6)==1);CHECK(!strcmp(r.rows[indices[0]].alias,"枚举(运输)"));
 CHECK(input_suggest(&r,124,"运",indices,6)==0);CHECK(input_suggest(&r,123,"同名",indices,6)==0);CHECK(input_suggest(&r,123,"",indices,6)==0);
 CHECK(input_suggest(&r,123,"回收",indices,6)==1);CHECK(input_suggest(&r,123,"不存在",indices,6)==0);
 static InputRegistry all;all.scope=987;unsigned fixed=0;
 for(unsigned i=0;i<sizeof input_command_aliases/sizeof input_command_aliases[0];i++){
  const InputCommandAlias *a=&input_command_aliases[i];if(strchr(a->native,'*')||strchr(a->native,'@'))continue;
  CHECK(input_register(&all,a->chinese,a->native,(uint64_t)a->key+1));fixed++;
 }
 CHECK(fixed==102);
 for(unsigned i=0;i<all.count;i++){CHECK(input_resolve(&all,987,all.rows[i].alias,out,sizeof out)==1);CHECK(!strcmp(out,all.rows[i].native));}
 puts("PASS terminal input: current scope, complete terms, ambiguity, same-name records, punctuation and atomic rejection");return 0;
}

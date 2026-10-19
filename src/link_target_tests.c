#include <stdio.h>
#include "linked_text.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){
 CHECK(argc==3);char error[128],target[16384],out[16384],prepared[16384];size_t count;
 ZhDictionary *para=zh_dictionary_load(argv[1],&count,error,sizeof error),*shorts=zh_dictionary_load(argv[2],&count,error,sizeof error);CHECK(para&&shorts);
 const ZhDictionary *dicts[]={shorts,para};
 const char *raw="Distributed [Storage] Facility. An area designated for storage of materials and components. Distribution serves two purposes, making local access to resources easier while also minimizing losses in the event that one is compromised. DSF entrances are temporarily sealed in the event that potential threats appear in the areas they serve.";
 CHECK(zh_translate_exact(para,raw,strlen(raw),target,sizeof target,NULL)==1);
 CHECK(linked_target_restore(raw,strlen(raw),target,strlen(target),1,dicts,2,out,sizeof out)==1);CHECK(strstr(out,"[存储]"));
 CHECK(linked_text_prepare(raw,strlen(raw),raw,strlen(raw),out,strlen(out),1,46,prepared,sizeof prepared)==1);CHECK(strstr(prepared,"[Storage]"));
 ZhPlan plan={0};CHECK(linked_plan_atomic(prepared,strlen(prepared),46,128,&plan)==1);zh_plan_free(&plan);
 const char *bad[]={"存储和存储","没有对应标签","[存储][额外]"};
 for(unsigned i=0;i<3;i++){memset(out,0x53,sizeof out);CHECK(linked_target_restore(raw,strlen(raw),bad[i],strlen(bad[i]),1,dicts,2,out,sizeof out)==0);CHECK(out[0]==0x53);}
 memset(out,0x53,sizeof out);CHECK(linked_target_restore(raw,strlen(raw),target,strlen(target),1,dicts,2,out,2)==-1);CHECK(out[0]==0x53);
 strcpy(out,target);CHECK(linked_target_restore(raw,strlen(raw),out,strlen(out),1,dicts,2,out,sizeof out)==1);CHECK(strstr(out,"[存储]"));
 zh_dictionary_free(para);zh_dictionary_free(shorts);puts("PASS DSF real corpus, native link preparation, atomic layout, ambiguity and capacity guards");return 0;
}

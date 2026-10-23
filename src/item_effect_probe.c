/* Offline use of the production matcher. LP input/output preserve multiline text. */
#include "terminal_text.h"
static const ZhDictionary *item_dictionary,*short_dictionary,*dictionary;
#include "message_resolver.h"
#include "linked_text.h"
#include <stdio.h>

int main(int argc,char **argv){
 if(argc!=8)return 2;char error[256];
 dictionary=zh_dictionary_load(argv[1],NULL,error,sizeof error);
 short_dictionary=zh_dictionary_load(argv[2],NULL,error,sizeof error);
 display_extra_dictionary=zh_dictionary_load(argv[3],NULL,error,sizeof error);
 item_dictionary=zh_dictionary_load(argv[4],NULL,error,sizeof error);
 terminal_runtime_dictionary=zh_dictionary_load(argv[5],NULL,error,sizeof error);
 terminal_item_dictionary=item_dictionary;
 repair_short_dictionary=short_dictionary;repair_extra_dictionary=display_extra_dictionary;repair_paragraph_dictionary=dictionary;
 if(!dictionary||!short_dictionary||!display_extra_dictionary||!item_dictionary||!terminal_runtime_dictionary)return 3;
 FILE *input=fopen(argv[6],"rb"),*output=fopen(argv[7],"wb");if(!input||!output)return 4;
 uint32_t count;if(fread(&count,4,1,input)!=1||count>10000)return 5;
 for(uint32_t i=0;i<count;i++){
  uint32_t n;char source[16385],target[32768]={0};
  if(fread(&n,4,1,input)!=1||!n||n>16384||fread(source,1,n,input)!=n||memchr(source,0,n))return 5;source[n]=0;
  int32_t result=message_resolve_entry(TEXT_ENTRY_DISPLAY,source,n,target,sizeof target,NULL),links=0;
  if(result>0){
   LinkedSpan spans[LINKED_COUNT_LIMIT];size_t expected=0;
   if(linked_spans(source,n,1,spans,&expected)&&expected){
    const ZhDictionary *dicts[]={display_extra_dictionary,item_dictionary,short_dictionary,dictionary};char restored[32768];
    links=linked_target_restore(source,n,target,strlen(target),expected,dicts,4,restored,sizeof restored);
    if(links==1){char prepared[32768];ZhPlan plan={0};
     if(linked_text_prepare(source,n,source,n,restored,strlen(restored),expected,46,prepared,sizeof prepared)!=1||
        linked_plan_atomic(prepared,strlen(prepared),46,128,&plan)!=1)links=-3;
     zh_plan_free(&plan);
    }
   }
  }
  uint32_t bytes=result>0?(uint32_t)strlen(target):0;
  if(fwrite(&result,4,1,output)!=1||fwrite(&bytes,4,1,output)!=1||fwrite(target,1,bytes,output)!=bytes||fwrite(&links,4,1,output)!=1)return 6;
 }
 if(fgetc(input)!=EOF)return 5;
 fclose(input);if(fclose(output))return 6;
 zh_dictionary_free((ZhDictionary*)dictionary);zh_dictionary_free((ZhDictionary*)short_dictionary);
 zh_dictionary_free((ZhDictionary*)display_extra_dictionary);zh_dictionary_free((ZhDictionary*)item_dictionary);
 zh_dictionary_free((ZhDictionary*)terminal_runtime_dictionary);return 0;
}

#include "terminal_text.h"
#include "display_text.h"
#include <stdio.h>
int main(int argc,char **argv){
 if(argc!=8)return 2;size_t n;char err[128];
 ZhDictionary *para=zh_dictionary_load(argv[1],&n,err,sizeof err),*shorts=zh_dictionary_load(argv[2],&n,err,sizeof err),*extra=zh_dictionary_load(argv[3],&n,err,sizeof err);
 terminal_item_dictionary=zh_dictionary_load(argv[4],&n,err,sizeof err);terminal_runtime_dictionary=zh_dictionary_load(argv[5],&n,err,sizeof err);display_extra_dictionary=extra;
 if(!para||!shorts||!extra||!terminal_item_dictionary||!terminal_runtime_dictionary)return 3;
 FILE *in=fopen(argv[6],"rb"),*out=fopen(argv[7],"wb");if(!in||!out)return 4;
 char source[16385],target[32768],display[32768];uint32_t bytes;
 while(fread(&bytes,4,1,in)==1){
  if(bytes>16384||fread(source,1,bytes,in)!=bytes)return 5;source[bytes]=0;target[0]=display[0]=0;
  int result=terminal_translate(para,shorts,extra,source,bytes,target,sizeof target,NULL);
  int dr=display_translate(terminal_item_dictionary,shorts,para,source,bytes,display,sizeof display);
  int rows=-1;if(result>0){ZhPlan p={0};if(!zh_plan(target,strlen(target),46,128,&p))rows=p.used_rows;zh_plan_free(&p);}
  fwrite(&result,4,1,out);fwrite(&dr,4,1,out);fwrite(&rows,4,1,out);
  uint32_t len=(uint32_t)strlen(target);fwrite(&len,4,1,out);fwrite(target,1,len,out);
  len=(uint32_t)strlen(display);fwrite(&len,4,1,out);fwrite(display,1,len,out);
 }
 fclose(in);fclose(out);return 0;
}

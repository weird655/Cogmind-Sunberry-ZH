#include <windows.h>
#include <stdio.h>
#include "display_text.h"
int main(void){
 size_t n;char error[128];
 ZhDictionary *items=zh_dictionary_load("item_names.tsv",&n,error,sizeof error),*shorts=zh_dictionary_load("zh_trans.tsv",&n,error,sizeof error),*paras=zh_dictionary_load("zh_para.tsv",&n,error,sizeof error);
 display_extra_dictionary=zh_dictionary_load("zh_extra.tsv",&n,error,sizeof error);if(!items||!shorts||!paras||!display_extra_dictionary)return 2;
 FILE *f=fopen("texts.txt","rb");if(!f)return 3;char line[16384],out[32768];LARGE_INTEGER frequency,a,b;QueryPerformanceFrequency(&frequency);unsigned count=0;double sum=0,max=0;
 while(fgets(line,sizeof line,f)){size_t length=strcspn(line,"\r\n");line[length]=0;QueryPerformanceCounter(&a);for(int j=0;j<10;j++)display_translate(items,shorts,paras,line,length,out,sizeof out);QueryPerformanceCounter(&b);double ms=(b.QuadPart-a.QuadPart)*1000.0/frequency.QuadPart;sum+=ms;if(ms>max)max=ms;if(ms>10)printf("SLOW %.3f %s\n",ms,line);count++;}
 printf("TOTAL strings=%u passes=10 milliseconds=%.3f max_batch_ms=%.3f\n",count,sum,max);fclose(f);return 0;
}

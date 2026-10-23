#include "terminal_text.h"
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"dialogue dynamic line %d\n",__LINE__);return 1;}}while(0)
int main(int argc,char **argv){
 if(argc!=5)return 2;char error[256],out[8192];
 ZhDictionary *items=zh_dictionary_load(argv[1],NULL,error,sizeof error),*shorts=zh_dictionary_load(argv[2],NULL,error,sizeof error),*extra=zh_dictionary_load(argv[3],NULL,error,sizeof error),*paragraphs=zh_dictionary_load(argv[4],NULL,error,sizeof error);
 if(!items||!shorts||!extra||!paragraphs)return 2;
 const char *bad[]={"Unregistered Relic 42 READY.","Lgt. Ion Engine\nImp. Assault Rifle READY.","They dubbed me R-42. Do I look like G-47 to you?","They dubbed me R-42. Do I look like R-42 to you?!","They dubbed me %1. Do I look like %1 to you?","Researching <CODE>, are we?","Researching this is arbitrary prose, are we?","In case you were wondering, some of my prized inventory items include Lgt. Ion Engine and Unknown."};
 for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++){strcpy(out,"untouched");int r=dialogue_dynamic_translate(extra,items,shorts,paragraphs,bad[i],strlen(bad[i]),out,sizeof out);CHECK(r==0);CHECK(!strcmp(out,"untouched"));}
 const char *s="Imp. Assault Rifle READY.";
 for(size_t cap=0;cap<=strlen("改进型突击步枪就绪。");cap++){memset(out,0x5a,sizeof out);char before[8192];memcpy(before,out,sizeof out);int r=dialogue_dynamic_translate(extra,items,shorts,paragraphs,s,strlen(s),out,cap);CHECK(r<=0);CHECK(!memcmp(before,out,sizeof out));}
 strcpy(out,s);CHECK(dialogue_dynamic_translate(extra,items,shorts,paragraphs,out,strlen(out),out,sizeof out)==1);CHECK(!strcmp(out,"改进型突击步枪就绪。"));
 const char *repeat="They dubbed me R-42. Do I look like R-42 to you?";CHECK(dialogue_dynamic_translate(extra,items,shorts,paragraphs,repeat,strlen(repeat),out,sizeof out)==1);CHECK(!strcmp(out,"他们叫我R-42。我像R-42吗？"));
 char long_name[1024];memset(long_name,'A',sizeof long_name);long_name[1023]=0;CHECK(dialogue_dynamic_translate(extra,items,shorts,paragraphs,long_name,strlen(long_name),out,sizeof out)==0);
 CHECK(dialogue_dynamic_translate(extra,items,shorts,paragraphs,"x\0x",3,out,sizeof out)==0);
 CHECK(dialogue_dynamic_translate(extra,items,shorts,paragraphs,NULL,0,out,sizeof out)==0);
 /* Every reviewed scaffold must reject an unsafe parameter without touching output. */
 for(unsigned i=0;i<sizeof dialogue_dynamic_rules/sizeof dialogue_dynamic_rules[0];i++){
  char input[8192];size_t at=0;const char *p=dialogue_dynamic_rules[i].source;
  while(*p){if(p[0]=='%'&&p[1]=='1'){const char *v="Invalid[Control]";size_t n=strlen(v);CHECK(at+n<sizeof input);memcpy(input+at,v,n);at+=n;p+=2;}else{CHECK(at+1<sizeof input);input[at++]=*p++;}}
  input[at]=0;strcpy(out,"untouched");CHECK(dialogue_dynamic_translate(extra,items,shorts,paragraphs,input,at,out,sizeof out)==0);CHECK(!strcmp(out,"untouched"));
 }
 zh_dictionary_free(items);zh_dictionary_free(shorts);zh_dictionary_free(extra);zh_dictionary_free(paragraphs);
 printf("PASS dialogue dynamic %u atomic, repeated-identity, multiline and typed-name boundary checks\n",checks);return 0;
}

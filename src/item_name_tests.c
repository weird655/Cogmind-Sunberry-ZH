#include <stdio.h>
#include <stdlib.h>
#include "item_name_text.h"
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static void reject(ZhDictionary *d,const char *s,int result){char out[672],before[672];memset(out,0x53,sizeof out);memcpy(before,out,sizeof out);CHECK(item_name_translate_exact(d,s,strlen(s),out,sizeof out)==result);CHECK(!memcmp(out,before,sizeof out));}
int main(int argc,char **argv){
 CHECK(argc==2);size_t count;char error[200];ZhDictionary *d=zh_dictionary_load(argv[1],&count,error,sizeof error);CHECK(d!=NULL);
 char out[672];const char *name="Imp. Carbon-fiber Leg";
 CHECK(item_name_translate_exact(d,name,strlen(name),out,sizeof out)==1);CHECK(!strcmp(out,"改进型碳纤维腿"));
 const char *source[]={"Imp. Carbon-fiber Leg+","Imp. Carbon-fiber Leg (0004)","Imp. Carbon-fiber Leg [4p]","Imp. Carbon-fiber Leg [!*/+?]","Imp. Spread Laser (0)"};
 const char *target[]={"改进型碳纤维腿+","改进型碳纤维腿 (0004)","改进型碳纤维腿 [4p]","改进型碳纤维腿 [!*/+?]","改进型散射激光 (0)"};
 for(unsigned i=0;i<5;i++){CHECK(item_name_translate_exact(d,source[i],strlen(source[i]),out,sizeof out)==1);CHECK(!strcmp(out,target[i]));}
 const char *bad[]={"Unknown Part+","Imp. Carbon-fiber Leg++","Imp. Carbon-fiber Leg +","Imp. Carbon-fiber Leg (1) [4p]","Imp. Carbon-fiber Leg [4p]+","Imp. Carbon-fiber Leg (-1)","Imp. Carbon-fiber Leg (+1)","Imp. Carbon-fiber Leg (1.5)","Imp. Carbon-fiber Leg ()","Imp. Carbon-fiber Leg (12345678901234567)","Imp. Carbon-fiber Leg []","Imp. Carbon-fiber Leg [[4p]]","Imp. Carbon-fiber Leg [foo bar]","Imp. Carbon-fiber Leg [人]","Imp. Carbon-fiber Leg [`f0`]","Imp. Carbon-fiber Leg [<tag>]","Imp. Carbon-fiber Leg [{1}]","Imp. Carbon-fiber Leg [4p]\n"," Imp. Carbon-fiber Leg","Imp. Carbon-fiber Leg ","Imp. Carbon-fiber Leg[4p]","Imp. Carbon-fiber Leg (1","Imp. Carbon-fiber Leg [4p","Imp. Carbon-fiber Leg [4p]extra"};
 for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++)reject(d,bad[i],0);
 char boundary[180];snprintf(boundary,sizeof boundary,"%s (0000000000000000)",name);CHECK(item_name_translate_exact(d,boundary,strlen(boundary),out,sizeof out)==1);CHECK(strstr(out,"(0000000000000000)")!=NULL);
 snprintf(boundary,sizeof boundary,"%s [",name);size_t start=strlen(boundary);memset(boundary+start,'A',32);strcpy(boundary+start+32,"]");CHECK(item_name_translate_exact(d,boundary,strlen(boundary),out,sizeof out)==1);
 memset(boundary+start,'A',33);strcpy(boundary+start+33,"]");reject(d,boundary,0);
 char tiny[2]={1,2};CHECK(item_name_translate_exact(d,name,strlen(name),tiny,sizeof tiny)==-1);CHECK(tiny[0]==1&&tiny[1]==2);
 strcpy(out,source[2]);CHECK(item_name_translate_exact(d,out,strlen(out),out,sizeof out)==1);CHECK(!strcmp(out,target[2]));
 size_t required=strlen(target[2])+1;CHECK(item_name_translate_exact(d,source[2],strlen(source[2]),out,required)==1);
 char before[672];memcpy(before,out,sizeof out);CHECK(item_name_translate_exact(d,source[2],strlen(source[2]),out,required-1)==-1);CHECK(!memcmp(before,out,sizeof out));
 char embedded[80];size_t n=strlen(name);memcpy(embedded,name,n);embedded[5]=0;CHECK(item_name_translate_exact(d,embedded,n,out,sizeof out)==0);CHECK(!memcmp(before,out,sizeof out));
 CHECK(item_name_translate_exact(d,name,n,out,0)==-1);CHECK(!memcmp(before,out,sizeof out));
 FILE *file=fopen(argv[1],"rb");CHECK(file!=NULL);char line[2048];size_t corpus=0;
 while(fgets(line,sizeof line,file)){
  if(line[0]=='#'||line[0]=='\n'||line[0]=='\r')continue;char *tab=strchr(line,'\t');CHECK(tab!=NULL);*tab++=0;tab[strcspn(tab,"\r\n")]=0;
  char decorated[180],expected[672];snprintf(decorated,sizeof decorated,"%s (0004)",line);snprintf(expected,sizeof expected,"%s (0004)",tab);
  CHECK(item_name_translate_exact(d,decorated,strlen(decorated),out,sizeof out)==1);CHECK(!strcmp(out,expected));corpus++;
 }fclose(file);CHECK(corpus==count);
 zh_dictionary_free(d);printf("PASS %u checks; %zu dedicated exact names; no dynamic templates enabled\n",checks,count);return 0;
}

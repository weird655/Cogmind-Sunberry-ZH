#include "terminal_text.h"
#include "linked_text.h"
#include <stdio.h>
#include <stdlib.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static ZhDictionary *fixture(const char *path,const char *content){FILE *f=fopen(path,"wb");if(!f)return NULL;fputs(content,f);fclose(f);return zh_dictionary_load(path,NULL,NULL,0);}
int main(int argc,char **argv){
 CHECK(argc==5);char out[32768],expected[32768],line[32768],variant[32768],canonical[16385];
 ZhDictionary *d[4]={0};size_t total=0,variants=0,ambiguous=0;
 for(unsigned i=0;i<4;i++){
  size_t count=0;d[i]=zh_dictionary_load(argv[i+1],&count,NULL,0);CHECK(d[i]);total+=count;
  FILE *f=fopen(argv[i+1],"rb");CHECK(f);
  while(fgets(line,sizeof line,f)){
   char *tab=strchr(line,'\t');if(!tab||line[0]=='#')continue;*tab=0;size_t n=strlen(line);
   if(!n||n>14000||strchr(line,'{')||strchr(line,'`')||strchr(line,'\\'))continue;
   if(zh_translate_exact(d[i],line,n,expected,sizeof expected,NULL)!=1||terminal_normalize(line,n,canonical,sizeof canonical)!=1)continue;
   /* Native wrapping variant: keep bracket labels byte-for-byte. */
   size_t at=0;variant[at++]=' ';int inside=0;
   for(size_t j=0;j<n;j++){char c=line[j];if(c=='[')inside=1;if(c==']')inside=0;
    if(c==' '&&!inside){variant[at++]='\r';variant[at++]='\n';variant[at++]=' ';}else variant[at++]=c;
   }
   variant[at++]=' ';variant[at]=0;strcpy(out,"sentinel");
   int r=zh_translate_normalized_exact(d[i],variant,at,out,sizeof out,NULL);
   CHECK(r==1||r==-2);if(r==1){CHECK(!strcmp(out,expected));variants++;}else{CHECK(!strcmp(out,"sentinel"));ambiguous++;}
  }
  fclose(f);
 }
 CHECK(total>70000);
 const char *p="terminal-normalize-fixture.tmp";
 ZhDictionary *a=fixture(p,"Alpha  Beta\tfirst\nAlpha Beta\tsecond\nStable\tstable result\nRead [Complex 0b10] now.\tRead [Complex 0b10] translated.\nAlert {n}\tlevel {n}\n");CHECK(a);remove(p);
 strcpy(out,"sentinel");CHECK(terminal_translate(a,NULL,NULL,"Alpha\r\nBeta",11,out,sizeof out,NULL)==-2);CHECK(!strcmp(out,"sentinel"));
 CHECK(terminal_translate(a,NULL,NULL,"Alpha Beta",10,out,sizeof out,NULL)==1);CHECK(!strcmp(out,"second"));
 CHECK(terminal_translate(a,NULL,NULL,"`f0`Stable`x`",13,out,sizeof out,NULL)==1);CHECK(!strcmp(out,"stable result"));
 strcpy(out,"sentinel");CHECK(terminal_translate(a,NULL,NULL,"`q`Stable`x`",12,out,sizeof out,NULL)==0);CHECK(!strcmp(out,"sentinel"));
 CHECK(terminal_translate(a,NULL,NULL," Alert\r\n 37 ",12,out,sizeof out,NULL)==2);CHECK(!strcmp(out,"level 37"));
 strcpy(out,"sentinel");CHECK(terminal_translate(a,NULL,NULL,"  Stable  ",10,out,2,NULL)==-1);CHECK(!strcmp(out,"sentinel"));
 const char *raw="Read [Complex 0b10]\r\n now.";
 CHECK(terminal_translate(a,NULL,NULL,raw,strlen(raw),out,sizeof out,NULL)==1);
 char linked[32768];CHECK(linked_text_prepare(raw,strlen(raw),raw,strlen(raw),out,strlen(out),1,50,linked,sizeof linked)==0);/* CR rejected by link layer, not silently altered. */
 const char *lf="Read [Complex 0b10]\n now.";
 CHECK(terminal_translate(a,NULL,NULL,lf,strlen(lf),out,sizeof out,NULL)==1);
 CHECK(linked_text_prepare(lf,strlen(lf),lf,strlen(lf),out,strlen(out),1,50,linked,sizeof linked)==1);CHECK(strstr(linked,"[Complex 0b10]"));ZhPlan plan={0};CHECK(linked_plan_atomic(linked,strlen(linked),50,16,&plan)==1);zh_plan_free(&plan);
 CHECK(terminal_normalize("[Complex  0b10]",15,canonical,sizeof canonical)==1);CHECK(!strcmp(canonical,"[Complex  0b10]"));
 strcpy(canonical,"sentinel");CHECK(terminal_normalize("`f0`X`x`",8,canonical,1)==-1);CHECK(!strcmp(canonical,"sentinel"));
 CHECK(terminal_normalize("[bad`f0`link]",13,canonical,sizeof canonical)==0);
 ZhDictionary *b=fixture(p,"Stable\tother result\n");CHECK(b);remove(p);strcpy(out,"sentinel");CHECK(terminal_translate(a,b,NULL," Stable ",8,out,sizeof out,NULL)==-2);CHECK(!strcmp(out,"sentinel"));
 zh_dictionary_free(b);zh_dictionary_free(a);for(unsigned i=0;i<4;i++)zh_dictionary_free(d[i]);
 printf("PASS normalization checks=%u reviewed-rows=%u wrapping-matches=%u canonical-conflicts=%u\n",checks,(unsigned)total,(unsigned)variants,(unsigned)ambiguous);return 0;
}

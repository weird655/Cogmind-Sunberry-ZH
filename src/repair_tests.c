#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "repair_text.h"
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static void reject(ZhDictionary *d,const char *s,int expected){
 char out[1536],before[1536];memset(out,0x5a,sizeof out);memcpy(before,out,sizeof out);
 CHECK(repair_translate_stage(d,s,strlen(s),out,sizeof out)==expected);CHECK(!memcmp(out,before,sizeof out));
}
int main(int argc,char **argv){
 CHECK(argc==2);size_t count;char error[256],out[1536];
 ZhDictionary *d=zh_dictionary_load(argv[1],&count,error,sizeof error);CHECK(d!=NULL);
 const char *real="Scanning Imp. Spread Laser...\nReady to Repair:\n  Imp. Spread Laser\n  Rating: 4p\n  Time: 23";
 CHECK(repair_translate_stage(d,real,strlen(real),out,sizeof out)==1);
 CHECK(!strcmp(out,"正在扫描 Imp. Spread Laser……\n准备修理：\n  Imp. Spread Laser\n  评级：4p\n  耗时：23"));
 const char *different="Scanning Unknown Mk. 2...\nReady to Repair:\n  Unknown Mk. 2\n  Rating: 004\n  Time: 000";
 CHECK(repair_translate_stage(d,different,strlen(different),out,sizeof out)==1);
 CHECK(strstr(out,"Unknown Mk. 2")&&strstr(out,"评级：004")&&strstr(out,"耗时：000"));
 const char *exact="Scanning Door Terminal...\nReady to Repair:\n  Door Terminal\n  Rating: 0p\n  Time: 0";
 CHECK(repair_translate_stage(d,exact,strlen(exact),out,sizeof out)==1);CHECK(strstr(out,"门终端")!=NULL);CHECK(strstr(out,"Door Terminal")==NULL);
 reject(d,"Scanning A...\nReady to Repair:\n  B\n  Rating: 4p\n  Time: 23",0);
 reject(d,"Scanning A...\nReady to Repair:\n  A\n  Rating: 4p",0);
 reject(d,"Scanning A...\nReady to Repair:\n  A\n  Rating: 4p\n  Time: 23\nextra",0);
 reject(d,"Scanning A...\nReady to Repair:\n  A\n  Rating: 4p\n  Time: 23\n",0);
 reject(d,"Scanning A...\nReady to Repair:\n  A\n  Rating: -1\n  Time: 23",0);
 reject(d,"Scanning A...\nReady to Repair:\n  A\n  Rating: 4P\n  Time: 23",0);
 reject(d,"Scanning A...\nReady to Repair:\n  A\n  Rating: p\n  Time: 23",0);
 reject(d,"Scanning A...\nReady to Repair:\n  A\n  Rating: 4p\n  Time: +23",0);
 reject(d,"Scanning A...\nReady to Repair:\n  A\n  Rating: 4p\n  Time: 23 ",0);
 reject(d,"Scanning [A]...\nReady to Repair:\n  [A]\n  Rating: 4p\n  Time: 23",0);
 reject(d,"Scanning `f0`A...\nReady to Repair:\n  `f0`A\n  Rating: 4p\n  Time: 23",0);
 reject(d,"Scanning <m=A>...\nReady to Repair:\n  <m=A>\n  Rating: 4p\n  Time: 23",0);
 reject(d,"Scanning {A}...\nReady to Repair:\n  {A}\n  Rating: 4p\n  Time: 23",0);
 reject(d,"Scanning A...\r\nReady to Repair:\n  A\n  Rating: 4p\n  Time: 23",0);
 reject(d,"Scanning A...\nReady to Repair:\n  A\n  Rating: 12345678901234567p\n  Time: 23",0);
 reject(d,"Scanning A...\nReady to Repair:\n  A\n  Rating: 4p\n  Time: 12345678901234567",0);
 char big[600],part[98];memset(part,'A',97);part[97]=0;snprintf(big,sizeof big,"Scanning %s...\nReady to Repair:\n  %s\n  Rating: 4p\n  Time: 23",part,part);reject(d,big,0);
 char tiny[5],before[5];memset(tiny,0x42,sizeof tiny);memcpy(before,tiny,sizeof tiny);
 CHECK(repair_translate_stage(d,real,strlen(real),tiny,sizeof tiny)==-1);CHECK(!memcmp(tiny,before,sizeof tiny));
 memset(out,0x32,sizeof out);CHECK(repair_translate_stage(d,real,strlen(real),out,0)==-1);CHECK(out[0]==0x32);
 CHECK(repair_translate_stage(d,real,strlen(real),out,sizeof out)==1);size_t needed=strlen(out)+1;
 CHECK(repair_translate_stage(d,real,strlen(real),out,needed)==1);CHECK(repair_translate_stage(d,real,strlen(real),out,needed-1)==-1);
 char embedded[200];size_t n=strlen(real);memcpy(embedded,real,n);embedded[12]=0;
 memset(out,0x66,sizeof out);CHECK(repair_translate_stage(d,embedded,n,out,sizeof out)==0);CHECK(out[0]==0x66);
 /* Length-delimited source need not have a terminator, and exact output alias
    is safe because commit occurs only after every read/check is finished. */
 memcpy(out,real,n);CHECK(repair_translate_stage(d,out,n,out,sizeof out)==1);CHECK(strstr(out,"耗时：23")!=NULL);
 for(size_t at=0;at<strlen(out);){size_t used;uint32_t cp;CHECK(zh_decode(out+at,strlen(out)-at,&used,&cp)==1);if(cp>127)CHECK(zh_glyph(cp)>=0);at+=used;}
 const char *boundary="Scanning A...\nReady to Repair:\n  A\n  Rating: 0000000000000000p\n  Time: 9999999999999999";
 CHECK(repair_translate_stage(d,boundary,strlen(boundary),out,sizeof out)==1);CHECK(strstr(out,"0000000000000000p")&&strstr(out,"9999999999999999"));
 memset(part,'A',96);part[96]=0;snprintf(big,sizeof big,"Scanning %s...\nReady to Repair:\n  %s\n  Rating: 4p\n  Time: 23",part,part);
 CHECK(repair_translate_stage(d,big,strlen(big),out,sizeof out)==1);
 const char *begin="Repairing Arm. Huge Wheel...\nETC: 14";
 CHECK(repair_translate_begin(d,begin,strlen(begin),out,sizeof out)==1);CHECK(strstr(out,"Arm. Huge Wheel")&&strstr(out,"预计耗时：14"));
 const char *invalid_begin[]={"Repairing A...\nETC: 1\n", "Repairing A...\nETC: -1", "Repairing [ISC]...\nETC: 1", "Repairing A.\nETC: 1"};
 for(unsigned i=0;i<4;i++){memset(out,0x5a,sizeof out);CHECK(repair_translate_begin(d,invalid_begin[i],strlen(invalid_begin[i]),out,sizeof out)==0);CHECK(out[0]==0x5a);}
 memset(out,0x5a,sizeof out);CHECK(repair_translate_begin(d,begin,strlen(begin),out,2)==-1);CHECK(out[0]==0x5a);
 CHECK(repair_translate_begin(d,begin,strlen(begin),out,sizeof out)==1);for(size_t at=0;at<strlen(out);){size_t used;uint32_t cp;CHECK(zh_decode(out+at,strlen(out)-at,&used,&cp)==1);if(cp>127)CHECK(zh_glyph(cp)>=0);at+=used;}
 zh_dictionary_free(d);printf("PASS %u checks; %zu rules; offline structured repair grammar\n",checks,count);return 0;
}

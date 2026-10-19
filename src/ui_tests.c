#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "ui_text.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
 assert(argc==2);char error[128],out[8192];size_t count=0,rule;
 ZhDictionary *d=zh_dictionary_load(argv[1],&count,error,sizeof error);assert(d&&count==56249&&zh_registry_count>0);
 const char *samples[]={"Storage","Utility","Weapon","Energy Gun"," ACTIVE ","`f0`Storage`x`"};
 for(unsigned i=0;i<sizeof samples/sizeof samples[0];i++){
  ZhPlan p={0};assert(ui_text_prepare(d,samples[i],strlen(samples[i]),80,out,sizeof out,&p,&rule)==1);
  for(size_t j=0;j<p.count;j++)assert(p.runs[j].y==0&&p.runs[j].x+p.runs[j].width<=ui_visible_width(samples[i],strlen(samples[i])));
  zh_plan_free(&p);
 }
 const char *bad[]={"Unknown UI label 9283","Storage\n","Storage\t","`f0Storage","`f0``x`","储存","{n}"};
 for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++){
  ZhPlan p={0},saved;p.rows=912;saved=p;memset(out,0x55,sizeof out);
  assert(!ui_text_prepare(d,bad[i],strlen(bad[i]),80,out,sizeof out,&p,0));assert(!memcmp(&p,&saved,sizeof p)&&out[0]==0x55);
 }
 ZhPlan p={0};memset(out,0x55,sizeof out);assert(!ui_text_prepare(d,"Storage",7,2,out,sizeof out,&p,0)&&out[0]==0x55);
 FILE *f=fopen(argv[1],"rb");assert(f);char line[32768];unsigned literal=0,fit=0,unsupported=0;
 while(fgets(line,sizeof line,f)){
  if(line[0]=='#'||line[0]=='\n'||line[0]=='\r')continue;char *tab=strchr(line,'\t');assert(tab);*tab++=0;tab[strcspn(tab,"\r\n")]=0;
  if(strchr(line,'{'))continue;literal++;char exact[16384];assert(zh_translate_exact(d,line,strlen(line),exact,sizeof exact,0)==1&&!strcmp(exact,tab));
  assert(!zh_plan(tab,strlen(tab),512,512,&p));zh_plan_free(&p);
  if(ui_text_prepare(d,line,strlen(line),512,out,sizeof out,&p,0)){
   fit++;int footprint=ui_visible_width(line,strlen(line));for(size_t i=0;i<p.count;i++)assert(p.runs[i].y==0&&p.runs[i].x+p.runs[i].width<=footprint);zh_plan_free(&p);
  }else unsupported++;
 }
 fclose(f);assert(literal>18000);printf("UI literal lookup: %u; fit within original columns: %u; unsupported footprint/control/layout: %u.\n",literal,fit,unsupported);
 HDC dc=CreateCompatibleDC(NULL);assert(dc);unsigned missing=0;
 for(int height=16;height<=24;height+=4){
  HFONT font=CreateFontW(-(height-2),0,0,0,FW_NORMAL,0,0,0,GB2312_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei");assert(font);HGDIOBJ old=SelectObject(dc,font);
  for(size_t i=0;i<zh_registry_count;i++){WCHAR cp=zh_registry[i];WORD glyph=0;assert(GetGlyphIndicesW(dc,&cp,1,&glyph,GGI_MARK_NONEXISTING_GLYPHS)!=GDI_ERROR);missing+=glyph==0xffff;}
  SelectObject(dc,old);DeleteObject(font);
 }
 DeleteDC(dc);assert(!missing);printf("Windows glyph availability: %zu characters at 3 sizes, missing=%u.\n",zh_registry_count,missing);
 zh_dictionary_free(d);return 0;
}

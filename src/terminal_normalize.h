#ifndef COGMIND_TERMINAL_NORMALIZE_H
#define COGMIND_TERMINAL_NORMALIZE_H
#include <stddef.h>
#include <string.h>
/* Matching only: never mutate native text or link labels. Unknown syntax fails
   closed. ASCII wrapping whitespace is collapsed outside square-bracket links. */
static inline int terminal_normalize(const char *src,size_t n,char *out,size_t cap){
 if(!src||!out||!cap||n>16384)return -1;
 char temp[16385];size_t at=0;int space=0,link=0;
 for(size_t i=0;i<n;i++){
  unsigned char c=(unsigned char)src[i];
  if(!c||c==127||(c<32&&c!='\r'&&c!='\n'&&c!='\t'))return 0;
  if(c=='`'){
   if(link)return 0;
   size_t left=n-i;
   if(left>=3&&!memcmp(src+i,"`x`",3)){i+=2;continue;}
   if(left>=4&&src[i+1]=='f'&&src[i+2]>='0'&&src[i+2]<='9'&&src[i+3]=='`'){i+=3;continue;}
   return 0;
  }
  if(c=='['){if(link)return 0;link=1;}
  else if(c==']'){if(!link)return 0;link=0;}
  if(c==' '||c=='\r'||c=='\n'||c=='\t'){
   if(link){if(c!=' ')return 0;temp[at++]=(char)c;}
   else if(at)space=1;
   continue;
  }
  if(space){temp[at++]=' ';space=0;}
  temp[at++]=(char)c;
 }
 if(link)return 0;if(at>=cap)return -1;temp[at]=0;memcpy(out,temp,at+1);return 1;
}
#endif

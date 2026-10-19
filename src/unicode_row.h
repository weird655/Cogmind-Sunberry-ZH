#ifndef COGMIND_UNICODE_ROW_H
#define COGMIND_UNICODE_ROW_H
#include "text_core.h"
#include <string.h>
/* Serialize semantic characters, never truncate native glyph tokens to bytes. */
static inline int unicode_row_text(const ZhPlan *p,int row,char *out,size_t capacity){
 if(!p||!out||row<0||row>=p->used_rows||capacity<1)return 0;
 char temporary[8192];size_t at=0;unsigned style=~0u;int x=0;
 for(size_t i=0;i<p->count;i++){
  const ZhRun *r=&p->runs[i];if(r->y!=row)continue;
  if(r->style>=p->style_count||at+260>=sizeof temporary)return 0;
  if(style!=r->style){const char *next=p->styles[r->style];size_t n=strlen(next),skip=0;
   if(style!=~0u){const char *prior=p->styles[style];size_t before=strlen(prior);
    if(n>=before&&!memcmp(next,prior,before))skip=before;
    else if(strncmp(next,"`x`",3)){memcpy(temporary+at,"`x`",3);at+=3;}
   }
   memcpy(temporary+at,next+skip,n-skip);at+=n-skip;style=r->style;}
  while(x<r->x){if(at+1>=sizeof temporary)return 0;temporary[at++]=' ';x++;}
  uint32_t cp=r->scalar;if(cp<128)temporary[at++]=(char)cp;
  else if(cp<2048){temporary[at++]=(char)(0xc0|(cp>>6));temporary[at++]=(char)(0x80|(cp&63));}
  else if(cp<=0xffff){temporary[at++]=(char)(0xe0|(cp>>12));temporary[at++]=(char)(0x80|((cp>>6)&63));temporary[at++]=(char)(0x80|(cp&63));}
  else return 0;x=r->x+r->width;
 }
 if(at>=capacity)return 0;temporary[at]=0;memcpy(out,temporary,at+1);return 1;
}
#endif

#ifndef COGMIND_REPLACEMENT_ANIMATION_H
#define COGMIND_REPLACEMENT_ANIMATION_H
#include <stdint.h>
#include <string.h>
/* A cell contains only native display data. Never erase the outgoing label:
   concatenate a growing target prefix with the still-unconsumed old suffix. */
static uint32_t replacement_glyph(const unsigned char *p){uint32_t g;memcpy(&g,p+4,4);return g;}
static int replacement_pair(const unsigned char *grid,unsigned rows,unsigned y,unsigned x,unsigned right,unsigned threshold){
 uint32_t g=replacement_glyph(grid+((size_t)x*rows+y)*20);
 return g>=threshold&&!(g&1)&&x<right&&replacement_glyph(grid+((size_t)(x+1)*rows+y)*20)==g+1;
}
static void replacement_span(const unsigned char *old,const unsigned char *target,unsigned char *out,unsigned rows,unsigned y,unsigned left,unsigned right,unsigned elapsed,unsigned duration,unsigned threshold){
 if(!duration||elapsed>=duration)return;
 if(!elapsed){for(unsigned x=left;x<=right;x++)memcpy(out+((size_t)x*rows+y)*20,old+((size_t)x*rows+y)*20,20);return;}
 unsigned end=right+1,total=0;
 while(end>left){uint32_t g=replacement_glyph(target+((size_t)(end-1)*rows+y)*20);if(g&&g!=32)break;end--;}
 for(unsigned x=left;x<end;x++){total++;if(replacement_pair(target,rows,y,x,end-1,threshold))x++;}
 if(!total){for(unsigned x=left;x<=right;x++)memcpy(out+((size_t)x*rows+y)*20,old+((size_t)x*rows+y)*20,20);return;}
 unsigned reveal=(unsigned)(((uint64_t)elapsed*total)/duration),prefix=left;
 for(unsigned i=0;i<reveal&&prefix<end;i++){unsigned width=replacement_pair(target,rows,y,prefix,end-1,threshold)?2:1;prefix+=width;}
 unsigned consumed=left;
 while(consumed<=right&&consumed-left<prefix-left)consumed+=replacement_pair(old,rows,y,consumed,right,threshold)?2:1;
 unsigned at=left;
 for(unsigned x=left;x<prefix;x++,at++)memcpy(out+((size_t)at*rows+y)*20,target+((size_t)x*rows+y)*20,20);
 for(unsigned x=consumed;x<=right;){
  unsigned width=replacement_pair(old,rows,y,x,right,threshold)?2:1;if(at+width>right+1)break;
  for(unsigned half=0;half<width;half++,at++)memcpy(out+((size_t)at*rows+y)*20,old+((size_t)(x+half)*rows+y)*20,20);x+=width;
 }
 /* Only the natural trailing padding is blank; the live prefix and suffix
    always meet, including when a one-cell English letter consumes a Hanzi. */
 for(;at<=right;at++){unsigned char *cell=out+((size_t)at*rows+y)*20;uint32_t blank=32;memcpy(cell+4,&blank,4);memcpy(cell+8,&blank,4);}
}
static void replacement_frame(const unsigned char *old,const unsigned char *target,unsigned char *out,unsigned columns,unsigned rows,unsigned elapsed,unsigned duration,unsigned threshold){
 memcpy(out,target,(size_t)columns*rows*20);if(!duration||elapsed>=duration)return;
 for(unsigned y=0;y<rows;y++){
  unsigned x=0;
  while(x<columns){
   while(x<columns&&replacement_glyph(old+((size_t)x*rows+y)*20)==replacement_glyph(target+((size_t)x*rows+y)*20))x++;
   if(x==columns)break;unsigned left=x,right=x,last=x;int han=0;
   for(;x<columns;x++){
    uint32_t a=replacement_glyph(old+((size_t)x*rows+y)*20),b=replacement_glyph(target+((size_t)x*rows+y)*20);
    if(a!=b){last=x;right=x;if(a>=threshold||b>=threshold)han=1;}
    else if(x-last>2)break;
   }
   if(han){
    /* Shared cells may be the other half of a boundary Hanzi. */
    if(left&&((replacement_glyph(old+((size_t)left*rows+y)*20)&1)||(replacement_glyph(target+((size_t)left*rows+y)*20)&1))){
     if(replacement_pair(old,rows,y,left-1,columns-1,threshold)||replacement_pair(target,rows,y,left-1,columns-1,threshold))left--;
    }
    if(right+1<columns&&(replacement_pair(old,rows,y,right,columns-1,threshold)||replacement_pair(target,rows,y,right,columns-1,threshold)))right++;
    replacement_span(old,target,out,rows,y,left,right,elapsed,duration,threshold);
   }
   if(x<=right)x=right+1;
  }
 }
}
#endif

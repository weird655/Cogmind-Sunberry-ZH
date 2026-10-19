#ifndef COGMIND_REPLACEMENT_ANIMATION_H
#define COGMIND_REPLACEMENT_ANIMATION_H
#include <stdint.h>
#include <string.h>
/* A cell contains native display data. Replace in place: an unreplaced old
   cell never changes its coordinates. No concatenation or suffix shifting. */
static uint32_t replacement_glyph(const unsigned char *p){uint32_t g;memcpy(&g,p+4,4);return g;}
static int replacement_pair(const unsigned char *grid,unsigned rows,unsigned y,unsigned x,unsigned right,unsigned threshold){
 uint32_t g=replacement_glyph(grid+((size_t)x*rows+y)*20);
 return g>=threshold&&!(g&1)&&x<right&&replacement_glyph(grid+((size_t)(x+1)*rows+y)*20)==g+1;
}
static void replacement_span(const unsigned char *old,const unsigned char *target,unsigned char *out,unsigned rows,unsigned y,unsigned left,unsigned right,unsigned elapsed,unsigned duration,unsigned threshold){
 if(!duration||elapsed>=duration)return;
 if(!elapsed){for(unsigned x=left;x<=right;x++)memcpy(out+((size_t)x*rows+y)*20,old+((size_t)x*rows+y)*20,20);return;}
 unsigned width=right-left+1;
 /* Scan the union footprint, not the shorter target's character count. This
    also consumes surplus outgoing text/padding without a deadline cut-off. */
 unsigned columns=(unsigned)(((uint64_t)elapsed*(width+1u))/duration);if(columns>width)columns=width;
 unsigned boundary=left+columns;
 /* An incoming Hanzi appears whole. It may consume one extra old column. */
 if(boundary>left&&boundary<=right&&replacement_pair(target,rows,y,boundary-1,right,threshold))boundary++;
 for(unsigned x=left;x<boundary;x++)memcpy(out+((size_t)x*rows+y)*20,target+((size_t)x*rows+y)*20,20);
 unsigned suffix=boundary;
 /* If an outgoing Hanzi straddles the frontier, retire its remaining half.
    Use only a one-cell seam; all later old cells retain their native positions. */
 if(boundary>left&&boundary<=right&&replacement_pair(old,rows,y,boundary-1,right,threshold)){
  unsigned char *cell=out+((size_t)boundary*rows+y)*20;memcpy(cell,target+((size_t)boundary*rows+y)*20,20);
  uint32_t blank=32;memcpy(cell+4,&blank,4);memcpy(cell+8,&blank,4);suffix++;
 }
 for(unsigned x=suffix;x<=right;x++)memcpy(out+((size_t)x*rows+y)*20,old+((size_t)x*rows+y)*20,20);

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

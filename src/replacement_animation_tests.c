#include "replacement_animation.h"
#include <assert.h>
#include <stdio.h>
static void cells(unsigned char *p,const uint32_t *g,unsigned n){for(unsigned i=0;i<n;i++){memset(p+i*20,0,20);memcpy(p+i*20+4,g+i,4);memcpy(p+i*20+8,g+i,4);p[i*20+13]=255;p[i*20+16]=55;}}
static void expect(const unsigned char *p,const uint32_t *g,unsigned n){for(unsigned i=0;i<n;i++)assert(replacement_glyph(p+i*20)==g[i]);}
int main(void){
 const uint32_t zh[]={320,321,322,323},en[]={'A','B','C','D'},first[]={'A',322,323,32},middle[]={'A','B',322,323},third[]={'A','B','C',32},reverse[]={320,321,'C','D'};
 unsigned char a[80],b[80],out[80],saved_a[80],saved_b[80];cells(a,zh,4);cells(b,en,4);memcpy(saved_a,a,80);memcpy(saved_b,b,80);
 replacement_frame(a,b,out,4,1,0,1600,320);expect(out,zh,4);
 replacement_frame(a,b,out,4,1,400,1600,320);expect(out,first,4);
 replacement_frame(a,b,out,4,1,800,1600,320);expect(out,middle,4);
 replacement_frame(a,b,out,4,1,1200,1600,320);expect(out,third,4);
 replacement_frame(a,b,out,4,1,1600,1600,320);expect(out,en,4);
 replacement_frame(b,a,out,4,1,800,1600,320);expect(out,reverse,4);
 replacement_frame(b,a,out,4,1,1600,1600,320);expect(out,zh,4);
 for(unsigned t=0;t<=1600;t++){
  replacement_frame(a,b,out,4,1,t,1600,320);unsigned nonblank=0;
  for(unsigned x=0;x<4;x++){uint32_t g=replacement_glyph(out+x*20);nonblank+=g!=32;if(g>=320){if(!(g&1)){assert(x+1<4&&replacement_glyph(out+(x+1)*20)==g+1);}else assert(x&&replacement_glyph(out+(x-1)*20)==g-1);}assert(out[x*20+16]==55);}
  assert(nonblank);replacement_frame(b,a,out,4,1,t,1600,320);
  for(unsigned x=0;x<4;x++){uint32_t g=replacement_glyph(out+x*20);if(g>=320&&!(g&1))assert(x+1<4&&replacement_glyph(out+(x+1)*20)==g+1);}
 }
 assert(!memcmp(a,saved_a,80)&&!memcmp(b,saved_b,80));
 /* ASCII map/art changes do not enter the bilingual text animation. */
 const uint32_t dots[]={'.','.','.','.'};cells(a,dots,4);replacement_frame(a,b,out,4,1,400,1600,320);expect(out,en,4);
 puts("PASS replacement: target prefix consumes old glyphs in both directions, no empty frame, whole Hanzi, colors, unchanged sources, nontext bypass (1601 time samples).");return 0;
}

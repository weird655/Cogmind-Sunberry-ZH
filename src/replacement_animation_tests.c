#include "replacement_animation.h"
#include <assert.h>
#include <stdio.h>
static void cells(unsigned char *p,const uint32_t *g,unsigned n){for(unsigned i=0;i<n;i++){memset(p+i*20,0,20);memcpy(p+i*20+4,g+i,4);memcpy(p+i*20+8,g+i,4);p[i*20+13]=255;p[i*20+16]=55;}}
static void expect(const unsigned char *p,const uint32_t *g,unsigned n){for(unsigned i=0;i<n;i++)assert(replacement_glyph(p+i*20)==g[i]);}
int main(void){
 const uint32_t zh[]={320,321,322,323},en[]={'A','B','C','D'},first[]={'A',32,322,323},middle[]={'A','B',322,323},third[]={'A','B','C',32},reverse[]={320,321,'C','D'};
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
 /* Different lengths used to retain a long outgoing suffix until deadline.
    Cover both directions and blank target rows at every millisecond. */
 unsigned length_cases=0;
 for(unsigned direction=0;direction<2;direction++)for(unsigned longer=4;longer<=48;longer+=4)for(unsigned shorter=1;shorter<=3;shorter++){
  uint32_t from[128],to[128];unsigned char oldgrid[2560],targetgrid[2560],frame[2560];
  for(unsigned x=0;x<128;x++)from[x]=to[x]=32;
  unsigned old_units=direction?shorter:longer,new_units=direction?longer:shorter;
  for(unsigned x=0;x<old_units;x++)from[x]='a'+x%26;for(unsigned x=0;x<new_units;x++){to[x*2]=320+x*2;to[x*2+1]=321+x*2;}
  cells(oldgrid,from,128);cells(targetgrid,to,128);
  for(unsigned t=0;t<=1600;t++){
   replacement_frame(oldgrid,targetgrid,frame,128,1,t,1600,320);
   for(unsigned x=0;x<128;x++){uint32_t g=replacement_glyph(frame+x*20);if(g>=320){if(!(g&1))assert(x+1<128&&replacement_glyph(frame+(x+1)*20)==g+1);else assert(x&&replacement_glyph(frame+(x-1)*20)==g-1);}}
   if(!t)assert(!memcmp(frame,oldgrid,sizeof frame));if(t==1600)assert(!memcmp(frame,targetgrid,sizeof frame));
  }
  replacement_frame(oldgrid,targetgrid,frame,128,1,1599,1600,320);unsigned different=0;
  for(unsigned x=0;x<128;x++)different+=replacement_glyph(frame+x*20)!=to[x];
  assert(different<=4);length_cases++;
  /* Reverse the same pair as well, including a long CJK outgoing label. */
  replacement_frame(targetgrid,oldgrid,frame,128,1,1599,1600,320);different=0;
  for(unsigned x=0;x<128;x++)different+=replacement_glyph(frame+x*20)!=from[x];assert(different<=4);
 }
 const uint32_t oldrow[]={320,321,322,323,324,325,326,327},blankrow[]={32,32,32,32,32,32,32,32};
 unsigned char c[160],d[160],v[160];cells(c,oldrow,8);cells(d,blankrow,8);
 replacement_frame(c,d,v,8,1,800,1600,320);unsigned remain=0;for(unsigned x=0;x<8;x++)remain+=replacement_glyph(v+x*20)>=320;assert(remain==4);
 replacement_frame(c,d,v,8,1,1599,1600,320);remain=0;for(unsigned x=0;x<8;x++)remain+=replacement_glyph(v+x*20)>=320;assert(remain<=2);
 /* Independent rows use native column-major coordinates, not a row-major copy. */
 unsigned char mr_old[320],mr_new[320],mr_out[320];
 for(unsigned x=0;x<8;x++)for(unsigned y=0;y<2;y++){
  uint32_t aa=y?'Q':oldrow[x],bb=y?'Q':blankrow[x];cells(mr_old+(x*2+y)*20,&aa,1);cells(mr_new+(x*2+y)*20,&bb,1);
 }
 replacement_frame(mr_old,mr_new,mr_out,8,2,800,1600,320);
 for(unsigned x=0;x<8;x++)assert(replacement_glyph(mr_out+(x*2+1)*20)=='Q');
 printf("PASS fixed-position lengths: %u cases, both directions, final-frame delta <=4 cells, blank-row retirement and column-major isolation.\n",length_cases);
 /* Small independently bounded labels must not all pop on the final frame. */
 const uint32_t small_old[]={'A',32},small_target[]={320,321};unsigned char so[40],st[40],sf[40];cells(so,small_old,2);cells(st,small_target,2);
 replacement_frame(so,st,sf,2,1,532,1600,320);expect(sf,small_old,2);
 replacement_frame(so,st,sf,2,1,534,1600,320);expect(sf,small_target,2);
 /* A mixed prefix/suffix carries each source's complete native cell style. */
 cells(a,en,4);cells(b,zh,4);for(unsigned x=0;x<4;x++){a[x*20+12]=201;a[x*20+13]=0;b[x*20+12]=0;b[x*20+13]=233;}
 replacement_frame(a,b,out,4,1,800,1600,320);
 for(unsigned x=0;x<2;x++)assert(out[x*20+12]==0&&out[x*20+13]==233);
 for(unsigned x=2;x<4;x++)assert(out[x*20+12]==201&&out[x*20+13]==0);
 /* No squashing: a long outgoing label's untouched tail stays stationary. */
 uint32_t fixed_old[24],fixed_new[24];unsigned char fo[480],ft[480],ff[480];
 for(unsigned x=0;x<24;x++){fixed_old[x]='A'+x;fixed_new[x]=32;}
 fixed_new[0]=320;fixed_new[1]=321;fixed_new[2]=322;fixed_new[3]=323;cells(fo,fixed_old,24);cells(ft,fixed_new,24);
 for(unsigned t=100;t<1600;t+=100){replacement_frame(fo,ft,ff,24,1,t,1600,320);
  unsigned cut=(unsigned)(((uint64_t)t*25)/1600);if(cut>24)cut=24;
  for(unsigned x=cut+2;x<24;x++)assert(!memcmp(ff+x*20,fo+x*20,20));
 }
 /* Shared digits, punctuation and spaces split spans at different places.
    Every generated source is valid; every mixed frame must remain valid too. */
 uint32_t seed=17493;
 for(unsigned trial=0;trial<300;trial++){
  uint32_t from[64],to[64];unsigned char og[1280],tg[1280],fg[1280];
  for(unsigned which=0;which<2;which++)for(unsigned x=0;x<64;){
   uint32_t *g=which?to:from;seed=seed*1664525u+1013904223u;unsigned choice=seed%7;
   if(choice<3&&x+1<64){g[x]=320+2*(seed%25);g[x+1]=g[x]+1;x+=2;}
   else g[x++]=choice==3?32:choice==4?'7':choice==5?'(':')';
  }
  cells(og,from,64);cells(tg,to,64);
  for(unsigned t=0;t<=1600;t+=100){replacement_frame(og,tg,fg,64,1,t,1600,320);
   for(unsigned x=0;x<64;x++){uint32_t g=replacement_glyph(fg+x*20);if(g>=320){if(!(g&1))assert(x+1<64&&replacement_glyph(fg+(x+1)*20)==g+1);else assert(x&&replacement_glyph(fg+(x-1)*20)==g-1);}}
  }
 }
 puts("PASS replacement: target overwrites old glyphs at fixed coordinates in both directions, no empty frame, whole Hanzi, colors, unchanged sources, nontext bypass (1601 time samples).");return 0;
}

#include <assert.h>
#include <stdio.h>
#define LIVE_CELLS_WORD uintptr_t
#define LIVE_CELLS_MAX_OWNERS 4u
#define LIVE_CELLS_MAX_CELLS 64u
#include "live_cells.h"
static void glyph(unsigned char *c,unsigned i,uint32_t g){memcpy(c+i*20+4,&g,4);}
static uint32_t getglyph(unsigned char *c,unsigned i){uint32_t g;memcpy(&g,c+i*20+4,4);return g;}
static int reject_read(const void *p,size_t n){(void)p;(void)n;return 0;}
int main(void){
 uintptr_t a[5]={1,0,8,4,0},b[5]={1,0,8,4,0},c[5]={1,0,1,1,0};
 unsigned char cells[640],other[640],tiny[20],before[640],*v;LiveCellsSnapshot *s;LiveCellsEntry *e;
 memset(cells,0,sizeof cells);memset(other,0,sizeof other);memset(tiny,0,sizeof tiny);
 a[4]=(uintptr_t)cells;b[4]=(uintptr_t)other;c[4]=(uintptr_t)tiny;
 glyph(cells,9,'A');s=live_cells_begin(a,2,1,2,1,0);assert(s);glyph(cells,9,320);glyph(cells,13,321);assert(live_cells_commit(s));
 cells[9*20+12]=77;memcpy(before,cells,sizeof cells);v=live_cells_view(a);assert(v);assert(getglyph(v,9)=='A');assert(getglyph(v,13)==0);assert(v[9*20+12]==77);assert(!memcmp(before,cells,sizeof cells));free(v);
 glyph(cells,9,'B');v=live_cells_view(a);assert(v && getglyph(v,9)=='B');free(v);
 s=live_cells_begin(a,3,1,2,1,2);assert(s && s->x==2 && s->width==2);assert(live_cells_commit(s));assert(!live_cells_view(a));
 s=live_cells_begin(a,4,2,3,1,1);assert(s && s->x==3 && s->width==3);assert(live_cells_commit(s));
 s=live_cells_begin(a,-2,-1,4,3,0);assert(s && s->x==0 && s->y==0 && s->width==2 && s->height==2);assert(live_cells_commit(s));assert(!live_cells_begin(a,20,0,1,1,0));
 s=live_cells_capture_full(a);assert(s);glyph(cells,0,400);assert(live_cells_commit(s));e=live_cells_find(a);assert(e && e->map[0].valid && !e->map[1].valid);v=live_cells_view(a);assert(v && !getglyph(v,0));free(v);
 s=live_cells_capture_full(b);assert(s);assert(live_cells_commit(s));assert(live_cells_count==64);assert(!live_cells_capture_full(c));assert(!live_cells_find(c));
 live_cells_remove(a);assert(!live_cells_view(a));s=live_cells_capture_full(c);assert(s);assert(live_cells_commit(s));
 s=live_cells_capture_full(a);assert(!s);live_cells_remove(b);s=live_cells_capture_full(a);assert(s);a[0]=2;assert(!live_cells_commit(s));assert(!live_cells_find(a));
 s=live_cells_capture_full(a);assert(s);a[4]=(uintptr_t)other;assert(!live_cells_commit(s));assert(!live_cells_find(a));
 live_cells_clear();assert(live_cells_count==0);
 /* Repeated English snapshots must use the reconstructed English baseline,
    never the shorter Chinese carrier's spaces. Preserve both tail and header. */
 a[0]=1;a[4]=(uintptr_t)cells;memset(cells,0,sizeof cells);
 for(unsigned i=0;i<8;i++)glyph(cells,i*4,'A'+i);
 for(unsigned pass=0;pass<20;pass++){
  v=live_cells_view(a);if(v){memcpy(cells,v,sizeof cells);free(v);}
  s=live_cells_begin(a,0,0,8,1,0);assert(s);
  for(unsigned i=0;i<8;i++)glyph(cells,i*4,i<4?320+i:32);
  assert(live_cells_commit(s));v=live_cells_view(a);assert(v);
  for(unsigned i=0;i<8;i++)assert(getglyph(v,i*4)=='A'+i);
  free(v);
 }
 live_cells_clear();
 live_cells_set_readable(reject_read);assert(!live_cells_capture_full(a));assert(!live_cells_view(a));
 live_cells_set_readable(NULL);s=live_cells_capture_full(a);assert(s);live_cells_cancel(s);live_cells_clear();
 puts("live_cells: reversible views, colors, stale glyphs, alignment, clipping, owner reuse and capacity passed");return 0;
}

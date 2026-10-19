#ifndef COGMIND_LIVE_CELLS_H
#define COGMIND_LIVE_CELLS_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Native CConsole words are 32-bit, including pointers. Tests may override the
 * word type to uintptr_t when running in a 64-bit host process. */
#ifndef LIVE_CELLS_WORD
#define LIVE_CELLS_WORD uint32_t
#endif
#ifndef LIVE_CELLS_MAX_OWNERS
#define LIVE_CELLS_MAX_OWNERS 4096u
#endif
#ifndef LIVE_CELLS_MAX_CELLS
#define LIVE_CELLS_MAX_CELLS 2000000u
#endif
typedef int (*LiveCellsReadable)(const void *, size_t);
typedef struct LiveCellMap {
 unsigned char english[20];
 uint32_t expected_glyph;
 unsigned char valid;
} LiveCellMap;
typedef struct LiveCellsEntry {
 void *owner;
 unsigned char *cells;
 uintptr_t vt;
 unsigned columns, rows;
 LiveCellMap *map; /* Column-major: x * rows + y. Only valid cells are text diffs. */
} LiveCellsEntry;
typedef struct LiveCellsSnapshot {
 void *owner;
 unsigned char *cells;
 uintptr_t vt;
 unsigned columns, rows, x, y, width, height;
 unsigned char *english;
} LiveCellsSnapshot;
static LiveCellsEntry live_cells_entries[LIVE_CELLS_MAX_OWNERS];
static size_t live_cells_count;
static LiveCellsReadable live_cells_readable;
static void live_cells_set_readable(LiveCellsReadable fn) { live_cells_readable=fn; }
static int live_cells_can_read(const void *p,size_t n) {
 return p && (uintptr_t)p+n >= (uintptr_t)p &&
  (!live_cells_readable || live_cells_readable(p,n));
}
static int live_cells_key(void *owner, LiveCellsEntry *key) {
 LIVE_CELLS_WORD *o=owner;
 size_t n;
 if(!live_cells_can_read(o,5*sizeof(*o)))return 0;
 memset(key,0,sizeof(*key));
 key->owner=owner; key->vt=(uintptr_t)o[0];
 key->columns=(unsigned)o[2];key->rows=(unsigned)o[3];
 key->cells=(unsigned char*)(uintptr_t)o[4];
 if(!key->columns || !key->rows || key->columns>LIVE_CELLS_MAX_CELLS ||
  key->rows>LIVE_CELLS_MAX_CELLS/key->columns)return 0;
 n=(size_t)key->columns*key->rows;
 return live_cells_can_read(key->cells,n*20);
}
static int live_cells_same(const LiveCellsEntry *a,const LiveCellsEntry *b) {
 return a->owner==b->owner && a->cells==b->cells && a->vt==b->vt &&
  a->columns==b->columns && a->rows==b->rows;
}
static void live_cells_remove(void *owner) {
 unsigned i;
 for(i=0;i<LIVE_CELLS_MAX_OWNERS;i++)if(live_cells_entries[i].owner==owner){
  LiveCellsEntry *e=&live_cells_entries[i];
  live_cells_count-=(size_t)e->columns*e->rows;
  free(e->map);memset(e,0,sizeof(*e));return;
 }
}
static LiveCellsEntry *live_cells_find(void *owner) {
 unsigned i;LiveCellsEntry key;
 if(!live_cells_key(owner,&key)){live_cells_remove(owner);return NULL;}
 for(i=0;i<LIVE_CELLS_MAX_OWNERS;i++)if(live_cells_entries[i].owner==owner){
  if(live_cells_same(&live_cells_entries[i],&key))return &live_cells_entries[i];
  live_cells_remove(owner);return NULL;
 }
 return NULL;
}
static LiveCellsEntry *live_cells_require(const LiveCellsEntry *key) {
 unsigned i;size_t n=(size_t)key->columns*key->rows;
 LiveCellsEntry *e=live_cells_find(key->owner);
 if(e)return e;
 if(n>LIVE_CELLS_MAX_CELLS-live_cells_count)return NULL;
 for(i=0;i<LIVE_CELLS_MAX_OWNERS;i++)if(!live_cells_entries[i].owner){
  LiveCellMap *map=calloc(n,sizeof(*map));if(!map)return NULL;
  e=&live_cells_entries[i];*e=*key;e->map=map;live_cells_count+=n;return e;
 }
 return NULL;
}
/* Call immediately AFTER native English rendering and BEFORE Chinese carrier
 * rendering. align: 0=left, 1=center (x is center), 2=right (x is last cell).
 * width/height describe the actual potentially affected rectangle, not the
 * available paragraph bounds. Negative/off-screen rectangles are clipped.
 * NULL means no transaction: caller should preserve native English output.
 * Successful begin invalidates previous mappings in its affected rectangle. */
static LiveCellsSnapshot *live_cells_begin(void *owner,int x,int y,int width,int height,int align) {
 LiveCellsEntry key,*e;LiveCellsSnapshot *s;int64_t l=x,t=y,r,b;
 size_t n;unsigned cx,cy;
 if(width<=0||height<=0||align<0||align>2||!live_cells_key(owner,&key))return NULL;
 if(align==2)l-=(int64_t)width-1;else if(align==1)l-=width/2;
 r=l+width;b=t+height;
 if(l<0)l=0;if(t<0)t=0;if(r>key.columns)r=key.columns;if(b>key.rows)b=key.rows;
 if(l>=r||t>=b)return NULL;
 n=(size_t)(r-l)*(size_t)(b-t);
 s=malloc(sizeof(*s));if(!s)return NULL;
 s->english=malloc(n*20);if(!s->english){free(s);return NULL;}
 e=live_cells_require(&key);if(!e){free(s->english);free(s);return NULL;}
 s->owner=owner;s->cells=key.cells;s->vt=key.vt;s->columns=key.columns;s->rows=key.rows;
 s->x=(unsigned)l;s->y=(unsigned)t;s->width=(unsigned)(r-l);s->height=(unsigned)(b-t);
 for(cx=0;cx<s->width;cx++)for(cy=0;cy<s->height;cy++){
  size_t ix=(size_t)(s->x+cx)*s->rows+s->y+cy;
  memcpy(s->english+((size_t)cx*s->height+cy)*20,key.cells+ix*20,20);
  e->map[ix].valid=0;
 }
 return s;
}
static LiveCellsSnapshot *live_cells_capture_full(void *owner) {
 LiveCellsEntry key;if(!live_cells_key(owner,&key))return NULL;
 return live_cells_begin(owner,0,0,(int)key.columns,(int)key.rows,0);
}
/* Commit consumes snapshot even on stale owner/cells. Only glyph changes are
 * retained; identical blanks and borders are never marked for animation. */
static int live_cells_commit(LiveCellsSnapshot *s) {
 LiveCellsEntry *e;unsigned x,y;int ok=0;
 if(!s)return 0;e=live_cells_find(s->owner);
 if(e && e->cells==s->cells && e->vt==s->vt && e->columns==s->columns && e->rows==s->rows){
  for(x=0;x<s->width;x++)for(y=0;y<s->height;y++){
   size_t ix=(size_t)(s->x+x)*s->rows+s->y+y;
   unsigned char *en=s->english+((size_t)x*s->height+y)*20;
   uint32_t a,b;memcpy(&a,en+4,4);memcpy(&b,e->cells+ix*20+4,4);
   if(a!=b){memcpy(e->map[ix].english,en,20);e->map[ix].expected_glyph=b;e->map[ix].valid=1;}
  }
  ok=1;
 }
 free(s->english);free(s);return ok;
}
/* For a caller abandoning its Chinese render after begin. Its native English
 * cells remain as rendered; the affected old cache mappings stay invalid. */
static void live_cells_cancel(LiveCellsSnapshot *s) {
 if(s){free(s->english);free(s);}
}
/* Returns an independently allocated English grid, or NULL when no applicable
 * mappings exist/allocation fails. Caller temporarily swaps owner[4] for draw,
 * restores it, then frees this view. Native cells are NEVER modified here.
 * Copy only bytes 0..11: current native colors/animation bytes 12..19 survive. */
static unsigned char *live_cells_view(void *owner) {
 LiveCellsEntry *e=live_cells_find(owner);unsigned char *copy;size_t i,n;int any=0;
 if(!e)return NULL;n=(size_t)e->columns*e->rows;
 for(i=0;i<n;i++)if(e->map[i].valid){uint32_t g;memcpy(&g,e->cells+i*20+4,4);if(g==e->map[i].expected_glyph){any=1;break;}}
 if(!any)return NULL;copy=malloc(n*20);if(!copy)return NULL;memcpy(copy,e->cells,n*20);
 for(i=0;i<n;i++)if(e->map[i].valid){uint32_t g;memcpy(&g,copy+i*20+4,4);if(g==e->map[i].expected_glyph)memcpy(copy+i*20,e->map[i].english,12);}
 return copy;
}
static void live_cells_clear(void) {
 unsigned i;for(i=0;i<LIVE_CELLS_MAX_OWNERS;i++)if(live_cells_entries[i].owner)live_cells_remove(live_cells_entries[i].owner);
}
#endif

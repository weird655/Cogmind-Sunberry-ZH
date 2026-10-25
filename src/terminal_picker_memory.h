#ifndef COGMIND_TERMINAL_PICKER_MEMORY_H
#define COGMIND_TERMINAL_PICKER_MEMORY_H
#include <stdint.h>
#include <string.h>
#define PICKER_MEMORY_KEYS 64
#define PICKER_MEMORY_VIEWS 8
typedef struct {unsigned code,page,scroll;uint64_t selected;char native[INPUT_NATIVE];} PickerRememberedView;
typedef struct {
 int enabled,initialized;uint64_t expanded[PICKER_MEMORY_KEYS];unsigned expanded_count,last_view;
 PickerRememberedView views[PICKER_MEMORY_VIEWS];
} PickerMemory;
static inline void picker_memory_init(PickerMemory *m){if(!m->initialized){memset(m,0,sizeof *m);m->enabled=m->initialized=1;}}
static inline uint64_t picker_memory_key(unsigned kind,const char *name){
 uint64_t h=UINT64_C(14695981039346656037);h=(h^kind)*UINT64_C(1099511628211);
 for(;*name;name++)h=(h^(unsigned char)*name)*UINT64_C(1099511628211);return h?h:1;
}
static inline int picker_memory_expanded(const PickerMemory *m,uint64_t key){for(unsigned i=0;i<m->expanded_count;i++)if(m->expanded[i]==key)return 1;return 0;}
static inline void picker_memory_toggle(PickerMemory *m,uint64_t key){
 for(unsigned i=0;i<m->expanded_count;i++)if(m->expanded[i]==key){memmove(m->expanded+i,m->expanded+i+1,(m->expanded_count-i-1)*sizeof *m->expanded);m->expanded_count--;return;}
 if(key&&m->expanded_count<PICKER_MEMORY_KEYS)m->expanded[m->expanded_count++]=key;
}
static inline PickerRememberedView *picker_memory_view(PickerMemory *m,unsigned code){
 for(unsigned i=0;i<PICKER_MEMORY_VIEWS;i++)if(m->views[i].code==code)return m->views+i;
 for(unsigned i=1;i<PICKER_MEMORY_VIEWS;i++)if(!m->views[i].code){m->views[i].code=code;return m->views+i;}return NULL;
}
static inline void picker_memory_reopen(PickerMemory *m){
 picker_memory_init(m);if(!m->enabled){int enabled=m->enabled;memset(m,0,sizeof *m);m->initialized=1;m->enabled=enabled;}
}
#endif

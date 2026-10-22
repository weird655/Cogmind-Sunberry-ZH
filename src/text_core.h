#ifndef COGMIND_TEXT_CORE_H
#define COGMIND_TEXT_CORE_H
#include <stddef.h>
#include <stdint.h>
typedef struct ZhDictionary ZhDictionary;
typedef struct {uint32_t scalar,token[2];int x,y,width;unsigned style;} ZhRun;
typedef struct {ZhRun *runs;size_t count;int columns,rows,used_rows;char styles[32][128];unsigned style_count;} ZhPlan;
extern const uint16_t zh_registry[];
extern const size_t zh_registry_count;
int zh_decode(const char*,size_t,size_t*,uint32_t*);
int zh_glyph(uint32_t);
ZhDictionary *zh_dictionary_load(const char*,size_t*,char*,size_t);
void zh_dictionary_free(ZhDictionary*);
/* 1 exact, 2 template, 0 no match, -1 invalid/capacity, -2 ambiguous. */
int zh_translate(const ZhDictionary*,const char*,size_t,char*,size_t,size_t*);
int zh_translate_exact(const ZhDictionary*,const char*,size_t,char*,size_t,size_t*);
/* Conservative whitespace/style fallback; conflicting canonical keys return -2. */
int zh_translate_normalized_exact(const ZhDictionary*,const char*,size_t,char*,size_t,size_t*);
/* Unique literal completion for a native clipped title; atomic on failure. */
int zh_translate_unique_prefix(const ZhDictionary*,const char*,size_t,char*,size_t);
int zh_rule_key_is(const ZhDictionary*,size_t,const char*);
unsigned zh_rule_specificity(const ZhDictionary*,size_t);
int zh_rule_is_template(const ZhDictionary*,size_t);
int zh_rule_is_numeric_template(const ZhDictionary*,size_t);
/* Strict UTF-8, ASCII=1 column, corpus non-ASCII=2. Failure does not alter out.
   A width-2 scalar never straddles rows. No partial/clipped success. */
int zh_plan(const char*,size_t,int,int,ZhPlan*);
void zh_plan_free(ZhPlan*);
/* Deep-copy one laid-out row into a compact one-row plan; atomic failure. */
int zh_plan_row(const ZhPlan*,int,ZhPlan*);
#endif

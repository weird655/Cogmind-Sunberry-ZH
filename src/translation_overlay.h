#ifndef COGMIND_TRANSLATION_OVERLAY_H
#define COGMIND_TRANSLATION_OVERLAY_H
#include "text_core.h"
#include <string.h>
static const ZhDictionary *display_extra_dictionary;
/* Shared by native terminal output and generic display. Extra rule IDs use
   a diagnostic sentinel; original business descriptors remain untouched. */
static inline int zh_translate_overlay(const ZhDictionary *base,const ZhDictionary *extra,const char *source,size_t bytes,char *out,size_t capacity,size_t *rule){
 if(!extra)return zh_translate(base,source,bytes,out,capacity,rule);
 size_t a=0,b=0;int r=zh_translate_exact(extra,source,bytes,out,capacity,&b);
 if(r){if(r>0&&rule)*rule=(size_t)-2;return r;}
 r=zh_translate_exact(base,source,bytes,out,capacity,&a);if(r){if(r>0&&rule)*rule=a;return r;}
 if(capacity>32768)return -1;char original[32768],external[32768];
 int ra=zh_translate(base,source,bytes,original,capacity,&a),rb=zh_translate(extra,source,bytes,external,capacity,&b);
 if(rb>0&&(ra<=0||zh_rule_specificity(extra,b)>=zh_rule_specificity(base,a))){strcpy(out,external);if(rule)*rule=(size_t)-2;return rb;}
 if(ra>0){strcpy(out,original);if(rule)*rule=a;return ra;}
 return ra<0?ra:rb<0?rb:0;
}
#endif

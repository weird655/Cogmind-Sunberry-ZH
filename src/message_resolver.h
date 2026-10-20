#ifndef COGMIND_MESSAGE_RESOLVER_H
#define COGMIND_MESSAGE_RESOLVER_H
/* Shared resolution, with explicit semantic profiles rather than hidden
   caller-specific dictionary order. Adapters own native layout and links. */
#include "display_text.h"
#include "repair_text.h"
#include "text_context.h"
typedef enum {MESSAGE_DISPLAY, MESSAGE_TERMINAL, MESSAGE_REPAIR, MESSAGE_NAME} MessageProfile;
static int message_resolve_inner(MessageProfile profile,const char *source,size_t bytes,char *target,size_t capacity,size_t *rule){
 if(rule)*rule=(size_t)-5;
 if(profile==MESSAGE_NAME)return canonical_item_name_translate(display_extra_dictionary,item_dictionary,short_dictionary,dictionary,source,bytes,target,capacity);
 if(profile==MESSAGE_REPAIR){
  int result=repair_translate_stage(item_dictionary,source,bytes,target,capacity);
  if(result==0)result=repair_translate_begin(item_dictionary,source,bytes,target,capacity);
  if(result!=0){if(result>0&&rule)*rule=(size_t)-1;return result;}
 }
 if(profile!=MESSAGE_DISPLAY||memchr(source,'\n',bytes)||memchr(source,'\r',bytes)){
  int result=terminal_translate(dictionary,short_dictionary,display_extra_dictionary,source,bytes,target,capacity,rule);
  if(result!=0)return result;
 }
 int result=display_translate(item_dictionary,short_dictionary,dictionary,source,bytes,target,capacity);
 if(result!=0)return result;
 if(profile!=MESSAGE_DISPLAY)return 0;
 return terminal_translate(dictionary,short_dictionary,display_extra_dictionary,source,bytes,target,capacity,rule);
}
/* Commit is the sole boundary into caller-owned output. Every path has the
   same law: failure/miss leaves output and rule untouched, including aliases.
   Sets stay typed: a NAME request cannot consume a sentence template. */
#ifndef MESSAGE_OBSERVE
#define MESSAGE_OBSERVE(profile,result,source,bytes) ((void)0)
#endif
static int message_resolve_profile(MessageProfile profile,const char *source,size_t bytes,char *target,size_t capacity,size_t *rule){
 if(profile<MESSAGE_DISPLAY||profile>MESSAGE_NAME||!source||!target||!capacity||capacity>32768||!bytes||bytes>16384||memchr(source,0,bytes))return -1;
 char staged[32768];size_t selected=(size_t)-5;
 int result=message_resolve_inner(profile,source,bytes,staged,sizeof staged,&selected);
 if(result>0){size_t length=strlen(staged);if(length>=capacity)result=-1;
  else{memmove(target,staged,length+1);if(rule)*rule=selected;}}
 /* Low-level 1=literal / 2=template is internal provenance. Every native
    adapter consumes one public success code; never leak 2 to ==1 callers. */
 if(result>0)result=1;
 MESSAGE_OBSERVE(profile,result,source,bytes);return result;
}
/* One typed boundary for registered native adapters. The registry owns policy;
   callers cannot silently turn a NAME request into a sentence fallback. */
#ifndef TEXT_OBJECT_OBSERVE
#define TEXT_OBJECT_OBSERVE(object,result) ((void)0)
#endif
static inline int message_resolve_object(const TextObject *object,char *target,size_t capacity,size_t *rule){
 if(!object||!object->source||!object->bytes||object->bytes>16384||memchr(object->source,0,object->bytes)||!target||!capacity||capacity>32768)return -1;
 const TextEntrySpec *spec=text_entry_spec(object->entry);if(!spec)return -1;
 static const MessageProfile profiles[]={MESSAGE_DISPLAY,MESSAGE_TERMINAL,MESSAGE_REPAIR,MESSAGE_NAME};
 int result=message_resolve_profile(profiles[spec->semantic],object->source,object->bytes,target,capacity,rule);
 TEXT_OBJECT_OBSERVE(object,result);return result;
}
static inline int message_resolve_entry(TextEntry entry,const char *source,size_t bytes,char *target,size_t capacity,size_t *rule){
 TextObject object={entry,source,bytes};return message_resolve_object(&object,target,capacity,rule);
}
static inline int message_resolve(const char *source,size_t bytes,char *target,size_t capacity){return message_resolve_profile(MESSAGE_DISPLAY,source,bytes,target,capacity,NULL);}
static inline int message_resolve_terminal(const char *source,size_t bytes,char *target,size_t capacity,size_t *rule){return message_resolve_profile(MESSAGE_TERMINAL,source,bytes,target,capacity,rule);}
#endif

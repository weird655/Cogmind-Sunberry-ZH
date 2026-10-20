#ifndef COGMIND_TEXT_CONTEXT_H
#define COGMIND_TEXT_CONTEXT_H
#include <stddef.h>
#include "record_binding.h"
/* Groups describe presentation; semantics select matching policy. Never infer
   meaning from screen coordinates or apply fuzzy sentence/name substitution. */
typedef enum {TEXT_GROUP_DISPLAY,TEXT_GROUP_TERMINAL,TEXT_GROUP_LOG,TEXT_GROUP_ITEM} TextGroup;
typedef enum {TEXT_SEM_DISPLAY,TEXT_SEM_TERMINAL,TEXT_SEM_REPAIR,TEXT_SEM_NAME} TextSemantic;
typedef enum {TEXT_ENTRY_DISPLAY,TEXT_ENTRY_TERMINAL_QUEUE,TEXT_ENTRY_TERMINAL_SERIAL,TEXT_ENTRY_REPAIR_SERIAL,TEXT_ENTRY_LOG,TEXT_ENTRY_LOG_CANONICAL,TEXT_ENTRY_ITEM_LABEL,TEXT_ENTRY_TERMINAL_SHELL,TEXT_ENTRY_SCRATCH,TEXT_ENTRY_INVENTORY,TEXT_ENTRY_EQUIPPED,TEXT_ENTRY_HOVER,TEXT_ENTRY_STORY_POPUP,TEXT_ENTRY_CREDITS,TEXT_ENTRY_INTRO,TEXT_ENTRY_MANUAL,TEXT_ENTRY_NOTIFICATION,TEXT_ENTRY_COUNT} TextEntry;
typedef struct {TextGroup group;TextSemantic semantic;const char *name;} TextEntrySpec;
static const TextEntrySpec text_entries[TEXT_ENTRY_COUNT]={
 {TEXT_GROUP_DISPLAY,TEXT_SEM_DISPLAY,"display.generic"},
 {TEXT_GROUP_TERMINAL,TEXT_SEM_TERMINAL,"terminal.queue"},
 {TEXT_GROUP_TERMINAL,TEXT_SEM_TERMINAL,"terminal.serial"},
 {TEXT_GROUP_TERMINAL,TEXT_SEM_REPAIR,"terminal.repair.serial"},
 {TEXT_GROUP_LOG,TEXT_SEM_DISPLAY,"log.message"},
 {TEXT_GROUP_LOG,TEXT_SEM_DISPLAY,"log.canonical"},
 {TEXT_GROUP_ITEM,TEXT_SEM_NAME,"item.label"},
 {TEXT_GROUP_TERMINAL,TEXT_SEM_TERMINAL,"terminal.shell"},
 {TEXT_GROUP_DISPLAY,TEXT_SEM_DISPLAY,"display.scratch"},
 {TEXT_GROUP_ITEM,TEXT_SEM_NAME,"item.inventory"},
 {TEXT_GROUP_ITEM,TEXT_SEM_NAME,"item.equipped"},
 {TEXT_GROUP_ITEM,TEXT_SEM_DISPLAY,"item.hover"},
 {TEXT_GROUP_DISPLAY,TEXT_SEM_DISPLAY,"display.story-popup"},
 {TEXT_GROUP_DISPLAY,TEXT_SEM_DISPLAY,"display.credits"},
 {TEXT_GROUP_DISPLAY,TEXT_SEM_DISPLAY,"display.intro"},
 {TEXT_GROUP_DISPLAY,TEXT_SEM_DISPLAY,"display.manual"},
 {TEXT_GROUP_DISPLAY,TEXT_SEM_DISPLAY,"display.notification"}
};
/* Original bytes are borrowed for this call only. Parameters and links remain
   in the original structured adapters; no invented IDs or lossy normalization. */
typedef struct {TextEntry entry;const char *source;size_t bytes;const RecordBinding *binding;} TextObject;
static inline const TextEntrySpec *text_entry_spec(TextEntry entry){return (unsigned)entry<TEXT_ENTRY_COUNT?&text_entries[entry]:NULL;}
#endif

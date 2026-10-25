#ifndef SUNBERRY_TERMINAL_PICKER_PARAMETER_TYPES_H
#define SUNBERRY_TERMINAL_PICKER_PARAMETER_TYPES_H
#include <stdint.h>
#include <wchar.h>
/* One UI contract; each source retains its own native identity rules. */
typedef struct {
 uint32_t source_index,ordinal,prototype,handle,slot,kind;
 char name[INPUT_NATIVE],native[INPUT_NATIVE];
 uint64_t token;
} PickerParameterCandidate;
typedef struct {unsigned code,group;const wchar_t *label;} PickerParameterBranch;
static const PickerParameterBranch picker_parameter_branches[]={
 {1,0,L"制造图纸：选择物品…"},{66,3,L"装载图纸：选择目标…"},
 {76,3,L"扫描：选择部件…"},{81,3,L"回收：选择部件…"},
 {94,3,L"放入：选择部件…"},{96,3,L"研究：选择部件…"}
};
static inline int input_picker_parameter_supported(unsigned code){
 for(unsigned i=0;i<sizeof picker_parameter_branches/sizeof picker_parameter_branches[0];i++)
  if(picker_parameter_branches[i].code==code)return 1;
 return 0;
}
#endif

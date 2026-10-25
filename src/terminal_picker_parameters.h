#ifndef SUNBERRY_TERMINAL_PICKER_PARAMETERS_H
#define SUNBERRY_TERMINAL_PICKER_PARAMETERS_H
#include "picker_schematic_candidates.h"
#include "picker_item_candidates46.h"
#include "picker_load_candidates46.h"

#include "terminal_picker_parameter_types.h"
static int input_picker_parameter_candidates(const InputContext *c,unsigned code,
 PickerParameterCandidate *out,unsigned capacity,unsigned offset,unsigned *total){
 if(total)*total=0;if(!out||!capacity||capacity>16384||!input_picker_parameter_supported(code))return -1;
 int count=-1;
 if(code==1){
  PickerSchematicCandidate *rows=calloc(capacity,sizeof *rows);if(!rows)return -1;
  count=picker_schematic_candidates45(c,code,rows,capacity,offset,total);
  for(int i=0;i<count;i++){PickerParameterCandidate row={0};row.source_index=rows[i].candidate_index;
   row.ordinal=rows[i].ordinal;row.prototype=rows[i].prototype;row.kind=1;row.token=rows[i].token;
   strcpy(row.name,rows[i].name);strcpy(row.native,rows[i].native);out[i]=row;}
  free(rows);
 }else if(code==66){
  PickerLoadCandidate *rows=calloc(capacity,sizeof *rows);if(!rows)return -1;
  count=picker_load_candidates46(c,code,rows,capacity,offset,total);
  for(int i=0;i<count;i++){PickerParameterCandidate row={0};row.ordinal=rows[i].ordinal;
   row.prototype=rows[i].prototype;row.kind=rows[i].kind;row.token=rows[i].token;
   strcpy(row.name,rows[i].name);strcpy(row.native,rows[i].native);out[i]=row;}
  free(rows);
 }else{
  PickerItemCandidate *rows=calloc(capacity,sizeof *rows);if(!rows)return -1;
  count=picker_item_candidates46(c,code,rows,capacity,offset,total);
  for(int i=0;i<count;i++){PickerParameterCandidate row={0};row.ordinal=rows[i].ordinal;
   row.prototype=rows[i].prototype;row.handle=rows[i].handle;row.slot=rows[i].slot;row.token=rows[i].token;
   strcpy(row.name,rows[i].name);strcpy(row.native,rows[i].native);out[i]=row;}
  free(rows);
 }
 return count;
}
#endif

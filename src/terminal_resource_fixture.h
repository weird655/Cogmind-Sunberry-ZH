/* Test-only export of known 45B740 fields. Never called in release builds.
   These are native record ordinals, not inferred file offsets or string scans. */
#ifdef TERMINAL_RESOURCE_PROBE
static void terminal_resource_fixture(void){
 const unsigned fields[]={4,0x3c,0x58,0x78,0x94};
 uint32_t *bank=(void*)va(0xd35b58);unsigned count=safe_native_vector(bank,8192),errors=0,bound=0,rejected=0;
 FILE *f=fopen("terminal-native-records.tsv","wb");if(!f){live_fixture_errors++;return;}
 fputs("ordinal\tfield_offset\tnative_id\traw_hex\tdecoded_hex\tmatch\ttarget_hex\tlink_restore\n",f);
 for(unsigned i=0;i<count;i++){
  void *record=((void**)bank[0])[i];if(!readable(record,0xc0)){errors++;continue;}
  for(unsigned j=0;j<sizeof fields/sizeof fields[0];j++){
   uint32_t *str=(void*)((char*)record+fields[j]);size_t n=str[4];const char *raw=str[5]<16?(void*)str:(void*)(uintptr_t)str[0];
   if(n>16384||n>str[5]||!readable(raw,n?n:1)){errors++;continue;}
   fprintf(f,"%u\t%u\t%u\t",i,fields[j],*(uint32_t*)record);
   for(size_t k=0;k<n;k++)fprintf(f,"%02x",(unsigned char)raw[k]);fputc('\t',f);
   NativeStringValue copy={{0}};typedef void* (__attribute__((thiscall)) *Copy)(void*,const void*);
   ((Copy)va(0x9aefb0))(&copy,str);typedef int (__cdecl *Decode)(void*);((Decode)va(0x4351e0))(&copy);
   uint32_t *v=(void*)&copy;const char *decoded=v[5]<16?(void*)v:(void*)(uintptr_t)v[0];size_t length=v[4];
   if(length>16384||!readable(decoded,length?length:1)){errors++;((NativeVoid)va(0x9af350))(&copy);continue;}
   for(size_t k=0;k<length;k++)fprintf(f,"%02x",(unsigned char)decoded[k]);
   RecordBinding binding={0};if(native_terminal_record_binding((int)i,decoded,length,&binding)){
    bound++;RecordBinding before=binding;
    if(native_terminal_record_binding((int)count,decoded,length,&binding)||memcmp(&before,&binding,sizeof binding))errors++;else rejected++;
   }
   char target[32768]={0};int match=length?message_resolve_entry(TEXT_ENTRY_TERMINAL_SERIAL,decoded,length,target,sizeof target,NULL):0;
   fprintf(f,"\t%d\t",match);if(match>0)for(size_t k=0;target[k];k++)fprintf(f,"%02x",(unsigned char)target[k]);
   LinkedSpan spans[LINKED_COUNT_LIMIT];size_t expected=0;int restored=0;
   if(match>0&&linked_spans(decoded,length,1,spans,&expected)&&expected){
    const ZhDictionary *dicts[]={display_extra_dictionary,item_dictionary,short_dictionary,dictionary};char linked[32768];
    restored=linked_target_restore(decoded,length,target,strlen(target),expected,dicts,4,linked,sizeof linked);
   }
   fprintf(f,"\t%d\n",restored);((NativeVoid)va(0x9af350))(&copy);
  }
 }
 fclose(f);live_fixture_errors+=errors;
 fprintf(logfile,"TERMINAL-RESOURCE-EXPORT records=%u fields=5 bound=%u invalid-index-rejected=%u errors=%u read-only=1\n",count,bound,rejected,errors);fflush(logfile);
}
#endif

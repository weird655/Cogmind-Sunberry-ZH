#ifndef COGMIND_NATIVE_RECORD_BINDING_H
#define COGMIND_NATIVE_RECORD_BINDING_H
#ifdef RECORD_IDENTITY_MANIFEST
#include <bcrypt.h>
typedef struct {RecordName stored;uint64_t decoded_hash;uint32_t decoded_length,field;} TerminalRecordName;
#include "record_identity_generated.h"
static int record_manifest_ready(void){
 static int checked,ready;if(checked)return ready;checked=1;
 wchar_t path[MAX_PATH];DWORD n=GetModuleFileNameW(NULL,path,MAX_PATH);if(!n||n>=MAX_PATH)return 0;
 wchar_t *slash=wcsrchr(path,L'\\');if(!slash||slash-path+32>=MAX_PATH)return 0;wcscpy(slash,L"\\data\\particles.bin");
 FILE *f=_wfopen(path,L"rb");if(!f)return 0;BCRYPT_ALG_HANDLE alg=NULL;BCRYPT_HASH_HANDLE hash=NULL;unsigned char buffer[32768],digest[32];char hex[65];size_t got;int ok=0;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0)<0||BCryptCreateHash(alg,&hash,NULL,0,NULL,0,0)<0)goto done;
 while((got=fread(buffer,1,sizeof buffer,f)))if(BCryptHashData(hash,buffer,(ULONG)got,0)<0)goto done;
 if(ferror(f)||BCryptFinishHash(hash,digest,32,0)<0)goto done;
 for(unsigned i=0;i<32;i++)sprintf(hex+2*i,"%02x",digest[i]);ok=!strcmp(hex,RECORD_PARTICLES_SHA256);
 done:if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);fclose(f);ready=ok;
 fprintf(logfile,"RECORD-MANIFEST ready=%d records=%u scope=version-pinned-item-name\n",ready,(unsigned)RECORD_NAME_COUNT);fflush(logfile);return ready;
}
static int native_record_binding(uint32_t handle,RecordBinding *out){
 if(!record_manifest_ready()||!handle||!out)return 0;
 uint32_t *global=(void*)va(0xd3c224);if(!readable(global,4))return 0;
 uint32_t *registry=(void*)(uintptr_t)*global;if(!readable(registry,0x20))return 0;
 uint32_t index=handle&65535,begin=registry[0],end=registry[1],gens=registry[4],gens_end=registry[5];
 if(end<begin||gens_end<gens||end-begin>4*65536u||gens_end-gens>4*65536u||index>=(end-begin)/4||index>=(gens_end-gens)/4)return 0;
 uint32_t *slot=(void*)(uintptr_t)(begin+4*index),*gen=(void*)(uintptr_t)(gens+4*index);if(!readable(slot,4)||!readable(gen,4)||*gen!=(handle>>16))return 0;
 uint32_t *item=(void*)(uintptr_t)*slot;if(!readable(item,0x14)||item[1]!=handle)return 0;
 uint32_t prototype=item[2];uint32_t *bank=(void*)va(0xd2d1c4);if(!readable(bank,12)||bank[1]<bank[0]||(bank[1]-bank[0])/4!=RECORD_NAME_COUNT)return 0;
 uint32_t *pointers=(void*)(uintptr_t)bank[0];if(!readable(pointers,RECORD_NAME_COUNT*4))return 0;
 unsigned ordinal=0;while(ordinal<RECORD_NAME_COUNT&&pointers[ordinal]!=prototype)ordinal++;if(ordinal==RECORD_NAME_COUNT)return 0;
 uint32_t *name=(void*)(uintptr_t)(prototype+8);if(!readable(name,28)||name[4]>160||name[4]>name[5])return 0;
 const char *text=name[5]>=16?(void*)(uintptr_t)name[0]:(void*)name;if(!readable(text,name[4]))return 0;
 return record_binding_verify(handle,*gen,item[2],pointers[ordinal],ordinal,&record_names[ordinal],text,name[4],out);
}
/* A terminal request carries the actual lore bank index. Its original encoded
   field and decoded output must both match the verified, version-pinned record.
   Identical fields are ambiguous: do not invent a field identity. */
static int native_terminal_record_binding(int index,const char *source,size_t bytes,RecordBinding *out){
#ifdef RECORD_TERMINAL_COUNT
 if(index<0||index>=RECORD_TERMINAL_COUNT||!source||!bytes||!out||!record_manifest_ready())return 0;
 uint32_t *bank=(void*)va(0xd35b58);if(!readable(bank,12)||bank[1]<bank[0]||bank[1]-bank[0]!=RECORD_TERMINAL_COUNT*4||!readable((void*)(uintptr_t)bank[0],RECORD_TERMINAL_COUNT*4))return 0;
 uint32_t pointer=((uint32_t*)(uintptr_t)bank[0])[index];uint32_t *record=(void*)(uintptr_t)pointer;if(!readable(record,0xc0)||record[0]!=(uint32_t)index)return 0;
 uint64_t decoded=record_text_hash(source,bytes);int have=0;RecordBinding staged={0};
 for(unsigned i=0;i<4;i++){
  const TerminalRecordName *field=&terminal_fields[index][i];if(field->decoded_length!=bytes||field->decoded_hash!=decoded)continue;
  uint32_t *str=(void*)(uintptr_t)(pointer+field->field);if(str[4]!=field->stored.length||str[4]>str[5]||str[4]>16384)continue;
  const char *raw=str[5]<16?(void*)str:(void*)(uintptr_t)str[0];if(!readable(raw,str[4]?str[4]:1)||record_text_hash(raw,str[4])!=field->stored.text_hash)continue;
  if(have)return 0;have=1;
  /* For this entry, runtime key is a bank record pointer, not an item handle. */
  staged=(RecordBinding){pointer,pointer,(uint32_t)index,field->stored.prefix,field->stored.payload,field->stored.physical_id};
 }
 if(!have)return 0;*out=staged;return 1;
#else
 (void)index;(void)source;(void)bytes;(void)out;return 0;
#endif
}

#else
static int native_record_binding(uint32_t handle,RecordBinding *out){(void)handle;(void)out;return 0;}
static int native_terminal_record_binding(int index,const char *source,size_t bytes,RecordBinding *out){(void)index;(void)source;(void)bytes;(void)out;return 0;}
#endif
#endif

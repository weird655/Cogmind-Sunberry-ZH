#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include "verified_build.h"
#define PERSONAL_LIMIT 16u
static unsigned personal_count,personal_bank_count;
#define PERSONAL_COUNT personal_count
#define PERSONAL_BANK_COUNT personal_bank_count
typedef struct {unsigned id,source_ordinal,clone_ordinal;char name[128];wchar_t definition[MAX_PATH];char sha[65];} PersonalItem;
static PersonalItem personal_items[PERSONAL_LIMIT];
#if defined(_WIN64)
#error Exact x86 ABI required
#endif
static HMODULE self;
static wchar_t root[MAX_PATH];
static FILE *logfile;
static INIT_ONCE once=INIT_ONCE_STATIC_INIT;
static INIT_ONCE chinese_once=INIT_ONCE_STATIC_INIT;
static int (__cdecl *real_init)(const char*);
static int mode,registered;
static unsigned replay_stage[PERSONAL_LIMIT];
static DWORD startup_thread,dataset_thread;
static unsigned char mt_source[PERSONAL_LIMIT][0x9CC],mt_after[PERSONAL_LIMIT][0x9CC];
void *rng_trampoline,*complete_trampoline;
typedef struct {int a,b;} Coord;
typedef DWORD* (__attribute__((thiscall)) *PlaceFn)(void*,DWORD*,void*,const Coord*);
typedef void* (__attribute__((thiscall)) *ResolveFn)(DWORD*);
static PlaceFn place_original;
static int generated;
static int join(wchar_t *p,const wchar_t *tail){return swprintf(p,MAX_PATH,L"%ls\\%ls",root,tail)>0&&wcslen(root)+wcslen(tail)+2<MAX_PATH;}
static void fail(const char *why){if(logfile){fprintf(logfile,"REFUSED %s\n",why);fflush(logfile);}ExitProcess(1114);}
static int readable(const void *p,size_t n){MEMORY_BASIC_INFORMATION m;if(!p||!VirtualQuery(p,&m,sizeof m)||m.State!=MEM_COMMIT||m.Protect&(PAGE_GUARD|PAGE_NOACCESS))return 0;return (uintptr_t)p+n<=(uintptr_t)m.BaseAddress+m.RegionSize;}
static DWORD u32(const void *p){if(!readable(p,4))fail("invalid 32-bit field");DWORD x;memcpy(&x,p,4);return x;}
static int hash(const wchar_t *relative,const char *expected){
 wchar_t p[MAX_PATH];if(!join(p,relative))return 0;HANDLE f=CreateFileW(p,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);if(f==INVALID_HANDLE_VALUE)return 0;
 BCRYPT_ALG_HANDLE a=NULL;BCRYPT_HASH_HANDLE h=NULL;BYTE buf[65536],digest[32];DWORD n;int ok=0;
 if(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
 if(BCryptCreateHash(a,&h,NULL,0,NULL,0,0)<0)goto done;
 for(;;){if(!ReadFile(f,buf,sizeof buf,&n,NULL))goto done;if(!n)break;if(BCryptHashData(h,buf,n,0)<0)goto done;}
 if(BCryptFinishHash(h,digest,32,0)<0)goto done;
 char hex[65];for(unsigned i=0;i<32;i++)sprintf(hex+2*i,"%02x",digest[i]);ok=!strcmp(hex,expected);
done:if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);CloseHandle(f);return ok;
}
static int raw_hash(const wchar_t *relative,const char *expected){
 wchar_t path[MAX_PATH];char ansi[MAX_PATH*3];if(!join(path,L"zlib1.dll"))return 0;
 HMODULE z=LoadLibraryExW(path,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);if(!z)return 0;
 void*(__cdecl *open)(const char*,const char*)=(void*(__cdecl*)(const char*,const char*))GetProcAddress(z,"gzopen");
 int(__cdecl *read)(void*,void*,unsigned)=(int(__cdecl*)(void*,void*,unsigned))GetProcAddress(z,"gzread");
 int(__cdecl *close)(void*)=(int(__cdecl*)(void*))GetProcAddress(z,"gzclose");
 if(!open||!read||!close||!join(path,relative))return 0;
 BOOL substituted=FALSE;if(!WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,path,-1,ansi,sizeof ansi,NULL,&substituted)||substituted)return 0;
 void *f=open(ansi,"rb");if(!f)return 0;BCRYPT_ALG_HANDLE a=NULL;BCRYPT_HASH_HANDLE h=NULL;BYTE buf[65536],digest[32];int n,ok=0;size_t total=0;
 if(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
 if(BCryptCreateHash(a,&h,NULL,0,NULL,0,0)<0)goto done;
 while((n=read(f,buf,sizeof buf))>0){total+=(unsigned)n;if(total>80000000||BCryptHashData(h,buf,(ULONG)n,0)<0)goto done;}
 if(n<0||BCryptFinishHash(h,digest,32,0)<0)goto done;char hex[65];for(unsigned i=0;i<32;i++)sprintf(hex+2*i,"%02x",digest[i]);ok=!strcmp(hex,expected);
done:close(f);if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);FreeLibrary(z);return ok;
}
static int digest_text(const char *p){if(strlen(p)!=64)return 0;for(unsigned i=0;i<64;i++)if(!((p[i]>='0'&&p[i]<='9')||(p[i]>='a'&&p[i]<='f')))return 0;return 1;}
static void load_registry(void){
 wchar_t ini[MAX_PATH];if(!join(ini,L"mods\\catalog.ini"))fail("catalog path overflow");
 if(GetPrivateProfileIntW(L"Registry",L"Schema",0,ini)!=1)fail("unsupported catalog schema; rebuild mod definitions");
 personal_count=GetPrivateProfileIntW(L"Registry",L"Count",0,ini);
 personal_bank_count=GetPrivateProfileIntW(L"Registry",L"BankCount",0,ini);
 if(!personal_count||personal_count>PERSONAL_LIMIT||personal_bank_count!=1359+personal_count)fail("invalid catalog count");
 char ansi[MAX_PATH*3];BOOL substituted=FALSE;
 if(!WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,ini,-1,ansi,sizeof ansi,NULL,&substituted)||substituted)fail("catalog path not representable");
 char particle_sha[65],message_sha[65];
 GetPrivateProfileStringA("Registry","ParticlesRawSHA","",particle_sha,sizeof particle_sha,ansi);
 GetPrivateProfileStringA("Registry","MessagesRawSHA","",message_sha,sizeof message_sha,ansi);
 if(!digest_text(particle_sha)||!digest_text(message_sha)||!raw_hash(L"data\\particles.bin",particle_sha)||!raw_hash(L"data\\messages.bin",message_sha))fail("catalog database fingerprint mismatch");
 for(unsigned i=0;i<personal_count;i++){
  char section[32];wchar_t wide[32];snprintf(section,sizeof section,"Item%u",i);swprintf(wide,32,L"Item%u",i);
  PersonalItem *p=&personal_items[i];
  p->id=GetPrivateProfileIntW(wide,L"ID",0,ini);
  p->source_ordinal=GetPrivateProfileIntW(wide,L"SourceOrdinal",0,ini);
  p->clone_ordinal=GetPrivateProfileIntW(wide,L"CloneOrdinal",0,ini);
  GetPrivateProfileStringA(section,"Name","",p->name,sizeof p->name,ansi);
  GetPrivateProfileStringA(section,"DefinitionSHA","",p->sha,sizeof p->sha,ansi);
  GetPrivateProfileStringW(wide,L"Definition",L"",p->definition,MAX_PATH,ini);
  if(p->id<1359||p->id>=personal_bank_count||p->source_ordinal+1!=p->clone_ordinal||p->clone_ordinal+1>=personal_bank_count||!p->name[0]||strlen(p->name)>100||!digest_text(p->sha))fail("invalid item catalog entry");
  for(const char *q=p->name;*q;q++)if((unsigned char)*q<32||(unsigned char)*q>126)fail("item catalog name must be ASCII");
  if(wcsncmp(p->definition,L"mods\\",5)||wcsstr(p->definition,L"..")||wcschr(p->definition,L':')||wcschr(p->definition,L'/'))fail("unsafe definition path");
  for(unsigned j=0;j<i;j++)if(p->id==personal_items[j].id||!strcmp(p->name,personal_items[j].name)||p->source_ordinal<=personal_items[j].clone_ordinal)fail("duplicate or overlapping item catalog");
  if(!hash(p->definition,p->sha))fail("definition changed; rebuild registry first");
  fprintf(logfile,"MOD_DISCOVERED id=%u name=%s definition=%ls\n",p->id,p->name,p->definition);
 }
 fflush(logfile);
}
static int native_name(void *prototype,char out[256]){
 BYTE *s=(BYTE*)prototype+8;DWORD n=u32(s+16),cap=u32(s+20);if(n>250||cap<n||cap>1048576)return 0;
 void *p=cap<16?s:(void*)(uintptr_t)u32(s);if(!readable(p,n))return 0;memcpy(out,p,n);out[n]=0;return 1;
}
static unsigned bank_count(void){DWORD b=u32((void*)0xD2D1C4),e=u32((void*)0xD2D1C8);if(e<b||(e-b)%4||(e-b)/4>PERSONAL_BANK_COUNT)fail("invalid prototype bank");return (e-b)/4;}
static void *find(const char *wanted){
 unsigned count=bank_count(),matches=0;void *found=NULL;DWORD b=u32((void*)0xD2D1C4);
 if(count!=(mode==3?PERSONAL_BANK_COUNT:1359u))fail("runtime prototype count mismatch");
 for(unsigned i=0;i<count;i++){void *p=(void*)(uintptr_t)u32((void*)(uintptr_t)(b+4*i));char name[256];if(!native_name(p,name))fail("invalid prototype string");if(!strcmp(name,wanted)){found=p;matches++;fprintf(logfile,"REGISTERED ordinal=%u scalar0=%lu mapped=%lu prototype=%p name=%s\n",i,u32(p),u32((BYTE*)p+4),p,name);}}
 if(matches!=1)fail("name lookup not unique");return found;
}
/* Called before the native envelope RNG. Nothing is read from a disk stream here.
   Original source and inserted clone are adjacent, so restoring B on the next
   loop preserves the exact random state for all remaining original tables. */
__attribute__((used,noinline)) void __cdecl mod_rng_before(DWORD frame){
 if(u32((void*)(uintptr_t)(frame+12))!=0xD2D1C4)return;
 if(!dataset_thread){dataset_thread=GetCurrentThreadId();fprintf(logfile,"DATASET_THREAD id=%lu\n",dataset_thread);}
 if(GetCurrentThreadId()!=dataset_thread)fail("dataset thread mismatch");
 unsigned n=bank_count();
 for(unsigned i=0;i<PERSONAL_COUNT;i++){
  const PersonalItem *p=&personal_items[i];
  if(n==p->source_ordinal){if(replay_stage[i])fail("duplicate source replay");memcpy(mt_source[i],(void*)0xD20D00,0x9CC);replay_stage[i]=1;}
  else if(n==p->clone_ordinal){if(replay_stage[i]!=1)fail("missing source RNG");memcpy(mt_after[i],(void*)0xD20D00,0x9CC);memcpy((void*)0xD20D00,mt_source[i],0x9CC);replay_stage[i]=2;fprintf(logfile,"MOD_REPLAY id=%u ordinal=%u\n",p->id,n);}
  else if(n==p->clone_ordinal+1){if(replay_stage[i]!=2)fail("clone incomplete");memcpy((void*)0xD20D00,mt_after[i],0x9CC);replay_stage[i]=3;fprintf(logfile,"MOD_RESTORED id=%u ordinal=%u\n",p->id,n);}
 }
}
__attribute__((naked)) static void rng_detour(void){__asm__ volatile("pushfl\n\tpushal\n\tpushl %ebp\n\tcall _mod_rng_before\n\taddl $4,%esp\n\tpopal\n\tpopfl\n\tjmp *_rng_trampoline");}
__attribute__((used,noinline)) void __cdecl mod_bank_complete(DWORD frame){
 if(u32((void*)(uintptr_t)(frame+12))!=0xD2D1C4)return;
 if(registered)fail("duplicate item bank load");
 for(unsigned i=0;i<PERSONAL_COUNT;i++){
  if(replay_stage[i]!=3)fail("incomplete item RNG restoration");
  void *p=find(personal_items[i].name);
  if(u32(p)!=personal_items[i].clone_ordinal)fail("registered ordinal mismatch");
  fprintf(logfile,"MOD_REGISTERED id=%u name=%s prototype=%p ordinal=%lu\n",personal_items[i].id,personal_items[i].name,p,u32(p));
 }
 registered=1;fprintf(logfile,"REGISTRY_COMPLETE count=%u gameplay_effects=NOT_VALIDATED save_reload=NOT_VALIDATED\n",PERSONAL_COUNT);fflush(logfile);
}
__attribute__((naked)) static void complete_detour(void){__asm__ volatile("pushfl\n\tpushal\n\tpushl %ebp\n\tcall _mod_bank_complete\n\taddl $4,%esp\n\tpopal\n\tpopfl\n\tjmp *_complete_trampoline");}
static DWORD* __attribute__((thiscall)) place_detour(void *world,DWORD *out,void *prototype,const Coord *coordinate){
 DWORD *result=place_original(world,out,prototype,coordinate);
 if(generated)return result;
 generated=1;
 wchar_t ini[MAX_PATH];join(ini,L"mods\\_dev\\runtime.ini");
 if(!GetPrivateProfileIntW(L"PersonalMods",L"DevelopmentSpawn",0,ini)){fprintf(logfile,"DEVELOPMENT_SPAWN_DISABLED factory_recipe=NOT_IMPLEMENTED\n");fflush(logfile);return result;}
 if(!registered||!readable(coordinate,sizeof *coordinate))fail("placement context not registered");
 for(unsigned i=0;i<PERSONAL_COUNT;i++){
  void *target=find(personal_items[i].name);DWORD handle=0;place_original(world,&handle,target,coordinate);
  if(!handle)fail("native placement rejected");
  void *instance=((ResolveFn)(uintptr_t)0x9B65B0)(&handle);
  if(!readable(instance,0x78)||u32((BYTE*)instance+8)!=(DWORD)(uintptr_t)target||u32((BYTE*)instance+12)!=5)fail("instance mismatch");
  if(personal_items[i].id==1359&&GetPrivateProfileIntW(L"PersonalMods",L"DebugToggleProbe",0,ini)){
   /* Isolated, freshly placed, unowned Hacking Suite clone only. Never run on
      installed gear or force any robot action. Keep the native setter path. */
   if(u32((BYTE*)instance+0x28)==0xffffffffu&&u32((BYTE*)instance+0x40)==0&&u32((BYTE*)target+0xf0)==121&&*((BYTE*)target+0x90)==0){
    typedef void (__attribute__((thiscall)) *SetActive)(void*,unsigned char);
    ((SetActive)(uintptr_t)0x5791a0)(instance,1);
    DWORD active=u32((BYTE*)instance+0x28);
    ((SetActive)(uintptr_t)0x5791a0)(instance,0);
    DWORD inactive=u32((BYTE*)instance+0x28);
    fprintf(logfile,"TOGGLE_NATIVE_PROBE id=1359 on=%lu off=%08lx result=%s\n",active,inactive,active!=0xffffffffu&&inactive==0xffffffffu?"PASS":"FAIL");fflush(logfile);
   }else{fprintf(logfile,"TOGGLE_NATIVE_PROBE skipped=not_fresh_unowned_manual_module\n");fflush(logfile);}
  }
  fprintf(logfile,"SPAWNED handle=%08lx instance=%p prototype=%p state=5 coordinate=%ld,%ld name=%s\n",handle,instance,target,(LONG)u32((BYTE*)instance+20),(LONG)u32((BYTE*)instance+24),personal_items[i].name);fflush(logfile);
 }
 fprintf(logfile,"DEVELOPMENT_GENERATION_COMPLETE independent_instances=%u save_reload=UNTESTED\n",PERSONAL_COUNT);fflush(logfile);return result;
}
static void *hook(DWORD address,const BYTE *expected,unsigned n,void *detour){
 BYTE *at=(BYTE*)(uintptr_t)address;if(memcmp(at,expected,n))fail("hook byte signature mismatch");
 BYTE *tramp=VirtualAlloc(NULL,n+5,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!tramp)fail("trampoline allocation");
 memcpy(tramp,at,n);tramp[n]=0xE9;DWORD delta=address+n-(DWORD)(uintptr_t)(tramp+n+5);memcpy(tramp+n+1,&delta,4);
 DWORD old,tmp;if(!VirtualProtect(tramp,n+5,PAGE_EXECUTE_READ,&old))fail("trampoline protect");FlushInstructionCache(GetCurrentProcess(),tramp,n+5);
 if(!VirtualProtect(at,n,PAGE_EXECUTE_READWRITE,&old))fail("hook protect");
 BYTE patch[16];memset(patch,0x90,n);patch[0]=0xE9;delta=(DWORD)(uintptr_t)detour-address-5;memcpy(patch+1,&delta,4);memcpy(at,patch,n);
 VirtualProtect(at,n,old,&tmp);FlushInstructionCache(GetCurrentProcess(),at,n);return tramp;
}
static BOOL CALLBACK initialize(PINIT_ONCE o,PVOID p,PVOID *c){
 (void)o;(void)p;(void)c;DWORD n=GetModuleFileNameW(self,root,MAX_PATH);if(!n||n>=MAX_PATH)return TRUE;wchar_t *s=wcsrchr(root,L'\\');if(!s)return TRUE;*s=0;
 wchar_t path[MAX_PATH];join(path,L"mods\\_dev\\runtime.log");logfile=_wfopen(path,L"w");if(!logfile)ExitProcess(1114);
 if(!hash(L"COGMIND.exe",EXE_SHA)||!hash(L"physfs_mod_original.dll",PHYSFS_SHA))fail("unsupported EXE or PhysicsFS");
 join(path,L"mods\\_dev\\runtime.ini");mode=GetPrivateProfileIntW(L"Runtime",L"mode",0,path);
 join(path,L"physfs_mod_original.dll");HMODULE original=LoadLibraryExW(path,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);real_init=original?(int(__cdecl*)(const char*))GetProcAddress(original,"PHYSFS_init"):NULL;if(!real_init)fail("original PhysicsFS absent");
 if(mode!=3)fail("personal runtime requires mode3");
 load_registry();
 startup_thread=GetCurrentThreadId();
 rng_trampoline=hook(0x9E1113,rng_bytes,sizeof rng_bytes,rng_detour);
 complete_trampoline=hook(0x9E11ED,complete_bytes,sizeof complete_bytes,complete_detour);
 place_original=(PlaceFn)hook(0x6C5400,place_bytes,sizeof place_bytes,place_detour);
 fprintf(logfile,"ARMED mode=%d thread=%lu spawn_on_first_native_ground_placement=1\n",mode,startup_thread);fflush(logfile);return TRUE;
}
static BOOL CALLBACK initialize_chinese(PINIT_ONCE o,PVOID p,PVOID *c){
 (void)o;(void)p;(void)c;wchar_t path[MAX_PATH];join(path,L"mods\\_dev\\runtime.ini");
 if(!GetPrivateProfileIntW(L"Chinese",L"enable",1,path)){fprintf(logfile,"CHINESE_DISABLED config=0\n");fflush(logfile);return TRUE;}
 /* This shared entry already verified the baseline EXE and either original or
    exact decompressed candidate data. Do not chain the old original-data-only
    autoload proxy or initialize the same native hooks twice. */
 wchar_t catalog_path[MAX_PATH];char catalog_ansi[MAX_PATH*3];BOOL substituted=FALSE;
 if(!join(catalog_path,L"mods\\catalog.ini")||!WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,catalog_path,-1,catalog_ansi,sizeof catalog_ansi,NULL,&substituted)||substituted)fail("Chinese catalog path mismatch");
 for(unsigned i=0;i<CHINESE_FILE_COUNT;i++){
  char key[24],expected[65];snprintf(key,sizeof key,"SHA%u",i);
  GetPrivateProfileStringA("Chinese",key,"",expected,sizeof expected,catalog_ansi);
  if(!digest_text(expected)||(i==0&&strcmp(expected,chinese_files[i].sha))||!hash(chinese_files[i].path,expected))fail("Chinese payload integrity mismatch");
 }
 if(GetModuleHandleW(L"native_zh.dll"))fail("Chinese native library already loaded by another entry");
 if(!SetCurrentDirectoryW(root)||!join(path,L"CogmindZH\\bin\\native_zh.dll"))fail("Chinese working directory unavailable");
 HMODULE zh=LoadLibraryExW(path,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
 fprintf(logfile,"CHINESE_LIBRARY handle=%p error=%lu path=%ls\n",zh,GetLastError(),path);fflush(logfile);
 DWORD(WINAPI *start)(void*)=zh?(DWORD(WINAPI*)(void*))GetProcAddress(zh,"ProbeStart"):NULL;
 if(!start||start(NULL)!=0)fail("Chinese engine initialization incomplete");
 fprintf(logfile,"CHINESE_LOADED engine=0.5.6.4.2.1 shared_entry=1 database_fingerprint_verified=1 mode=%d toggle=RightAlt+F9\n",mode);fflush(logfile);return TRUE;
}
__declspec(dllexport) int __cdecl PHYSFS_init(const char *argv0){
 InitOnceExecuteOnce(&once,initialize,NULL,NULL);int result=real_init?real_init(argv0):0;
 if(result)InitOnceExecuteOnce(&chinese_once,initialize_chinese,NULL,NULL);return result;
}
BOOL WINAPI DllMain(HINSTANCE m,DWORD reason,LPVOID r){(void)r;if(reason==DLL_PROCESS_ATTACH){self=m;DisableThreadLibraryCalls(m);}return TRUE;}

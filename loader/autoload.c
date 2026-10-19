#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include "payload_hashes.h"

static HMODULE self_module;
static INIT_ONCE original_once=INIT_ONCE_STATIC_INIT, chinese_once=INIT_ONCE_STATIC_INIT;
static int (__cdecl *real_init)(const char*);
static wchar_t root[MAX_PATH];

static int join(wchar_t *out,const wchar_t *name){
 return swprintf(out,MAX_PATH,L"%ls\\%ls",root,name)>0 && wcslen(root)+wcslen(name)+2<MAX_PATH;
}
static int file_hash(const wchar_t *name,const char *expected){
 wchar_t path[MAX_PATH];if(!join(path,name))return 0;
 HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);if(f==INVALID_HANDLE_VALUE)return 0;
 BCRYPT_ALG_HANDLE alg=NULL;BCRYPT_HASH_HANDLE hash=NULL;unsigned char digest[32],buf[65536];DWORD n;int ok=0;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
 if(BCryptCreateHash(alg,&hash,NULL,0,NULL,0,0)<0)goto done;
 for(;;){if(!ReadFile(f,buf,sizeof buf,&n,NULL))goto done;if(!n)break;if(BCryptHashData(hash,buf,n,0)<0)goto done;}
 if(BCryptFinishHash(hash,digest,32,0)<0)goto done;
 char hex[65];for(unsigned i=0;i<32;i++)sprintf(hex+i*2,"%02x",digest[i]);ok=strcmp(hex,expected)==0;
done:
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(f);return ok;
}
static void note(const char *message){
 wchar_t path[MAX_PATH];if(!join(path,L"CogmindZH\\autoload.log"))return;
 FILE *f=_wfopen(path,L"ab");if(f){fprintf(f,"%s\n",message);fclose(f);}
}
static BOOL CALLBACK load_original(PINIT_ONCE once,PVOID param,PVOID *context){
 (void)once;(void)param;(void)context;
 wchar_t path[MAX_PATH];DWORD n=GetModuleFileNameW(self_module,root,MAX_PATH);
 if(!n||n>=MAX_PATH)return TRUE;wchar_t *slash=wcsrchr(root,L'\\');if(!slash)return TRUE;*slash=0;
 if(!file_hash(L"physfs_zh_original.dll",ORIGINAL_PHYSFS_SHA))return TRUE;
 if(!join(path,L"physfs_zh_original.dll"))return TRUE;
 HMODULE original=LoadLibraryExW(path,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
 if(original)real_init=(int(__cdecl*)(const char*))GetProcAddress(original,"PHYSFS_init");
 return TRUE;
}
static BOOL CALLBACK load_chinese(PINIT_ONCE once,PVOID param,PVOID *context){
 (void)once;(void)param;(void)context;
 if(!file_hash(L"COGMIND.exe",GAME_EXE_SHA)||!file_hash(L"data\\particles.bin",PARTICLES_SHA)||!file_hash(L"data\\messages.bin",MESSAGES_SHA)){
  note("AUTOLOAD_SKIPPED: unsupported game fingerprint; original PhysicsFS remains active");return TRUE;
 }
 for(unsigned i=0;i<PAYLOAD_FILE_COUNT;i++)if(!file_hash(payload_files[i].path,payload_files[i].sha)){
  note("AUTOLOAD_SKIPPED: Chinese payload integrity mismatch; original English remains active");
  MessageBoxW(NULL,L"汉化文件缺失或被改动，本次使用原生英文。请重新运行安装器修复。",L"Cogmind 汉化",MB_OK|MB_ICONWARNING);return TRUE;
 }
 wchar_t exe[MAX_PATH],current[MAX_PATH];DWORD count=GetModuleFileNameW(NULL,exe,MAX_PATH);
 if(!count||count>=MAX_PATH)return TRUE;
 /* ProbeStart uses relative diagnostic/language paths. Normalize to the actual
    EXE directory even when Steam/Explorer supplied another working directory. */
 DWORD prior=GetCurrentDirectoryW(MAX_PATH,current);
 if(!SetCurrentDirectoryW(root)){note("AUTOLOAD_SKIPPED: cannot select game working directory");return TRUE;}
 wchar_t path[MAX_PATH];if(!join(path,L"CogmindZH\\bin\\native_zh.dll"))return TRUE;
 HMODULE patch=LoadLibraryExW(path,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
 DWORD (WINAPI *start)(void*)=patch?(DWORD(WINAPI*)(void*))GetProcAddress(patch,"ProbeStart"):NULL;
 if(!start){if(prior&&prior<MAX_PATH)SetCurrentDirectoryW(current);note("AUTOLOAD_SKIPPED: native DLL entry missing");return TRUE;}
 note("AUTOLOAD_BEGIN: PHYSFS_init normal C ABI call; not DllMain; main startup thread");
 if(start(NULL)!=0){
  note("AUTOLOAD_FAILED: ProbeStart reported partial initialization; process stopped before gameplay");
  MessageBoxW(NULL,L"汉化初始化未完成，已在进入游戏前停止，避免半加载。请运行卸载汉化.cmd恢复。",L"Cogmind 汉化",MB_OK|MB_ICONERROR);
  ExitProcess(1114);
 }
 note("AUTOLOAD_OK: native Chinese initialized; direct EXE and Steam use this same import path");
 return TRUE;
}
__declspec(dllexport) int __cdecl PHYSFS_init(const char *argv0){
 InitOnceExecuteOnce(&original_once,load_original,NULL,NULL);
 if(!real_init)return 0;
 int result=real_init(argv0);
 if(result)InitOnceExecuteOnce(&chinese_once,load_chinese,NULL,NULL);
 return result;
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved){
 (void)reserved;if(reason==DLL_PROCESS_ATTACH){self_module=module;DisableThreadLibraryCalls(module);}return TRUE;
}

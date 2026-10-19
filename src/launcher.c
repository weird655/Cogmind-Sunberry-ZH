#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <wchar.h>
#include <bcrypt.h>
static int valid_exe(const wchar_t *path) {
 const unsigned char expected[32]={0x6c,0x96,0x19,0x2b,0x9b,0x7a,0x81,0x95,0x64,0x16,0xab,0xdb,0x11,0x76,0x69,0x33,0xbc,0xa2,0x1f,0xce,0x8c,0x2c,0x0d,0x57,0xb9,0x7b,0x17,0x2e,0x13,0xcd,0x81,0x84};
 HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);if(f==INVALID_HANDLE_VALUE)return 0;
 BCRYPT_ALG_HANDLE alg=NULL;BCRYPT_HASH_HANDLE hash=NULL;unsigned char digest[32],buf[65536];DWORD n;int ok=0;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
 if(BCryptCreateHash(alg,&hash,NULL,0,NULL,0,0)<0)goto done;
 for(;;){if(!ReadFile(f,buf,sizeof buf,&n,NULL))goto done;if(!n)break;if(BCryptHashData(hash,buf,n,0)<0)goto done;}
 if(BCryptFinishHash(hash,digest,sizeof digest,0)>=0)ok=memcmp(digest,expected,32)==0;
done:
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(f);return ok;
}
static int remote(HANDLE p,LPTHREAD_START_ROUTINE fn,void *arg,DWORD *result) {
 HANDLE t=CreateRemoteThread(p,NULL,0,fn,arg,0,NULL);if(!t)return 0;
 if(WaitForSingleObject(t,20000)!=WAIT_OBJECT_0){CloseHandle(t);return 0;}
 int ok=GetExitCodeThread(t,result);CloseHandle(t);return ok;
}
int wmain(int argc,wchar_t **argv) {
 if(argc<3||argc>4){fwprintf(stderr,L"Usage: ProbeLauncher.exe isolated-game-dir probe-dll [seconds 0..1800, 0=interactive]\n");return 2;}
 wchar_t *seconds_end=NULL;unsigned long seconds=argc==4?wcstoul(argv[3],&seconds_end,10):25;if(seconds>1800||(argc==4&&(!*argv[3]||*seconds_end)))return 2;
 wchar_t exe[32768],dll[32768],game[32768];
 DWORD gn=GetFullPathNameW(argv[1],32768,game,NULL),dn=GetFullPathNameW(argv[2],32768,dll,NULL);if(!gn||gn>=32700||!dn||dn>=32768)return 2;
 for(wchar_t *p=game;*p;p++)if(*p==L'/')*p=L'\\';
 swprintf(exe,32768,L"%ls\\COGMIND.exe",game);
 if(!valid_exe(exe)){fputs("Target SHA256 mismatch; refused\n",stderr);return 6;}
 wchar_t plugin[32768];swprintf(plugin,32768,L"%ls\\WININET.dll",game);
 if(GetFileAttributesW(plugin)!=INVALID_FILE_ATTRIBUTES){fputs("WININET.dll present; use isolated copy without legacy plugin\n",stderr);return 7;}
 STARTUPINFOW si={0};PROCESS_INFORMATION pi={0};si.cb=sizeof si;
 if(!CreateProcessW(exe,NULL,NULL,NULL,FALSE,CREATE_SUSPENDED,NULL,game,&si,&pi)){fprintf(stderr,"CreateProcess %lu\n",GetLastError());return 3;}
 int rc=4;void *buf=VirtualAllocEx(pi.hProcess,NULL,(wcslen(dll)+1)*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!buf || !WriteProcessMemory(pi.hProcess,buf,dll,(wcslen(dll)+1)*2,NULL))goto done;
 DWORD loaded=0;
 if(!remote(pi.hProcess,(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW"),buf,&loaded)||!loaded){fprintf(stderr,"LoadLibrary failed\n");goto done;}
 HMODULE local=LoadLibraryExW(dll,NULL,DONT_RESOLVE_DLL_REFERENCES);if(!local)goto done;
 FARPROC entry=GetProcAddress(local,"ProbeStart");uintptr_t offset=(uintptr_t)entry-(uintptr_t)local;FreeLibrary(local);
 DWORD initialized=1;
 if(!entry || !remote(pi.hProcess,(LPTHREAD_START_ROUTINE)(loaded+offset),NULL,&initialized)||initialized!=0){fprintf(stderr,"ProbeStart failed\n");goto done;}
 VirtualFreeEx(pi.hProcess,buf,0,MEM_RELEASE);
 ResumeThread(pi.hThread);printf("Isolated probe PID %lu; capture %lu seconds\n",pi.dwProcessId,seconds);fflush(stdout);
 DWORD waited=WaitForSingleObject(pi.hProcess,seconds?seconds*1000:INFINITE);
 if(waited==WAIT_OBJECT_0){DWORD ec;GetExitCodeProcess(pi.hProcess,&ec);printf("Game exit %lu\n",ec);rc=ec?5:0;}else rc=0;
done:
 TerminateProcess(pi.hProcess,0);WaitForSingleObject(pi.hProcess,5000);
 CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return rc;
}

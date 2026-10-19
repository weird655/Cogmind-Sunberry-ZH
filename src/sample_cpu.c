#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
int main(int argc,char **argv){if(argc!=2)return 2;DWORD pid=(DWORD)strtoul(argv[1],NULL,10);HANDLE p=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION,FALSE,pid);if(!p)return 3;
 THREADENTRY32 te={0};te.dwSize=sizeof te;HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);DWORD tid=0;ULONGLONG best=0;
 if(Thread32First(snap,&te))do{if(te.th32OwnerProcessID!=pid)continue;HANDLE t=OpenThread(THREAD_QUERY_INFORMATION,FALSE,te.th32ThreadID);FILETIME c,e,k,u;if(t&&GetThreadTimes(t,&c,&e,&k,&u)){ULARGE_INTEGER a,b;a.LowPart=k.dwLowDateTime;a.HighPart=k.dwHighDateTime;b.LowPart=u.dwLowDateTime;b.HighPart=u.dwHighDateTime;if(a.QuadPart+b.QuadPart>best){best=a.QuadPart+b.QuadPart;tid=te.th32ThreadID;}}if(t)CloseHandle(t);}while(Thread32Next(snap,&te));CloseHandle(snap);
 HANDLE t=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT,FALSE,tid);if(!t)return 4;printf("pid=%lu thread=%lu\n",pid,tid);
 for(unsigned i=0;i<100;i++){CONTEXT ctx={0};ctx.ContextFlags=CONTEXT_FULL;DWORD frames[20]={0};SIZE_T n;int ok=0;if(SuspendThread(t)!=(DWORD)-1){ok=GetThreadContext(t,&ctx);if(ok){DWORD frame=ctx.Ebp;for(unsigned j=0;j<10;j++){DWORD f[2];if(!ReadProcessMemory(p,(void*)(uintptr_t)frame,f,8,&n)||n!=8)break;frames[j]=f[1];if(f[0]<=frame||f[0]-frame>1048576)break;frame=f[0];}}ResumeThread(t);}if(ok){printf("%08lx",ctx.Eip);for(unsigned j=0;j<10&&frames[j];j++)printf(" %08lx",frames[j]);putchar('\n');}Sleep(20);}
 CloseHandle(t);CloseHandle(p);return 0;}

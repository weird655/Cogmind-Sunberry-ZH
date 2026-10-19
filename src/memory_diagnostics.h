#include <psapi.h>
static FILE *memory_log;
static LONG memory_exception_count;
static _Thread_local int memory_handler_busy;
static void diagnostic_memory_tick(void){
 static DWORD last;static unsigned last_rows;DWORD now=GetTickCount();if(!memory_log||(last&&now-last<30000&&last_rows==scratch_row_count))return;last=now;last_rows=scratch_row_count;
 typedef BOOL (WINAPI *MemoryInfoFn)(HANDLE,PPROCESS_MEMORY_COUNTERS,DWORD);
 MemoryInfoFn fn=(MemoryInfoFn)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"K32GetProcessMemoryInfo");
 PROCESS_MEMORY_COUNTERS_EX info={0};info.cb=sizeof info;
 if(fn&&fn(GetCurrentProcess(),(PPROCESS_MEMORY_COUNTERS)&info,sizeof info)){
  size_t pool=0;for(unsigned i=0;i<scratch_row_count;i++)pool+=strlen(scratch_rows[i].text)+strlen(scratch_rows[i].english)+2;
  fprintf(memory_log,"MEMORY tick=%lu working-set=%llu private=%llu peak=%llu unicode-rows=%u unicode-bytes=%llu\n",now,(unsigned long long)info.WorkingSetSize,(unsigned long long)info.PrivateUsage,(unsigned long long)info.PeakWorkingSetSize,scratch_row_count,(unsigned long long)pool);fflush(memory_log);
 }
}
static LONG WINAPI diagnostic_exception(EXCEPTION_POINTERS *p){
 DWORD code=p->ExceptionRecord->ExceptionCode;if(!memory_log||memory_handler_busy||(code!=0xc0000005&&code!=0xe06d7363)||InterlockedIncrement(&memory_exception_count)>16)return EXCEPTION_CONTINUE_SEARCH;
 memory_handler_busy=1;CONTEXT *c=p->ContextRecord;
 fprintf(memory_log,"EXCEPTION code=%08lx address=%p eip=%08lx eax=%08lx ecx=%08lx edx=%08lx esp=%08lx ebp=%08lx\n",code,p->ExceptionRecord->ExceptionAddress,c->Eip,c->Eax,c->Ecx,c->Edx,c->Esp,c->Ebp);
 if(code==0xc0000005&&p->ExceptionRecord->NumberParameters>=2)fprintf(memory_log,"ACCESS operation=%llu target=%08llx\n",(unsigned long long)p->ExceptionRecord->ExceptionInformation[0],(unsigned long long)p->ExceptionRecord->ExceptionInformation[1]);
 if(readable((void*)c->Esp,64)){uint32_t *s=(void*)c->Esp;fprintf(memory_log,"STACK");for(unsigned i=0;i<16;i++)fprintf(memory_log," %08lx",(unsigned long)s[i]);fputc('\n',memory_log);}
 uintptr_t frame=c->Ebp;for(unsigned i=0;i<24&&readable((void*)frame,8);i++){uintptr_t *f=(void*)frame;fprintf(memory_log,"FRAME %u return=%08lx\n",i,(unsigned long)f[1]);if(f[0]<=frame||f[0]-frame>1024*1024)break;frame=f[0];}
 fflush(memory_log);memory_handler_busy=0;return EXCEPTION_CONTINUE_SEARCH;
}
static void diagnostic_memory_start(void){
 memory_log=fopen("memory-diagnostics.log","wb");if(!memory_log)return;
 fprintf(memory_log,"pid=%lu diagnostics=exception-context+bounded-stack+process-memory; handler=continue-search\n",GetCurrentProcessId());fflush(memory_log);
 AddVectoredExceptionHandler(1,diagnostic_exception);diagnostic_memory_tick();
}

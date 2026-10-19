static LARGE_INTEGER perf_frequency;
static unsigned long long perf_ticks[4],perf_counts[4],perf_max[4];
static unsigned long long perf_now(void){LARGE_INTEGER n;QueryPerformanceCounter(&n);return (unsigned long long)n.QuadPart;}
static void perf_add(unsigned slot,unsigned long long start){unsigned long long elapsed=perf_now()-start;perf_ticks[slot]+=elapsed;perf_counts[slot]++;if(elapsed>perf_max[slot])perf_max[slot]=elapsed;}
static void perf_flush(void){
 static DWORD last;DWORD now=GetTickCount();if(now-last<3000)return;last=now;if(!perf_frequency.QuadPart)QueryPerformanceFrequency(&perf_frequency);
 FILE *f=fopen("render-performance.log","ab");if(!f)return;
 fprintf(f,"PERF tick=%lu lookup_ms=%.3f bridge_ms=%.3f draw_ms=%.3f root_max_ms=%.3f lookup_calls=%llu bridge_calls=%llu draw_calls=%llu root_calls=%llu\n",now,perf_ticks[0]*1000.0/perf_frequency.QuadPart,perf_ticks[1]*1000.0/perf_frequency.QuadPart,perf_ticks[2]*1000.0/perf_frequency.QuadPart,perf_max[3]*1000.0/perf_frequency.QuadPart,perf_counts[0],perf_counts[1],perf_counts[2],perf_counts[3]);fclose(f);
 memset(perf_ticks,0,sizeof perf_ticks);memset(perf_counts,0,sizeof perf_counts);memset(perf_max,0,sizeof perf_max);
}

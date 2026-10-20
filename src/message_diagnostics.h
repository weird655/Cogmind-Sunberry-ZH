/* Bounded route counters describe observed requests, not translation completeness. */
typedef struct {uint64_t calls,matched,missing,ambiguous,invalid,bound,layouts,layout_failures;} MessageRouteCounts;
static MessageRouteCounts message_route_counts[TEXT_ENTRY_COUNT];
static void message_route_dump(void){
 FILE *f=fopen("text-route-coverage.log","wb");if(!f)return;
 fprintf(f,"TEXT-ROUTES observed-only=1 entries=%u\n",(unsigned)TEXT_ENTRY_COUNT);
 for(unsigned i=0;i<TEXT_ENTRY_COUNT;i++){MessageRouteCounts *c=&message_route_counts[i];fprintf(f,"ROUTE entry=%s calls=%llu matched=%llu missing=%llu ambiguous=%llu invalid=%llu bound=%llu layouts=%llu layout-failures=%llu\n",text_entries[i].name,(unsigned long long)c->calls,(unsigned long long)c->matched,(unsigned long long)c->missing,(unsigned long long)c->ambiguous,(unsigned long long)c->invalid,(unsigned long long)c->bound,(unsigned long long)c->layouts,(unsigned long long)c->layout_failures);}
 fclose(f);
}
static void message_route_flush(void){static DWORD last;DWORD now=GetTickCount();if(now-last<10000)return;last=now;message_route_dump();}
/* Per-thread bounded miss set: no allocations, no per-frame duplicate spam.
   One physical log line per source; escapes preserve tabs/newlines safely. */
static void message_observe(int profile,int result,const char *source,size_t bytes){
 if(result>0||!logfile||!source)return;
 static _Thread_local uint64_t seen[512];static _Thread_local unsigned used;
 if(used>=384)return;
 uint64_t hash=1469598103934665603ULL;
 hash^=(unsigned)profile;hash*=1099511628211ULL;hash^=(unsigned)(result+3);hash*=1099511628211ULL;
 for(size_t i=0;i<bytes;i++){hash^=(unsigned char)source[i];hash*=1099511628211ULL;}if(!hash)hash=1;
 unsigned slot=(unsigned)hash&511;while(seen[slot]&&seen[slot]!=hash)slot=(slot+1)&511;if(seen[slot])return;
 seen[slot]=hash;used++;
 const TextEntrySpec *entry=profile>=100?text_entry_spec((TextEntry)(profile-100)):NULL;
 fprintf(logfile,"MESSAGE-UNRESOLVED profile=%d entry=%s reason=%s result=%d bytes=%u raw=",profile,entry?entry->name:"legacy",result==0?"missing":result==-2?"ambiguous":"invalid-or-capacity",result,(unsigned)bytes);
 for(size_t i=0;i<bytes;i++){unsigned char c=source[i];if(c=='\n')fputs("\\n",logfile);else if(c=='\r')fputs("\\r",logfile);else if(c=='\t')fputs("\\t",logfile);else if(c=='\\')fputs("\\\\",logfile);else if(c<32||c==127)fprintf(logfile,"\\x%02x",c);else fputc(c,logfile);}fputc('\n',logfile);
}

/* One layout diagnostic per entry, bounded independently of missing translations.
   These are layout failures, not missing TSV records. */
static void message_layout_observe(int entry,int result,int columns,int rows){
 if(text_entry_spec((TextEntry)entry)){message_route_counts[entry].layouts++;message_route_counts[entry].layout_failures+=result!=0;}
 if(!result||!logfile)return;
 static _Thread_local unsigned char seen[TEXT_ENTRY_COUNT];
 const TextEntrySpec *spec=text_entry_spec((TextEntry)entry);if(!spec||seen[entry])return;
 seen[entry]=1;fprintf(logfile,"TEXT-LAYOUT-FAIL entry=%s columns=%d rows=%d result=%d\n",spec->name,columns,rows,result);
}

static void message_binding_observe(const TextObject *object,int result){
 MessageRouteCounts *counts=&message_route_counts[object->entry];if(!counts->calls&&logfile){fprintf(logfile,"TEXT-ENTRY-FIRST entry=%s\n",text_entries[object->entry].name);fflush(logfile);}
 counts->calls++;counts->matched+=result>0;counts->missing+=result==0;counts->ambiguous+=result==-2;counts->invalid+=result<0&&result!=-2;counts->bound+=object->binding!=NULL;
 if(!object->binding){message_observe(100+(int)object->entry,result,object->source,object->bytes);return;}
 const RecordBinding *b=object->binding;static struct {uint32_t handle;TextEntry entry;} seen[128];static unsigned count;
 for(unsigned i=0;i<count;i++)if(seen[i].handle==b->handle&&seen[i].entry==object->entry){message_observe(100+(int)object->entry,result,object->source,object->bytes);return;}if(count>=128){message_observe(100+(int)object->entry,result,object->source,object->bytes);return;}seen[count].handle=b->handle;seen[count++].entry=object->entry;
 const TextEntrySpec *entry=text_entry_spec(object->entry);
 fprintf(logfile,"TEXT-RECORD-BOUND entry=%s handle=%08lx ordinal=%lu prefix=%lu payload=%lu id=%s result=%d\n",entry?entry->name:"invalid",(unsigned long)b->handle,(unsigned long)b->ordinal,(unsigned long)b->prefix,(unsigned long)b->payload,b->physical_id,result);fflush(logfile);
 message_observe(100+(int)object->entry,result,object->source,object->bytes);
}

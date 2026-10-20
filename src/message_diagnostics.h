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
 if(!result||!logfile)return;
 static _Thread_local unsigned char seen[TEXT_ENTRY_COUNT];
 const TextEntrySpec *spec=text_entry_spec((TextEntry)entry);if(!spec||seen[entry])return;
 seen[entry]=1;fprintf(logfile,"TEXT-LAYOUT-FAIL entry=%s columns=%d rows=%d result=%d\n",spec->name,columns,rows,result);
}

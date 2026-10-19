/* Test-only native renderer fixture; never installed in production. */
static void repair_popup_fixture(void *parent){
 static int done;if(done||GetTickCount()-probe_start<6500)return;done=1;
 const char *source[]={"Lrg. Battery (27/35)","Beam Cannon (75/100)","Core Shielding (42/50)","Lgt. Ion Engine (26/40)","Ion Engine (44/80)","Sunberry Melta Gun (81/160)","Lgt. Treads (112/140)","Lgt. Treads (116/140)"};
 unsigned passed=0,before=vector_count((char*)parent+0x44);
 for(unsigned i=0;i<8;i++){
  void *o=(*(NativeNew*)va(0xb6132c))(0x128);memset(o,0,0x128);((ConsoleCtor)va(0x48c060))(o,parent,28,1,0,0,0,1,20);
  unsigned char native[28]={0};((StringCtor)va(0x9af0f0))(native,source[i]);unsigned prior=display_translations;
  ((TextFn)va(0x4181d0))(o,0,0,native);((NativeVoid)va(0x9af350))(native);
  uint32_t *v=o;unsigned chinese=0,bad=0;const char *number=strchr(source[i],'(');unsigned column=(unsigned)(number-source[i]);
  for(unsigned x=0;x<v[2];x++){unsigned char *cell=(void*)(v[4]+x*20);unsigned g=*(uint32_t*)(cell+4);
   if(g>=320&&g<320+2*zh_registry_count&&!(g&1)){chinese++;if(x+1>=v[2]||*(uint32_t*)(cell+24)!=g+1||memcmp(cell+12,cell+32,6))bad++;}}
  for(size_t j=0;j<strlen(number);j++)if(*(uint32_t*)(v[4]+(column+j)*20+4)!=(unsigned char)number[j])bad++;
  int ok=display_translations>prior&&chinese>0&&!bad;passed+=ok;
  fprintf(logfile,"REPAIR-POPUP-FIXTURE case=%u passed=%d chinese=%u errors=%u number-column=%u raw=%s\n",i,ok,chinese,bad,column,source[i]);
  uint32_t *pv=(void*)((char*)parent+0x44);unsigned count=vector_count(pv);for(unsigned j=0;j<count;j++)if(((void**)pv[0])[j]==o){memmove((void*)(pv[0]+j*4),(void*)(pv[0]+(j+1)*4),(count-j-1)*4);pv[1]-=4;break;}
  ((DeleteFn)(*(uintptr_t**)o)[0])(o,1);
 }
 fprintf(logfile,"REPAIR-POPUP-FIXTURE-SUMMARY passed=%u expected=8 child-before=%u child-after=%u\n",passed,before,vector_count((char*)parent+0x44));fflush(logfile);ExitProcess(passed==8?0:87);
}

static void zoom_font_surfaces(int);
static void zoom_font_release(void*);
static int zoom_font_prepare(void*);
static void intro_font_surfaces(int);
static void intro_font_release(void*);
static int intro_font_prepare(void*);
/* Full corpus atlas, two cached active sizes. Original native surfaces remain
   owned by the game and are restored during native delayed preparation. */
typedef struct {uint32_t palette;uint8_t bpp,bytes,loss[4],shift[4];uint16_t pad;uint32_t mask[4];uint32_t key;uint8_t alpha;} PF;
typedef struct {uint32_t flags;PF *format;int w,h;uint16_t pitch,pad;void *pixels;} Surface;
typedef Surface* (__cdecl *CreateSurface)(uint32_t,int,int,int,uint32_t,uint32_t,uint32_t,uint32_t);
typedef void (__cdecl *FreeSurface)(Surface*);
typedef void (__attribute__((thiscall)) *Prepare)(void*);
typedef void (__attribute__((thiscall)) *AssignInts)(void*,unsigned,int);
typedef void (__attribute__((thiscall)) *PutCell)(void*,int,int,int);
typedef struct {uint32_t *atlas;Surface *original,*expanded;unsigned age;} FontCache;
static FontCache font_cache[2];
static Prepare original_prepare_set;
static Prepare original_release_atlas;
static unsigned font_detaches,font_owned_frees;
static int demo_state;
static unsigned demo_writes,font_clock,font_capacity;
static uint32_t *font_meta;
static void *last_font_set;
static unsigned last_font_index;
static int glyph_token(uint32_t cp){int id=zh_glyph(cp);return id<0?-1:320+id*2;}
static uint32_t *role0_atlas(void *set){
 if(!readable((char*)set+28,12))return NULL;uint32_t *v=(void*)((char*)set+28);
 return v[1]>=v[0]+4&&readable((void*)v[0],4)?*(void**)v[0]:NULL;
}
static void __attribute__((fastcall)) hooked_release_atlas(void *self,void *unused){
 (void)unused;intro_font_release(self);zoom_font_release(self);
 for(unsigned i=0;i<2;i++)if(font_cache[i].atlas==self){
  FontCache cache=font_cache[i];memset(&font_cache[i],0,sizeof font_cache[i]);uint32_t *atlas=self;
  if((Surface*)atlas[10]!=cache.original&&(Surface*)atlas[10]!=cache.expanded){fprintf(logfile,"FONT-OWNERSHIP-ERROR atlas=%p\n",self);fflush(logfile);ExitProcess(88);}
  if(last_font_set&&role0_atlas(last_font_set)==self){last_font_set=NULL;last_font_index=~0u;}
  atlas[10]=(uint32_t)cache.original;FreeSurface free_surface=(FreeSurface)GetProcAddress(GetModuleHandleW(L"SDL.dll"),"SDL_FreeSurface");
  if(cache.expanded&&cache.expanded!=cache.original){free_surface(cache.expanded);font_owned_frees++;}font_detaches++;
  fprintf(logfile,"FONT-DETACH atlas=%p owned-frees=%u detach-count=%u\n",self,font_owned_frees,font_detaches);
 }
 original_release_atlas(self);fflush(logfile);
}
static int rasterize_corpus(Surface *s,int cw,int ch,unsigned first_slot){
 BITMAPINFO bi={0};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=cw*2;bi.bmiHeader.biHeight=-ch;
 bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
 HDC dc=CreateCompatibleDC(NULL);void *pixels=NULL;if(!dc)return 0;
 HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&pixels,NULL,0);
 HFONT font=CreateFontW(-(ch-2),0,0,0,FW_NORMAL,0,0,0,GB2312_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei");
 if(!bitmap||!font){if(bitmap)DeleteObject(bitmap);if(font)DeleteObject(font);DeleteDC(dc);return 0;}
 HGDIOBJ old_bitmap=SelectObject(dc,bitmap),old_font=SelectObject(dc,font);SetTextColor(dc,RGB(255,255,255));SetBkColor(dc,0);
 int ok=1;unsigned ink_total=0,missing=0;
 for(unsigned g=0;g<zh_registry_count;g++){
  WCHAR cp=zh_registry[g];WORD index=0;GetGlyphIndicesW(dc,&cp,1,&index,GGI_MARK_NONEXISTING_GLYPHS);if(index==0xffff)missing++;
  memset(pixels,0,cw*2*ch*4);SIZE size;GetTextExtentPoint32W(dc,&cp,1,&size);TextOutW(dc,(cw*2-size.cx)/2,(ch-size.cy)/2,&cp,1);GdiFlush();unsigned ink=0;
  for(int y=0;y<ch;y++)for(int x=0;x<cw*2;x++){
   uint32_t p=((uint32_t*)pixels)[y*cw*2+x];unsigned alpha=((p&255)+((p>>8)&255)+((p>>16)&255))/3;
   unsigned token=first_slot+g*2+x/cw;int dx=(token%32)*cw+x%cw,dy=(token/32)*ch+y;
   uint32_t out=alpha?s->format->mask[0]|s->format->mask[1]|s->format->mask[2]|((alpha>>s->format->loss[3])<<s->format->shift[3]):0;
   *(uint32_t*)((char*)s->pixels+dy*s->pitch+dx*4)=out;ink+=alpha>0;
  }
  if(!ink){fprintf(logfile,"FONT-EMPTY-GLYPH U+%04x\n",cp);ok=0;break;}ink_total+=ink;
 }
 fprintf(logfile,"FONT-RASTER glyphs=%u missing=%u ink=%u cell=%dx%d\n",(unsigned)zh_registry_count,missing,ink_total,cw,ch);
 SelectObject(dc,old_font);SelectObject(dc,old_bitmap);DeleteObject(font);DeleteObject(bitmap);DeleteDC(dc);return ok&&!missing;
}
static void native_surfaces(int originals){
 intro_font_surfaces(originals);zoom_font_surfaces(originals);
 for(unsigned i=0;i<2;i++)if(font_cache[i].atlas)font_cache[i].atlas[10]=(uint32_t)(originals?font_cache[i].original:font_cache[i].expanded);
 if(font_meta)font_meta[2]=originals?10:font_capacity/32;
}
static int build_active_font(void *set){
 uint32_t *atlas=role0_atlas(set);if(!atlas||!readable(atlas,0x4c))return 0;
 for(unsigned i=0;i<2;i++)if(font_cache[i].atlas==atlas){atlas[10]=(uint32_t)font_cache[i].expanded;font_cache[i].age=++font_clock;return 1;}
 Surface *old=(void*)atlas[10];int cw=atlas[11],ch=atlas[12];
 if(!readable(old,sizeof *old)||!readable(old->format,sizeof(PF))||cw<1||ch<3||old->w!=cw*32||old->h!=ch*10||(old->flags&0x4001)||old->format->bytes!=4||!old->format->mask[3])return 0;
 int height=ch*(int)(font_capacity/32);if(height>32767||old->w*4>65535)return 0;
 HMODULE sdl=GetModuleHandleW(L"SDL.dll");CreateSurface create=(CreateSurface)GetProcAddress(sdl,"SDL_CreateRGBSurface");FreeSurface free_surface=(FreeSurface)GetProcAddress(sdl,"SDL_FreeSurface");if(!create||!free_surface)return 0;
 PF *f=old->format;Surface *expanded=create(0,old->w,height,32,f->mask[0],f->mask[1],f->mask[2],f->mask[3]);if(!expanded)return 0;
 if(expanded->pitch!=old->pitch||memcmp(expanded->format->mask,f->mask,sizeof f->mask)||memcmp(expanded->format->shift,f->shift,sizeof f->shift)){free_surface(expanded);return 0;}
 memset(expanded->pixels,0,expanded->pitch*expanded->h);
 for(int y=0;y<old->h;y++)memcpy((char*)expanded->pixels+y*expanded->pitch,(char*)old->pixels+y*old->pitch,old->w*4);
 DWORD began=GetTickCount();if(!rasterize_corpus(expanded,cw,ch,320)){free_surface(expanded);return 0;}
 unsigned slot=!font_cache[0].atlas?0:!font_cache[1].atlas?1:font_cache[0].age<font_cache[1].age?0:1;
 if(font_cache[slot].atlas){font_cache[slot].atlas[10]=(uint32_t)font_cache[slot].original;free_surface(font_cache[slot].expanded);fprintf(logfile,"FONT-EVICT atlas=%p\n",font_cache[slot].atlas);}
 font_cache[slot]=(FontCache){atlas,old,expanded,++font_clock};atlas[10]=(uint32_t)expanded;
 fprintf(logfile,"FONT-CACHE set=%p atlas=%p surface=%dx%d bytes=%u built-ms=%lu cache-limit=2\n",set,(void*)atlas,expanded->w,expanded->h,expanded->pitch*expanded->h,GetTickCount()-began);fflush(logfile);return 1;
}
static int extend_demo_fonts(void){
 if(demo_state)return demo_state==1;
 uint32_t *roles=(void*)va(0xd2248c);if(!readable(roles,12)||roles[1]-roles[0]<64)return 0;
 font_meta=(void*)roles[0];if(!readable(font_meta,64)||font_meta[0]!=0||font_meta[1]!=32||font_meta[2]!=10)return 0;
 font_capacity=(unsigned)((320+2*zh_registry_count+31)/32)*32;
 void *active=*(void**)va(0xd224c0);if(!active)return 0;
 original_prepare_set(active);if(!build_active_font(active))return 0;
 uint32_t saved[320];memcpy(saved,(void*)font_meta[4],sizeof saved);
 ((AssignInts)va(0x9ceaf0))((char*)font_meta+16,font_capacity,0);memcpy((void*)font_meta[4],saved,sizeof saved);
 for(unsigned i=320;i<font_capacity;i++)((uint32_t*)font_meta[4])[i]=i;
 font_meta[2]=font_capacity/32;((Prepare)va(0x416960))(font_meta);demo_state=1;
 last_font_set=active;last_font_index=*(unsigned*)va(0xd224bc);
 fprintf(logfile,"EXTENDED role=0 glyphs=%u slots=%u mapping=%u color-cache=%u state-cache=%u active-only=1\n",(unsigned)zh_registry_count,font_capacity,(font_meta[5]-font_meta[4])/4,(font_meta[9]-font_meta[8])/16,(font_meta[13]-font_meta[12])/20);fflush(logfile);return 1;
}
#ifdef FONT_SWITCH_SMOKE
static void font_release_fixture(void){
 uint32_t *active=role0_atlas(*(void**)va(0xd224c0));Surface *source=NULL;
 for(unsigned i=0;i<2;i++)if(font_cache[i].atlas==active)source=font_cache[i].original;
 if(!source){fprintf(logfile,"FONT-FIXTURE-ERROR no-source\n");return;}
 HMODULE sdl=GetModuleHandleW(L"SDL.dll");CreateSurface create=(CreateSurface)GetProcAddress(sdl,"SDL_CreateRGBSurface");PF *f=source->format;
 Surface *copy=create(0,source->w,source->h,32,f->mask[0],f->mask[1],f->mask[2],f->mask[3]);if(!copy){fprintf(logfile,"FONT-FIXTURE-ERROR allocation\n");return;}
 for(int y=0;y<source->h;y++)memcpy((char*)copy->pixels+y*copy->pitch,(char*)source->pixels+y*source->pitch,source->w*4);
 typedef void* (__cdecl *Allocate)(size_t);typedef void* (__attribute__((thiscall)) *StrCtor)(void*,const char*);
 typedef void* (__attribute__((thiscall)) *AtlasCtor)(void*,void*,void*,void*,int);typedef void* (__attribute__((thiscall)) *DeleteAtlas)(void*,unsigned);
 uint32_t *atlas=(*(Allocate*)va(0xb6132c))(0x4c);unsigned char name[28]={0};((StrCtor)va(0x9af0f0))(name,"native04-release-fixture");
 ((AtlasCtor)va(0x416ad0))(atlas,font_meta,name,NULL,1);((Prepare)va(0x9af350))(name);atlas[10]=(uint32_t)copy;for(unsigned i=11;i<=14;i++)atlas[i]=active[i];
 uint32_t fake_set[16]={0};void *entry=atlas;fake_set[7]=(uint32_t)&entry;fake_set[8]=fake_set[9]=fake_set[7]+4;
 if(!build_active_font(fake_set)){fprintf(logfile,"FONT-FIXTURE-ERROR build\n");((DeleteAtlas)va(0x9e7090))(atlas,1);return;}
 unsigned before=font_owned_frees;((Prepare)va(0x416b40))(atlas);unsigned first=font_owned_frees-before;((Prepare)va(0x416b40))(atlas);unsigned remaining=0;
 for(unsigned i=0;i<2;i++)remaining+=font_cache[i].atlas==atlas;
 fprintf(logfile,"FONT-RELEASE-FIXTURE null=%u owned-free-delta=%u repeat-free-delta=%u stale-cache=%u\n",atlas[10]==0,first,font_owned_frees-before-first,remaining);
 ((DeleteAtlas)va(0x9e7090))(atlas,1);fprintf(logfile,"FONT-FIXTURE-DELETED\n");fflush(logfile);
}
#endif
static void __attribute__((fastcall)) hooked_prepare_set(void *self,void *unused){
 (void)unused;if(demo_state!=1){original_prepare_set(self);return;}
 native_surfaces(1);original_prepare_set(self);native_surfaces(0);
 if(self!=*(void**)va(0xd224c0))return;
 if(!build_active_font(self)||!intro_font_prepare(self)||!zoom_font_prepare(self)){
  if(!last_font_set){fprintf(logfile,"FONT-NO-VALID-ROLLBACK\n");fflush(logfile);ExitProcess(88);}
  *(void**)va(0xd224c0)=last_font_set;*(unsigned*)va(0xd224bc)=last_font_index;
  fprintf(logfile,"FONT-ROLLBACK active=%u\n",last_font_index);
 }else {last_font_set=self;last_font_index=*(unsigned*)va(0xd224bc);}
 font_meta[2]=font_capacity/32;((Prepare)va(0x416960))(font_meta);
 fprintf(logfile,"FONT-ACTIVATED index=%u slots=%u\n",last_font_index,font_capacity);fflush(logfile);
}
static int try_demo(void *self,int x,int y,void *s){
 if(!readable(s,24)||!readable(self,24))return 0;
 unsigned len=*(uint32_t*)((char*)s+16),cap=*(uint32_t*)((char*)s+20);const char *data=cap<16?s:*(void**)s;
 if(len!=9||!readable(data,9)||memcmp(data,"/ L O G /",9)||((unsigned*)self)[5]!=0)return 0;
 unsigned *o=self;if(x<0||y<0||x+9>(int)o[2]||y>=(int)o[3]||!extend_demo_fonts())return 0;
 original_text(self,x,y,s);LiveCellsSnapshot *live_title_snapshot=live_cells_begin(self,x,y,9,1,0);
 int a=glyph_token(0x65e5),b=glyph_token(0x5fd7);int cells[9]={'/',' ',a,a+1,b,b+1,' ','/',' '};
 for(unsigned i=0;i<9;i++)((PutCell)va(0x4180b0))(self,x+i,y,cells[i]);demo_writes++;
 if(live_title_snapshot)live_cells_commit(live_title_snapshot);
 fprintf(logfile,"TITLE-UNICODE slots=%d,%d,%d,%d\n",a,a+1,b,b+1);return 1;
}

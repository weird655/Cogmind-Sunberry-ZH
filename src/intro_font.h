/* Intro text uses font region 2. Preserve all 672 original art slots. */
static uint32_t *intro_font_meta;
static FontCache intro_font_cache[2];
static unsigned intro_font_capacity,intro_font_clock;
static int intro_font_enabled;
static uint32_t *intro_atlas(void *set){
 if(!readable((char*)set+28,12))return NULL;uint32_t *v=(void*)((char*)set+28);
 return v[1]>=v[0]+12&&readable((void*)v[0],12)?((void**)v[0])[2]:NULL;
}
static void intro_font_surfaces(int originals){
 for(unsigned i=0;i<2;i++)if(intro_font_cache[i].atlas)intro_font_cache[i].atlas[10]=(uint32_t)(originals?intro_font_cache[i].original:intro_font_cache[i].expanded);
 if(intro_font_meta)intro_font_meta[2]=originals?21:intro_font_capacity/32;
}
static void intro_font_release(void *self){
 for(unsigned i=0;i<2;i++)if(intro_font_cache[i].atlas==self){
  FontCache c=intro_font_cache[i];memset(&intro_font_cache[i],0,sizeof c);uint32_t *atlas=self;
  if((Surface*)atlas[10]!=c.original&&(Surface*)atlas[10]!=c.expanded){fprintf(logfile,"INTRO-FONT-OWNERSHIP-ERROR\n");fflush(logfile);ExitProcess(88);}
  atlas[10]=(uint32_t)c.original;((FreeSurface)GetProcAddress(GetModuleHandleW(L"SDL.dll"),"SDL_FreeSurface"))(c.expanded);
  fprintf(logfile,"INTRO-FONT-DETACH atlas=%p\n",self);
 }
}
static int intro_font_prepare(void *set){
 if(!intro_font_enabled)return 1;uint32_t *atlas=intro_atlas(set);if(!atlas||!readable(atlas,0x4c))return 0;
 for(unsigned i=0;i<2;i++)if(intro_font_cache[i].atlas==atlas){atlas[10]=(uint32_t)intro_font_cache[i].expanded;intro_font_cache[i].age=++intro_font_clock;return 1;}
 Surface *old=(void*)atlas[10];int cw=atlas[11],ch=atlas[12];
 if(!readable(old,sizeof *old)||!readable(old->format,sizeof(PF))||cw<1||ch<3||old->w!=cw*32||old->h!=ch*21||(old->flags&0x4001)||old->format->bytes!=4||!old->format->mask[3])return 0;
 int height=ch*(int)(intro_font_capacity/32);if(height>32767||old->w*4>65535)return 0;
 HMODULE sdl=GetModuleHandleW(L"SDL.dll");CreateSurface create=(CreateSurface)GetProcAddress(sdl,"SDL_CreateRGBSurface");FreeSurface free_surface=(FreeSurface)GetProcAddress(sdl,"SDL_FreeSurface");if(!create||!free_surface)return 0;
 PF *f=old->format;Surface *expanded=create(0,old->w,height,32,f->mask[0],f->mask[1],f->mask[2],f->mask[3]);if(!expanded)return 0;
 if(expanded->pitch!=old->pitch||memcmp(expanded->format->mask,f->mask,sizeof f->mask)||memcmp(expanded->format->shift,f->shift,sizeof f->shift)){free_surface(expanded);return 0;}
 memset(expanded->pixels,0,expanded->pitch*expanded->h);for(int y=0;y<old->h;y++)memcpy((char*)expanded->pixels+y*expanded->pitch,(char*)old->pixels+y*old->pitch,old->w*4);
 if(!rasterize_corpus(expanded,cw,ch,672)){free_surface(expanded);return 0;}
 unsigned slot=!intro_font_cache[0].atlas?0:!intro_font_cache[1].atlas?1:intro_font_cache[0].age<intro_font_cache[1].age?0:1;
 if(intro_font_cache[slot].atlas){intro_font_cache[slot].atlas[10]=(uint32_t)intro_font_cache[slot].original;free_surface(intro_font_cache[slot].expanded);}
 intro_font_cache[slot]=(FontCache){atlas,old,expanded,++intro_font_clock};atlas[10]=(uint32_t)expanded;
 fprintf(logfile,"INTRO-FONT-CACHE atlas=%p cell=%dx%d slots=%u\n",atlas,cw,ch,intro_font_capacity);fflush(logfile);return 1;
}
static int intro_font_extend(void){
 if(!extend_demo_fonts())return 0;void *active=*(void**)va(0xd224c0);
 if(intro_font_meta)return intro_font_prepare(active);
 uint32_t *roles=(void*)va(0xd2248c);if(!readable(roles,12)||roles[1]-roles[0]<192)return 0;
 uint32_t *meta=(void*)(roles[0]+128);if(!readable(meta,64)||meta[0]!=1||meta[1]!=32||meta[2]!=21||meta[5]-meta[4]!=672*4)return 0;
 intro_font_capacity=(unsigned)((672+2*zh_registry_count+31)/32)*32;intro_font_enabled=1;
 if(!intro_font_prepare(active)){intro_font_enabled=0;return 0;}
 uint32_t saved[672];memcpy(saved,(void*)meta[4],sizeof saved);
 ((AssignInts)va(0x9ceaf0))((char*)meta+16,intro_font_capacity,0);memcpy((void*)meta[4],saved,sizeof saved);
 for(unsigned i=672;i<intro_font_capacity;i++)((uint32_t*)meta[4])[i]=i;
 intro_font_meta=meta;meta[2]=intro_font_capacity/32;((Prepare)va(0x416960))(meta);
 fprintf(logfile,"INTRO-FONT-EXTENDED preserved=672 slots=%u\n",intro_font_capacity);fflush(logfile);return 1;
}

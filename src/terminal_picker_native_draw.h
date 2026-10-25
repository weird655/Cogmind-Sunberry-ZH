#ifndef COGMIND_PICKER_NATIVE_DRAW_H
#define COGMIND_PICKER_NATIVE_DRAW_H
/* A temporary final canvas, drawn by the existing native CConsole renderer.
   Only the fixed panel is clipped in; native world tile effects stay outside. */
static void input_picker_cell(unsigned char *grid,unsigned columns,unsigned rows,int x,int y,unsigned glyph,int fg,int bg){
 if(x<0||y<0||(unsigned)x>=columns||(unsigned)y>=rows)return;unsigned char *c=grid+((size_t)x*rows+y)*20;uint32_t zero=0,mapped=((uint32_t*)(uintptr_t)font_meta[4])[glyph];memcpy(c,&zero,4);memcpy(c+4,&glyph,4);memcpy(c+8,&mapped,4);
 c[12]=(unsigned char)(fg>>16);c[13]=(unsigned char)(fg>>8);c[14]=(unsigned char)fg;c[15]=(unsigned char)(bg>>16);c[16]=(unsigned char)(bg>>8);c[17]=(unsigned char)bg;
}
static void input_picker_text(unsigned char *grid,unsigned columns,unsigned rows,int x,int y,int width,int height,const char *text,int fg,int bg){
 int start=x,end=x+width,row=0;size_t left=strlen(text);
 while(left&&row<height){size_t used;uint32_t cp;if(!zh_decode(text,left,&used,&cp))break;text+=used;left-=used;if(cp=='\r')continue;if(cp=='\n'){x=start;y++;row++;continue;}int cells=cp<128?1:2;
  int token=cp<128?(int)cp:glyph_token(cp);if(token<0){token='?';cells=1;}if(x+cells>end){if(height==1)break;x=start;y++;if(++row>=height)break;}
  for(int h=0;h<cells;h++)input_picker_cell(grid,columns,rows,x+h,y,(unsigned)(token+h),fg,bg);x+=cells;
 }
}
static int input_picker_view_metrics(void){
 typedef Surface* (__cdecl *GetSurface)(void);GetSurface get=(GetSurface)GetProcAddress(GetModuleHandleW(L"SDL.dll"),"SDL_GetVideoSurface");Surface *surface=get?get():NULL;
 uint32_t *atlas=role0_atlas(*(void**)va(0xd224c0));if(!readable(surface,sizeof *surface)||!readable(atlas,0x34))return 0;int cw=(int)atlas[11],ch=(int)atlas[12];
 if(cw<1||ch<1||surface->w!=input_context.columns*cw||surface->h!=input_context.rows*ch)return 0;input_picker_cw=cw;input_picker_ch=ch;return 1;
}
static void *terminal_input_draw_view(void *owner){
 if((uint32_t)(uintptr_t)owner!=input_context.canvas)return NULL;input_picker_drawn=0;
 if(!input_context.scope||!live_chinese||live_phase||live_replay)return NULL;
 uint32_t *root=owner;if(!readable(root,24)||root[5]||root[2]!=(unsigned)input_context.columns||root[3]!=(unsigned)input_context.rows||!input_picker_view_metrics()||!picker_native_layout(root[2],root[3],input_context.x,input_context.y,input_context.width,&input_picker_box))return NULL;
 size_t size=(size_t)root[2]*root[3]*20;if(!readable((void*)(uintptr_t)root[4],size)||!extend_demo_fonts())return NULL;
 if(input_picker_pixel_capacity<size){void *p=realloc(input_picker_pixels,size);if(!p)return NULL;input_picker_pixels=p;input_picker_pixel_capacity=size;}
 memcpy(input_picker_pixels,(void*)(uintptr_t)root[4],size);unsigned char *grid=input_picker_pixels;unsigned columns=root[2],rows=root[3];PickerNativeLayout b=input_picker_box;const int green=0x00c800,dim=0x006a00,bright=0x00ff00,yellow=0xe0d000;
 for(int y=b.y;y<b.y+b.h;y++)for(int x=b.x;x<b.x+b.w;x++)input_picker_cell(grid,columns,rows,x,y,32,green,0);
 for(int x=b.x;x<b.x+b.w;x++){input_picker_cell(grid,columns,rows,x,b.y,'-',green,0);input_picker_cell(grid,columns,rows,x,b.y+b.h-1,'-',green,0);}
 for(int y=b.y;y<b.y+b.h;y++){input_picker_cell(grid,columns,rows,b.x,y,'|',green,0);input_picker_cell(grid,columns,rows,b.x+b.w-1,y,'|',green,0);}
 input_picker_text(grid,columns,rows,b.x+2,b.y,b.w-4,1," / 终端指令 / ",bright,0);
 input_picker_text(grid,columns,rows,b.x+2,b.y+1,14,1,input_picker_memory.enabled?"[记忆:开]":"[记忆:关]",green,0);
 input_picker_text(grid,columns,rows,b.x+b.w-18,b.y+1,16,1,"[重复上次命令]",input_picker_mode&&input_last_command.valid&&!input_pending?green:dim,0);
 if(input_picker_mode){
  unsigned visible=(unsigned)b.body_rows,max=input_ui_count>visible?input_ui_count-visible:0;if(input_ui_scroll>max)input_ui_scroll=max;
  for(unsigned row=0;row<visible&&input_ui_scroll+row<input_ui_count;row++){
   PickerUiRow *item=input_ui_rows+input_ui_scroll+row;int selected=item->kind==PUI_COMMAND&&item->node==input_picker_selected,fg=selected?0:green,bg=selected?0x00b800:0;
   if(selected)for(int x=b.x+1;x<b.x+b.w-1;x++)input_picker_cell(grid,columns,rows,x,b.body+row,32,fg,bg);
   char prefix[16];snprintf(prefix,sizeof prefix,"%*s%s",(int)item->depth*2,"",item->kind==PUI_COMMAND?"  ":item->kind==PUI_PARAMETER?"> ":picker_memory_expanded(&input_picker_memory,item->key)?"- ":"+ ");int indent=(int)strlen(prefix);
   input_picker_text(grid,columns,rows,b.x+2,b.body+row,b.w-4,1,prefix,fg,bg);input_picker_text(grid,columns,rows,b.x+2+indent,b.body+row,b.w-4-indent,1,item->label,fg,bg);
  }
  input_picker_text(grid,columns,rows,b.x+2,b.y+b.h-6,b.w-4,2,input_picker_preview,yellow,0);
 }
 if(input_ui_count>(unsigned)b.body_rows){int thumb=b.body+(int)((uint64_t)input_ui_scroll*(b.body_rows-1)/(input_ui_count-b.body_rows));for(int y=b.body;y<b.body+b.body_rows;y++)input_picker_cell(grid,columns,rows,b.x+b.w-2,y,y==thumb?'#':'|',dim,0);}
 char nav[80];unsigned end=input_ui_scroll+(unsigned)b.body_rows;if(end>input_ui_count)end=input_ui_count;
 if(input_parameter_active)snprintf(nav,sizeof nav,"%u/%u %u-%u/%u",input_parameter_offset/64+1,input_parameter_total?(input_parameter_total+63)/64:1,input_ui_count?input_ui_scroll+1:0,end,input_ui_count);else snprintf(nav,sizeof nav,"%u-%u/%u",input_ui_count?input_ui_scroll+1:0,end,input_ui_count);
 input_picker_text(grid,columns,rows,b.x+2,b.y+b.h-4,6,1,input_parameter_active?"[上页]":"[上移]",green,0);
 input_picker_text(grid,columns,rows,b.x+10,b.y+b.h-4,b.w-22,1,nav,dim,0);
 input_picker_text(grid,columns,rows,b.x+b.w-10,b.y+b.h-4,8,1,input_parameter_active?"[下页]":"[下移]",green,0);
 input_picker_text(grid,columns,rows,b.x+2,b.y+b.h-2,10,1,"[执行指令]",input_picker_mode&&input_picker_selected<input_picker_model.count&&!input_pending?bright:dim,0);
 input_picker_text(grid,columns,rows,b.x+14,b.y+b.h-2,8,1,input_parameter_active?"[上一级]":"[返回]",green,0);
 input_picker_text(grid,columns,rows,b.x+b.w-12,b.y+b.h-2,10,1,input_picker_mode?"[键盘输入]":"[鼠标选择]",green,0);
 input_picker_drawn=1;input_picker_draw_scope=input_context.scope;input_picker_draw_grid=root[4];return grid;
}
static int terminal_input_draw_background_full(void *owner,void *view){
 static void *previous_owner;static PickerNativeLayout previous;static int cw,ch;
 int full=previous_owner==owner&&(!view||memcmp(&previous,&input_picker_box,sizeof previous)||cw!=input_picker_cw||ch!=input_picker_ch);
 if(view){previous_owner=owner;previous=input_picker_box;cw=input_picker_cw;ch=input_picker_ch;}else if(previous_owner==owner)previous_owner=NULL;return full;
}
static void terminal_input_draw_overlay(void *owner,void *destination,void *view){
 typedef struct {int16_t x,y;uint16_t w,h;} ClipRect;
 typedef void (__cdecl *GetClip)(void*,ClipRect*);typedef int (__cdecl *SetClip)(void*,const ClipRect*);
 HMODULE sdl=GetModuleHandleW(L"SDL.dll");GetClip get=(GetClip)GetProcAddress(sdl,"SDL_GetClipRect");SetClip set=(SetClip)GetProcAddress(sdl,"SDL_SetClipRect");Surface *surface=destination;
 if(!get||!set||!readable(surface,sizeof *surface)||surface->w!=input_context.columns*input_picker_cw||surface->h!=input_context.rows*input_picker_ch){input_picker_drawn=0;return;}
 PickerNativeLayout b=input_picker_box;ClipRect old,clip={(int16_t)(b.x*input_picker_cw),(int16_t)(b.y*input_picker_ch),(uint16_t)(b.w*input_picker_cw),(uint16_t)(b.h*input_picker_ch)};get(destination,&old);
 int right=clip.x+clip.w,bottom=clip.y+clip.h;if(clip.x<old.x)clip.x=old.x;if(clip.y<old.y)clip.y=old.y;if(right>old.x+old.w)right=old.x+old.w;if(bottom>old.y+old.h)bottom=old.y+old.h;
 if(right<=clip.x||bottom<=clip.y){input_picker_drawn=0;return;}clip.w=(uint16_t)(right-clip.x);clip.h=(uint16_t)(bottom-clip.y);if(!set(destination,&clip)){input_picker_drawn=0;return;}
 /* The flat native path uses the same glyph atlas but omits map-cell sprite
    animation. Restore the renderer flag, source pointer and clip immediately. */
 unsigned char *flat=(void*)va(0xd224c4),saved_flat=*flat;uint32_t *root=owner,saved=root[4];*flat=1;root[4]=(uint32_t)(uintptr_t)view;original_draw(owner,destination,NULL);root[4]=saved;*flat=saved_flat;set(destination,&old);
}
enum {PICKER_HIT_MEMORY=1,PICKER_HIT_REPEAT,PICKER_HIT_UP,PICKER_HIT_DOWN,PICKER_HIT_EXECUTE,PICKER_HIT_BACK,PICKER_HIT_MODE,PICKER_HIT_ROW=100};
static int input_picker_hit(int x,int y,uint64_t *key){
 PickerNativeLayout b=input_picker_box;*key=0;if(x<b.x||x>=b.x+b.w||y<b.y||y>=b.y+b.h)return 0;int col=x-b.x,row=y-b.y;
 if(row==1){if(col>=b.w-18)return PICKER_HIT_REPEAT;if(col>=2&&col<16)return PICKER_HIT_MEMORY;}
 if(row==b.h-2){if(col>=b.w-12)return PICKER_HIT_MODE;if(col>=14&&col<22)return PICKER_HIT_BACK;if(col>=2&&col<12)return PICKER_HIT_EXECUTE;}
 if(row==b.h-4)return col<10?PICKER_HIT_UP:col>=b.w-10?PICKER_HIT_DOWN:-1;
 if(input_picker_mode&&y>=b.body&&y<b.body+b.body_rows){unsigned n=input_ui_scroll+(unsigned)(y-b.body);if(n<input_ui_count){*key=input_ui_rows[n].key;return PICKER_HIT_ROW+(int)n;}}return -1;
}
static void input_picker_scroll(int delta){if(delta<0){unsigned n=(unsigned)-delta;input_ui_scroll=input_ui_scroll>n?input_ui_scroll-n:0;}else{unsigned n=input_ui_scroll+(unsigned)delta;input_ui_scroll=n<input_ui_count?n:input_ui_count?input_ui_count-1:0;}input_picker_remember();}
static void input_picker_activate(int hit){
 if(input_pending)return;
 if(hit>=PICKER_HIT_ROW){unsigned n=(unsigned)(hit-PICKER_HIT_ROW);if(n>=input_ui_count)return;PickerUiRow row=input_ui_rows[n];if(row.kind==PUI_GROUP||row.kind==PUI_FAMILY){picker_memory_toggle(&input_picker_memory,row.key);input_picker_flatten();input_picker_remember();}else if(row.kind==PUI_PARAMETER)input_picker_action=100+(int)row.code;else input_picker_select(row.node);return;}
 switch(hit){
 case PICKER_HIT_MEMORY:input_picker_memory.enabled=!input_picker_memory.enabled;input_picker_message_text(input_picker_memory.enabled?"已记住展开状态、页码和滚动位置。":"记忆已关闭，下次打开恢复初始列表。");break;
 case PICKER_HIT_REPEAT:(void)input_picker_repeat();break;case PICKER_HIT_EXECUTE:(void)input_picker_queue();break;
 case PICKER_HIT_BACK:if(input_parameter_active)input_picker_action=4;else input_pending=2;break;
 case PICKER_HIT_MODE:input_picker_set_mode(!input_picker_mode);break;
 case PICKER_HIT_UP:if(input_parameter_active){if(input_parameter_offset)input_picker_action=3;}else input_picker_scroll(-input_picker_box.body_rows);break;
 case PICKER_HIT_DOWN:if(input_parameter_active){if(input_parameter_offset+input_parameter_count<input_parameter_total)input_picker_action=2;}else input_picker_scroll(input_picker_box.body_rows);break;
 }
}
static int terminal_input_event(void *event){
 if(!event||!input_context.scope||!input_picker_drawn||!live_chinese||live_phase||input_picker_draw_scope!=input_context.scope)return 0;unsigned char *e=event;
 if(e[0]==2||e[0]==3){if(!input_picker_mode)return 0;int symbol=0;memcpy(&symbol,e+8,4);if(e[0]==2&&symbol==27&&!input_pending)input_pending=2;return 1;}
 if(e[0]!=4&&e[0]!=5&&e[0]!=6)return 0;InputContext current;if(!input_get_context(&current)||current.scope!=input_context.scope||current.canvas!=input_context.canvas)return 0;
 uint32_t *root=(void*)(uintptr_t)current.canvas;if(root[4]!=input_picker_draw_grid||!input_picker_view_metrics())return 0;
 uint16_t px=0,py=0;memcpy(&px,e+4,2);memcpy(&py,e+6,2);uint64_t key;int hit=input_picker_hit(px/input_picker_cw,py/input_picker_ch,&key);if(e[0]==4)return hit!=0;
 unsigned button=e[2];if(button>7)return hit!=0;unsigned mask=1u<<button;
 if(e[0]==5){if(!hit)return 0;input_picker_capture|=mask;if(button==4||button==5){if(input_picker_mode)input_picker_scroll(button==4?-3:3);return 1;}if(button==1){input_picker_pressed=hit;input_picker_pressed_scope=current.scope;input_picker_pressed_key=key;}return 1;}
 if(input_picker_capture&mask){input_picker_capture&=~mask;if(button==1){int same=hit==input_picker_pressed&&key==input_picker_pressed_key&&current.scope==input_picker_pressed_scope;input_picker_pressed=0;if(same)input_picker_activate(hit);}return 1;}return hit!=0;
}
#endif

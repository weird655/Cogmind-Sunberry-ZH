#ifndef SUNBERRY_TERMINAL_PICKER_LAYOUT_H
#define SUNBERRY_TERMINAL_PICKER_LAYOUT_H
#include <stdint.h>
typedef struct {int x,y,w,h;} PickerRect;
typedef struct {
 int x,y,width,height,font_height,row;
 PickerRect panel,title,mode,tree,preview,execute,back,edit,status;
} PickerLayout;
static inline int picker_layout_min(int a,int b){return a<b?a:b;}
static inline int picker_layout_clamp(int value,int low,int high){return value<low?low:(value>high?high:value);}
/* All rectangles use absolute game-client coordinates. Every control is
   inside panel. tree/preview and edit/status are alternative body views.
   1 success, 0 unavailable/invalid; failure leaves out untouched. */
static inline int picker_layout(int clientW,int clientH,int inputXcells,int inputYcells,int inputWidthcells,int columns,int rows,PickerLayout *out){
 if(!out||clientW<180||clientH<120||columns<=0||rows<=0||inputWidthcells<=0)return 0;
 int64_t left=(int64_t)inputXcells*clientW/columns;
 int64_t nativeWidth=(int64_t)inputWidthcells*clientW/columns;
 int64_t right=left+nativeWidth;
 if(left<0)left=0;if(right>clientW)right=clientW;
 if(left>=clientW||right<=left||right-left<180)return 0;
 int width=(int)(right-left),pad=6,gap=4,innerWidth=width-pad*2;
 int font=picker_layout_clamp(clientH/rows,14,24);
 /* Both four-character header labels and the compact 120 px arrangement
    must fit at the chosen font size; never solve this by widening panel. */
 int widthFont=(innerWidth-gap-24)/8,heightFont=(clientH-30)/4-6;
 font=picker_layout_min(font,picker_layout_min(widthFont,heightFont));
 if(font<14)return 0;
 int row=font+6,header=row+2,footer=row+4;
 int comfortable=7*row+30,ideal=17*row;
 int64_t inputY=(int64_t)inputYcells*clientH/rows;
 int anchor=inputY<0?0:(inputY>=clientH?clientH-1:(int)inputY);
 int below=clientH-anchor,height;
 if(below>=comfortable)height=picker_layout_min(ideal,below);
 else height=picker_layout_min(comfortable,clientH);
 int y=anchor;if(y>clientH-height)y=clientH-height;
 int bodyY=y+pad+header+gap;
 int footerY=y+height-pad-footer;
 int bodyHeight=footerY-gap-bodyY;
 int previewHeight=bodyHeight>=5*row+gap?2*row:row;
 int treeHeight=bodyHeight-gap-previewHeight;
 int editHeight=row+4,statusHeight=bodyHeight-editHeight-gap;
 if(treeHeight<row||statusHeight<font)return 0;
 int modeWidth=4*font+12,backWidth=2*font+16;
 int titleWidth=innerWidth-modeWidth-gap,executeWidth=innerWidth-backWidth-gap;
 if(titleWidth<=0||executeWidth<=0)return 0;
 PickerLayout next={0};next.x=(int)left;next.y=y;next.width=width;next.height=height;next.font_height=font;next.row=row;
 next.panel=(PickerRect){next.x,y,width,height};
 next.title=(PickerRect){next.x+pad,y+pad,titleWidth,header};
 next.mode=(PickerRect){next.x+width-pad-modeWidth,y+pad,modeWidth,header};
 next.tree=(PickerRect){next.x+pad,bodyY,innerWidth,treeHeight};
 next.preview=(PickerRect){next.x+pad,bodyY+treeHeight+gap,innerWidth,previewHeight};
 next.execute=(PickerRect){next.x+pad,footerY,executeWidth,footer};
 next.back=(PickerRect){next.x+width-pad-backWidth,footerY,backWidth,footer};
 next.edit=(PickerRect){next.x+pad,bodyY,innerWidth,editHeight};
 next.status=(PickerRect){next.x+pad,bodyY+editHeight+gap,innerWidth,statusHeight};
 *out=next;return 1;
}
#endif

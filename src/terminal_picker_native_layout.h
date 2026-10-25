#ifndef COGMIND_PICKER_NATIVE_LAYOUT_H
#define COGMIND_PICKER_NATIVE_LAYOUT_H
typedef struct {int x,y,w,h,body,body_rows;} PickerNativeLayout;
static inline int picker_native_layout(int columns,int rows,int x,int y,int width,PickerNativeLayout *out){
 if(!out||columns<36||rows<17||columns>512||rows>512||width<32||x<0||x>=columns||y<0||y>=rows)return 0;
 int w=width+2;if(w>columns-2)w=columns-2;if(w<34)return 0;
 int left=x?x-1:0;if(left+w>columns)left=columns-w;
 int h=rows-2;if(h>23)h=23;int top=y?y-1:0;if(top+h>=rows)top=rows-h-1;if(top<0)top=0;
 PickerNativeLayout result={left,top,w,h,top+3,h-10};*out=result;return result.body_rows>=5;
}
#endif

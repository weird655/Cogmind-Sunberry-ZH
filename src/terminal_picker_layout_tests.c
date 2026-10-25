#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "terminal_picker_layout.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"layout FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static int inside(PickerRect child,PickerRect outer){return child.w>0&&child.h>0&&child.x>=outer.x&&child.y>=outer.y&&(int64_t)child.x+child.w<=(int64_t)outer.x+outer.w&&(int64_t)child.y+child.h<=(int64_t)outer.y+outer.h;}
static int overlap(PickerRect a,PickerRect b){return (int64_t)a.x+a.w>b.x&&(int64_t)b.x+b.w>a.x&&(int64_t)a.y+a.h>b.y&&(int64_t)b.y+b.h>a.y;}
static int valid(PickerLayout p,int width,int height,int nativeWidth,int columns){
 if(!inside(p.panel,(PickerRect){0,0,width,height})||p.x!=p.panel.x||p.y!=p.panel.y||p.width!=p.panel.w||p.height!=p.panel.h||p.font_height<14||p.font_height>24||p.height>17*p.row||(int64_t)p.width>(int64_t)nativeWidth*width/columns||p.tree.h<p.row||p.preview.h<p.row)return 0;
 PickerRect controls[]={p.title,p.mode,p.tree,p.preview,p.execute,p.back,p.edit,p.status};
 for(unsigned i=0;i<sizeof controls/sizeof controls[0];i++)if(!inside(controls[i],p.panel))return 0;
 PickerRect mouse[]={p.title,p.mode,p.tree,p.preview,p.execute,p.back};
 for(unsigned i=0;i<sizeof mouse/sizeof mouse[0];i++)for(unsigned j=i+1;j<sizeof mouse/sizeof mouse[0];j++)if(overlap(mouse[i],mouse[j]))return 0;
 PickerRect keyboard[]={p.title,p.mode,p.edit,p.status,p.execute,p.back};
 for(unsigned i=0;i<sizeof keyboard/sizeof keyboard[0];i++)for(unsigned j=i+1;j<sizeof keyboard/sizeof keyboard[0];j++)if(overlap(keyboard[i],keyboard[j]))return 0;
 return 1;
}
int main(void){
 PickerLayout p,old;memset(&p,0x53,sizeof p);old=p;
 CHECK(!picker_layout(179,480,0,0,80,80,40,&p));CHECK(!memcmp(&p,&old,sizeof p));
 CHECK(!picker_layout(640,119,0,0,80,80,40,&p));CHECK(!memcmp(&p,&old,sizeof p));
 CHECK(!picker_layout(640,480,0,0,80,0,40,&p));CHECK(!memcmp(&p,&old,sizeof p));
 CHECK(!picker_layout(640,480,0,0,-1,80,40,&p));CHECK(!memcmp(&p,&old,sizeof p));
 CHECK(!picker_layout(640,480,0,0,80,80,0,&p));CHECK(!memcmp(&p,&old,sizeof p));
 CHECK(!picker_layout(640,480,0,0,1,80,40,&p));CHECK(!memcmp(&p,&old,sizeof p));
 CHECK(!picker_layout(640,480,79,0,80,80,40,&p));CHECK(!memcmp(&p,&old,sizeof p));
 CHECK(!picker_layout(640,480,0,0,80,80,40,NULL));
 int dimensions[][2]={{640,480},{800,600},{1694,958},{1920,1080},{180,120},{180,121},{200,120},{300,150},{500,200},{600,240}};
 unsigned successful=0,rejected=0;
 for(unsigned d=0;d<sizeof dimensions/sizeof dimensions[0];d++){
  int w=dimensions[d][0],h=dimensions[d][1];
  for(int cols=40;cols<=160;cols+=40)for(int rows=1;rows<=80;rows+=13)
   for(int x=-cols;x<=cols*2;x+=cols/4)for(int y=-rows;y<=rows*2;y+=rows/4+1)
    for(int nw=cols/4;nw<=cols*2;nw+=cols/4){
     memset(&p,0x53,sizeof p);old=p;int result=picker_layout(w,h,x,y,nw,cols,rows,&p);
     if(!result){CHECK(!memcmp(&p,&old,sizeof p));rejected++;}
     else{CHECK(valid(p,w,h,nw,cols));successful++;}
    }
 }
 CHECK(picker_layout(1694,958,72,22,38,160,50,&p));CHECK(valid(p,1694,958,38,160));CHECK(p.tree.h>=3*p.row&&p.preview.h>=2*p.row);
 CHECK(picker_layout(180,120,0,0,80,80,1,&p));CHECK(valid(p,180,120,80,80));CHECK(p.preview.h==p.row);
 CHECK(picker_layout(1920,1080,INT_MIN,INT_MAX,INT_MAX,INT_MAX,INT_MAX,&p)==0);
 CHECK(picker_layout(INT_MAX,INT_MAX,0,0,INT_MAX,INT_MAX,INT_MAX,&p));CHECK(valid(p,INT_MAX,INT_MAX,INT_MAX,INT_MAX));
 printf("PASS terminal picker layout: %u valid geometries, %u atomic unavailability cases; 640/800/1694/1920, narrow/short, cell offsets, header/body/footer and integer bounds\n",successful,rejected);return 0;
}

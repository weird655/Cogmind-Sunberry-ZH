#include <stdio.h>
#include <string.h>
#include "terminal_picker_native_layout.h"
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"layout FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){unsigned checks=0;PickerNativeLayout b;
 const int sizes[][2]={{160,45},{140,60},{80,25},{36,17},{512,512}};
 for(unsigned i=0;i<sizeof sizes/sizeof sizes[0];i++)for(int bottom=0;bottom<2;bottom++){
  int cols=sizes[i][0],rows=sizes[i][1];CHECK(picker_native_layout(cols,rows,cols/2,bottom?rows-1:1,44,&b));
  CHECK(b.x>=0&&b.y>=0&&b.x+b.w<=cols&&b.y+b.h<rows);
  CHECK(b.body==b.y+3&&b.body_rows>=5&&b.body+b.body_rows<=b.y+b.h-6);
  CHECK(b.y+b.h-2>=b.body+b.body_rows+4&&b.y+b.h-2<rows);
  CHECK(b.h<=23&&b.w<=cols-2);
 }
 CHECK(picker_native_layout(160,45,55,40,44,&b));CHECK(b.y==21&&b.h==23&&b.body_rows==13);
 PickerNativeLayout stable=b;
 for(unsigned expanded=0;expanded<4096;expanded++){PickerNativeLayout next;CHECK(picker_native_layout(160,45,55,40,44,&next));CHECK(!memcmp(&stable,&next,sizeof next));}
 const int invalid[][5]={{35,45,1,1,44},{160,16,1,1,44},{160,45,1,1,31},{160,45,-1,1,44},{160,45,160,1,44},{160,45,1,45,44},{513,45,1,1,44}};
 for(unsigned i=0;i<sizeof invalid/sizeof invalid[0];i++)CHECK(!picker_native_layout(invalid[i][0],invalid[i][1],invalid[i][2],invalid[i][3],invalid[i][4],&b));
 CHECK(!picker_native_layout(160,45,1,1,44,NULL));printf("native layout: %u checks passed; footer bounded at bottom; height independent of expansion\n",checks);return 0;
}

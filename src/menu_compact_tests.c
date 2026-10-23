#define MENU_COMPACT_PURE
#include "menu_compact.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 int a,z;assert(menu_color_bounds("Schematic \"Adv. Terrain Scanner\"","图纸 \"高级地形扫描器\"",0,8,&a,&z)&&a==0&&z==3);
 assert(menu_color_bounds("Record! \"Derelict\"","记录! \"流亡者\"",0,5,&a,&z)&&a==0&&z==3);
 assert(menu_color_bounds("Schematic \"Adv. Terrain Scanner\"","图纸 \"高级地形扫描器\"",11,30,&a,&z)&&a==6&&z==19);
 assert(!menu_color_bounds("Schematic \"Adv. Terrain Scanner\"","图纸 \"高级地形扫描器\"",-1,-1,&a,&z));

 uintptr_t b=0x400000;
 for(int target=1;target<=35;target++)for(int source=target;source<=35;source++){
  assert(menu_compact_width_policy(0x8fdee3,b,1,source,target));
  assert(menu_compact_width_policy(0x8fdfc4,b,1,source,target));
  assert(menu_compact_width_policy(0x8fe072,b,1,source,target));
  assert(!menu_compact_width_policy(0x8fdee3,b,0,source,target));
 }
 assert(!menu_compact_width_policy(0x8fdee3,b,1,10,11));
 assert(!menu_compact_width_policy(0x8fdee3,b,1,36,10));
 assert(!menu_compact_width_policy(0x8fdee3,b,1,20,0));
 assert(!menu_compact_width_policy(0x8fc91d,b,1,20,12));
 assert(menu_compact_width_policy(0x18fdee3,0x1400000,1,22,12));
 /* Record! "Derelict" => compact 12 columns: closing bracket x=17,
    dash x=19, probability x=40 remains separate. */
 int source=18,target=12;assert(menu_compact_width_policy(0x8fdfc4,b,1,source,target));assert(5+target==17);assert(5+target+2==19);assert(5+target<40);
 puts("menu compact policy: PASS (all bounded width pairs and caller/scope guards)");return 0;
}


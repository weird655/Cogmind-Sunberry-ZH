#include <stdio.h>
#define INPUT_NATIVE 640
#include "terminal_picker_memory.h"
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"memory FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){unsigned checks=0;PickerMemory m={0};picker_memory_init(&m);CHECK(m.enabled&&m.initialized);
 uint64_t a=picker_memory_key(1,"查询"),b=picker_memory_key(2,"查询");CHECK(a&&b&&a!=b);CHECK(a==picker_memory_key(1,"查询"));
 picker_memory_toggle(&m,a);picker_memory_toggle(&m,b);CHECK(m.expanded_count==2&&picker_memory_expanded(&m,a));
 picker_memory_toggle(&m,a);CHECK(m.expanded_count==1&&!picker_memory_expanded(&m,a)&&picker_memory_expanded(&m,b));
 PickerRememberedView *main=picker_memory_view(&m,0);CHECK(main==m.views);main->scroll=17;main->selected=99;strcpy(main->native,"Query(Storage)");
 const unsigned codes[]={1,66,76,81,94,96};
 for(unsigned i=0;i<6;i++){PickerRememberedView *v=picker_memory_view(&m,codes[i]);CHECK(v!=NULL&&v!=main);v->page=i+1;v->scroll=i+2;v->selected=100+i;strcpy(v->native,"Target");}
 m.last_view=96;PickerMemory before=m;picker_memory_reopen(&m);CHECK(!memcmp(&m,&before,sizeof m));
 for(unsigned i=0;i<6;i++){PickerRememberedView *v=picker_memory_view(&m,codes[i]);CHECK(v->page==i+1&&v->scroll==i+2&&v->selected==100+i&&!strcmp(v->native,"Target"));}
 CHECK(main->scroll==17&&main->selected==99&&!strcmp(main->native,"Query(Storage)"));
 for(unsigned i=m.expanded_count;i<PICKER_MEMORY_KEYS;i++)picker_memory_toggle(&m,1000+i);
 CHECK(m.expanded_count==PICKER_MEMORY_KEYS);picker_memory_toggle(&m,9999);CHECK(m.expanded_count==PICKER_MEMORY_KEYS&&!picker_memory_expanded(&m,9999));
 picker_memory_toggle(&m,b);CHECK(m.expanded_count==PICKER_MEMORY_KEYS-1);picker_memory_toggle(&m,9999);CHECK(picker_memory_expanded(&m,9999));
 m.enabled=0;picker_memory_reopen(&m);CHECK(!m.enabled&&m.initialized&&!m.expanded_count&&!m.last_view);CHECK(!m.views[0].scroll&&!m.views[1].code&&!m.views[0].native[0]);
 m.enabled=1;picker_memory_reopen(&m);CHECK(m.enabled&&!m.expanded_count);printf("picker memory: %u checks passed; independent views, bounded expansion, reopen and opt-out\n",checks);return 0;
}

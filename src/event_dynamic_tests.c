#include "terminal_text.h"
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"event dynamic line %d\n",__LINE__);return 1;}}while(0)
static void run_rule(EventSearch *c,const EventDynamicRule *rule,const EventDictionaries *d,const char *input){
 memset(c,0,sizeof *c);c->dicts=d;c->rule=rule;c->source=input;c->bytes=strlen(input);
 const char *p=rule->source,*last=p;while(*p){size_t z=0;int id=event_marker(p,&z);if(id<0){p++;continue;}
 c->literal[c->count]=last;c->length[c->count]=(size_t)(p-last);c->marker[c->count++]=id;p+=z;last=p;}
 c->literal[c->count]=last;c->length[c->count]=strlen(last);event_walk(c,0,c->length[0]);
}
int main(int argc,char **argv){
 if(argc!=5)return 2;char error[256],out[8192];
 ZhDictionary *items=zh_dictionary_load(argv[1],NULL,error,sizeof error),*shorts=zh_dictionary_load(argv[2],NULL,error,sizeof error),*extra=zh_dictionary_load(argv[3],NULL,error,sizeof error),*para=zh_dictionary_load(argv[4],NULL,error,sizeof error);
 if(!items||!shorts||!extra||!para)return 2;EventDictionaries d={extra,items,shorts,para};
 const char *valid[]={"0","+12","-12.5","0.001"};for(unsigned i=0;i<4;i++)CHECK(event_number(valid[i],strlen(valid[i])));
 const char *bad[]={"NaN","1e9","1.","+","12%"," 12","12\n"};for(unsigned i=0;i<7;i++)CHECK(!event_number(bad[i],strlen(bad[i])));
 strcpy(out,"unchanged");CHECK(event_value(&d,EV_NUMBER,"NaN",3,out,sizeof out,0)==0&&!strcmp(out,"unchanged"));
 CHECK(event_value(&d,EV_IDENTITY,"RA-1N3",6,out,sizeof out,0)==1&&!strcmp(out,"RA-1N3"));
 CHECK(event_value(&d,EV_ATOM,"Unregistered Test Widget",24,out,sizeof out,0)==0);
 strcpy(out,"unchanged");CHECK(event_value(&d,EV_NUMBER,"12",2,out,1,0)<0&&!strcmp(out,"unchanged"));
 CHECK(event_value(&d,EV_MESSAGE,"unknown text without a registered meaning",41,out,sizeof out,0)==0);
 CHECK(event_value(&d,EV_LINES,"Ion Engine\nunknown text without a registered meaning",52,out,sizeof out,0)==0);
 CHECK(event_value(&d,EV_ACTOR,"Acquired",8,out,sizeof out,0)==0);
 CHECK(event_value(&d,EV_ACTOR,"EX-DEC",6,out,sizeof out,0)==1);
 CHECK(event_value(&d,EV_STATS,"Mass -1, Max Integrity +12",25,out,sizeof out,0)==1);
 strcpy(out,"unchanged");CHECK(event_value(&d,EV_STATS,"Mass -1, Unknown Field +3",24,out,sizeof out,0)==0&&!strcmp(out,"unchanged"));
 CHECK(event_value(&d,EV_STATS,"Mass NaN",8,out,sizeof out,0)==0);
 CHECK(event_value(&d,EV_CODE,"COM_CAVE_SEALS",15,out,sizeof out,0)==0); /* length includes NUL */
 CHECK(event_value(&d,EV_CODE,"COM_CAVE_SEALS",14,out,sizeof out,0)==1);
 EventSearch c;const EventDynamicRule repeated={"[%1] meets %1.","%1遇见%1。",{EV_IDENTITY}};
 run_rule(&c,&repeated,&d,"[Bob] meets Bob.");CHECK(c.have&&!c.ambiguous&&!strcmp(c.result,"Bob遇见Bob。"));
 run_rule(&c,&repeated,&d,"[Bob] meets Alice.");CHECK(!c.have);
 const EventDynamicRule ambiguous={"%1 %2.","%2-%1",{EV_IDENTITY,EV_IDENTITY}};
 run_rule(&c,&ambiguous,&d,"Bob Smith Jones.");CHECK(c.ambiguous);
 CHECK(event_dynamic_inner(&d,"Ion Engine",10,out,sizeof out,4)==0);
 zh_dictionary_free(items);zh_dictionary_free(shorts);zh_dictionary_free(extra);zh_dictionary_free(para);
 printf("PASS event dynamic %u numeric, identity, repetition, ambiguity, depth and atomic output guards\n",checks);return 0;
}

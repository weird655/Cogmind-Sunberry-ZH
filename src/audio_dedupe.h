#ifndef AUDIO_DEDUPE_H
#define AUDIO_DEDUPE_H
typedef struct {const void *def;int channel,volume,p4,p5,result;uint32_t tick;int valid;} AudioBurst;
typedef struct {AudioBurst entries[64];unsigned next;} AudioBurstState;
/* No wait/queue. Drops do not extend the fixed window. Explicit channels bypass. */
static int audio_duplicate(AudioBurstState *s,const void *def,int channel,int volume,int p4,int p5,uint32_t now,unsigned window,int *result){
 if(!window||channel!=-1||p5!=0)return 0;
 for(unsigned i=0;i<64;i++){
  AudioBurst *e=&s->entries[i];
  if(e->valid&&e->def==def&&e->channel==channel&&e->volume==volume&&e->p4==p4&&e->p5==p5&&(uint32_t)(now-e->tick)<window){*result=e->result;return 1;}
 }
 return 0;
}
static void audio_accepted(AudioBurstState *s,const void *def,int channel,int volume,int p4,int p5,uint32_t now,int result){
 if(result<0||channel!=-1||p5!=0)return;
 AudioBurst *e=&s->entries[s->next++%64];
 *e=(AudioBurst){def,channel,volume,p4,p5,result,now,1};
}
#endif

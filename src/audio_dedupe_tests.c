#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "audio_dedupe.h"
int main(void){
 AudioBurstState s={0};int a=0,b=0,r=0;
 audio_accepted(&s,&a,-1,50,0,0,100,7);
 assert(audio_duplicate(&s,&a,-1,50,0,0,101,15,&r)&&r==7);
 assert(!audio_duplicate(&s,&b,-1,50,0,0,101,15,&r));
 assert(!audio_duplicate(&s,&a,0,50,0,0,101,15,&r));
 assert(!audio_duplicate(&s,&a,-1,51,0,0,101,15,&r));
 assert(!audio_duplicate(&s,&a,-1,50,1,0,101,15,&r));
 assert(!audio_duplicate(&s,&a,-1,50,0,1,101,15,&r));
 assert(!audio_duplicate(&s,&a,-1,50,0,0,101,0,&r));
 assert(!audio_duplicate(&s,&a,-1,50,0,0,115,15,&r));
 audio_accepted(&s,&b,-1,50,0,0,100,-1);
 assert(!audio_duplicate(&s,&b,-1,50,0,0,101,15,&r));
 audio_accepted(&s,&b,-1,50,0,0,UINT32_MAX-3,8);
 assert(audio_duplicate(&s,&b,-1,50,0,0,2,15,&r)&&r==8);
 for(unsigned i=0;i<1000000;i++)audio_accepted(&s,&a,-1,50,(int)i,0,i,9);
 assert(s.next==1000002);
 puts("PASS: bursts, distinct keys, fixed expiry, disable, failure, tick wrap, bounded million requests");return 0;
}

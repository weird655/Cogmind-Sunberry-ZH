#ifndef COGMIND_EVENT_STATS_H
#define COGMIND_EVENT_STATS_H
/* Finite field vocabulary from native Scrap Engine producer (see the retained disassembly evidence).
   Accept whole stat deltas, never search/replace words in arbitrary prose. */
static const struct {const char *source,*target;} event_stat_labels[]={
 {"Mass","质量"},{"Max Integrity","最大耐久"},{"Coverage","覆盖"},{"Heat","热量"},
 {"Energy Upkeep","能量维持费"},{"Matter Upkeep","物质维持费"},{"Supply","供应"},
 {"Storage","容量"},{"Instability","不稳定性"},{"Time/Move","每步时间"},
 {"Energy","能量"},{"Move Heat","移动热量"},{"Support","支撑力"},{"Penalty","惩罚"},
 {"Burnout","烧毁"},{"Special","特殊"},{"Waypoints","路径点"},{"Energy Cost","能量消耗"},{"Matter Cost","物质消耗"},{"Range","射程"},{"Shot Energy","发射能耗"},
 {"Shot Matter","发射物耗"},{"Shot Heat","发射热量"},{"Recoil","后坐力"},
 {"Targeting","瞄准"},{"Critical","暴击"},{"Disruption","干扰"},
 {"Delay","延迟"},{"Projectiles","弹丸数"},{"Arc","弧度"},{"Min Damage","最低伤害"},
 {"Max Damage","最高伤害"},{"Salvage","回收"},{"Penetration","穿透"},
 {"None-Critical","无暴击"},{"Burn-Critical","燃烧暴击"},{"Meltdown-Critical","熔毁暴击"},
 {"Destroy-Critical","摧毁暴击"},{"Blast-Critical","爆破暴击"},{"Corrupt-Critical","损坏暴击"},
 {"Smash-Critical","粉碎暴击"},{"Sever-Critical","切断暴击"},{"Sunder-Critical","撕裂暴击"},
 {"Intensify-Critical","强化暴击"}
};
static inline int event_stat_delta(const char *s,size_t n,char *out,size_t cap){
 for(unsigned i=0;i<sizeof event_stat_labels/sizeof event_stat_labels[0];i++){
  const char *key=event_stat_labels[i].source,*label=event_stat_labels[i].target;
  size_t k=strlen(key),prefix=0,tail=0;
  if(n>k+1&&!memcmp(s,key,k)&&s[k]==' '){
   size_t a=k+1,b=n;if(b>a&&s[b-1]=='%')b--;
   if(!event_number(s+a,b-a))continue;tail=k;
  }else if(n==k+1&&(s[0]=='+'||s[0]=='-')&&!memcmp(s+1,key,k)){prefix=1;tail=n;}
  else if(n==k&&!memcmp(s,key,k)&&strstr(key,"-Critical")){tail=n;}
  else continue;
  size_t z=strlen(label),remaining=n-tail;if(prefix+z+remaining>=cap)return -1;
  if(prefix)out[0]=s[0];memcpy(out+prefix,label,z);memcpy(out+prefix+z,s+tail,remaining);out[prefix+z+remaining]=0;return 1;
 }
 return 0;
}
static inline int event_stats(const EventDictionaries *d,const char *s,size_t n,char *out,size_t cap,unsigned depth){
 if(!n||n>4096)return 0;char staged[8192];size_t at=0,pos=0;unsigned count=0;
 while(pos<n){
  size_t end=pos;while(end<n&&s[end]!='\n'&&!(s[end]==','&&end+1<n&&s[end+1]==' '))end++;
  size_t left=pos;while(left<end&&s[left]==' ')left++;
  char value[4096];int r;
  if(end-left>12&&!memcmp(s+left,"Expanded to ",12)){
   size_t a=left+12,b=a;while(b<end&&s[b]>='0'&&s[b]<='9')b++;
   if(b==a||end-b<10||memcmp(s+b," slots (",8)||s[end-1]!=')')return 0;
   char name[2048];r=event_value(d,EV_IDENTITY,s+b+8,end-b-9,name,sizeof name,depth);if(r!=1)return r;
   int written=snprintf(value,sizeof value,"扩展至%.*s个槽位（%s）",(int)(b-a),s+a,name);if(written<0||(size_t)written>=sizeof value)return -1;r=1;
  }else r=event_stat_delta(s+left,end-left,value,sizeof value);
  if(r!=1)return r;if(++count>64)return 0;
  size_t z=strlen(value),indent=left-pos;if(at+indent+z+3>=sizeof staged)return -1;
  memcpy(staged+at,s+pos,indent);at+=indent;memcpy(staged+at,value,z);at+=z;
  if(end<n){if(s[end]=='\n')staged[at++]='\n';else{staged[at++]=',';staged[at++]=' ';end++;}}
  pos=end+1;
 }
 staged[at]=0;return event_copy(staged,at,out,cap);
}
#endif

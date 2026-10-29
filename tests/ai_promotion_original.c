/* Complete original 00434f40 plus its original axis/ftol dependencies. */
#include "../src/bh_actor_route.h"
#include "../src/bh_actor_route_lifetime.h"
#include "../src/bh_route_guided.h"
#include "../src/bh_route_query.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
static void __cdecl oracle_unavailable(void){puts("Excluded route sound dependency executed");exit(3);}
static BHEntityPool *oracle_pool;
static BHSoundQueue *oracle_route_sound;
static unsigned long route_sound_stops;
static unsigned long route_sound_mode,route_sound_stops_by_mode[4];
static int __cdecl oracle_sound_stop_original(unsigned long handle){++route_sound_stops;++route_sound_stops_by_mode[route_sound_mode];return bh_sound_stop(oracle_route_sound,handle);}
#ifdef BH_DUE_CLOCK_CHECK
static unsigned char *oracle_due_image;
static unsigned long due_clock_step,due_clock_calls[2];
int __cdecl oracle_due_advance_source(BHActorRouteContext *c,BHEntity *actor){
 int result=bh_actor_route_advance(c,actor);
 if(result){*c->live_tick+=due_clock_step;++due_clock_calls[0];}
 return result;
}
static void __cdecl oracle_due_advance_original(BHEntity *actor){
 ((void(__cdecl *)(BHEntity*))(oracle_due_image+16384))(actor);
 *(unsigned long*)(oracle_due_image+12288)+=due_clock_step;++due_clock_calls[1];
}
#endif
static void __cdecl oracle_release(BHEntity *e){if(!bh_entity_release(oracle_pool,e)){puts("Real route pool release failed");exit(4);}}
#include "../build/ai-promotion-match/original-promotion.h"
static unsigned long rng=0x434f40UL;
static float sample(void){rng=rng*1664525UL+1013904223UL;return(float)((long)(rng>>8)-8388608)/65536;}
static void word(void *p,unsigned long o,unsigned long v){memcpy((char*)p+o,&v,4);}
static unsigned long value(void *p,unsigned long o){unsigned long v;memcpy(&v,(char*)p+o,4);return v;}
static unsigned long canonical(unsigned long v,BHEntity **source,BHEntity **ref,BHWorldGeometry *worlds,unsigned char raw[3][0x340]){unsigned long i;for(i=0;i<3;++i){if(v==(unsigned long)ref[i])return(unsigned long)source[i];if(v==(unsigned long)raw[i])return(unsigned long)(worlds+i);}return v;}
int main(void){
 BHMathEnvironment env;BHEntityPool source_pool,reference_pool;BHEntity *a[3],*b[3],ref;
 BHSoundQueue sounds[2];BHSoundResource *resources[2];unsigned long handles[2],sound_bad=0,live_sound_cases=0;
 unsigned long live_sound_cases_by_mode[4]={0,0,0,0};
 BHWorldGeometry world[3];BHGeometryModel model;BHPickupRoom room[3];BHActorRouteContext c;BHActorRouteState s;
 unsigned char raw[3][0x340],*code;void(__cdecl *original)(BHEntity*,long,long);void(__cdecl *original_advance)(BHEntity*);
 void(__cdecl *original_due)(BHEntity*);void(__cdecl *original_release)(BHEntity*);
 int(__cdecl *original_guided)(BHEntity*,void*,int);void(__cdecl *original_center)(void*,int,float*);
 BHVec3 source_center,reference_center;long selected;unsigned long guided_bad=0,guided_cases=0,center_bad=0,center_cases=0,count;
 static const unsigned long directions[]={0,1,2,4,5,6};
 static const float widths[]={0,10,20,29.999998f,30,30.000002f,40,60};
 static const unsigned short clocks[][3]={{100,90,11},{100,90,10},{100,90,9},{3,65530,5},{65530,65535,10},{0,0,32768},{0,0,32767},{32767,0,0},{32768,0,0},{65535,0,0},{0,65535,0}};
 unsigned long mode,i,j,k,body_bad[4]={0,0,0,0},list_bad=0,errors=0,pool_bad=0;long incoming=0,outgoing;
 unsigned long advance_counts[6]={0,0,0,0,0,0},advance_repeated=0,advance_fallback=0;
 unsigned long release_shapes[4]={0,0,0,0};
#ifdef BH_DUE_CLOCK_CHECK
 unsigned long live_clock=0,clock_bad=0;
#endif
 memset(&c,0,sizeof(c));memset(&model,0,sizeof(model));memset(world,0,sizeof(world));memset(room,0,sizeof(room));memset(raw,0,sizeof(raw));model.portal_count=2;
 if(!bh_math_environment_enter(&env)||!bh_entities_init(&source_pool,4,0)||!bh_entities_init(&reference_pool,4,0))return 2;
 for(i=0;i<2;++i){if(!bh_sound_queue_init(sounds+i,0,0,0))return 2;resources[i]=bh_sound_resource_allocate(sounds+i);if(!resources[i])return 2;resources[i]->streaming=1;resources[i]->base_frequency=44100;}
 c.sound=sounds;oracle_route_sound=sounds+1;
 for(i=0;i<3;++i){a[i]=bh_entity_acquire(&source_pool);b[i]=bh_entity_acquire(&reference_pool);if(!a[i]||!b[i])return 2;room[i].world=world+i;world[i].model=&model;world[i].navigation.count=2;world[i].minimum.x=-100;world[i].minimum.y=-200;world[i].minimum.z=-300;world[i].maximum.x=100;world[i].maximum.y=200;world[i].maximum.z=300;memcpy(raw[i]+0x1b0,&world[i].minimum,12);memcpy(raw[i]+0x1bc,&world[i].maximum,12);}
 c.rooms=room;c.room_count=3;c.actor_limit=16;c.tick=0xbeef;
 code=VirtualAlloc(0,32768,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!code)return 2;memset(code,0xcc,32768);install_original_promotion(code);memcpy(code+12288,&c.tick,2);memset(code+24576,0,0x174);word(code,24576+0x18,2);FlushInstructionCache(GetCurrentProcess(),code,32768);original=(void(__cdecl *)(BHEntity*,long,long))code;original_advance=(void(__cdecl *)(BHEntity*))(code+16384);
 original_guided=(int(__cdecl *)(BHEntity*,void*,int))(code+17408);original_center=(void(__cdecl *)(void*,int,float*))(code+19456);
 original_due=(void(__cdecl *)(BHEntity*))(code+22528);original_release=(void(__cdecl *)(BHEntity*))(code+23040);oracle_pool=&reference_pool;
#ifdef BH_DUE_CLOCK_CHECK
 oracle_due_image=code;
#endif
 for(mode=0;mode<4;++mode)for(i=0;i<8192;++i){
  route_sound_mode=mode;
  memset(&s,0,sizeof(s));s.current=world;s.next=world+1;s.speed=(sample()+129)/8;s.speed_scale=(sample()+129)/128;s.u=(sample()+128)/256;s.v=(sample()+128)/256;s.jitter=(sample()+128)/256;s.sound=BH_SOUND_INVALID;s.sound_frames=i%3;s.random=rng;
  handles[0]=handles[1]=BH_SOUND_INVALID;
  if((mode==1||mode==2||mode==3)&&i%5==0){for(j=0;j<2;++j)handles[j]=bh_sound_request(sounds+j,resources[j]->index,1,1,0,0);if(handles[0]==BH_SOUND_INVALID||handles[0]!=handles[1])return 2;s.sound=handles[0];++live_sound_cases;++live_sound_cases_by_mode[mode];}
  for(j=0;j<12;++j)((float*)s.controls)[j]=sample();
  if(mode==2){c.tick=clocks[i%11][0];s.tick=clocks[i%11][1];s.duration_ticks=clocks[i%11][2];word(code,12288,c.tick);}
#ifdef BH_DUE_CLOCK_CHECK
  c.live_tick=mode==2?&live_clock:NULL;
  if(mode==2){live_clock=0xa5a50000UL|c.tick;c.tick^=0x1234;due_clock_step=i%3==0?1UL:i%3==1?0xffff0001UL:0x10003UL;word(code,12288,live_clock);}
#endif
  for(j=0;j<3;++j){memset(a[j]->payload,0xa5,sizeof(a[j]->payload));memset(b[j]->payload,0xa5,sizeof(b[j]->payload));word(a[j],0xf4,0);word(a[j],0xf8,0);word(b[j],0xf4,0);word(b[j],0xf8,0);room[j].actors=0;word(raw[j],0x128,0);}
  outgoing=i%3?1:0;
  count=(mode==1||mode==2)?i%6+1:2;model.portal_count=count;world[1].navigation.count=count;word(code,24576+0x18,count);
  if(mode==1){++advance_counts[count-1];if(count>2&&i%8!=7)++advance_repeated;if(i%8==7)++advance_fallback;}
  world[1].query_flags=i%4<2?8:0;word(raw[1],0x114,world[1].query_flags);
  for(j=0;j<count;++j){BHPortal *p=world[1].navigation.portals+j;unsigned long neighbor=(mode==1||mode==2)?(i%8==7?2:(j&1?2:0)):(j?2:0);memset(p,0,sizeof(*p));p->direction=j?(i&1?(directions[(i/3)%6]^4):directions[(i/17)%6]):directions[(i/3)%6];p->plane=(long)sample();p->scratch[0]=sample();p->scratch[1]=p->scratch[0]+widths[(i+j)%8];p->scratch[2]=sample();p->scratch[3]=p->scratch[2]+widths[(i/8+j)%8];p->neighbor=(BHPortalNode*)(world+neighbor);memcpy(raw[1]+0x1cc+j*60,p,60);word(raw[1],0x204+j*60,(unsigned long)raw[neighbor]);}
  bh_actor_route_write(a[0],&s);bh_actor_route_write(b[0],&s);word(a[0],0xc4,(unsigned long)world);word(b[0],0xc4,(unsigned long)raw[0]);word(b[0],0x1a4,(unsigned long)raw[0]);word(b[0],0x1a8,(unsigned long)raw[1]);
  word(a[0],0xfc,i%4==0?6:i%4==1?2:i%4==2?1:0);word(b[0],0xfc,value(a[0],0xfc));
  world[0].query_flags=i%5==0?0x10:i%5==1?0x20:0;world[2].query_flags=i%7==0?0x10:i%7==1?0x20:0;word(raw[0],0x114,world[0].query_flags);word(raw[2],0x114,world[2].query_flags);word(raw[1],0xf8,0);
  if(mode==3&&i%4==3){room[0].actors=a[1];word(a[1],0xf4,(unsigned long)a[0]);word(a[0],0xf8,(unsigned long)a[1]);word(a[0],0xf4,(unsigned long)a[2]);word(a[2],0xf8,(unsigned long)a[0]);word(raw[0],0x128,(unsigned long)b[1]);word(b[1],0xf4,(unsigned long)b[0]);word(b[0],0xf8,(unsigned long)b[1]);word(b[0],0xf4,(unsigned long)b[2]);word(b[2],0xf8,(unsigned long)b[0]);}
  else if(i%3==0){room[0].actors=a[1];word(a[1],0xf4,(unsigned long)a[0]);word(a[0],0xf8,(unsigned long)a[1]);word(raw[0],0x128,(unsigned long)b[1]);word(b[1],0xf4,(unsigned long)b[0]);word(b[0],0xf8,(unsigned long)b[1]);}
  else{room[0].actors=a[0];word(raw[0],0x128,(unsigned long)b[0]);if(i%3==1){word(a[0],0xf4,(unsigned long)a[1]);word(a[1],0xf8,(unsigned long)a[0]);word(b[0],0xf4,(unsigned long)b[1]);word(b[1],0xf8,(unsigned long)b[0]);}}
  if(mode==3)++release_shapes[i%4==3?3:i%3];
  if((i&1)&&!(mode==3&&i%4==3)){room[1].actors=a[2];word(raw[1],0x128,(unsigned long)b[2]);}
  if(mode==3)original_release(b[0]);else if(mode==2)original_due(b[0]);else if(mode==1)original_advance(b[0]);else original(b[0],incoming,outgoing);
  if(!(mode==3?bh_actor_route_release(&c,&source_pool,a[0]):mode==2?bh_actor_route_due(&c,a[0]):mode==1?bh_actor_route_advance(&c,a[0]):bh_actor_route_promote(&c,a[0],incoming,outgoing))){++errors;continue;}
#ifdef BH_DUE_CLOCK_CHECK
  if(mode==2&&live_clock!=value(code,12288))++clock_bad;
#endif
  ref=*b[0];for(j=0;j<5;++j){static const unsigned long offsets[]={0xc4,0xf4,0xf8,0x1a4,0x1a8};word(&ref,offsets[j],canonical(value(&ref,offsets[j]),a,b,world,raw));}
  if(memcmp((char*)a[0]+0xe4,(char*)&ref+0xe4,0x108)||value(a[0],0xc4)!=value(&ref,0xc4)){
   if(body_bad[mode]<4){for(k=0xe4;k<0x1ec;++k)if(((unsigned char*)a[0])[k]!=((unsigned char*)&ref)[k])break;printf("route mode=%lu case=%lu first_offset=%lx\n",mode,i,k);}++body_bad[mode];
  }
  for(j=0;j<3;++j){if((unsigned long)room[j].actors!=canonical(value(raw[j],0x128),a,b,world,raw))++list_bad;if(j&&((value(a[j],0xf4)!=canonical(value(b[j],0xf4),a,b,world,raw))||(value(a[j],0xf8)!=canonical(value(b[j],0xf8),a,b,world,raw))))++list_bad;}
  if(handles[0]!=BH_SOUND_INVALID){
   unsigned long live[2];for(j=0;j<2;++j)live[j]=bh_sound_voice(sounds+j,handles[j])!=0;
   if(live[0]!=live[1]||sounds[0].voice_count!=sounds[1].voice_count||sounds[0].error!=sounds[1].error)++sound_bad;
   for(j=0;j<2;++j)if(live[j]&&!bh_sound_stop(sounds+j,handles[j]))return 2;
  }
  if(mode==3){if(source_pool.used!=2||reference_pool.used!=2||source_pool.free_slots!=a[0]||reference_pool.free_slots!=b[0]||a[0]->generation!=b[0]->generation)++pool_bad;a[0]=bh_entity_acquire(&source_pool);b[0]=bh_entity_acquire(&reference_pool);if(!a[0]||!b[0])return 2;}
 }
 for(i=0;i<8192;++i){
  count=i%6+1;model.portal_count=count;world[1].navigation.count=count;word(code,24576+0x18,count);incoming=i%count;
  word(a[0],0xfc,i%5==0?0:i%5==1?1:i%5==2?6:i%5==3?2:0xffffffffUL);
  world[0].query_flags=i%3==0?0x10:i%3==1?0x20:0;world[2].query_flags=i%7==0?0x10:i%7==1?0x20:0;word(raw[0],0x114,world[0].query_flags);word(raw[2],0x114,world[2].query_flags);
  for(j=0;j<count;++j){
   BHPortal *p=world[1].navigation.portals+j;memset(p,0,sizeof(*p));p->direction=j==incoming?directions[(i/6)%6]:(i&1?directions[(i/6)%6]^4:directions[(i+j)%6]);p->plane=(long)sample();
   p->scratch[0]=sample();p->scratch[1]=p->scratch[0]+widths[(i+j)%8];p->scratch[2]=sample();p->scratch[3]=p->scratch[2]+widths[(i/8+j)%8];
   p->neighbor=(j!=incoming&&(i+j)%5==0)?0:(BHPortalNode*)(world+(j&1?2:0));memcpy(raw[1]+0x1cc+j*60,p,60);word(raw[1],0x204+j*60,p->neighbor?(unsigned long)raw[j&1?2:0]:0);
   original_center(raw[1]+0xe8,j,(float*)&reference_center);if(!bh_portal_center(p,&source_center)||memcmp(&source_center,&reference_center,12))++center_bad;++center_cases;
  }
  selected=original_guided(a[0],raw[1],incoming);{long source_selected;if(!bh_route_guided_exit(a[0],world+1,incoming,&source_selected)||selected!=source_selected)++guided_bad;}++guided_cases;
 }
 /* Extreme guided-score inputs distinguish a retained Z displacement from
    the original binary32 reload before multiplying by the up-vector zero. */
 {
  static const unsigned long bits[]={0,0x80000000UL,1,0x80000001UL,0x007fffffUL,0x00800000UL,0x3f800000UL,0xbf800000UL,0x4b800001UL,0x7f7fffffUL,0xff7fffffUL,0x7f800000UL,0xff800000UL,0x7fc12345UL,0xffc54321UL,0x7f812345UL};
  unsigned long x,y,d,lane,edge_cases=0,edge_bad=0;
  model.portal_count=3;world[1].navigation.count=3;word(code,24576+0x18,3);
  world[0].query_flags=world[2].query_flags=0;word(raw[0],0x114,0);word(raw[2],0x114,0);
  word(a[0],0xfc,2);
  for(lane=0;lane<3;++lane)for(x=0;x<16;++x)for(y=0;y<16;++y)for(d=0;d<6;++d){
   for(j=0;j<3;++j){
    BHPortal *p=world[1].navigation.portals+j;
    unsigned long u=j==0?x:j==1?y:6,v=j==0?y:j==1?x:0;
    if(lane==1)u=0;
    if(lane==2)v=0;
    memset(p,0,sizeof(*p));p->direction=directions[d]^(j?4UL:0);p->plane=j?1:0;
    memcpy(p->scratch,bits+u,4);memcpy(p->scratch+1,bits+u,4);
    memcpy(p->scratch+2,bits+v,4);memcpy(p->scratch+3,bits+v,4);
    p->neighbor=(BHPortalNode*)(world+(j&1?2:0));
    memcpy(raw[1]+0x1cc+j*60,p,sizeof(*p));word(raw[1],0x204+j*60,(unsigned long)raw[j&1?2:0]);
   }
   selected=original_guided(a[0],raw[1],0);
   {long actual;if(!bh_route_guided_exit(a[0],world+1,0,&actual)||actual!=selected){
    if(edge_bad<4)printf("guided IEEE x=%lu y=%lu d=%lu expected=%ld actual=%ld\n",x,y,d,selected,actual);
    ++edge_bad;
   }}
   ++edge_cases;++guided_cases;
  }
  printf("{\"guided_ieee_cases\":%lu,\"guided_ieee_differences\":%lu,\"precision\":\"PC53\"}\n",edge_cases,edge_bad);
  guided_bad+=edge_bad;
 }
 /* The real cross-product dependency receives only up and one of six exact
    axis normals here. Do not transfer a different variant's byte-match proof. */
 {
  BHVec3 up={0,1,0},normal,side,expected;
  void(__cdecl *cross)(const BHVec3*,const BHVec3*,BHVec3*)=(void(__cdecl *)(const BHVec3*,const BHVec3*,BHVec3*))(code+21504);
  unsigned long d,bad=0;
  for(d=0;d<6;++d){
   BHPortal p;memset(&p,0,sizeof(p));p.direction=directions[d];
   if(!bh_portal_normal(&p,&normal))return 3;
   cross(&up,&normal,&expected);bh_vec3_cross(&up,&normal,&side);
   if(memcmp(&side,&expected,sizeof(side)))++bad;
  }
  printf("{\"guided_cross_axis_cases\":6,\"guided_cross_axis_differences\":%lu}\n",bad);
  guided_bad+=bad;
 }
 /* Independent full-body comparison for the newly admitted center variant:
    rounding boundaries, integer planes and IEEE payloads at all x87 profiles.
    This does not establish the game's runtime precision transitions. */
 {
  static const unsigned long bits[]={0,0x80000000UL,1,0x80000001UL,0x007fffffUL,0x00800000UL,0x3f800000UL,0xbf800000UL,0x4b800001UL,0x7f7fffffUL,0xff7fffffUL,0x7f800000UL,0xff800000UL,0x7fc12345UL,0xffc54321UL,0x7f812345UL};
  static const unsigned int precision[]={_PC_24,_PC_53,_PC_64};
  static const long planes[]={16777217L,2147483647L,(-2147483647L-1)};
  unsigned long profile,x,y,d,plane,edge_cases=0,edge_bad=0;
  BHPortal *p=world[1].navigation.portals;
  for(profile=0;profile<3;++profile){
   _controlfp(precision[profile],_MCW_PC);
   for(x=0;x<16;++x)for(y=0;y<16;++y)for(d=0;d<6;++d)for(plane=0;plane<3;++plane){
    memset(p,0,sizeof(*p));p->direction=directions[d];p->plane=planes[plane];
    memcpy(p->scratch,bits+x,4);memcpy(p->scratch+1,bits+y,4);
    memcpy(p->scratch+2,bits+15-x,4);memcpy(p->scratch+3,bits+15-y,4);
    memcpy(raw[1]+0x1cc,p,sizeof(*p));
    original_center(raw[1]+0xe8,0,(float*)&reference_center);
    if(!bh_portal_center(p,&source_center)||memcmp(&source_center,&reference_center,12))++edge_bad;
    ++edge_cases;
   }
  }
  _controlfp(_PC_53,_MCW_PC);
  printf("{\"center_ieee_cases\":%lu,\"center_ieee_differences\":%lu,\"precision\":\"PC24/53/64 controlled; runtime transitions unverified\"}\n",edge_cases,edge_bad);
  center_bad+=edge_bad;
 }
 printf("{\"cases_each\":8192,\"promotion_differences\":%lu,\"advance_differences\":%lu,\"due_differences\":%lu,\"release_differences\":%lu,\"list_differences\":%lu,\"pool_differences\":%lu,\"source_errors\":%lu,\"precision\":\"PC53\"}\n",body_bad[0],body_bad[1],body_bad[2],body_bad[3],list_bad,pool_bad,errors);
 printf("{\"guided_cases\":%lu,\"guided_differences\":%lu,\"center_cases\":%lu,\"center_differences\":%lu}\n",guided_cases,guided_bad,center_cases,center_bad);
 printf("{\"route_live_sound_cases\":%lu,\"original_sound_stops\":%lu,\"sound_differences\":%lu,\"native_audio_interfaces\":0}\n",live_sound_cases,route_sound_stops,sound_bad);
 printf("{\"advance_portal_counts\":[%lu,%lu,%lu,%lu,%lu,%lu],\"advance_repeated_neighbors\":%lu,\"advance_no_matching_incoming\":%lu}\n",advance_counts[0],advance_counts[1],advance_counts[2],advance_counts[3],advance_counts[4],advance_counts[5],advance_repeated,advance_fallback);
 printf("{\"release_shapes_tail_head_single_middle\":[%lu,%lu,%lu,%lu]}\n",release_shapes[0],release_shapes[1],release_shapes[2],release_shapes[3]);
 printf("{\"live_sounds_promotion_advance_due_release\":[%lu,%lu,%lu,%lu],\"original_stops_promotion_advance_due_release\":[%lu,%lu,%lu,%lu]}\n",live_sound_cases_by_mode[0],live_sound_cases_by_mode[1],live_sound_cases_by_mode[2],live_sound_cases_by_mode[3],route_sound_stops_by_mode[0],route_sound_stops_by_mode[1],route_sound_stops_by_mode[2],route_sound_stops_by_mode[3]);
#ifdef BH_DUE_CLOCK_CHECK
 printf("{\"live_clock_due_cases\":8192,\"source_clock_mutations\":%lu,\"original_clock_mutations\":%lu,\"clock_differences\":%lu}\n",due_clock_calls[0],due_clock_calls[1],clock_bad);
 errors+=clock_bad+(due_clock_calls[0]!=due_clock_calls[1]);
#endif
 VirtualFree(code,0,MEM_RELEASE);bh_entities_dispose(&source_pool);bh_entities_dispose(&reference_pool);for(i=0;i<2;++i)if(!bh_sound_queue_dispose(sounds+i))return 2;bh_math_environment_leave(&env);return body_bad[0]||body_bad[1]||body_bad[2]||body_bad[3]||list_bad||pool_bad||errors||guided_bad||center_bad||sound_bad?1:0;
}

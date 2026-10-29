/* Original response/damage owners, real dependencies, no game/renderer/device play. */
#include "actor_response.c"
#include "../src/bh_camera_curve.h"
#include "../src/bh_remote_prediction.h"
#include <windows.h>
typedef struct TailRow {unsigned long event,w[16];BHVec3 v[5];}TailRow;
typedef struct TailTrace {TailRow rows[64];unsigned long count;}TailTrace;
static ResponseFixture fixtures[2];static BHEntity *publisher[2],*watchers[2][2];
static BHPickupRoom tail_rooms[2][3];static BHWorldGeometry tail_world;
static unsigned char raw_world[3][0x340],*tail_code;
#ifdef DAMAGE_WHOLE_ORIGINAL
static BHWorldGeometry ordinary_worlds[2][3];static BHGeometryModel ordinary_models[2];
static unsigned long ordinary_advances,ordinary_promotions[2];
static int world_index(unsigned long side,BHWorldGeometry *world){unsigned long i;for(i=0;i<3;++i)if(world==ordinary_worlds[side]+i||(side&&world==(BHWorldGeometry*)raw_world[i]))return i;puts("Unbound ordinary world");exit(3);return -1;}
static void ordinary_route(unsigned long side,unsigned long iteration,int due){
 ResponseFixture *f=fixtures+side;BHEntity *e=publisher[side];BHPortal *p;unsigned long i;
 memset(ordinary_worlds[side],0,sizeof(ordinary_worlds[side]));memset(ordinary_models+side,0,sizeof(ordinary_models[side]));ordinary_models[side].portal_count=2;
 for(i=0;i<3;++i){BHWorldGeometry *w=ordinary_worlds[side]+i;w->model=ordinary_models+side;w->navigation.count=2;w->minimum.x=w->minimum.y=w->minimum.z=-20;w->maximum.x=w->maximum.y=w->maximum.z=20;identity(&w->transform);tail_rooms[side][i].world=side?(BHWorldGeometry*)raw_world[i]:w;if(i)tail_rooms[side][i].actors=NULL;}
 ordinary_worlds[side][1].minimum.x=0;ordinary_worlds[side][1].maximum.x=100;
 for(i=0;i<2;++i){p=ordinary_worlds[side][1].navigation.portals+i;p->direction=i?0:4;p->plane=i?100:0;p->scratch[0]=p->scratch[2]=-20;p->scratch[1]=p->scratch[3]=20;p->neighbor=&ordinary_worlds[side][i?2:0].navigation;}
 f->route.rooms=tail_rooms[side];f->route.room_count=3;f->route.tick=(unsigned short)(iteration*1021+32760);
 setword(e,0x1a4,(unsigned long)tail_rooms[side][0].world);setword(e,0x1a8,(unsigned long)tail_rooms[side][1].world);setword(e,0xc4,(unsigned long)tail_rooms[side][0].world);
 *(unsigned short *)((char*)e+0x1e4)=(unsigned short)(f->route.tick-(due?30+iteration%7:iteration%30));
}
#endif
static BHEntityStep source_normal_step;
static TailTrace traces[2];static unsigned long active_side,recording,move_requested,moved[2],dependencies;
static void(__cdecl *original_curve)(float,BHVec3*,const BHVec3*,const BHVec3*,const BHVec3*,const BHVec3*);
static unsigned long role(unsigned long side,BHEntity *e){if(!e)return 0;if(e==publisher[side])return 1;if(e==watchers[side][0])return 2;if(e==watchers[side][1])return 3;if(e==fixtures[side].base.owner)return 4;return 5;}
static TailRow *event(unsigned long id){TailTrace *t=traces+active_side;TailRow *r;if(!recording)return NULL;if(t->count>=64)exit(3);r=t->rows+t->count++;memset(r,0,sizeof(*r));r->event=id;return r;}
static void scalar(TailRow *r,unsigned long n,float f){if(r)memcpy(r->w+n,&f,4);}
static void vector(TailRow *r,unsigned long n,const BHVec3 *v){if(r&&v)r->v[n]=*v;}
static void __cdecl tail_unavailable(void){puts("Unreviewed death dependency");exit(3);}
int __cdecl tail_rand(void){int value=rand();TailRow *r=event(0);if(r)r->w[0]=value;return value;}
int __cdecl tail_set_query(BHEntityPool *p,BHEntity *e,BHEntityCallback q){TailRow *r=event(1);if(r){r->w[0]=role(active_side,e);r->w[1]=q!=NULL;}if(p!=&fixtures[active_side].base.pool)++dependencies;return bh_entity_set_query(p,e,q);}
static int __cdecl oracle_set_query(BHEntity *e,BHEntityCallback q){return tail_set_query(&fixtures[1].base.pool,e,q);}
int __cdecl tail_stop(BHSoundQueue *q,unsigned long h){TailRow *r=event(2);if(r)r->w[0]=h;if(q!=&fixtures[active_side].base.sound)++dependencies;return bh_sound_stop(q,h);}
static int __cdecl oracle_stop(unsigned long h){return tail_stop(&fixtures[1].base.sound,h);}
void __cdecl tail_curve(float t,BHVec3 *out,const BHVec3 *a,const BHVec3 *b,const BHVec3 *c,const BHVec3 *d){TailRow *r=event(3);scalar(r,0,t);vector(r,0,a);vector(r,1,b);vector(r,2,c);vector(r,3,d);if(active_side)original_curve(t,out,a,b,c,d);else bh_camera_curve(t,out,a,b,c,d);vector(r,4,out);}
unsigned long __cdecl tail_debris(BHScriptDebrisContext *c,long count,const BHVec3 *p,float life,float speed,float size){TailRow *r=event(4);unsigned long n;if(r)r->w[0]=count;vector(r,0,p);scalar(r,1,life);scalar(r,2,speed);scalar(r,3,size);if(c!=&fixtures[active_side].base.fragments)++dependencies;n=bh_script_debris_create(c,count,p,life,speed,size);if(r)r->w[4]=n;return n;}
static unsigned long __cdecl oracle_debris(long count,const BHVec3*p,float a,float b,float c){return tail_debris(&fixtures[1].base.fragments,count,p,a,b,c);}
unsigned long __cdecl tail_sound(BHSoundQueue *q,unsigned long id,float gain,float pitch,const BHVec3 *p,const BHVec3 *v){TailRow *r=event(5);if(r)r->w[0]=id;scalar(r,1,gain);scalar(r,2,pitch);vector(r,0,p);vector(r,1,v);if(q!=&fixtures[active_side].base.sound)++dependencies;return bh_sound_request(q,id,gain,pitch,p,v);}
static unsigned long __cdecl oracle_sound(unsigned long id,float a,float b,const BHVec3*p,const BHVec3*v){return tail_sound(&fixtures[1].base.sound,id,a,b,p,v);}
int __cdecl tail_loot(const unsigned long (*table)[32],unsigned long count,long row,long first,long second,long *out){TailRow *r=event(6);int ok;if(r){r->w[0]=row;r->w[1]=first;r->w[2]=second;}ok=bh_remote_choose_slot(table,count,row,first,second,out);if(r){r->w[3]=*out;r->w[4]=ok;}return ok;}
static long __cdecl oracle_loot(long row,long first,long second){long out;if(!tail_loot(rows,4,row,first,second,&out))exit(3);return out;}
BHEntity *__cdecl tail_acquire(BHEntityPool *p){BHEntity *e;TailRow *r=event(7);if(p!=&fixtures[active_side].base.pool)++dependencies;e=bh_entity_acquire(p);if(r)r->w[0]=e!=NULL;return e;}
static BHEntity *__cdecl oracle_acquire(void){BHEntity *e=tail_acquire(&fixtures[1].base.pool);if(e&&!bh_script_lifetime_bind_context(&fixtures[1].mission,e))exit(3);return e;}
BHEntity *__cdecl tail_construct(BHTargetConstructContext *c,const BHVec3 *p,const BHVec3 *angles,const BHVec3 *v,const BHVec3 *spin,unsigned long material,float mass,float strength,BHEntity *owner,long token,long loot,long special){TailRow *r=event(8);BHEntity *e;vector(r,0,p);vector(r,1,angles);vector(r,2,v);vector(r,3,spin);if(r){r->w[0]=material;r->w[1]=role(active_side,owner);r->w[2]=token;r->w[3]=loot;r->w[4]=special;}scalar(r,5,mass);scalar(r,6,strength);if(c!=&fixtures[active_side].base.construct)++dependencies;e=bh_target_construct_moving(c,p,angles,v,spin,material,mass,strength,owner,token,loot,special);if(r)r->w[7]=e!=NULL;return e;}
static BHEntity *__cdecl oracle_construct(const BHVec3*p,const BHVec3*a,const BHVec3*v,const BHVec3*s,unsigned long m,float mass,float strength,BHEntity*o,long token,long loot,long special){return tail_construct(&fixtures[1].base.construct,p,a,v,s,m,mass,strength,o,token,loot,special);}
int __cdecl tail_broadcast(BHEntityPool *p,unsigned long mask,unsigned long value,const BHVec3 *center,float radius,void *packet){TailRow *r=event(9);unsigned long *w=packet;if(r){r->w[0]=w[0];r->w[1]=role(active_side,(BHEntity*)w[1]);if(w[0]==14)r->w[2]=w[2];r->w[3]=mask;r->w[4]=value;}if(p!=&fixtures[active_side].base.pool||center||radius)++dependencies;return bh_entities_query(p,mask,value,center,radius,packet);}
static int __cdecl oracle_broadcast(unsigned long m,unsigned long v,const BHVec3*p,float radius,void *packet){return tail_broadcast(&fixtures[1].base.pool,m,v,p,radius,packet);}
int __cdecl tail_release(BHEntityPool *p,BHEntity *e){TailRow *r=event(10);if(r){r->w[0]=role(active_side,e);r->w[1]=fixtures[active_side].vehicles.count;r->w[2]=role(active_side,fixtures[active_side].vehicles.head);r->w[3]=role(active_side,tail_rooms[active_side][0].actors);r->w[4]=role(active_side,tail_rooms[active_side][1].actors);}return bh_entity_release(p,e);}
static int __cdecl oracle_release(BHEntity *e){fixtures[1].vehicles.head=(BHEntity*)word((BHEntity*)tail_code,8200);fixtures[1].vehicles.count=word((BHEntity*)tail_code,8204);tail_rooms[1][0].actors=(BHEntity*)word((BHEntity*)raw_world[0],0x128);tail_rooms[1][1].actors=(BHEntity*)word((BHEntity*)raw_world[1],0x128);return tail_release(&fixtures[1].base.pool,e);}
#ifdef DAMAGE_WHOLE_ORIGINAL
static void __cdecl oracle_advance(BHEntity *e){
 unsigned long i;int current=world_index(1,(BHWorldGeometry*)word(e,0x1a4)),next=world_index(1,(BHWorldGeometry*)word(e,0x1a8));
 for(i=0;i<3;++i){tail_rooms[1][i].actors=(BHEntity*)word((BHEntity*)raw_world[i],0x128);tail_rooms[1][i].world=ordinary_worlds[1]+i;}
 setword(e,0x1a4,(unsigned long)(ordinary_worlds[1]+current));setword(e,0x1a8,(unsigned long)(ordinary_worlds[1]+next));setword(e,0xc4,(unsigned long)(ordinary_worlds[1]+current));
 if(!bh_actor_route_advance(&fixtures[1].route,e)){printf("Original ordinary advance adapter failed error=%d current=%d next=%d speed=%g scale=%g count=%lu model=%lu h=%p e=%p prev=%p next=%p n0=%p n1=%p worlds=%p/%p/%p\n",fixtures[1].route.error,current,next,field(e,0x188),field(e,0x198),ordinary_worlds[1][1].navigation.count,ordinary_worlds[1][1].model->portal_count,tail_rooms[1][0].actors,e,(void*)word(e,0xf8),(void*)word(e,0xf4),ordinary_worlds[1][1].navigation.portals[0].neighbor,ordinary_worlds[1][1].navigation.portals[1].neighbor,ordinary_worlds[1],ordinary_worlds[1]+1,ordinary_worlds[1]+2);exit(3);}++ordinary_advances;
 current=world_index(1,(BHWorldGeometry*)word(e,0x1a4));next=world_index(1,(BHWorldGeometry*)word(e,0x1a8));
 for(i=0;i<3;++i){setword((BHEntity*)raw_world[i],0x128,(unsigned long)tail_rooms[1][i].actors);tail_rooms[1][i].world=(BHWorldGeometry*)raw_world[i];}
 setword(e,0x1a4,(unsigned long)raw_world[current]);setword(e,0x1a8,(unsigned long)raw_world[next]);setword(e,0xc4,(unsigned long)raw_world[current]);
}
#endif
#ifdef RESPONSE_WHOLE_ORIGINAL
static void __cdecl oracle_attack(BHEntity *e){if(!bh_actor_attack(&fixtures[1].attack,e)){puts("Original attack adapter failed");exit(3);}}
static void __cdecl oracle_pursuit(BHEntity *e){if(!bh_actor_pursuit_route(&fixtures[1].pursuit,e)){puts("Original pursuit adapter failed");exit(3);}}
static BHEntity *__cdecl oracle_particle(unsigned long id,unsigned long bucket,const BHVec3*p,const BHVec3*v,const BHVec3*a,float size,float angle,float rate,float spin,float decay,float angular_decay,float life,float alpha){return bh_script_particle_create(&fixtures[1].base.particles,id,bucket,p,v,a,size,angle,rate,spin,decay,angular_decay,life,alpha);}
#ifdef DAMAGE_WHOLE_ORIGINAL
#include "../build/ai-damage-tick-match/original-finish.h"
#else
#include "../build/ai-response-whole-match/original-finish.h"
#endif
#else
#include "../build/ai-response-finish-match/original-finish.h"
#endif
static void relocate(unsigned long side){BHEntity *e=publisher[side],*p=(BHEntity*)word(e,0xf8),*n=(BHEntity*)word(e,0xf4);if(p)setword(p,0xf4,(unsigned long)n);else tail_rooms[side][0].actors=n;if(n)setword(n,0xf8,(unsigned long)p);setword(e,0xf8,0);setword(e,0xf4,0);tail_rooms[side][1].actors=e;setword(e,0x1a4,(unsigned long)tail_rooms[side][1].world);if(side){setword((BHEntity*)raw_world[0],0x128,(unsigned long)tail_rooms[side][0].actors);setword((BHEntity*)raw_world[1],0x128,(unsigned long)e);}moved[side]=1;}
static void __cdecl observer(BHEntity *e,void *packet){unsigned long *w=packet;TailRow *r=event(11);if(r){r->w[0]=role(active_side,e);r->w[1]=w[0];r->w[2]=role(active_side,(BHEntity*)w[1]);r->w[3]=fixtures[active_side].vehicles.count;r->w[4]=bh_entity_is_active(&fixtures[active_side].base.pool,publisher[active_side]);}bh_actor_response_query(e,packet);if(w[0]==17&&move_requested&&!moved[active_side])relocate(active_side);}
static void chain(unsigned long side,unsigned long shift,int global){BHEntity *a[3];unsigned long i,prev=global?0xec:0xf8,next=global?0xf0:0xf4;a[0]=publisher[side];a[1]=watchers[side][0];a[2]=watchers[side][1];for(i=0;i<3;++i){BHEntity *e=a[(i+shift)%3];setword(e,prev,i?(unsigned long)a[(i+shift+2)%3]:0);setword(e,next,i<2?(unsigned long)a[(i+shift+1)%3]:0);}if(global)fixtures[side].vehicles.head=a[shift];else tail_rooms[side][0].actors=a[shift];}
static int fixture_error(ResponseFixture *f){
#ifdef DAMAGE_WHOLE_ORIGINAL
 return f->damage.error;
#else
 return f->response.error;
#endif
}
static int expected_death(ResponseFixture *f){
#ifdef DAMAGE_WHOLE_ORIGINAL
 return f->damage.mode!=3;
#else
 (void)f;return 1;
#endif
}
int main(void){BHMathEnvironment environment;BHVec3 position={2,3,4};void(__cdecl *original)(BHEntity*,const BHVec3*,const BHVec3*);unsigned long i,j,side,trace_bad=0,state_bad=0,errors=0,next_rng[2],cues[2],used[2],cases=0,relocations=0,healthy_cases=0,healthy_bad=0,live_sounds=0,constructed=0,whole_live_cases=0;BHEntity *e;BHEntityStep expected_step;void(__cdecl *original_healthy)(BHEntity*);
 if(!bh_math_environment_enter(&environment)||!model_file())return 2;
#ifdef DAMAGE_WHOLE_ORIGINAL
 active_side=0;recording=0;if(!response_fixture(fixtures))return 2;e=actor(fixtures,2);if(!bh_actor_damage_bind(&fixtures[0].damage,e))return 2;bh_actor_damage_tick(e);source_normal_step=e->step;if(!source_normal_step)return 2;finish(&fixtures[0].base);
#endif
 tail_code=VirtualAlloc(0,16384,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(!tail_code)return 2;install_original_finish(tail_code);FlushInstructionCache(GetCurrentProcess(),tail_code,16384);original=(void(__cdecl *)(BHEntity*,const BHVec3*,const BHVec3*))tail_code;original_curve=(void(__cdecl *)(float,BHVec3*,const BHVec3*,const BHVec3*,const BHVec3*,const BHVec3*))(tail_code+4096);
 #ifdef RESPONSE_WHOLE_ORIGINAL
 original_healthy=(void(__cdecl *)(BHEntity*))(tail_code+3072);
#else
 original_healthy=(void(__cdecl *)(BHEntity*))(tail_code+2048);
#endif
 for(i=0;i<256;++i){recording=0;memset(traces,0,sizeof(traces));move_requested=i%7==0;moved[0]=moved[1]=0;memset(rows,0,sizeof(rows));for(j=0;j<4;++j){rows[j][3]=rows[j][10]=rows[j][12]=1;rows[j][31]=3;}
  for(side=0;side<2;++side){ResponseFixture *f=fixtures+side;active_side=side;if(!response_fixture(f))return 2;publisher[side]=actor(f,i%3==0?6:i%3==1?1:2);watchers[side][0]=actor(f,2);watchers[side][1]=actor(f,2);
#ifdef DAMAGE_WHOLE_ORIGINAL
   if(!bh_actor_damage_bind(&f->damage,publisher[side]))return 2;
   setword(publisher[side],0x1e8,i*2654435761UL^0xa5a55a5aUL);publisher[side]->step=bh_actor_damage_tick;f->damage.mode=i%5==0?3:0;
#endif
   tail_rooms[side][0]=f->base.room;tail_rooms[side][1]=f->base.room;tail_rooms[side][1].actors=0;tail_rooms[side][0].world=side?(BHWorldGeometry*)raw_world[0]:&f->base.world;tail_rooms[side][1].world=side?(BHWorldGeometry*)raw_world[1]:&tail_world;f->route.rooms=tail_rooms[side];f->route.room_count=2;
   for(j=0;j<3;++j){e=j?watchers[side][j-1]:publisher[side];setword(e,0x1a4,(unsigned long)tail_rooms[side][0].world);setword(e,0x1a8,(unsigned long)tail_rooms[side][0].world);if(j)bh_entity_set_query(&f->base.pool,e,observer);}
   e=publisher[side];
   for(j=0;j<3;++j){real(e,0x15c+4*j,i%4==0?100:(float)((long)(i%19)-9)*(j+1)/10);real(e,0x168+4*j,(float)((long)(i%11)-5)*(j+1));}
   real(e,0x194,1.5f);real(e,0x180,.2f+(i%13)*.03125f);setword(e,0x11c,i&1?123:0);setword(e,0x1dc,(i%4)|((i/4%4)<<16));setword(e,0x1ac,(unsigned long)f->base.owner);f->angles.x=.1f;f->angles.y=.2f;f->angles.z=.3f;
   chain(side,i%3,0);chain(side,i/3%3,1);f->response.error=0;
#ifdef DAMAGE_WHOLE_ORIGINAL
   ordinary_route(side,i,i%4==0);
#endif
   if(i&2){unsigned long voice=bh_sound_request(&f->base.sound,0,1,1,NULL,NULL);if(voice==BH_SOUND_INVALID)return 2;setword(e,0x104,voice);}
  }
  setword((BHEntity*)raw_world[0],0x128,(unsigned long)tail_rooms[1][0].actors);setword((BHEntity*)raw_world[1],0x128,0);setword((BHEntity*)tail_code,8192,fixtures[1].response.mission_resource);setword((BHEntity*)tail_code,8196,fixtures[1].damage.death_sound);setword((BHEntity*)tail_code,8200,(unsigned long)fixtures[1].vehicles.head);setword((BHEntity*)tail_code,8204,3);setword((BHEntity*)tail_code,8208,fixtures[1].route.tick);setword((BHEntity*)tail_code,8212,fixtures[1].damage.smoke_resource);setword((BHEntity*)tail_code,8216,fixtures[1].damage.mode);
  for(side=0;side<2;++side){ResponseFixture *f=fixtures+side;active_side=side;srand(i*17+3);recording=1;if(side)original(publisher[side],&position,&f->angles);else {
#ifdef RESPONSE_WHOLE_ORIGINAL
bh_actor_response_tick(publisher[side]);
#else
bh_actor_response_finish(publisher[side],&f->response,&position,1);
#endif
}recording=0;next_rng[side]=rand();used[side]=f->base.pool.used;cues[side]=0;for(e=f->base.pool.active;e;e=e->next)if(e->step==bh_mission_sound_tick){++cues[side];if(field(e,0x180)!=1.5f||word(e,0x104)!=f->response.mission_resource)++errors;}
#ifdef DAMAGE_WHOLE_ORIGINAL
if(world_index(side,(BHWorldGeometry*)word(publisher[side],0x1a4))==1){++ordinary_promotions[side];if(*(unsigned short *)((char*)publisher[side]+0x1e4)!=(unsigned short)(fixtures[side].route.tick+i%7))++errors;}
#endif
if(fixture_error(f)||bh_entity_is_active(&f->base.pool,publisher[side])==expected_death(f)||(expected_death(f)&&(!f->base.slots[0].body.entity||f->base.construct.error))||!bh_entities_valid(&f->base.pool))++errors;}
  ++cases;if(fixtures[0].base.slots[0].body.entity)++constructed;if(memcmp(&fixtures[0].base.slots[0].state,&fixtures[1].base.slots[0].state,sizeof(BHPhysicsState)))++state_bad;if(i&2)++live_sounds;if(memcmp(traces,traces+1,sizeof(traces[0])))++trace_bad;if(word(publisher[0],0x1e8)!=word(publisher[1],0x1e8)||memcmp((char*)publisher[0]+0x120,(char*)publisher[1]+0x120,0x84)||next_rng[0]!=next_rng[1]||used[0]!=used[1]||cues[0]!=cues[1]||fixtures[0].vehicles.count!=fixtures[1].vehicles.count||role(0,fixtures[0].vehicles.head)!=role(1,fixtures[1].vehicles.head)||role(0,tail_rooms[0][0].actors)!=role(1,tail_rooms[1][0].actors)||role(0,tail_rooms[0][1].actors)!=role(1,tail_rooms[1][1].actors))++state_bad;relocations+=moved[0];for(side=0;side<2;++side)finish(&fixtures[side].base);
 }
#ifdef RESPONSE_WHOLE_ORIGINAL
 for(i=0;i<64;++i){
  recording=0;memset(traces,0,sizeof(traces));move_requested=0;
  for(side=0;side<2;++side){ResponseFixture *f=fixtures+side;active_side=side;if(!response_fixture(f))return 2;publisher[side]=e=actor(f,i&1?6:2);watchers[side][0]=watchers[side][1]=NULL;
   real(e,0x194,i%3==0?0:i%3==1?.01f:.04f);real(e,0x198,i&2?2:1);real(e,0x19c,i&4?.5f:1);e->step=bh_actor_response_tick;f->route.tick=(unsigned short)(i*1024+32760);
#ifdef DAMAGE_WHOLE_ORIGINAL
   if(!bh_actor_damage_bind(&f->damage,e))return 2;tail_rooms[side][0].actors=e;ordinary_route(side,i,i%4==0);
#endif
  }
  setword((BHEntity*)tail_code,8208,fixtures[1].route.tick);
#ifdef DAMAGE_WHOLE_ORIGINAL
  for(j=0;j<3;++j)setword((BHEntity*)raw_world[j],0x128,(unsigned long)tail_rooms[1][j].actors);
  setword((BHEntity*)tail_code,8216,fixtures[1].damage.mode);
#endif
  for(side=0;side<2;++side){active_side=side;srand(i*41+1);recording=1;if(side)original(publisher[side],&position,&fixtures[side].angles);else bh_actor_response_tick(publisher[side]);recording=0;next_rng[side]=rand();
#ifdef DAMAGE_WHOLE_ORIGINAL
if(world_index(side,(BHWorldGeometry*)word(publisher[side],0x1a4))==1){++ordinary_promotions[side];if(*(unsigned short *)((char*)publisher[side]+0x1e4)!=(unsigned short)(fixtures[side].route.tick+i%7))++errors;}
#endif
if(fixture_error(fixtures+side)||!bh_entity_is_active(&fixtures[side].base.pool,publisher[side]))++errors;}
  ++whole_live_cases;
  if(memcmp(traces,traces+1,sizeof(traces[0])))++trace_bad;
  if(next_rng[0]!=next_rng[1]||fixtures[0].base.pool.used!=fixtures[1].base.pool.used||publisher[0]->step!=publisher[1]->step||memcmp((char*)publisher[0]+0x120,(char*)publisher[1]+0x120,0x84)||memcmp((char*)publisher[0]+0x1e4,(char*)publisher[1]+0x1e4,4)){
   if(!state_bad)printf("callbacks source=%p reference=%p captured=%p damage=%p vehicle=%p\n",publisher[0]->step,publisher[1]->step,source_normal_step,bh_actor_damage_tick,bh_actor_vehicle_tick);
   if(state_bad<15)printf("live difference i=%lu rng=%d pool=%d step=%d data=%d tick=%d\n",i,next_rng[0]!=next_rng[1],fixtures[0].base.pool.used!=fixtures[1].base.pool.used,publisher[0]->step!=publisher[1]->step,memcmp((char*)publisher[0]+0x120,(char*)publisher[1]+0x120,0x84),memcmp((char*)publisher[0]+0x1e4,(char*)publisher[1]+0x1e4,4));++state_bad;
  }
  for(side=0;side<2;++side)finish(&fixtures[side].base);
 }
#endif
#ifndef DAMAGE_WHOLE_ORIGINAL
 recording=0;active_side=0;if(!response_fixture(fixtures))return 2;e=actor(fixtures,2);
 for(i=0;i<8;++i)for(j=0;j<9;++j)for(side=0;side<2;++side){
  static const unsigned long damage_bits[8]={0,0x80000000UL,1,0x3d4cccccUL,0x3d4ccccdUL,0x3d4cccceUL,0xbd4cccccUL,0x3f000000UL};
  setword(e,0x194,damage_bits[i]);setword(e,0x198,0x3f800000UL+j%3-1);setword(e,0x19c,0x3f800000UL+j/3-1);
  e->step=bh_actor_response_tick;original_healthy(e);expected_step=e->step;e->step=bh_actor_response_tick;
  bh_actor_response_finish(e,&fixtures[0].response,NULL,side);++healthy_cases;if(e->step!=expected_step)++healthy_bad;
 }
 finish(&fixtures[0].base);
#endif
 printf("{\"death_tail_cases\":%lu,\"trace_differences\":%lu,\"state_rng_pool_cue_list_differences\":%lu,\"errors\":%lu,\"dependency_errors\":%lu,\"callback_room_moves_each_side\":%lu,\"live_sound_cases\":%lu,\"real_wrecks_each_side\":%lu,\"healthy_return_cases\":%lu,\"healthy_return_differences\":%lu,\"whole_live_tick_cases\":%lu,\"native_whole_tick\":%d,\"ordinary_damage_tick\":%d,\"strict_all_profile_match\":false}\n",cases,trace_bad,state_bad,errors,dependencies,relocations,live_sounds,constructed,healthy_cases,healthy_bad,whole_live_cases,
#ifdef RESPONSE_WHOLE_ORIGINAL
 1
#else
 0
#endif
 ,
#ifdef DAMAGE_WHOLE_ORIGINAL
 1
#else
 0
#endif
 );
#ifdef DAMAGE_WHOLE_ORIGINAL
 printf("{\"ordinary_advance_calls\":%lu,\"source_reference_promotions\":[%lu,%lu]}\n",ordinary_advances,ordinary_promotions[0],ordinary_promotions[1]);
 if(ordinary_advances!=80||ordinary_promotions[0]!=80||ordinary_promotions[1]!=80)++errors;
#endif
 VirtualFree(tail_code,0,MEM_RELEASE);remove("actor-damage-fixture/HUN_LOD1.BSP");_rmdir("actor-damage-fixture");bh_math_environment_leave(&environment);return trace_bad||state_bad||errors||dependencies||healthy_bad||failed?1:0;
}

/* Isolated diagnostic level adapter. Not an original gameplay implementation.
   Only level setup and recorded input are synthetic; all mechanic owners are real. */
static BHGameApp *box_observer;
static unsigned long box_emissions(BHGameApp *a,unsigned long index){
 if(index==1)return a->weapon_pulse.shots;
 if(index==4)return a->weapon_arc_fire.shots;
 if(index==6)return a->weapon_fire.bursts;
 return a->weapon_fire.shots;
}
static void __cdecl box_event(BHEntity *e,void *raw) {
 long *p=(long*)raw;(void)e;
 if(box_observer&&p[0]==14&&p[2]==9002){box_observer->box_objective=1;bh_message_enqueue(&box_observer->messages,"1BOX OBJECTIVE COMPLETE\nTarget destroyed. Collect its drop.");}
}
static int box_setup(BHGameApp *a) {
 unsigned long i;BHEntity *e,*wait,*action;BHWeaponObjectState *weapon;BHVec3 p={0,0,-45};BHScriptWaitState state;BHScriptWaitPacket activation;
 long args[5]={0,9002,0,0,0},selectors[5]={-1,-1,-1,-1,-1};
 if(!a->box_level)return 1;if(!a->hardware||a->editor_world.count!=3||a->navigation.node_count!=2)return failure(a,14,"Box level requires isolated assets and HAL");
 srand(1979);box_observer=a;
 for(i=0;i<BH_BOOTSTRAP_MENUS;++i){e=a->bootstrap.menus[i].object.entity;e->step=0;e->refresh=0;bh_input_set_enabled(a->bootstrap.menus[i].object.input,&a->snapshot,0);}
 a->player_state.mode=1;a->player_state.world_mode=2;a->player_state.paused=0;a->player_state.ending=0;a->editor_world.mode=2;
 a->starts[0].position.x=0;a->starts[0].position.y=0;a->starts[0].position.z=-60;a->starts[0].angles=a->zero;a->player_camera.player_count=1;
 if(!bh_player_camera_reset(&a->player_camera,0)||!bh_physics_step_read_entity(&a->player_motion,a->player)||!bh_input_set_enabled(a->player_tick.input,&a->snapshot,1))return failure(a,14,"Box player initialization failed");
 /* Fixed diagnostic bindings, isolated from the saved user profile. */
 bh_player_input_bind_axis(a->player_tick.input,0,"",0,0,VK_LEFT,VK_RIGHT,a->box_auto?1:0,0);
 bh_player_input_bind_axis(a->player_tick.input,1,"",0,0,VK_DOWN,VK_UP,a->box_auto?3:0,0);
 bh_player_input_bind_axis(a->player_tick.input,2,"",0,0,'S','W',0,0);
 bh_player_input_bind_key(a->player_tick.input,22,"",0,0,VK_CONTROL);
 bh_player_input_bind_key(a->player_tick.input,3,"",0,0,VK_SPACE);
 for(i=0;i<10;++i)bh_player_input_bind_key(a->player_tick.input,12+i,"",0,0,i==9?'0':'1'+i);
 a->collection.enabled[0]=a->collection.enabled[1]=0;
 a->box_pickup=bh_pickup_basic_spawn(&a->pickups,&p,0,50,50,0,0);if(!a->box_pickup)return failure(a,14,"Box pickup creation failed");a->box_pickup_generation=a->box_pickup->generation;
 p.x=-15;bh_pickup_spawn(&a->pickups,&p,1,1,1,1,0,0);
 if(!a->box_auto)for(i=0;i<10;++i){p.x=-30+15*(long)(i%5);p.y=0;p.z=20+20*(long)(i/5);if(!bh_pickup_spawn(&a->pickups,&p,i,1,1,1,0,0))return failure(a,14,"Box weapon pickup creation failed");}
 /* Actual route AI, damage, wreck and loot pipeline; fixed seed is level data. */
 memset(a->availability[3],0,sizeof(a->availability[3]));a->availability[3][0]=1;a->availability[3][31]=1;
 a->box_ai=bh_actor_construct_room(&a->actor_construct,a->rooms[2].world,6,5000,10,9001,12345);
 if(!a->box_ai||a->actor_construct.error)return failure(a,14,"Box AI creation failed");
 a->box_target_generation=a->box_ai->generation;
 /* Original waitfor -> VM broadcast chain, with a diagnostic result observer. */
 action=bh_script_vm_create(&a->vm,0,18,args,selectors);wait=bh_entity_acquire(&a->pool);e=bh_entity_acquire(&a->pool);
 if(!action||!wait||!e||!bh_entity_set_query(&a->pool,e,box_event))return failure(a,14,"Box objective allocation failed");
 memset(&state,0,sizeof(state));state.fired=action;state.remaining=1;state.expected=9001;state.variable_remaining=state.variable_expected=-1;
 if(!bh_script_wait_bind(&a->lifetime.timers,wait,&state))return failure(a,14,"Box waitfor binding failed");
 activation.event=8;activation.actor=a->nodes[2]->entity;activation.expected=0;wait->query(wait,&activation);
 if(a->lifetime.timers.error)return failure(a,14,"Box objective activation failed");
 bh_weapon_registry_resolve(&a->weapons.registry,0,&e,&weapon);
 a->box_trace=fopen("box-trace.csv","w");if(!a->box_trace)return failure(a,14,"Box trace open failed");
 fprintf(a->box_trace,"tick,x,y,z,vx,vy,vz,contacts,shots,damage_packets,ammo,pickup_alive,ai_alive,ai_x,ai_y,ai_z,objective,drop_seen,drop_taken,drop_x,drop_y,drop_z,collected_mask,fired_mask,primary,secondary,spawned_mask\n");
 bh_message_enqueue(&a->messages,"1BOX TEST\nW/S: thrust/brake\nArrows: steering\nCtrl/Space: primary/secondary\n1-5/6-0: select weapon\nAll weapons in far room.\nEsc: exit this test.");
 stage(a,"Box level: direct gameplay; actual AI, projectiles, pickup and objective callbacks");return 1;
}
static void box_inputs(BHGameApp *a) {
 BHEntityPositionPacket p;BHVec3 d,right,up,forward;double x,y,z,yaw,pitch,distance;int aim=0,combat=0;
 unsigned long index,phase;BHEntity *weapon;BHWeaponObjectState *state;
 if(!a->box_auto)return;
 memset(a->snapshot.current,0,sizeof(a->snapshot.current));
 a->snapshot.axes[0]=a->snapshot.axes[1]=0;
 if(a->box_ticks>=900){
  index=(a->box_ticks-900)/120;phase=(a->box_ticks-900)%120;if(index>=10)return;
  if(!bh_weapon_registry_resolve(&a->weapons.registry,index,&weapon,&state)){failure(a,14,"Box weapon registry failed");return;}
  if(!phase){
   a->box_ammo_before=state->common.ammo;
   a->box_weapon_pickup=bh_pickup_spawn(&a->pickups,&a->player->position,index,1,1,1,0,0);
   if(!a->box_weapon_pickup){failure(a,14,"Box staged weapon pickup failed");return;}
   a->box_weapon_generation=a->box_weapon_pickup->generation;
  }
  if(phase==20)a->snapshot.current[index==9?'0':'1'+index]=0x80;
  if(phase==59){a->box_fire_ammo=state->common.ammo;a->box_emit_before=box_emissions(a,index);}
  if(phase>=60&&phase<80)a->snapshot.current[index<5?VK_CONTROL:VK_SPACE]=0x80;
  return;
 }
 if(a->box_ticks<35)a->snapshot.current['W']=0x80;
 if(a->box_ticks>=35){
  if(a->box_ai->generation==a->box_target_generation&&a->box_ai->query){
   p.event=3;a->box_ai->query(a->box_ai,&p);aim=combat=1;
  }else if(a->box_drop&&a->box_drop->generation==a->box_drop_generation){p.position=a->box_drop->position;aim=1;}
  if(aim){
   d.x=p.position.x-a->player->position.x;d.y=p.position.y-a->player->position.y;d.z=p.position.z-a->player->position.z;
   memcpy(&right,(char*)a->player+0x18,12);memcpy(&up,(char*)a->player+0x24,12);memcpy(&forward,(char*)a->player+0x30,12);
   x=d.x*right.x+d.y*right.y+d.z*right.z;y=d.x*up.x+d.y*up.y+d.z*up.z;z=d.x*forward.x+d.y*forward.y+d.z*forward.z;
   yaw=atan2(x,z)*1.5;pitch=atan2(y,sqrt(x*x+z*z))*1.5;
   /* Bounded input amplitudes belong to this test driver, not game arithmetic. */
   a->snapshot.axes[0]=(float)(yaw>1?1:yaw< -1?-1:yaw);a->snapshot.axes[1]=(float)(pitch>1?1:pitch< -1?-1:pitch);
   distance=sqrt(x*x+y*y+z*z);
   if(!combat&&z>0&&fabs(yaw)<.3&&fabs(pitch)<.3&&distance>4)a->snapshot.current['W']=0x80;
  }
  if(combat)a->snapshot.current[VK_CONTROL]=0x80;
 }
}
static void box_observe(BHGameApp *a) {
 BHEntity *e,*drop;BHWeaponObjectState *weapon;BHEntityPositionPacket p;BHVec3 dp={0,0,0};int alive,pickup;
 if(!a->box_level)return;++a->box_ticks;
 if(a->box_auto&&a->box_ticks>900){unsigned long index=(a->box_ticks-901)/120,phase=(a->box_ticks-901)%120;BHEntity *selected;BHWeaponObjectState *state;float boost;
  if(index<10&&bh_weapon_registry_resolve(&a->weapons.registry,index,&selected,&state)){
   if(a->box_weapon_pickup->generation!=a->box_weapon_generation&&state->common.available_ec&&state->common.ammo>a->box_ammo_before)a->box_collected_mask|=1UL<<index;
   memcpy(&boost,(char*)a->player+0x240,4);
   if(phase>=60&&state->common.ammo<a->box_fire_ammo&&((index==9&&boost>0)||(index!=9&&box_emissions(a,index)>a->box_emit_before)))a->box_fired_mask|=1UL<<index;
   if(phase>=60)for(e=a->pool.active;e;e=e->next){
    if((index==0&&e->step==bh_ballistic_projectile_tick)||(index==2&&e->step==bh_homing_projectile_tick)||(index==3&&e->step==bh_effect_split_tick)||(index==5&&e->step==bh_effect_orbit_tick)||(index==6&&e->step==bh_area_projectile_tick)||(index==7&&e->step==bh_pulse_projectile_tick)||(index==8&&e->step==bh_effect_accelerated_tick))a->box_spawned_mask|=1UL<<index;
   }
   if(phase==119&&(!(a->box_collected_mask&(1UL<<index))||!(a->box_fired_mask&(1UL<<index)))){char text[160];sprintf(text,"Box weapon%lu incomplete: collected=%lx fired=%lx ammo=%ld selection=%u",index,a->box_collected_mask,a->box_fired_mask,state->common.ammo,*((unsigned char*)a->player+0x2b6+(index>=5)));failure(a,14,text);}
  }
 }
 alive=a->box_ai->generation==a->box_target_generation&&a->box_ai->query!=NULL;
 pickup=a->box_pickup->generation==a->box_pickup_generation;
 memset(&p,0,sizeof(p));p.event=3;if(alive)a->box_ai->query(a->box_ai,&p);
 if(!bh_weapon_registry_resolve(&a->weapons.registry,0,&e,&weapon)){failure(a,14,"Box inventory binding failed");return;}
 if(!a->box_drop_seen)for(drop=a->pickups.head;drop;drop=bh_pickup_state(drop)->global_next){
  if(bh_pickup_state(drop)->fade>0){a->box_drop=drop;a->box_drop_generation=drop->generation;a->box_drop_seen=1;a->box_drop_ammo=weapon->common.ammo;break;}
 }
 if(a->box_drop){
  if(a->box_drop->generation==a->box_drop_generation)dp=a->box_drop->position;
  else if(weapon->common.ammo>a->box_drop_ammo)a->box_drop_taken=1;
 }
 fprintf(a->box_trace,"%lu,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%lu,%lu,%lu,%ld,%d,%d,%.9g,%.9g,%.9g,%d,%d,%d,%.9g,%.9g,%.9g,%lu,%lu,%u,%u,%lu\n",a->box_ticks,a->player->position.x,a->player->position.y,a->player->position.z,a->player_motion.velocity.x,a->player_motion.velocity.y,a->player_motion.velocity.z,a->physics.contacts,a->weapon_fire.shots,a->ballistic.bindings.damage_packets,weapon->common.ammo,pickup,alive,p.position.x,p.position.y,p.position.z,a->box_objective,a->box_drop_seen,a->box_drop_taken,dp.x,dp.y,dp.z,a->box_collected_mask,a->box_fired_mask,*((unsigned char*)a->player+0x2b6),*((unsigned char*)a->player+0x2b7),a->box_spawned_mask);
 fflush(a->box_trace);
 if(a->box_auto&&a->box_ticks>=2100)a->quit=1;
}

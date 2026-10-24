/* Diagnostic steering estimates and child keyboard policy, never game state. */
static DWORD combat_query_count,combat_projectiles;
static int combat_phase,combat_firing;
static float combat_anchor[3],combat_velocity[3];
static void combat_frame(const float *basis,const float *aim,const float *velocity,float norm,float *out) {
    int j;for(j=0;j<4;j++)out[j]=0;
    for(j=0;j<3;j++) {
        out[0]+=aim[j]*basis[j]/norm;out[1]+=aim[j]*basis[3+j]/norm;
        out[2]+=aim[j]*basis[6+j]/norm;out[3]+=velocity[j]*basis[6+j];
    }
}
static void combat_turn_keys(float right,float up,int *wanted) {
    /* Intro calibration observed -basis.forward; reverse its response signs
       when steering the actual +basis.forward used by thrust and the gun. */
    if(fabs(right)>0.025)wanted[right*flight_yaw_sign<0?3:2]=1;
    if(fabs(up)>0.025)wanted[up*flight_pitch_sign<0?1:0]=1;
}
static void combat_curve(float t,const float *a,const float *b,const float *c,const float *d,float *out) {
    double u=1.0-t,w=(double)t*t*(3.0-2.0*t);int j;
    for(j=0;j<3;j++) {
        if(t<=0)out[j]=(float)(a[j]+(b[j]-a[j])*(double)t);
        else if(t>=1)out[j]=(float)(d[j]+(c[j]-d[j])*u);
        else out[j]=(float)((1.0-w)*(u*a[j]+t*b[j])+w*(u*c[j]+t*d[j]));
    }
}
static int combat_estimate(DWORD count,float *position) {
    float curve[15],shift[3],phase[2],future[3],t;int j;
    if(!training_target || !training_finished || training_released || !read_bytes(training_target+0x120,curve,60) || !read_bytes(training_target+0x15c,shift,12) || !read_bytes(training_target+0x180,phase,8))return 0;
    for(j=0;j<15;j++)if(!flight_finite(curve[j]))return 0;
    for(j=0;j<3;j++)if(!flight_finite(shift[j]))return 0;
    if(!flight_finite(phase[0]) || !flight_finite(phase[1]) || phase[1]<=0)return 0;
    t=phase[0]/phase[1];combat_curve(t,curve,curve+9,curve+12,curve+3,position);
    combat_curve(t+0.033333333f/phase[1],curve,curve+9,curve+12,curve+3,future);
    for(j=0;j<3;j++){combat_velocity[j]=(future[j]-position[j])*30;position[j]+=shift[j];if(!flight_finite(position[j]) || !flight_finite(combat_velocity[j]))return 0;}
    printf("{\"event\":\"combat-aim-estimate\",\"completed_simulation_callbacks\":%lu,\"entity\":%lu,\"position\":[%.9g,%.9g,%.9g],\"velocity_estimate\":[%.9g,%.9g,%.9g],\"phase\":[%.9g,%.9g],\"original_position_observation\":false}\n",count,training_target,position[0],position[1],position[2],combat_velocity[0],combat_velocity[1],combat_velocity[2],phase[0],phase[1]);
    return 1;
}
static int combat_tick(DWORD pid,DWORD count) {
    DWORD player=word(0x4dfab8),weapon=word(player+0x258),ammo;float p[3],basis[9],v[3],gun[3],target[3],d[3],aim[3],waypoint[3],shot[3];
    float distance=0,horizontal,norm=0,forward=0,right=0,up=0,along=0,gun_dot=0,gun_norm=0,desired,lead,frame[4],shot_norm=0,waypoint_distance=0,cap;int j,wanted[6]={0,0,0,0,0,0},fire,same_owner=0;
    if(!player || word(0x48f958)!=0x416120 || word(0x4dfb08)!=1 || !read_bytes(player+0xc,p,12) || !read_bytes(player+0x18,basis,36) || !read_bytes(player+0x48,v,12) || !read_bytes(player+0x280,gun,12) || !weapon || !read_bytes(weapon+0x10c,&ammo,4) || !combat_estimate(count,target))return 0;
    if((word(player+0x2b6)&255)!=0 || (word(word(0x4df240)+0x28)&255)!=VK_CONTROL)return 0;
    for(j=0;j<9;j++)if(!flight_finite(basis[j]))return 0;
    for(j=0;j<3;j++)if(!flight_finite(p[j]) || !flight_finite(v[j]) || !flight_finite(gun[j]))return 0;
    if(!combat_phase){for(j=0;j<3;j++)combat_anchor[j]=p[j];combat_phase=2;}
    horizontal=(float)sqrt((target[0]-p[0])*(target[0]-p[0])+(target[2]-p[2])*(target[2]-p[2]));
    if(pilot_enabled){if(!pilot_waypoint(count,player,target,waypoint,&same_owner))return 0;combat_phase=same_owner?3:2;}
    else if(combat_phase==2 && horizontal<110)combat_phase=3;
    for(j=0;j<3;j++){d[j]=target[j]-p[j];distance+=d[j]*d[j];}
    distance=(float)sqrt(distance);lead=distance/150;if(lead>1)lead=1;
    for(j=0;j<3;j++) {
        shot[j]=d[j]+(combat_velocity[j]-v[j])*lead;shot_norm+=shot[j]*shot[j];
        if(pilot_enabled && !same_owner){aim[j]=waypoint[j]-p[j]-v[j]*0.35f;waypoint_distance+=(waypoint[j]-p[j])*(waypoint[j]-p[j]);}
        else if(combat_phase==2)aim[j]=(j==1?combat_anchor[j]:target[j])-p[j]-v[j]*0.7f;
        else aim[j]=d[j]+(combat_velocity[j]-v[j])*lead;
        norm+=aim[j]*aim[j];gun_norm+=gun[j]*gun[j];
    }
    norm=(float)sqrt(norm);gun_norm=(float)sqrt(gun_norm);shot_norm=(float)sqrt(shot_norm);if(norm<0.001f || gun_norm<0.001f || shot_norm<0.001f)return 0;
    combat_frame(basis,aim,v,norm,frame);right=frame[0];up=frame[1];forward=frame[2];along=frame[3];
    for(j=0;j<3;j++)gun_dot+=(pilot_enabled?shot[j]/shot_norm:aim[j]/norm)*gun[j]/gun_norm;
    combat_turn_keys(right,up,wanted);
    desired=combat_phase==2?90:(distance-60)*0.7f;
    if(desired>70 && combat_phase==3)desired=70;if(desired<5)desired=5;
    if(pilot_enabled && !same_owner){cap=pilot_opening<20?12:pilot_opening<40?20:40;desired=(float)sqrt(waypoint_distance)*0.6f;if(desired>cap)desired=cap;if(desired<4)desired=4;}
    if((along>desired+4 && forward>0.8f) || (pilot_enabled && along>5 && forward<0.7f))wanted[5]=1;
    else if(forward>0.9f && along<desired && (combat_phase!=3 || distance>60))wanted[4]=1;
    fire=(combat_phase==3 || (pilot_enabled && pilot_distance[pilot_current]<160)) && distance<150 && gun_dot>0.996f && ammo>0;
    if(fire!=combat_firing){if(!post_child_key(pid,count,VK_CONTROL,fire))return 0;combat_firing=fire;}
    printf("{\"event\":\"combat-policy\",\"completed_simulation_callbacks\":%lu,\"phase\":%d,\"player_position\":[%.9g,%.9g,%.9g],\"distance_estimate\":%g,\"forward_velocity\":%g,\"gun_alignment\":%g,\"ammo\":%lu,\"fire\":%s,\"keys\":[%d,%d,%d,%d,%d,%d]",count,combat_phase,p[0],p[1],p[2],distance,along,gun_dot,ammo,fire?"true":"false",wanted[0],wanted[1],wanted[2],wanted[3],wanted[4],wanted[5]);
    printf(",\"basis_forward\":[%.9g,%.9g,%.9g],\"basis_up\":[%.9g,%.9g,%.9g],\"basis_right\":[%.9g,%.9g,%.9g],\"gun_direction\":[%.9g,%.9g,%.9g],\"velocity\":[%.9g,%.9g,%.9g],\"aim_local\":[%g,%g,%g],\"policy_forward_convention\":\"positive\"}\n",basis[6],basis[7],basis[8],basis[3],basis[4],basis[5],basis[0],basis[1],basis[2],gun[0],gun[1],gun[2],v[0],v[1],v[2],right,up,forward);
    return flight_set_keys(pid,count,wanted);
}

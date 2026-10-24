/* Host-side diagnostic route policy. Reads the real graph; never writes it. */
#define PILOT_MAX_NODES 2048
typedef struct PilotNode {
    DWORD entity,placement;float center[3],bounds[6];DWORD port_count;
    int neighbor[6];float point[6][3],size[6][2];int axis[6];
} PilotNode;
static PilotNode pilot_nodes[PILOT_MAX_NODES];
static DWORD pilot_count,pilot_goal=0xffffffffUL,pilot_array;
static double pilot_distance[PILOT_MAX_NODES];
static int pilot_toward[PILOT_MAX_NODES],pilot_enabled;
static DWORD pilot_current,pilot_destination,pilot_contacts;
static float pilot_opening;
static double pilot_edge(DWORD a,DWORD b) {
    double sum=0,d;int j;for(j=0;j<3;j++){d=pilot_nodes[a].center[j]-pilot_nodes[b].center[j];sum+=d*d;}return sqrt(sum);
}
static int pilot_compute(DWORD goal) {
    unsigned char visited[PILOT_MAX_NODES];DWORD i,k,u,v;int j;double best,cost;
    if(!pilot_count || goal>=pilot_count)return 0;
    memset(visited,0,sizeof(visited));
    for(i=0;i<pilot_count;i++){pilot_distance[i]=1e100;pilot_toward[i]=-1;}
    pilot_distance[goal]=0;
    for(k=0;k<pilot_count;k++) {
        best=1e100;u=0xffffffffUL;
        for(i=0;i<pilot_count;i++)if(!visited[i] && pilot_distance[i]<best){best=pilot_distance[i];u=i;}
        if(u==0xffffffffUL)break;visited[u]=1;
        for(j=0;j<6;j++)if(pilot_nodes[u].neighbor[j]>=0) {
            v=pilot_nodes[u].neighbor[j];if(v>=pilot_count)return 0;
            if(!visited[v]){cost=best+pilot_edge(u,v);if(cost<pilot_distance[v]){pilot_distance[v]=cost;pilot_toward[v]=u;}}
        }
    }
    pilot_goal=goal;return 1;
}
static int pilot_next(DWORD source,DWORD goal,int *slot) {
    int j;if(source>=pilot_count || goal>=pilot_count)return 0;
    if(pilot_goal!=goal && !pilot_compute(goal))return 0;
    if(source==goal){*slot=-1;return 1;}
    if(pilot_toward[source]<0)return 0;
    for(j=0;j<6;j++)if(pilot_nodes[source].neighbor[j]==pilot_toward[source]){*slot=j;return 1;}
    return 0;
}
static int pilot_load(void) {
    DWORD models=word(0x4df25c),i,j,k,model,words[15],neighbor,index;PilotNode *n;float overlap[4];int axis,other[2],q;
    pilot_count=word(0x4df26c);pilot_array=word(0x4df268);
    if(!pilot_count || pilot_count>PILOT_MAX_NODES || !pilot_array)return 0;
    for(i=0;i<pilot_count;i++) {
        n=pilot_nodes+i;memset(n,0,sizeof(*n));for(j=0;j<6;j++)n->neighbor[j]=-1;
        if(!read_bytes(pilot_array+i*4,&n->entity,4) || !n->entity || word(n->entity+0x108)!=i || word(n->entity+0x1c8) || !read_bytes(n->entity+0x118,&n->placement,4) || !read_bytes(n->entity+0xf8,&model,4) || model>=models || !read_bytes(n->entity+0x1b0,n->bounds,24))return 0;
        n->port_count=word(0x497248+model*0x174);if(!n->port_count || n->port_count>6)return 0;
        for(j=0;j<3;j++){if(!flight_finite(n->bounds[j]) || !flight_finite(n->bounds[j+3]) || n->bounds[j]>n->bounds[j+3])return 0;n->center[j]=(n->bounds[j]+n->bounds[j+3])*0.5f;}
        for(j=0;j<n->port_count;j++) {
            if(!read_bytes(n->entity+0x1cc+j*0x3c,words,60))return 0;
            neighbor=words[14];if(!neighbor)continue;index=word(neighbor+0x108);
            if(index>=pilot_count || word(pilot_array+index*4)!=neighbor)return 0;n->neighbor[j]=index;
            if(words[0]==0 || words[0]==4)axis=0;else if(words[0]==1 || words[0]==5)axis=1;else if(words[0]==2 || words[0]==6)axis=2;else return 0;
            n->axis[j]=axis;memcpy(overlap,words+6,16);
            for(k=0;k<4;k++)if(!flight_finite(overlap[k]))return 0;
            if(overlap[0]>=overlap[1] || overlap[2]>=overlap[3])return 0;
            q=0;for(k=0;k<3;k++)if(k!=(DWORD)axis)other[q++]=k;
            n->point[j][axis]=(float)(long)words[1];n->point[j][other[0]]=(overlap[0]+overlap[1])*0.5f;n->point[j][other[1]]=(overlap[2]+overlap[3])*0.5f;
            n->size[j][0]=overlap[1]-overlap[0];n->size[j][1]=overlap[3]-overlap[2];
        }
    }
    for(i=0;i<pilot_count;i++)for(j=0;j<6;j++)if(pilot_nodes[i].neighbor[j]>=0) {
        neighbor=pilot_nodes[i].neighbor[j];q=0;
        for(k=0;k<6;k++)if(pilot_nodes[neighbor].neighbor[k]==(int)i)q=1;
        if(!q)return 0;
        for(k=0;k<3;k++)if(pilot_nodes[i].point[j][k]<pilot_nodes[neighbor].bounds[k]-0.001f || pilot_nodes[i].point[j][k]>pilot_nodes[neighbor].bounds[k+3]+0.001f)return 0;
    }
    printf("{\"event\":\"portal-policy-graph\",\"nodes\":%lu,\"junctions\":%lu,\"array\":%lu}\n",pilot_count,word(0x4df264),pilot_array);return 1;
}
static int pilot_owner_index(DWORD owner,DWORD *index) {
    DWORD parent;if(!owner)return 0;parent=word(owner+0x1c8);if(parent)owner=parent;
    *index=word(owner+0x108);
    return *index<pilot_count && pilot_nodes[*index].entity==owner && word(owner+0x118)==pilot_nodes[*index].placement && word(pilot_array+*index*4)==owner;
}
static int pilot_waypoint(DWORD count,DWORD player,const float *target,float *point,int *same_owner) {
    DWORD source_owner=word(player+0xc4),target_owner=word(training_target+0x1a8),neighbor;int slot,j,axis;float sign;
    if(!pilot_count && !pilot_load())return 0;
    if(!pilot_owner_index(source_owner,&pilot_current) || !pilot_owner_index(target_owner,&pilot_destination)) {
        printf("{\"event\":\"portal-owner-rejected\",\"player_owner\":%lu,\"target_owner\":%lu}\n",source_owner,target_owner);return 0;
    }
    if(!pilot_next(pilot_current,pilot_destination,&slot))return 0;
    *same_owner=slot<0;pilot_opening=1000;
    if(slot<0){for(j=0;j<3;j++)point[j]=target[j];neighbor=pilot_current;}
    else {
        PilotNode *n=pilot_nodes+pilot_current;neighbor=n->neighbor[slot];axis=n->axis[slot];
        for(j=0;j<3;j++)point[j]=n->point[slot][j];
        sign=pilot_nodes[neighbor].center[axis]>n->center[axis]?1.0f:-1.0f;point[axis]+=sign*4;
        pilot_opening=n->size[slot][0]<n->size[slot][1]?n->size[slot][0]:n->size[slot][1];
    }
    printf("{\"event\":\"portal-guidance\",\"completed_simulation_callbacks\":%lu,\"player_owner\":%lu,\"target_owner\":%lu,\"source\":%lu,\"source_placement\":%lu,\"target\":%lu,\"target_placement\":%lu,\"exit_slot\":%d,\"next_node\":%lu,\"point\":[%.9g,%.9g,%.9g],\"opening\":%g,\"graph_distance\":%g}\n",count,source_owner,target_owner,pilot_current,pilot_nodes[pilot_current].placement,pilot_destination,pilot_nodes[pilot_destination].placement,slot,neighbor,point[0],point[1],point[2],pilot_opening,pilot_distance[pilot_current]);return 1;
}

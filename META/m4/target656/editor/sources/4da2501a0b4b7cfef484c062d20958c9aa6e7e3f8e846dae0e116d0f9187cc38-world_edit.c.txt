#include "../src/bh_world_edit.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
static unsigned long checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;if(failures<10)printf("FAIL world edit %d\n",__LINE__);}}while(0)
static void section(unsigned char *b,unsigned long *at,unsigned long n,const void *p,unsigned long bytes){memcpy(b+*at,&n,4);*at+=4;memcpy(b+*at,p,bytes);*at+=bytes;}
static int cube(BHBsp *b,int portals){
    unsigned char bytes[8192];unsigned long at=0,i,j;BHBspMaterial materials[6];BHBspVertex vertices[8];
    BHBspEdge edges[24];BHBspFace faces[6];BHBspNode nodes[6];BHBspLeaf leaves[6];
    static const long indices[6][4]={{1,3,7,5},{0,4,6,2},{2,6,7,3},{0,1,5,4},{4,5,7,6},{0,2,3,1}};
    memset(materials,0,sizeof(materials));memset(vertices,0,sizeof(vertices));memset(edges,0,sizeof(edges));memset(faces,0,sizeof(faces));memset(nodes,0,sizeof(nodes));
    for(i=0;i<6;i++)strcpy(materials[i].texture,portals?"PORTAL.GIF":"STONE.PCX");
    for(i=0;i<8;i++){vertices[i].x=i&1?10:-10;vertices[i].y=i&2?10:-10;vertices[i].z=i&4?10:-10;}
    for(i=0;i<6;i++){
        faces[i].material=i;faces[i].first_edge=i*4;
        for(j=0;j<4;j++){edges[i*4+j].vertex=indices[i][j];edges[i*4+j].next=j==3?-1:i*4+j+1;}
        for(j=0;j<3;j++){nodes[i].minimum[j]=-10;nodes[i].maximum[j]=10;}
        nodes[i].normal[i/2]=i&1?-1:1;nodes[i].front_leaf=i;nodes[i].back_leaf=nodes[i].front_node=nodes[i].back_node=-1;leaves[i].face=i;leaves[i].next=-1;
    }
    section(bytes,&at,6,materials,sizeof(materials));section(bytes,&at,8,vertices,sizeof(vertices));section(bytes,&at,24,edges,sizeof(edges));
    section(bytes,&at,6,faces,sizeof(faces));section(bytes,&at,6,nodes,sizeof(nodes));section(bytes,&at,6,leaves,sizeof(leaves));return bh_bsp_decode(b,bytes,at);
}
int bh_world_edit_checkpoint(unsigned long *count){
    BHBsp bsp[2];BHGeometry geometry[2];BHGeometryModel model[2];BHWorldEditPlacement p[3],saved;
    unsigned long i,j;
    checks=failures=0;memset(p,0,sizeof(p));
    for(i=0;i<2;i++){CHECK(bh_bsp_init(bsp+i,NULL));CHECK(cube(bsp+i,i==0));CHECK(bh_geometry_init(geometry+i,bsp+i,NULL));CHECK(bh_geometry_model_init(model+i,geometry+i,2,0));}
    for(i=0;i<3;i++){
        p[i].geometry.navigation.next=&p[(i+1)%3].geometry.navigation;p[i].geometry.previous=&p[(i+2)%3].geometry;
        p[i].geometry.model=model+(i==1?1:0);p[i].coordinates[0]=i==0?0:i==1?6:20;p[i].geometry.position.x=(float)p[i].coordinates[0];
    }
    for(i=0;i<3;i++)CHECK(bh_world_geometry_build(&p[i].geometry,3));
    CHECK(p[1].geometry.owner==&p[0].geometry&&!p[0].geometry.owner&&!p[2].geometry.owner);
    CHECK(bh_world_edit_rebuild(&p[0].geometry,3));CHECK(p[0].geometry.navigation.portals[0].neighbor==&p[2].geometry.navigation);
    CHECK(p[0].geometry.navigation.portals[0].plane==10&&!p[2].geometry.navigation.portals[1].neighbor);
    CHECK(bh_world_edit_move(p,3,41,0,0,0));
    CHECK(p[0].coordinates[0]==40&&p[1].coordinates[0]==46&&p[2].coordinates[0]==20);
    CHECK(p[0].geometry.position.x==40&&p[0].geometry.minimum.x==30&&p[0].geometry.maximum.x==50);
    CHECK(p[1].geometry.position.x==46&&p[1].geometry.minimum.x==36&&p[1].geometry.maximum.x==56);
    CHECK(p[0].geometry.navigation.portals[1].neighbor==&p[2].geometry.navigation&&!p[2].geometry.navigation.portals[0].neighbor);
    CHECK(bh_world_edit_move(p,3,0,-5,0,1));CHECK(p[0].coordinates[1]==-4&&p[1].coordinates[1]==0);
    CHECK(p[0].geometry.navigation.portals[1].overlap[0]==-10&&p[0].geometry.navigation.portals[1].overlap[1]==6);
    CHECK(bh_world_edit_rotate(p+1,3,1,40,0));CHECK(p[1].coordinates[0]==40&&p[1].coordinates[2]==-6&&p[1].geometry.flags==1);
    CHECK(fabs(p[1].angles.x-1.5707963267948966)<.000001&&p[1].angles.y==0&&p[1].angles.z==0);
    CHECK(bh_world_edit_rotate(p+1,3,-1,40,0));CHECK(p[1].coordinates[0]==46&&p[1].coordinates[2]==0&&p[1].geometry.flags==0);
    for(i=0;i<4;i++)CHECK(bh_world_edit_rotate(p+1,3,1,40,0));
    CHECK(p[1].coordinates[0]==46&&p[1].coordinates[2]==0&&p[1].geometry.flags==0);
    p[1].geometry.flags=0xf0;CHECK(bh_world_edit_rotate(p+1,3,0,40,0));
    CHECK(p[1].coordinates[0]==40&&p[1].coordinates[2]==-6&&p[1].geometry.flags==0);
    saved=p[0];CHECK(bh_world_edit_rotate(p,3,1,0,0)&&!memcmp(&saved,p,sizeof(saved)));
    model[1].grid=0;saved=p[1];CHECK(!bh_world_edit_move(p+1,3,7,3,1,1)&&!memcmp(&saved,p+1,sizeof(saved)));model[1].grid=1;
    p[1].coordinates[0]=0x7fffffffL;CHECK(bh_world_edit_move(p+1,3,1,0,0,1));CHECK((unsigned long)p[1].coordinates[0]==0x80000000UL);
    CHECK(p[1].geometry.position.x==-2147483648.0);
    CHECK(!bh_world_edit_move(NULL,3,0,0,0,0));CHECK(!bh_world_edit_rotate(p,0,1,0,0));
    for(i=0;i<2;i++){CHECK(bh_geometry_dispose(geometry+i));bh_bsp_dispose(bsp+i);}
    if(count)*count=checks;printf("{\"scenario\":\"world-edit\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_WORLD_EDIT_STANDALONE
int main(void){unsigned long n;_controlfp(_PC_53|_RC_NEAR,_MCW_PC|_MCW_RC);return bh_world_edit_checkpoint(&n);}
#endif

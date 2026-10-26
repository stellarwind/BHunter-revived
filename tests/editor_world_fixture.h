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

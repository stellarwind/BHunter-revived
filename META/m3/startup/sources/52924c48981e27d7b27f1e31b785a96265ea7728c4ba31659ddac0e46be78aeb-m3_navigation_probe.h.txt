/* Read-only completed original navigation graph. No graph construction here. */
static int navigation_snapshot(const char *folder) {
    static const DWORD placements[]={4939,5055,5064,4827};
    DWORD count=word(0x4df26c),junctions=word(0x4df264),array=word(0x4df268),matrix=word(0x4df260),models=word(0x4df25c);
    DWORD i,j,k,node,model,ports,words[15],neighbor,index,written,bytes;float position[3],bounds[6];char path[1024];void *copy;HANDLE file;int ok=1;
    if(!count || count>10000 || !junctions || junctions>2048 || junctions>count || !array || !matrix || !models || strlen(folder)>900)return 0;
    printf("{\"event\":\"navigation-graph-header\",\"nodes\":%lu,\"junctions\":%lu,\"array\":%lu,\"matrix\":%lu}\n",count,junctions,array,matrix);
    for(i=0;i<count;i++) {
        if(!read_bytes(array+i*4,&node,4) || !node || word(node+0x108)!=i || word(node+0x1c8) || !read_bytes(node+0xf8,&model,4) || model>=models || !read_bytes(node+0xc,position,12) || !read_bytes(node+0x1b0,bounds,24))return 0;
        ports=word(0x497248+model*0x174);if(!ports || ports>6)return 0;
        for(j=0;j<3;j++)if(!flight_finite(position[j]) || !flight_finite(bounds[j]) || !flight_finite(bounds[j+3]) || bounds[j]>bounds[j+3])return 0;
        printf("{\"event\":\"navigation-node\",\"index\":%lu,\"entity\":%lu,\"placement\":%lu,\"model_index\":%lu,\"position\":[%.9g,%.9g,%.9g],\"bounds\":[%.9g,%.9g,%.9g,%.9g,%.9g,%.9g],\"ports\":%lu}\n",i,node,word(node+0x118),model,position[0],position[1],position[2],bounds[0],bounds[1],bounds[2],bounds[3],bounds[4],bounds[5],ports);
        for(j=0;j<ports;j++) {
            if(!read_bytes(node+0x1cc+j*0x3c,words,60))return 0;
            neighbor=words[14];index=0xffffffffUL;
            if(neighbor){index=word(neighbor+0x108);if(index>=count || word(array+index*4)!=neighbor)return 0;}
            printf("{\"event\":\"navigation-port\",\"node\":%lu,\"slot\":%lu,\"neighbor\":%lu,\"neighbor_index\":%lu,\"raw_words\":[",i,j,neighbor,index);
            for(k=0;k<15;k++){if(k)putchar(',');printf("%lu",words[k]);}printf("]}\n");
        }
    }
    for(i=0;i<sizeof(placements)/sizeof(placements[0]);i++) {
        DWORD head=word(0x4df254),owner;node=head;
        for(j=0;j<10000;j++){if(word(node+0x118)==placements[i])break;node=word(node+0x124);if(!node || node==head)return 0;}
        if(j==10000)return 0;owner=word(node+0x1c8);if(!owner)owner=node;index=word(owner+0x108);
        if(index>=count || word(array+index*4)!=owner)index=0xffffffffUL;
        if(!read_bytes(owner+0x1b0,bounds,24))return 0;
        printf("{\"event\":\"navigation-placement-owner\",\"placement\":%lu,\"entity\":%lu,\"owner\":%lu,\"node_index\":%lu,\"owner_in_graph\":%s,\"owner_bounds\":[%.9g,%.9g,%.9g,%.9g,%.9g,%.9g]}\n",placements[i],node,owner,index,index==0xffffffffUL?"false":"true",bounds[0],bounds[1],bounds[2],bounds[3],bounds[4],bounds[5]);
    }
    bytes=junctions*junctions*8;copy=malloc(bytes);if(!copy)return 0;
    if(!read_bytes(matrix,copy,bytes)){free(copy);return 0;}
    strcpy(path,folder);strcat(path,"\\navigation-matrix.bin");file=CreateFileA(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE){free(copy);return 0;}
    if(!WriteFile(file,copy,bytes,&written,NULL) || written!=bytes)ok=0;if(!CloseHandle(file))ok=0;free(copy);
    printf("{\"event\":\"navigation-matrix-saved\",\"bytes\":%lu,\"passed\":%s}\n",bytes,ok?"true":"false");return ok;
}

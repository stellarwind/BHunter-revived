#include "../src/bh_software_raster.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
static void *__cdecl allocate(unsigned long n,void *u){return malloc(n);}
static void __cdecl release(void *p,void *u){free(p);}
#define ENTRY(P,N) bh_raster_##P##_##N,
static BHRasterKernel kernels[28]={BH_RASTER_KERNELS(ENTRY,indexed) BH_RASTER_KERNELS(ENTRY,packed)};
#undef ENTRY
static unsigned long checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;if(failures<12)printf("FAIL software raster %d\n",__LINE__);}}while(0)
static void vertices(BHClipVertex *v,unsigned long shape){
    static const float xy[3][8]={{0,0,0,8,8,8,8,0},{4,0,0,8,8,8,0,0},{0,0,4,8,8,0,0,0}};
    unsigned long n=shape?3:4,i;float scalar=.25f;
    memset(v,0,4*sizeof(*v));
    for(i=0;i<n;i++){
        v[i].sx=xy[shape][i*2];v[i].sy=xy[shape][i*2+1];
        v[i].u=v[i].sx*.125f;v[i].v=v[i].sy*.125f;v[i].point.z=2;
        memcpy(&v[i].unknown28,&scalar,4);v[i].next=v+(i+1)%n;v[i].previous=v+(i+n-1)%n;
    }
}
static void bounds(unsigned long shape,unsigned long y,double *left,double *right){
    if(shape==0){*left=0;*right=8;}
    else if(shape==1){*left=4-(y+1)*.5;*right=4+(y+1)*.5;}
    else{*left=(y+1)*.5;*right=8-(y+1)*.5;}
}
static void cases(unsigned long packed){
    BHTextureContext textures;BHSoftwareTables tables;BHHorizonColor gradient;
    BHTextureImage image;BHSoftwareRaster c;BHClipVertex v[4],saved[4];
    DDPIXELFORMAT format;unsigned long masks[3]={0xf800,0x7e0,0x1f};
    unsigned char texels[1024],dither[1024],color[160];unsigned short screen[1024],depth[80];
    unsigned long k,s,d,g,p,i,y,x,start,n,visited,written,mode,policy,masked,shaded,blend;
    unsigned long raw,mapped,expected,old,outer,index,physical,offset,reverse;
    unsigned short old_depth;double left,right;
    memset(&textures,0,sizeof(textures));memset(&tables,0,sizeof(tables));memset(&image,0,sizeof(image));memset(&c,0,sizeof(c));
    textures.allocator.allocate=allocate;textures.allocator.release=release;
    memset(&format,0,sizeof(format));format.dwSize=sizeof(format);format.dwFlags=DDPF_RGB;
    format.dwRGBBitCount=16;format.dwRBitMask=masks[0];format.dwGBitMask=masks[1];format.dwBBitMask=masks[2];
    CHECK(bh_texture_bind_display(&textures,&format));CHECK(bh_software_tables_initialize(&tables,&textures,packed?2:1,packed?masks:NULL));
    CHECK(bh_horizon_color_init(&gradient,&textures));CHECK(bh_horizon_color_prepare(&gradient,packed?2:1,0x204060));
    for(i=0;i<1024;i++)texels[i]=i%5?(unsigned char)(i*7):0;
    for(p=0;p<4;p++)for(i=0;i<256;i++){
        dither[p*256+i]=(unsigned char)((i*17+p*13)&255);screen[p*256+i]=(unsigned short)((p<<14)|((i*37)&0x3fff));
    }
    /* Mapped zero is distinct from a masked raw zero. */
    dither[7]=dither[256+7]=dither[512+7]=dither[768+7]=0;
    image.dither=dither;image.screen=screen;c.image=&image;c.tables=&tables;c.gradient=&gradient;
    c.texels=texels;c.texture_width=4;c.width=c.height=8;c.near_z=1;c.shade_offset=143*256;
    for(k=0;k<14;k++)for(s=0;s<3;s++)for(d=0;d<3;d++)for(g=0;g<2;g++)for(reverse=0;reverse<2;reverse++){
        mode=k<12?k/3:4;policy=k<12?k%3:1;masked=mode==1||mode==3||mode==4;shaded=mode==2||mode==3||k==13;blend=k>=12;
        c.attribute=g;c.blend_plane=(k+s)%8;
        memset(color,0xa5,sizeof(color));for(i=0;i<80;i++)depth[i]=0xbeef;
        old_depth=d==0?0:d==1?0x8000:0x8100;
        for(i=0;i<64;i++){depth[8+i]=old_depth;if(packed)((unsigned short*)(color+16))[i]=0x5a69;else color[16+i]=57;}
        c.pixels=color+16+(reverse?7*8*(packed?2:1):0);c.pitch=(reverse?-1:1)*8*(packed?2:1);
        c.depth=depth+8+(reverse?56:0);c.depth_pitch=reverse?-16:16;
        vertices(v,s);memcpy(saved,v,sizeof(v));CHECK(kernels[packed*14+k](&c,v));CHECK(!memcmp(v,saved,sizeof(v)));
        visited=written=0;
        for(y=0;y<8;y++){
            bounds(s,y,&left,&right);start=(unsigned long)left;n=(unsigned long)(right-start);
            for(x=0;x<8;x++){
                physical=(reverse?7-y:y)*8+x;old=packed?0x5a69:57;expected=old;index=old_depth;
                if(x>=start&&x<start+n){
                    ++visited;
                    if(policy==2||old_depth<=0x8000){
                        /* Independent dyadic closed-form UV plane evaluated at
                           the original left edge rather than rounded pixel X. */
                        offset=x-start;raw=texels[(((unsigned long)((left+offset)*3.0/8.0))&3)+4*(((y+1)*3/8)&3)];
                        p=(x&1)*256+(y&1)*512;
                        if(!masked||raw){
                            if(packed&&!shaded&&!blend)expected=screen[p+raw];
                            else{
                                mapped=dither[p+raw];
                                if(shaded&&(!packed||blend))mapped=(g&&!packed?gradient.indexed+191*256:tables.shade_indexed+c.shade_offset)[mapped];
                                if(!blend)expected=packed?(g?gradient.packed+191*256:tables.shade_native+c.shade_offset)[mapped]:mapped;
                                else if(mapped){
                                    if(packed){outer=((old>>8)&0xe0)|((old>>6)&0x1c)|((old>>3)&3);expected=tables.blend_native[c.blend_plane*65536+outer*256+mapped];}
                                    else expected=tables.blend_indexed[c.blend_plane*65536+old*256+mapped];
                                }
                            }
                            if(!blend||mapped){++written;if(!blend&&policy!=1)index=0x8000;}
                        }
                    }
                }
                CHECK((packed?((unsigned short*)(color+16))[physical]:color[16+physical])==expected);
                CHECK(depth[8+physical]==index);
            }
        }
        CHECK(c.visited==visited&&c.written==written&&c.spans==8);
        if(g)CHECK(c.attribute_row==191);
        for(i=0;i<16;i++)CHECK(color[i]==0xa5);
        for(i=16+64*(packed?2:1);i<sizeof(color);i++)CHECK(color[i]==0xa5);
        for(i=0;i<8;i++)CHECK(depth[i]==0xbeef&&depth[i+72]==0xbeef);
    }
    /* A nonplanar reciprocal-depth/UV patch has different top and bottom
       integer gradients. These dyadic closed forms independently check row
       gradient increments, changing depth pass masks and actual32px textures. */
    for(s=0;s<2;s++)for(k=0;k<3;k++){
        c.texture_width=s?32:4;c.attribute=0;c.pitch=8*(packed?2:1);c.depth_pitch=16;
        c.pixels=color+16;c.depth=depth+8;
        vertices(v,0);v[0].point.z=8;v[1].point.z=4;v[2].point.z=2;v[3].point.z=4;v[3].u=.5f;
        for(i=0;i<64;i++){depth[8+i]=0x6000;if(packed)((unsigned short*)(color+16))[i]=0x5a69;else color[16+i]=57;}
        CHECK(kernels[packed*14+k](&c,v));
        for(y=0;y<8;y++)for(x=0;x<8;x++){
            index=8192+(y+1)*1024+x*(1024+y*128);p=(x&1)*256+(y&1)*512;
            raw=texels[((x*(c.texture_width-1)*(4096+y*512))>>16)+((y+1)*(c.texture_width-1)/8)*c.texture_width];
            expected=packed?0x5a69:57;old=0x6000;
            if(k==2||index>=0x6000){expected=packed?screen[p+raw]:dither[p+raw];if(k!=1)old=index;}
            CHECK((packed?((unsigned short*)(color+16))[y*8+x]:color[16+y*8+x])==expected);
            CHECK(depth[8+y*8+x]==old);
        }
    }
    c.texture_width=4;
    vertices(v,0);v[0].next=NULL;CHECK(!kernels[packed*14](&c,v)&&c.error==BH_RASTER_TOPOLOGY);
    vertices(v,0);v[0].sx=-1;CHECK(!kernels[packed*14](&c,v)&&c.error==BH_RASTER_BOUNDS);
    vertices(v,0);v[0].point.z=0;CHECK(!kernels[packed*14](&c,v)&&c.error==BH_RASTER_NUMERICAL);
    vertices(v,0);for(i=0;i<4;i++)v[i].sy=0;CHECK(kernels[packed*14](&c,v)&&c.visited==0);
    bh_horizon_color_dispose(&gradient);bh_software_tables_dispose(&tables);
}
int bh_software_raster_checkpoint(unsigned long *count){
    checks=failures=0;cases(0);cases(1);if(count)*count=checks;
    printf("{\"scenario\":\"software-raster\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_RASTER_STANDALONE
int main(void){unsigned long count;_controlfp(_PC_53|_RC_NEAR,_MCW_PC|_MCW_RC);return bh_software_raster_checkpoint(&count);}
#endif

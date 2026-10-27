typedef struct Memory{unsigned long live,calls;int deny;unsigned long fail_at;}Memory;
static void *__cdecl allocate(unsigned long n,void *user){Memory*m=(Memory*)user;void*p;++m->calls;if(m->deny||m->calls==m->fail_at)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){Memory*m=(Memory*)user;CHECK(m->live>0,"actual allocator balance");--m->live;free(p);}
static void __cdecl on_refresh(void *user,long index){unsigned long *count=(unsigned long*)user;*count+=(unsigned long)index;}
static unsigned short unit(const unsigned char*p){unsigned short n;memcpy(&n,p,2);return n;}
static int fixture(void){unsigned char *data;FILE*f;unsigned long g,x,y,c,r;int ok;
 _mkdir("textures");data=(unsigned char*)calloc(1,128+7500+768);if(!data)return 0;data[0]=10;data[1]=2;data[2]=1;data[3]=8;data[8]=99;data[10]=74;data[65]=1;data[66]=100;memset(data+128,7,7500);
 for(g=0;g<300;++g){c=g%20;r=g/20;for(y=1+r*5;y<4+r*5;++y)for(x=1+c*5;x<1+c*5+(g%3)+1;++x)data[128+y*100+x]=(unsigned char)(1+g%6);}
 for(g=1;g<=7;++g){data[7628+g*3]=(unsigned char)(17*g);data[7628+g*3+1]=(unsigned char)(23*g);data[7628+g*3+2]=(unsigned char)(31*g);}
 f=fopen("textures\\tvfont.pcx","wb");if(!f){free(data);return 0;}ok=fwrite(data,1,8396,f)==8396;if(fclose(f))ok=0;free(data);return ok;
}
static int context(BHSceneView*v,BHTextureContext*t,BHAtlasContext*a,Memory*m,unsigned long step){BHSceneAllocator allocator={allocate,release,m};DDPIXELFORMAT pixel;unsigned long i;
 if(!bh_texture_context_init(t,v->draw,v->device,&allocator)||!bh_texture_formats(t))return 0;t->palette_mode=2;t->slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
 memset(&pixel,0,sizeof(pixel));pixel.dwSize=32;pixel.dwFlags=DDPF_RGB;pixel.dwRGBBitCount=16;pixel.dwRBitMask=0xf800;pixel.dwGBitMask=0x7e0;pixel.dwBBitMask=31;for(i=0;i<9;++i)t->formats[i].pixel=pixel;
 if(!bh_texture_bind_display(t,&pixel))return 0;memset(a,0,sizeof(*a));return bh_atlas_context_init(a,t,step);
}

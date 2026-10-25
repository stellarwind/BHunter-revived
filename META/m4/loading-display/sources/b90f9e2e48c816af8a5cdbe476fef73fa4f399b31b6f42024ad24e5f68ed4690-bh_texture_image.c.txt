#include "bh_texture.h"
#include "bh_pcx_reader.h"
#include <string.h>
#include <ctype.h>

static int fail(BHTextureContext *q,BHTextureError error){if(q)q->error=error;return 0;}
static int native(BHTextureContext *q,HRESULT hr){q->last_hresult=hr;return hr==DD_OK?1:fail(q,BH_TEXTURE_BACKEND);}
int __cdecl bh_texture_set_path(BHTextureContext *q,const char *prefix,long policy) {
    unsigned long n;if(!q || !prefix || (n=strlen(prefix))>254)return fail(q,BH_TEXTURE_ARGUMENT);
    strcpy(q->prefix,prefix);q->search=-1;
    if(n) {
        if(prefix[n-1]!='\\'){q->prefix[n]='\\';q->prefix[n+1]=0;}
        if(policy>=0 && policy<3)q->search=policy;
    }
    return 1;
}
static void message(BHTextureContext *q,const char *tag,const char *name,int deduplicate) {
    unsigned long used=strlen(q->errors),a=strlen(tag),b=strlen(name);
    if(deduplicate && strstr(q->errors,name))return;
    if(used+a+b<sizeof(q->errors)){memcpy(q->errors+used,tag,a);memcpy(q->errors+used+a,name,b+1);}
}
static int canonicalize(const char *name,char *out) {
    unsigned long i,n;if(!name || !(n=strlen(name)) || n>=28)return 0;
    for(i=0;i<=n;++i)out[i]=(char)toupper((unsigned char)name[i]);
    if(n>=4 && (!_stricmp(out+n-4,".BMP") || !_stricmp(out+n-4,".GIF") || !_stricmp(out+n-4,".TGA") || !_stricmp(out+n-4,".JPG")))strcpy(out+n-4,".PCX");
    return 1;
}
static int open_reader(BHTextureContext *q,BHPcxReader *reader,const char *name) {
    char path[284];strcpy(path,q->prefix);strcat(path,name);
    switch(q->search) {
    case -1:return bh_pcx_reader_open(reader,name);
    case 0:return bh_pcx_reader_open(reader,path) || bh_pcx_reader_open(reader,name);
    case 1:return bh_pcx_reader_open(reader,name) || bh_pcx_reader_open(reader,path);
    case 2:return bh_pcx_reader_open(reader,path);
    default:return 0;
    }
}
static unsigned long packed(const unsigned char *rgb,const DDPIXELFORMAT *p) {
    unsigned long masks[3],i,mask,out=0;int shift;
    masks[0]=p->dwRBitMask;masks[1]=p->dwGBitMask;masks[2]=p->dwBBitMask;
    for(i=0;i<3;++i){shift=-8;for(mask=masks[i];mask;mask/=2)++shift;out+=((shift<0?(unsigned long)rgb[i]>>(-shift):(unsigned long)rgb[i]<<shift)&masks[i]);}
    return out;
}
static void discard(BHTextureContext *q,BHTextureImage *image) {
    if(!image)return;
    if(image->palette)IDirectDrawPalette_Release(image->palette);
    if(image->source)IDirectDrawSurface_Release(image->source);
    if(image->runs)q->allocator.release(image->runs,q->allocator.user);
    if(image->vertices)q->allocator.release(image->vertices,q->allocator.user);
    if(image->dither)q->allocator.release(image->dither,q->allocator.user);
    if(image->screen)q->allocator.release(image->screen,q->allocator.user);
    q->allocator.release(image,q->allocator.user);
}
BHTextureImage *__cdecl bh_texture_load(BHTextureContext *q,const char *name) {
    char key[28];BHTextureImage *image;BHPcxReader reader;BHPcxPalette palette;
    DDSURFACEDESC desc,lock;PALETTEENTRY entries[256];unsigned long colors[256],i,j,x,y,bytes;
    unsigned char index;int ok=1;
    if(!q || !q->allocator.allocate || !q->draw || !canonicalize(name,key)){fail(q,BH_TEXTURE_ARGUMENT);return NULL;}
    q->error=BH_TEXTURE_OK;
    for(image=q->images;image;image=image->next)if(!_stricmp(key,image->name))return image;
    image=(BHTextureImage *)q->allocator.allocate(sizeof(*image),q->allocator.user);
    if(!image){fail(q,BH_TEXTURE_MEMORY);return NULL;}
    memset(image,0,sizeof(*image));strcpy(image->name,key);image->flags=1;
    bh_pcx_reader_init(&reader);
    if(!open_reader(q,&reader,key)){fail(q,BH_TEXTURE_IO);message(q,"\nLOADERR:",key,1);goto failed;}
    bh_pcx_palette(&palette,reader.palette,q->palette_mode,q->palette_color);memcpy(reader.map,palette.indices,256);
    image->format=bh_texture_request_format(q,reader.width,reader.height,8,1);
    if(!image->format){message(q,"\nFORMATERR:",key,0);goto failed;}
    bytes=(image->format->pixel.dwRGBBitCount+7)/8;
    if(bytes<1 || bytes>4){fail(q,BH_TEXTURE_FORMAT);message(q,"\nFORMATERR:",key,0);goto failed;}
    memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);desc.dwFlags=0x1007;
    desc.dwWidth=reader.width;desc.dwHeight=reader.height;desc.ddpfPixelFormat=image->format->pixel;
    desc.ddsCaps.dwCaps=q->software?0x840:0x1800;
    if(!native(q,IDirectDraw_CreateSurface(q->draw,&desc,&image->source,NULL))){message(q,"\nALLOCERR:",key,0);goto failed;}
    if(bytes==1) {
        for(i=0;i<256;++i){entries[i].peRed=palette.rgb[i*3];entries[i].peGreen=palette.rgb[i*3+1];entries[i].peBlue=palette.rgb[i*3+2];entries[i].peFlags=0;}
        if(!native(q,IDirectDraw_CreatePalette(q->draw,0x44,entries,&image->palette,NULL)) ||
           !native(q,IDirectDrawSurface_SetPalette(image->source,image->palette)))goto failed;
        if(!bh_texture_palette_tables(q,image,(const unsigned char *)entries,1024))goto failed;
    } else for(i=0;i<256;++i)colors[i]=packed(palette.rgb+i*3,&image->format->pixel);
    memset(&lock,0,sizeof(lock));lock.dwSize=sizeof(lock);
    if(!native(q,IDirectDrawSurface_Lock(image->source,NULL,&lock,0,NULL)))goto failed;
    if(lock.lPitch<(long)(reader.width*bytes))ok=fail(q,BH_TEXTURE_TOPOLOGY);
    for(y=0;ok && y<reader.height;++y)for(x=0;x<reader.width;++x) {
        unsigned char *out=(unsigned char *)lock.lpSurface+y*lock.lPitch+x*bytes;
        if(!bh_pcx_reader_next(&reader,&index)){ok=fail(q,BH_TEXTURE_IO);break;}
        if(bytes==1)*out=index;else for(j=0;j<bytes;++j)out[j]=(unsigned char)(colors[index]>>(j*8));
    }
    if(!native(q,IDirectDrawSurface_Unlock(image->source,NULL)))ok=0;if(!ok)goto failed;
    if(!bh_pcx_reader_close(&reader)){fail(q,BH_TEXTURE_IO);goto failed;}
    image->runs=(unsigned long *)q->allocator.allocate(0x80,q->allocator.user);
    image->vertices=(D3DTLVERTEX *)q->allocator.allocate(0xc00,q->allocator.user);
    if(!image->runs || !image->vertices){fail(q,BH_TEXTURE_MEMORY);goto failed;}
    image->vertex_capacity=0x60;image->next=q->images;q->images=image;return image;
failed:
    if(reader.file)bh_pcx_reader_close(&reader);discard(q,image);return NULL;
}

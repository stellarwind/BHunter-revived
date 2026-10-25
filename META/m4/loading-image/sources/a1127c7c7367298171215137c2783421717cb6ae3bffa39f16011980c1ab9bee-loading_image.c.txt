#include "../src/bh_loading_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<18)printf("FAIL loading-image: %s\n",s);}}while(0)
typedef struct Memory {unsigned long calls,live,releases,deny;} Memory;
static void put16(unsigned char *p,unsigned long x){p[0]=(unsigned char)x;p[1]=(unsigned char)(x>>8);}
static void put32(unsigned char *p,unsigned long x){put16(p,x);put16(p+2,x>>16);}
static unsigned long get32(const unsigned char *p){return (unsigned long)p[0]|((unsigned long)p[1]<<8)|((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24);}
static void *__cdecl allocate(unsigned long bytes,void *user){Memory *m=(Memory*)user;unsigned char *p;++m->calls;if(m->calls==m->deny)return NULL;
    p=(unsigned char*)malloc(bytes+16);if(!p)return NULL;put32(p,bytes);put32(p+4,0x13572468);put32(p+8+bytes,0x24681357);
    memset(p+8,0xa5,bytes);++m->live;return p+8;}
static void __cdecl release(void *memory,void *user){Memory *m=(Memory*)user;unsigned char *p=(unsigned char*)memory-8;
    CHECK(m->live&&get32(p+4)==0x13572468&&get32(p+8+get32(p))==0x24681357,"real arena allocation guard words remain intact");
    --m->live;++m->releases;free(p);}
static unsigned char sample(unsigned long x,unsigned long y,unsigned long seed){return (unsigned char)(x*3+y*17+seed);}
static void colors(unsigned char *palette,unsigned long seed){unsigned long i;for(i=0;i<256;++i){
    palette[i*3]=(unsigned char)(i*5+seed);palette[i*3+1]=(unsigned char)(i*7+seed*3);palette[i*3+2]=(unsigned char)(i*11+seed*5);}
    palette[3]=palette[4]=palette[5]=0;palette[6]=palette[7]=palette[8]=2;}
static int fixture(const char *name,unsigned long height,unsigned long seed,unsigned long width,unsigned long limit){
    unsigned char header[128],palette[768],value;unsigned long x,y,n=0;FILE *file=fopen(name,"wb");int ok=1;
    if(!file)return 0;memset(header,0,128);header[0]=10;header[1]=5;header[2]=1;header[3]=8;
    put16(header+8,width-1);put16(header+10,height-1);header[65]=1;put16(header+66,width);colors(palette,seed);
    if(fwrite(header,1,128,file)!=128)ok=0;
    for(y=0;y<height;++y)for(x=0;x<width;++x){if(n++>=limit)goto tail;
        value=sample(x,y,seed);if(value>=192&&fputc(193,file)==EOF)ok=0;if(fputc(value,file)==EOF)ok=0;}
tail:
    if(fputc(12,file)==EOF||fwrite(palette,1,768,file)!=768)ok=0;if(fclose(file))ok=0;return ok;
}
static unsigned long crc(const unsigned char *p,unsigned long n){unsigned long c=0xffffffffUL,i;int bit;
    for(i=0;i<n;++i){c^=p[i];for(bit=0;bit<8;++bit)c=(c>>1)^(0xedb88320UL&(0UL-(c&1)));}return c^0xffffffffUL;}
static unsigned long native_color(const unsigned char *rgb,int quarter,int kind){unsigned long r=rgb[0],g=rgb[1],b=rgb[2];
    if(quarter){r/=4;g/=4;b/=4;}return kind==0?(r/8)*2048+(g/4)*32+b/8:r*65536+g*256+b;}
static unsigned char index_color(const unsigned char *rgb,int quarter){unsigned long r=rgb[0],g=rgb[1],b=rgb[2];
    if(quarter){r/=4;g/=4;b/=4;}return (unsigned char)((r*5/255)*42+(g*6/255)*6+b*5/255);}
static void tables(BHLoadingImages *c,const BHPcxPalette *palette,int kind){unsigned long i;
    CHECK(!memcmp(c->palette,palette->rgb,768)&&!memcmp(c->map,palette->indices,256),"mode0 palette and map survive owning PCX close before LUT construction");
    for(i=0;i<256;++i){if(c->pixel_bytes==1){
        CHECK(c->indexed[i]==index_color(palette->rgb+i*3,0)&&c->quarter_indexed[i]==index_color(palette->rgb+i*3,1),"all indexed normal/quarter lookup values match independent integer quantization");
    }else CHECK(c->native[i]==native_color(palette->rgb+i*3,0,kind)&&c->quarter_native[i]==native_color(palette->rgb+i*3,1,kind),"all native normal/quarter lookup values match independent packed RGB arithmetic");}
}
static void seed_inactive(BHLoadingImages *c){unsigned long i;for(i=0;i<256;++i){c->indexed[i]=(unsigned char)(i*3+7);c->quarter_indexed[i]=(unsigned char)(i*5+9);
    c->native[i]=0x12340000UL+i;c->quarter_native[i]=0x56780000UL+i;}}
static void inactive(BHLoadingImages *c){unsigned long i;for(i=0;i<256;++i)if(c->pixel_bytes==1){
    CHECK(c->native[i]==0x12340000UL+i&&c->quarter_native[i]==0x56780000UL+i&&c->midas_native[i]==0x12340000UL+i&&c->iel_native[i]==0x12340000UL+i,"inactive native payload remains unchanged and is still copied to both retained slots");
    }else CHECK(c->indexed[i]==(unsigned char)(i*3+7)&&c->quarter_indexed[i]==(unsigned char)(i*5+9)&&
        c->midas_indexed[i]==(unsigned char)(i*3+7)&&c->iel_indexed[i]==(unsigned char)(i*3+7),"inactive indexed payload remains unchanged and is still copied to both retained slots");}
static void synthetic(void){
    const char *names[3]={"loading-midas.pcx","loading-iel.pcx","loading-bad.pcx"};
    unsigned long right[3]={3,2,3},left[3]={11,5,0},widths[6]={640,320,800,0,640,320},depths[6]={1,2,3,4,2,1};
    Memory memory={0,0,0,0};BHSceneAllocator allocator={allocate,release,&memory};BHLoadingImages c;
    BHPcxPalette palettes[3];unsigned char rgb[768],*expected,*old;unsigned long i,j,k,x,y,at,n,reads,before,after,old_deadline,old_stride;int kind;
    for(i=0;i<3;++i){CHECK(fixture(names[i],3,i*13,640,1920),"owned odd-height640-wide PCX fixture");colors(rgb,i*13);CHECK(bh_pcx_palette(palettes+i,rgb,0,0),"actual admitted palette transformation reference");}
    CHECK(fixture("loading-short.pcx",4,0,640,10)&&fixture("loading-width.pcx",3,0,639,1917),"owned truncated-stream and dimension guards");
    expected=(unsigned char*)malloc(0x12c000UL);CHECK(expected!=NULL,"independent expected arena allocation");if(!expected)goto done;
    for(k=0;k<6;++k){kind=k>=2&&k!=4;
        if(kind){right[0]=right[1]=right[2]=32;left[0]=48;left[1]=40;left[2]=32;}
        else{right[0]=3;right[1]=2;right[2]=3;left[0]=11;left[1]=5;left[2]=0;}
        CHECK(bh_loading_images_bind(&c,widths[k],depths[k],right,left,&allocator),"fresh explicit display/allocator binding without invented device");seed_inactive(&c);
        CHECK(!bh_loading_image_load(&c,names[0],0)&&c.error==BH_LOADING_IMAGE_ARGUMENT&&!c.loads&&!memory.live,
              "original absent-arena early return performs no allocation or image publication");
        for(i=0;i<3;++i)c.names[i]=names[i];c.redraw_count=99;c.stage=1234;
        before=GetTickCount();CHECK(bh_loading_images_init(&c),"complete three-image initializer");after=GetTickCount();
        CHECK(c.arena_bytes==0x12c000UL&&c.redraw_count==2&&c.stage==0&&c.tick_calls==1&&
            (unsigned long)(c.deadline-5000UL-before)<=(unsigned long)(after-before),"actual late GetTickCount+5000 unsigned bound/redraw-count/stage publication");
        CHECK(c.frame_stride==(widths[k]==640?0x4b000UL:0x12c00UL),"page stride stays fixed480/240 rows even odd-height source image");
        memset(expected,0xa5,0x12c000UL);reads=0;
        for(i=0;i<3;++i){at=i==0?0x96000UL:i==1?0xe1000UL:0;n=widths[k]==640?3:1;
            for(y=0;y<n;++y)for(x=0;x<(widths[k]==640?640UL:320UL);++x)
                expected[at++]=palettes[i].indices[sample(widths[k]==640?x:x*2,widths[k]==640?y:y*2,i*13)];
            reads+=widths[k]==640?1920:1280;
            for(j=0;j<256;++j){if(depths[k]==1){unsigned char *retained=i==0?c.midas_indexed:c.iel_indexed;
                if(i<2)CHECK(retained[j]==index_color(palettes[i].rgb+j*3,0),"retained MIDAS/IEL indexed palettes keep their own image colors");
            }else if(i<2){unsigned long *retained=i==0?c.midas_native:c.iel_native;
                CHECK(retained[j]==native_color(palettes[i].rgb+j*3,0,kind),"retained MIDAS/IEL native palettes keep their own image colors");}}
        }
        CHECK(!memcmp(c.arena,expected,0x12c000UL)&&c.pixels_read==reads,"all image offsets, remapped pixels, half-pair consumption and untouched arena gaps match independent reference");
        tables(&c,palettes+2,kind);inactive(&c);
        if(depths[k]!=1){
            for(i=0;i<3;++i){c.right[i]=32;c.left[i]=32;}
            CHECK(bh_loading_image_load(&c,names[2],0)&&c.native[2]==24&&c.quarter_native[2]==6,
                  "native color channels add rather than OR and shifts use original low-five-bit masking");
        }
        old=c.arena;old_deadline=c.deadline;old_stride=c.frame_stride;memory.deny=memory.calls+1;
        CHECK(!bh_loading_images_init(&c)&&c.error==BH_LOADING_IMAGE_MEMORY&&c.arena==old&&memory.live==1&&c.deadline==old_deadline,"allocation failure retains established owning arena and timing");memory.deny=0;
        c.names[1]="missing-loading-image.pcx";
        CHECK(!bh_loading_images_init(&c)&&c.arena==old&&memory.live==1&&c.deadline==old_deadline&&!memcmp(c.arena,expected,0x12c000UL),"mid-sequence file failure releases new arena and preserves previous image pixels/tables");c.names[1]=names[1];
        before=c.pixels_read;c.arena_bytes=10;
        CHECK(!bh_loading_image_load(&c,names[0],0)&&c.error==BH_LOADING_IMAGE_SPAN&&c.pixels_read==before,"insufficient destination span closes owning reader before any pixel write");c.arena_bytes=0x12c000UL;
        CHECK(!bh_loading_image_load(&c,"loading-width.pcx",0)&&c.error==BH_LOADING_IMAGE_DIMENSION,"explicit640-source layout guard");
        CHECK(!bh_loading_image_load(&c,"loading-short.pcx",0)&&c.error==BH_LOADING_IMAGE_STREAM&&c.frame_stride==old_stride&&!memcmp(c.palette,palettes[2].rgb,768),"truncated decode closes stream and retains published palette/stride despite documented partial pixels");
        bh_loading_images_dispose(&c);CHECK(!memory.live,"all current and failed replacement arena ownership freed");
    }
    free(expected);
done:
    for(i=0;i<3;++i)remove(names[i]);remove("loading-short.pcx");remove("loading-width.pcx");bh_vfile_shutdown();
}
static void real_images(const char *archive){
    const char *names[3]={"TEXTURES\\MIDAS.PCX","TEXTURES\\IEL.PCX","TEXTURES\\BAD.PCX"};
    unsigned long expected_crc[3]={202574826UL,2972740151UL,185827195UL};
    unsigned long rgb_crc[3]={718118636UL,2017339188UL,4207020760UL};
    unsigned long raw_crc[3]={2155497038UL,3019750310UL,4262433660UL};
    BHPcx images[3];BHPcxPalette palettes[3];BHVFile *file;BHLoadingImages c;Memory memory={0,0,0,0};
    BHSceneAllocator allocator={allocate,release,&memory};unsigned long right[3],left[3],mode,i,j,x,y,at,rows,bytes;
    unsigned char *expected=NULL,*mapped=NULL;int ready=1;
    for(i=0;i<3;++i)bh_pcx_init(images+i,NULL);
    CHECK(bh_vfile_mount(archive),"actual immutable archive mounted as data only");
    for(i=0;i<3;++i){file=bh_vfile_open(names[i],"rb");CHECK(file!=NULL,"independent bounded whole PCX reference opened");
        if(!file){ready=0;break;}CHECK(bh_pcx_read(images+i,file),"existing whole-image source decoder provides independent pixel reference");bh_vfile_close(file);
        if(!images[i].pixels){ready=0;break;}CHECK(crc(images[i].pixels,images[i].pixel_bytes)==raw_crc[i],"real source pixels match pinned independent original-data CRC");
        CHECK(bh_pcx_palette(palettes+i,images[i].palette,0,0)&&crc(palettes[i].rgb,768)==rgb_crc[i],"mode0 palette matches pinned independent transformation CRC");
        mapped=(unsigned char*)malloc(images[i].pixel_bytes);if(!mapped){ready=0;break;}
        for(j=0;j<images[i].pixel_bytes;++j)mapped[j]=palettes[i].indices[images[i].pixels[j]];
        CHECK(crc(mapped,images[i].pixel_bytes)==expected_crc[i],"entire mapped image matches pinned independent mode0 CRC");free(mapped);mapped=NULL;
    }
    if(!ready)goto done;expected=(unsigned char*)malloc(0x12c000UL);CHECK(expected!=NULL,"independent complete real-image arena reference");if(!expected)goto done;
    for(mode=0;mode<6;++mode){
        if(mode%3==1){right[0]=3;right[1]=2;right[2]=3;left[0]=11;left[1]=5;left[2]=0;}
        else{right[0]=right[1]=right[2]=0;left[0]=16;left[1]=8;left[2]=0;}
        CHECK(bh_loading_images_bind(&c,mode<3?640:320,mode%3==0?1:mode%3==1?2:4,right,left,&allocator),"actual full/half indexed/RGB565/RGB888 source bindings");seed_inactive(&c);
        CHECK(bh_loading_images_init(&c),"whole original initializer loads actual MIDAS IEL BAD assets at original offsets");
        if(!c.arena){bh_loading_images_dispose(&c);continue;}
        memset(expected,0xa5,0x12c000UL);
        for(i=0;i<3;++i){at=i==0?0x96000UL:i==1?0xe1000UL:0;rows=mode<3?images[i].height:images[i].height/2;
            bytes=mode<3?640:320;
            for(y=0;y<rows;++y)for(x=0;x<bytes;++x)expected[at++]=palettes[i].indices[images[i].pixels[(mode<3?y:y*2)*640+(mode<3?x:x*2)]];
            for(j=0;j<256&&i<2;++j){if(c.pixel_bytes==1)CHECK((i==0?c.midas_indexed:c.iel_indexed)[j]==index_color(palettes[i].rgb+j*3,0),"all retained real indexed colors match source-specific palette");
                else CHECK((i==0?c.midas_native:c.iel_native)[j]==native_color(palettes[i].rgb+j*3,0,mode%3==2),"all retained real native colors match source-specific palette");}
        }
        CHECK(!memcmp(expected,c.arena,0x12c000UL)&&c.pixels_read==1228800UL&&c.pixels_written==(mode<3?1228800UL:307200UL),"complete real full/half arena pixels, original offsets, gaps and read/write totals");
        tables(&c,palettes+2,mode%3==2);inactive(&c);bh_loading_images_dispose(&c);CHECK(!memory.live,"actual image arena disposed without leaked replacement storage");
    }
done:
    if(mapped)free(mapped);if(expected)free(expected);for(i=0;i<3;++i)bh_pcx_dispose(images+i);bh_vfile_shutdown();
}
int bh_loading_image_checkpoint(const char *archive,unsigned long *count){checks=failures=0;synthetic();if(archive)real_images(archive);if(count)*count=checks;
    printf("{\"scenario\":\"loading-image\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false,\"ui_capture\":false}\n",checks,failures);return failures!=0;}
#ifdef BH_LOADING_IMAGE_STANDALONE
int main(int argc,char **argv){return bh_loading_image_checkpoint(argc>1?argv[1]:NULL,NULL);}
#endif

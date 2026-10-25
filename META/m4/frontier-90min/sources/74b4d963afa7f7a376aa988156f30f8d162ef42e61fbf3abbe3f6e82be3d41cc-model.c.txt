#include "../src/bh_model.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<15)printf("FAIL model %d: %s\n",line,label);}}
typedef struct Memory {unsigned long calls,fail_at,live;} Memory;
static void *__cdecl allocate(unsigned long n,void *user){Memory *m=(Memory *)user;void *p;++m->calls;if(m->calls==m->fail_at)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){Memory *m=(Memory *)user;--m->live;free(p);}
static void word(unsigned char *p,unsigned long n){memcpy(p,&n,4);}
static void scalar(unsigned char *p,float n){memcpy(p,&n,4);}
static unsigned long bits(float n){unsigned long r;memcpy(&r,&n,4);return r;}
static int save(const char *name,const unsigned char *p,unsigned long n){FILE *f=fopen(name,"wb");int ok;if(!f)return 0;ok=fwrite(p,1,n,f)==n;return !fclose(f) && ok;}
static unsigned char fixture[272];
static void make(float radius) {
    memset(fixture,0,sizeof(fixture));word(fixture,3);
    strcpy((char *)fixture+13,"UNUSED.GIF");strcpy((char *)fixture+45,"MODELIMG.GIF");
    word(fixture+100,1);memset(fixture+116,0x78,48);
    word(fixture+164,1);word(fixture+172,0xffffffffUL);
    word(fixture+184,1);word(fixture+188,1);word(fixture+196,0xdeadbeefUL);
    word(fixture+200,1);scalar(fixture+216,radius);scalar(fixture+228,1);
    word(fixture+244,0xffffffffUL);word(fixture+248,0xffffffffUL);word(fixture+252,0xffffffffUL);word(fixture+256,0xffffffffUL);
    word(fixture+260,1);word(fixture+268,0xffffffffUL);
}
static int image_fixture(void) {
    unsigned char data[2048];unsigned long at=128,left=1024;memset(data,0,sizeof(data));
    data[0]=10;data[1]=5;data[2]=1;data[3]=8;data[8]=31;data[10]=31;data[65]=1;data[66]=32;
    while(left){unsigned long n=left>63?63:left;data[at++]=(unsigned char)(0xc0|n);data[at++]=1;left-=n;}
    data[at++]=12;data[at+3]=255;return save("MODELIMG.PCX",data,at+768);
}
static int initialized(const BHModel *m,float r) {
    unsigned long i,j;if(!m || m->frame || m->radius!=r)return 0;
    for(i=0;i<3;++i)for(j=0;j<4;++j)if(m->transform.m[i][j]!=(float)(i==j))return 0;
    for(i=0;i<5;++i)if(m->runtime[i])return 0;return 1;
}
static void synthetic(BHSceneView *view) {
    BHTextureContext textures;BHModelContext c;BHSceneAllocator a;Memory memory={0,0,0};
    BHModel *m,*second;BHTextureImage *image;BHVFile *f;unsigned long i,calls,live;char long_name[257];
    a.allocate=allocate;a.release=release;a.user=&memory;
    CHECK(bh_texture_context_init(&textures,view->draw,view->device,NULL) && bh_texture_formats(&textures),"real native model image dependency");
    CHECK(bh_model_context_init(&c,&textures,&a),"explicit model context");
    CreateDirectoryA("bhmodel-prefix",NULL);make(6);
    CHECK(save("MODEL.BSP",fixture,sizeof(fixture)) && image_fixture(),"owned model and actual PCX fixtures");make(8);
    CHECK(save("bhmodel-prefix\\MODEL.BSP",fixture,sizeof(fixture)),"distinguishable prefixed model");
    for(i=0;i<4;++i) {
        bh_model_set_path(&c,"bhmodel-prefix\\",(BHBspSearch)((long)i-1));
        m=bh_model_load(&c,"MODEL.BSP");
        CHECK(initialized(m,i==1 || i==3?8:6),"all four policies select correct model with zero runtime and identity transform");
        CHECK(m && m->material_count==3 && m->vertex_count==1 && m->face_count==1 && m->nodes && m->leaves,"all six original owned sections");
        CHECK(m && !m->images[0] && m->images[1] && !m->images[2] && !strstr(textures.errors,"UNUSED"),"only used nonempty material loads an actual image");
        CHECK(m && m->faces[0].runtime==(long)0xdeadbeefUL && m->vertices[0].opaque[47]==0x78,"raw runtime bytes preserved without collision-loader reset");
        image=m?m->images[1]:NULL;second=bh_model_load(&c,"MODEL.BSP");
        CHECK(second && second!=m && second->vertices!=m->vertices && second->images[1]==image,"models independently own geometry but share true image-cache identity");
        bh_model_delete(&c,m);CHECK(second && second->images[1]==image && image->source,"deleting one model retains borrowed cache/native image");
        bh_model_delete(&c,second);CHECK(!memory.live,"all original model arrays and object released");
    }
    CHECK(save("bhmodel-prefix\\MODEL.BSP",fixture,7),"malformed preferred model");bh_model_set_path(&c,"bhmodel-prefix\\",BH_BSP_PREFIX_FIRST);
    CHECK(!bh_model_load(&c,"MODEL.BSP") && c.error==BH_BSP_TRUNCATED && !memory.live,"opened malformed preferred model does not fall back to valid root");
    remove("bhmodel-prefix\\MODEL.BSP");m=bh_model_load(&c,"MODEL.BSP");CHECK(initialized(m,6),"open failure permits preferred-path fallback");
    f=bh_vfile_open("MODEL.BSP","rb");CHECK(f!=NULL,"caller-owned VFILE for transactional replacement");
    live=memory.live;calls=memory.calls;memory.fail_at=calls+1;
    CHECK(!bh_model_read(&c,f,m) && initialized(m,6) && memory.live==live,"failed validation allocation retains published model");
    memory.fail_at=0;bh_vfile_close(f);bh_model_delete(&c,m);
    /* Cached PCX makes every remaining allocation belong to model construction.
       Find the successful allocation count, then fail each step independently. */
    calls=memory.calls;m=bh_model_load(&c,"MODEL.BSP");calls=memory.calls-calls;bh_model_delete(&c,m);
    for(i=1;i<=calls;++i) {
        memory.fail_at=memory.calls+i;m=bh_model_load(&c,"MODEL.BSP");
        CHECK(!m && c.error==BH_BSP_MEMORY && !memory.live,"each nested model allocation failure cleans unpublished arrays/object and closes file");
        memory.fail_at=0;
    }
    make(8);CHECK(save("bhmodel-prefix\\MODEL.BSP",fixture,sizeof(fixture)),"restore valid prefix for malformed bare-first control");
    CHECK(save("MODEL.BSP",fixture,7),"malformed opened root");bh_model_set_path(&c,"bhmodel-prefix\\",BH_BSP_BARE_FIRST);
    CHECK(!bh_model_load(&c,"MODEL.BSP") && c.error==BH_BSP_TRUNCATED && !memory.live,"opened malformed root does not fall back to valid prefix");
    remove("MODEL.BSP");m=bh_model_load(&c,"MODEL.BSP");CHECK(initialized(m,8),"bare-first open failure falls back to prefix");
    make(6);CHECK(save("MODEL.BSP",fixture,sizeof(fixture)),"valid direct reader replacement");f=bh_vfile_open("MODEL.BSP","rb");
    CHECK(f && bh_model_read(&c,f,m) && initialized(m,6) && bh_vfile_tell(f)==272,"successful direct reader replaces model and keeps caller file open at end");
    if(f)bh_vfile_close(f);bh_model_delete(&c,m);
    bh_model_set_path(&c,"missing-model-prefix\\",BH_BSP_PREFIX_ONLY);
    CHECK(!bh_model_load(&c,"MODEL.BSP") && c.error==BH_BSP_IO && !memory.live,"prefix-only never falls back to available bare model");
    bh_model_set_path(&c,"bhmodel-prefix\\",BH_BSP_BARE_ONLY);
    memset(long_name,'X',256);long_name[256]=0;CHECK(!bh_model_load(&c,long_name) && c.error==BH_BSP_LIMIT && !memory.live,"bounded source path guard");
    /* Successful model with unavailable used image is still published. */
    make(6);strcpy((char *)fixture+45,"MISSING.GIF");CHECK(save("MODEL.BSP",fixture,sizeof(fixture)),"missing image model fixture");
    m=bh_model_load(&c,"MODEL.BSP");CHECK(m && !m->images[1] && c.error==BH_BSP_OK && textures.error==BH_TEXTURE_IO,"missing used image stays NULL without failing model owner");bh_model_delete(&c,m);
    make(6);fixture[45]=0;CHECK(save("MODEL.BSP",fixture,sizeof(fixture)),"used empty texture fixture");
    m=bh_model_load(&c,"MODEL.BSP");CHECK(m && !m->images[1],"empty used material retains NULL");bh_model_delete(&c,m);
    CHECK(!memory.live,"synthetic ownership balanced");bh_texture_context_dispose(&textures);
    remove("MODEL.BSP");remove("MODELIMG.PCX");remove("bhmodel-prefix\\MODEL.BSP");RemoveDirectoryA("bhmodel-prefix");bh_vfile_shutdown();
}
typedef struct Manifest {const char *name;unsigned long offset,size,crc,counts[6],bounds_crc,root;int sidecar;unsigned long script_bytes,script_crc;} Manifest;
static const Manifest manifest[]={
#include "bsp_manifest.h"
};
static void real_assets(BHSceneView *view,const char *archive) {
    BHTextureContext textures;BHModelContext c;BHSceneAllocator a;Memory memory={0,0,0};BHModel *m;
    BHTextureImage *image;FILE *bin;unsigned char *raw;const void *sections[6];unsigned long strides[6]={32,60,16,12,56,8};
    unsigned long i,j,k,at,used,loaded=0,bindings=0,missing=0;
    a.allocate=allocate;a.release=release;a.user=&memory;
    CHECK(bh_vfile_mount(archive),"actual original archive as data");
    bh_texture_context_init(&textures,view->draw,view->device,NULL);CHECK(bh_texture_formats(&textures) && bh_texture_set_path(&textures,"TEXTURES",2),"real archive texture owner");
    bh_model_context_init(&c,&textures,&a);bin=fopen(archive,"rb");CHECK(bin!=NULL,"independent raw asset read");
    for(i=0;i<505 && bin;++i) {
        const Manifest *f=manifest+i;m=bh_model_load(&c,f->name);CHECK(m!=NULL,"all 505 original BSPs construct complete render models");if(!m)continue;++loaded;
        CHECK(m->material_count==f->counts[0] && m->vertex_count==f->counts[1] && m->edge_count==f->counts[2] && m->face_count==f->counts[3] && m->node_count==f->counts[4] && m->leaf_count==f->counts[5],"all independent manifest section counts match actual model");
        CHECK(bits(m->radius)==f->root && initialized(m,m->radius),"independently pinned original root radius and initialized model state");
        sections[0]=m->materials;sections[1]=m->vertices;sections[2]=m->edges;sections[3]=m->faces;sections[4]=m->nodes;sections[5]=m->leaves;
        raw=(unsigned char *)malloc(f->size);CHECK(raw && !fseek(bin,f->offset,SEEK_SET) && fread(raw,1,f->size,bin)==f->size,"independent raw archive sections");
        if(raw){at=0;for(j=0;j<6;++j){at+=4;CHECK(!memcmp(sections[j],raw+at,f->counts[j]*strides[j]),"original model arrays retain all raw bytes including runtime/cache records");at+=f->counts[j]*strides[j];}free(raw);}
        for(j=0;j<m->material_count;++j) {
            used=0;for(k=0;k<m->face_count;++k)if(m->faces[k].material==(long)j){used=1;break;}
            if(!used || !m->materials[j].texture[0])CHECK(!m->images[j],"unused/empty real material has no image");
            else {image=bh_texture_load(&textures,m->materials[j].texture);CHECK(image==m->images[j],"real used material points to actual canonical cache owner");if(image)++bindings;else ++missing;}
        }
        bh_model_delete(&c,m);CHECK(!memory.live,"real model lifetime balances all own allocations");
    }
    if(bin)fclose(bin);CHECK(loaded==505 && bindings>0,"complete asset model family beyond shell-only view");
    printf("{\"render_models_loaded\":%lu,\"material_image_bindings\":%lu,\"null_used_images\":%lu}\n",loaded,bindings,missing);
    bh_texture_context_dispose(&textures);bh_vfile_shutdown();
}
int bh_model_checkpoint(const char *archive,unsigned long *count) {
    BHSceneView view;BHMathEnvironment fp;checks=0;failures=0;
    if(!bh_math_environment_enter(&fp))return 2;memset(&view,0,sizeof(view));
    CHECK(bh_scene_view_init(&view,64,64),"actual native model image environment");
    if(view.device){synthetic(&view);if(archive)real_assets(&view,archive);}
    bh_scene_view_dispose(&view);bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"render-model-owner\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}

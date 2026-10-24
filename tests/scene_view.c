#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) do {++checks;if(!(x)){++failures;printf("FAIL scene view %d: %s\n",__LINE__,label);}} while(0)
static unsigned long pixel(const char *path,unsigned long x,unsigned long y) {
    BITMAPFILEHEADER file;BITMAPINFOHEADER info;unsigned char bgr[3];FILE *f=fopen(path,"rb");
    unsigned long stride,color=0xffffffffUL;
    if(!f)return color;
    if(fread(&file,sizeof(file),1,f)==1 && fread(&info,sizeof(info),1,f)==1 && file.bfType==0x4d42 && info.biBitCount==24) {
        stride=(info.biWidth*3+3)&~3UL;
        if(!fseek(f,file.bfOffBits+(info.biHeight-y-1)*stride+x*3,SEEK_SET) && fread(bgr,3,1,f)==1)
            color=((unsigned long)bgr[2]<<16)|((unsigned long)bgr[1]<<8)|bgr[0];
    }
    fclose(f);return color;
}
int main(void) {
    BHSceneView view;BHMathEnvironment fp;unsigned long i,before;
    BHVec3 near_triangle[3]={{-.5f,-.5f,1},{.5f,-.5f,1},{0,.5f,1}};
    BHVec3 far_triangle[3]={{-1,-1,2},{1,-1,2},{0,1,2}};
    BHVec3 behind[3]={{-1,-1,-2},{1,-1,-2},{0,1,-2}};
    BHVec3 crossing[3]={{-2,-2,.1f},{2,-2,2},{0,2,2}};
    if(!bh_math_environment_enter(&fp))return 2;
    for(i=0;i<2;++i) {
        memset(&view,0,sizeof(view));CHECK(bh_scene_view_init(&view,64,64),"native offscreen color/depth/device/viewport owners");
        if(!view.device){bh_scene_view_dispose(&view);break;}
        CHECK(bh_scene_view_begin(&view) && !bh_scene_view_begin(&view),"one native BeginScene; reject nested begin");
        CHECK(bh_scene_view_polygon(&view,near_triangle,3,0x00ff00),"near green analytic triangle");
        CHECK(bh_scene_view_polygon(&view,far_triangle,3,0xff0000),"far red identical footprint submitted later");
        before=view.triangles;
        CHECK(bh_scene_view_polygon(&view,behind,3,0xffffff) && view.triangles==before,"behind-camera polygon discarded");
        CHECK(!bh_scene_view_polygon(&view,near_triangle,2,0xffffff),"invalid polygon rejects before native draw");
        CHECK(bh_scene_view_end(&view,"depth-control.bmp"),"native EndScene, read-only Lock, BMP output");
        CHECK(pixel("depth-control.bmp",32,32)==0x00ff00,"native z-buffer preserves nearer color despite later farther draw");
        CHECK(pixel("depth-control.bmp",0,0)==0,"off-triangle pixel remains clear");
        CHECK(!bh_scene_view_end(&view,"invalid.bmp"),"unpaired end rejected");
        CHECK(bh_scene_view_begin(&view) && bh_scene_view_polygon(&view,crossing,3,0x0000ff),"polygon crossing near and screen planes clips before projection");
        CHECK(bh_scene_view_end(&view,"clip-control.bmp") && view.triangles>0 && view.pixels>0,"clipped triangle produces bounded native pixels");
        CHECK(pixel("clip-control.bmp",32,32)==0x0000ff,"clipped polygon center has expected blue color");
        bh_scene_view_dispose(&view);
        CHECK(!view.device && !view.color && !view.depth && !view.viewport && !view.draw,"dispose clears all interface owners before repeat initialization");
        bh_scene_view_dispose(&view);
    }
    bh_math_environment_leave(&fp);
    printf("{\"scenario\":\"scene-view-controls\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures?1:0;
}

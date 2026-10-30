#include "../../src/bh_video_layout.h"
#include "../../src/bh_main_camera.h"
#include "../../src/bh_fp.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int close_to(float a, float b) { return fabs(a - b) < .001; }
int main(void)
{
    static const int sizes[][2] = {{640,480},{1280,960},{1920,1080},{1920,1200},
                                  {1280,1024},{2560,1080},{3440,1440},{3840,1080},
                                  {1000,700},{700,1000}};
    BHMathEnvironment fp;
    BHViewConfig view;
    BHMainCamera camera, original;
    BHViewCamera model, original_model;
    unsigned i;
    float margin_x,margin_y;
    assert(bh_math_environment_enter(&fp));
    video_layout_canvas(4.0f/3.0f,&margin_x,&margin_y);
    assert(margin_x==0 && margin_y==0);
    assert(bh_view_config_init(&view,640,480));
    memset(&camera,0,sizeof(camera));
    assert(bh_main_camera_pose(&view,&camera,0,0,0,0,0,0));
    assert(bh_view_camera_build(&view,&camera.view,&original_model));
    original=camera;
    for(i=0;i<sizeof(sizes)/sizeof(sizes[0]);++i) {
        float mx,my,width,height,scale,screen_x,screen_y;
        video_layout_canvas((float)sizes[i][0]/sizes[i][1],&mx,&my);
        width=640+2*mx; height=480+2*my;
        scale=sizes[i][0]/width;
        assert(fabs(scale-sizes[i][1]/height)<.001);
        /* Square HUD items and equal row spacing keep the same scale on both axes. */
        screen_x=100*scale; screen_y=100*sizes[i][1]/height;
        assert(close_to(screen_x,screen_y));
        view.margin_x=mx;view.margin_y=my;
        assert(bh_view_clip_rect(&view,-10000,-10000,20000,20000));
        assert(close_to(view.left,-mx) && close_to(view.right,640+mx));
        assert(close_to(view.top,-my) && close_to(view.bottom,480+my));
        assert(bh_main_camera_pose(&view,&camera,0,0,0,0,0,0));
        assert(bh_view_camera_build(&view,&camera.view,&model));
        /* World and attached cockpit/HUD keep identical focal scale and center. */
        assert(!memcmp(&camera.projected,&original.projected,sizeof(camera.projected)));
        assert(!memcmp(&model.projected,&original_model.projected,sizeof(model.projected)));
        if(mx>0) {
            float point=(320+mx*.5f)/camera.focal;
            assert(original.planes[2].normal.x*point+original.planes[2].normal.z>0);
            assert(camera.planes[2].normal.x*point+camera.planes[2].normal.z<0);
            assert(model.planes[BH_VIEW_RIGHT].normal.x*point+model.planes[BH_VIEW_RIGHT].normal.z<0);
        }
        if(my>0) {
            float point=(240+my*.5f)/camera.focal;
            assert(original.planes[5].normal.y*point+original.planes[5].normal.z>0);
            assert(camera.planes[5].normal.y*point+camera.planes[5].normal.z<0);
        }
    }
    assert(video_layout_aspect_matches(1366,768,16,9));
    assert(video_layout_aspect_matches(3440,1440,21,9));
    assert(!video_layout_aspect_matches(1920,1080,4,3));
    assert(!video_layout_aspect_matches(1920,1200,16,9));
    bh_math_environment_leave(&fp);
    puts("Video layout: ten aspect ratios/window sizes; square UI, spacing, stable focal scale, expanded frusta and resolution filters passed");
    return 0;
}

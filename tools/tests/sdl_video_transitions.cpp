/* Run the real adapter in a hidden window; no desktop automation or settings
   writes. Internal mode changes let this cover same-target aspect transitions
   as well as target reallocation, without changing the user's display. */
#ifndef BH_VIDEO_ADAPTER_SOURCE
#define BH_VIDEO_ADAPTER_SOURCE "../../src/bh_sdl_gpu.cpp"
#endif
#include BH_VIDEO_ADAPTER_SOURCE
static void draw_quad(float left,float top,float right,float bottom,float z,unsigned color)
{
    states[D3DRENDERSTATE_TEXTUREHANDLE]=0;states[D3DRENDERSTATE_ALPHABLENDENABLE]=0;
    states[D3DRENDERSTATE_ZFUNC]=D3DCMP_GREATEREQUAL;states[D3DRENDERSTATE_ZWRITEENABLE]=1;
    states[D3DRENDERSTATE_CULLMODE]=D3DCULL_NONE;states[D3DRENDERSTATE_FOGENABLE]=0;
    D3DTLVERTEX quad[4]{};const float xy[]={left,top,right,top,right,bottom,left,bottom};
    for(int i=0;i<4;++i) {quad[i].sx=xy[i*2];quad[i].sy=xy[i*2+1];quad[i].sz=z;quad[i].rhw=1;quad[i].color=color;}
    bh_IDirect3DDevice2_DrawPrimitive(nullptr,D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,quad,4,0);
}
static std::vector<unsigned> presented(unsigned width,unsigned height)
{
    /* Exercise the exact production swapchain blit in a downloadable GPU
       texture. No screenshots, desktop mode changes or settings writes. */
    if(!flush() || !ensure_commands()) return {};
    end_pass();
    auto *output=make_texture(width,height,SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,SDL_GPU_TEXTUREUSAGE_COLOR_TARGET);
    SDL_GPUTransferBufferCreateInfo info{SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,width*height*4,0};
    auto *transfer=SDL_CreateGPUTransferBuffer(device,&info);
    if(!output || !transfer) return {};
    present_target(output,width,height);
    auto *copy=SDL_BeginGPUCopyPass(commands);
    SDL_GPUTextureRegion src{output,0,0,0,0,0,width,height,1};
    SDL_GPUTextureTransferInfo dst{transfer,0,width,height};
    SDL_DownloadFromGPUTexture(copy,&src,&dst);SDL_EndGPUCopyPass(copy);
    auto *fence=SDL_SubmitGPUCommandBufferAndAcquireFence(commands);commands=nullptr;
    std::vector<unsigned> pixels;
    if(fence && SDL_WaitForGPUFences(device,true,&fence,1)) {
        auto *data=(unsigned*)SDL_MapGPUTransferBuffer(device,transfer,false);
        if(data) {pixels.assign(data,data+width*height);SDL_UnmapGPUTransferBuffer(device,transfer);}
    }
    if(fence) SDL_ReleaseGPUFence(device,fence);
    SDL_ReleaseGPUTransferBuffer(device,transfer);SDL_ReleaseGPUTexture(device,output);
    return pixels;
}
int main()
{
    if (!bh_sdl_open(1, nullptr, nullptr)) {
        std::fprintf(stderr, "Video transition init: %s\n", bh_sdl_error());
        return 1;
    }
    const auto available_modes=modes;
    modes={{1920,1080},{640,480}};selected_resolution=1;selected_aspect=0;
    finish_modes(1,0,0);
    if(selected_aspect!=1 || modes[selected_resolution]!=std::make_pair(1920,1080)) {
        std::fprintf(stderr,"Version 1 display index migration failed\n");bh_sdl_close();return 1;
    }
    modes={{640,480}};selected_resolution=999;selected_aspect=1;
    finish_modes(3,2048,1152);
    if(modes[selected_resolution]!=std::make_pair(2048,1152)) {
        std::fprintf(stderr,"Saved render resolution lost after monitor change\n");bh_sdl_close();return 1;
    }
    modes=available_modes;selected_resolution=0;
    mode = 1;
    for (int aspect=0;aspect<3;++aspect) {
        selected_aspect=aspect;select_matching_mode();
        int first=selected_resolution, count=0;
        do {
            if (!matching_mode(selected_resolution) || !std::strcmp(bh_sdl_video_label(2),"Unavailable")) {
                std::fprintf(stderr,"Missing Borderless resolution group %s\n",aspects[aspect].label);
                hidden=true;bh_sdl_close();return 1;
            }
            ++count;bh_sdl_video_cycle(2);
        } while (selected_resolution!=first && count<=(int)modes.size());
        if(count<2 || count>(int)modes.size()) {
            std::fprintf(stderr,"Broken Borderless resolution cycling\n");
            hidden=true;bh_sdl_close();return 1;
        }
        std::printf("Borderless %s: %d render-resolution choices\n",aspects[aspect].label,count);
    }
    /* Disable capture overrides, but leave the native window hidden. */
    hidden = false;
    modes = {{640,480},{1280,960},{960,540},{1920,1080},{1680,720},{2560,1080}};
    selected_resolution=selected_aspect=0;
    float original_mx,original_my;canvas_margins(original_mx,original_my);
    if(original_mx!=0 || original_my!=0) {
        std::fprintf(stderr,"4:3 acquired a rounding margin\n");hidden=true;bh_sdl_close();return 1;
    }
    const unsigned outputs[][2] = {{960,540},{1260,540},{720,540}};
    int checks = 0, failures = 0;
    for (int old_aspect = 0; old_aspect < 3; ++old_aspect)
        for (int new_aspect = 0; new_aspect < 3; ++new_aspect)
            for (int resolution = 0; resolution < 2; ++resolution) {
                selected_resolution = old_aspect*2 + (1-resolution);
                selected_aspect = old_aspect;
                if (!clear_target(true, true, 0xff0000)) { ++failures; continue; }
                draw_quad(200,150,440,330,.9f,0xffff0000);
                readback();
                selected_resolution = new_aspect*2 + resolution;
                selected_aspect = new_aspect;
                /* Force the invalidation path to clear BOTH attachments. A
                   stale .9 depth must not hide the new .5 foreground. */
                if (!begin_pass()) { ++failures; continue; }
                draw_quad(200,150,440,330,.5f,0xff00ff00);
                auto after = readback();
                float mx,my; canvas_margins(mx,my);
                ++checks;
                if (target_w != (unsigned)modes[selected_resolution].first ||
                    target_h != (unsigned)modes[selected_resolution].second ||
                    std::fabs((640+2*mx)/(480+2*my)-(float)target_w/target_h)>.0001f ||
                    after.empty() || std::any_of(after.begin(),after.end(),[](unsigned p){return (p&0xffffff)==255;}) ||
                    std::none_of(after.begin(),after.end(),[](unsigned p){return (p&0xffffff)==0xff00;})) {
                    ++failures; std::fprintf(stderr,"Aspect/resolution/clear failure %d -> %d, resolution %d\n",old_aspect,new_aspect,resolution);
                }
                for (const auto &output : outputs) {
                    auto pixels = presented(output[0],output[1]);
                    unsigned min_x=output[0],min_y=output[1],max_x=0,max_y=0,green=0;
                    for (unsigned at=0;at<pixels.size();++at) if ((pixels[at]&0xffffff)==0xff00) {
                        ++green; min_x=std::min(min_x,at%output[0]);max_x=std::max(max_x,at%output[0]);
                        min_y=std::min(min_y,at/output[0]);max_y=std::max(max_y,at/output[0]);
                    }
                    if (!green || std::fabs((max_x-min_x+1.0)/(max_y-min_y+1.0)-4.0/3.0)>.025) {
                        ++failures;std::fprintf(stderr,"Presentation stretches UI at %dx%d -> %ux%u\n",
                            modes[selected_resolution].first,modes[selected_resolution].second,output[0],output[1]);
                    }
                }
                draw_quad(-mx-1,-my-1,641+mx,481+my,1,0xffffffff);
                auto full=readback();
                if (full.empty() || std::any_of(full.begin(),full.end(),[](unsigned p){return (p&0xffffff)!=0xffffff;})) {
                    ++failures;std::fprintf(stderr,"Selected viewport does not fill its own render target\n");
                }
                for (const auto &output : outputs) {
                    unsigned x,y,w,h;video_layout_fit(target_w,target_h,output[0],output[1],&x,&y,&w,&h);
                    auto pixels=presented(output[0],output[1]);
                    bool bad=pixels.size()!=output[0]*output[1];
                    for(unsigned at=0;at<pixels.size();++at) {
                        unsigned px=at%output[0],py=at/output[0];
                        unsigned expected=px>=x&&px<x+w&&py>=y&&py<y+h ? 0xffffff : 0;
                        if ((pixels[at]&0xffffff)!=expected) {bad=true;break;}
                    }
                    if (bad || (w!=output[0] && h!=output[1])) {
                        ++failures;std::fprintf(stderr,"Presentation has stale bars, cropping or double letterboxing\n");
                    }
                }
            }
    /* Same-size aspect/mode changes must also invalidate stale pixels. */
    modes={{640,480}};selected_resolution=0;selected_aspect=0;mode=1;
    clear_target(true,true,0xff0000);readback();
    selected_aspect=1;begin_pass();
    auto cleared=readback();
    if(cleared.empty() || std::any_of(cleared.begin(),cleared.end(),[](unsigned p){return (p&0xffffff)!=0;})) ++failures;
    clear_target(true,true,0xff0000);readback();mode=0;begin_pass();cleared=readback();
    if(cleared.empty() || std::any_of(cleared.begin(),cleared.end(),[](unsigned p){return (p&0xffffff)!=0;})) ++failures;
    hidden = true;
    bh_sdl_close();
    std::printf("Video transitions: %d GPU changes, 108 presented frames across three monitor shapes, same-size invalidation; %d failures\n",checks,failures);
    return failures ? 1 : 0;
}

#include "../src/bh_message_panel.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL message-panel: %s\n",s);}}while(0)
static float word_float(const unsigned long *word){float value;memcpy(&value,word,4);return value;}
int bh_message_panel_checkpoint(unsigned long *count){
    BHEntityPool pool;BHSoundQueue sound;BHTextQueue text;BHDrawQueue draw;
    BHMessageState messages;BHMessagePanel panel;BHMessagePanelFields *fields;
    BHAtlasPage page;BHEntity *entity;BHMathEnvironment fp;
    LPDIRECTSOUND device=NULL;BHWave wave;unsigned char samples[64];
    unsigned long on_id,off_id,char_id,i;long packet;
    checks=failures=0;
    memset(&pool,0,sizeof(pool));memset(&sound,0,sizeof(sound));
    memset(&draw,0,sizeof(draw));
    memset(&messages,0,sizeof(messages));memset(&panel,0,sizeof(panel));
    memset(&page,0,sizeof(page));memset(&wave,0,sizeof(wave));
    bh_text_queue_init(&text);
    bh_message_state_init(&messages);
    CHECK(messages.last_index==-1 && messages.ring_count==0 &&
        messages.arena_used==0,"static message state begins with no displayed ring index");
    CHECK(bh_math_environment_enter(&fp),"masked PC53 profile for panel timers");
    text.commands[0].position.color=0x12345678UL;
    CHECK(bh_message_text_plain(&text,&page,"A") &&
        text.commands[0].kind==BH_TEXT_COMMAND_PLAIN &&
        text.commands[0].page==&page &&
        !strcmp((char*)text.commands[0].text,"A") &&
        text.commands[0].position.color==0x12345678UL,
        "complete plain text producer keeps coordinates and unused record state");
    bh_text_queue_init(&text);
    CHECK(bh_entities_init(&pool,4,NULL),"actual pool owns panel entity");
    CHECK(bh_draw_queue_init(&draw,32,NULL),"actual draw queue owns panel records");
    CHECK(DirectSoundCreate(NULL,&device,NULL)==DS_OK && device,
        "real native sound interface, no playback");
    if(!device)goto done;
    CHECK(IDirectSound_SetCooperativeLevel(device,GetDesktopWindow(),DSSCL_NORMAL)==DS_OK &&
          bh_sound_queue_init(&sound,device,NULL,NULL),"real sound queue initialized");
    memset(samples,0,sizeof(samples));wave.format_tag=1;wave.channels=1;
    wave.sample_rate=8000;wave.byte_rate=16000;wave.block_align=2;
    wave.bits=16;wave.sample_bytes=64;wave.samples=samples;
    on_id=bh_sound_register_wave(&sound,&wave,0,3,0);
    off_id=bh_sound_register_wave(&sound,&wave,0,3,0);
    char_id=bh_sound_register_wave(&sound,&wave,0,3,0);
    CHECK(on_id!=BH_SOUND_INVALID && off_id!=BH_SOUND_INVALID &&
        char_id!=BH_SOUND_INVALID,"actual queued on/off/character sound resources");
    if(on_id==BH_SOUND_INVALID || off_id==BH_SOUND_INVALID || char_id==BH_SOUND_INVALID)goto done;
    panel.pool=&pool;panel.messages=&messages;panel.text=&text;panel.sound=&sound;
    panel.draw=&draw;panel.screen_width=640;panel.near_depth=1.0f;
    panel.flash_image=150;panel.alert_image=151;
    panel.page=&page;panel.tv_on_sound=on_id;panel.tv_off_sound=off_id;
    panel.tv_char_sound=char_id;
    for(i=0;i<12;i++)panel.icon_images[i]=100+i;
    entity=bh_message_panel_create(&panel);fields=bh_message_panel_fields(entity);
    CHECK(entity && fields && fields->timer==10.0f && fields->scale==0 &&
        entity->step==bh_message_panel_step && entity->query==bh_message_panel_query &&
        entity->refresh==bh_message_panel_refresh &&
        panel.mode_identity==0x00442bf0UL,
        "constructor subset allocates real entity and installs source step/query owners");
    CHECK(messages.panel_entity==entity &&
        messages.panel_step_identity==&panel.mode_identity,
        "message enqueue observes authoritative live source panel mode");
    CHECK(bh_message_enqueue(&messages,"5Hi\n") && fields->timer==10.0f,
        "message owner retains startup timer before source typing mode");
    CHECK(bh_entities_refresh(&pool) && draw.count==10 &&
        draw.commands[9].kind==BH_DRAW_RECT &&
        messages.display_ticks==63 && panel.draw_commands==1,
        "refresh renders last ring alert through real rectangle queue and ages notice");
    CHECK(bh_draw_queue_begin(&draw),"new render frame clears source command headers");
    {unsigned long nan_bits=0x7fc00000UL;
     memcpy(&fields->timer,&nan_bits,4);fields->scale=1.0f;
     CHECK(bh_entities_refresh(&pool) && draw.count==10,
         "unordered panel timer suppresses television while ring continues");
     fields->timer=10.0f;fields->scale=0.0f;
     CHECK(bh_draw_queue_begin(&draw),"render queue resets after unordered control");}
    fields->timer=.01f;
    CHECK(bh_entities_step(&pool) && panel.mode_identity==0x00442b60UL &&
        fields->icon_image==104 && fields->cursor==messages.arena+1 &&
        fields->timer==.3f && fields->scale==0 &&
        panel.clear_commands==1 && bh_text_queue_count(&text)==1 &&
        panel.sound_requests==1,
        "startup step chooses verified TV image, enters panel and queues real clear/sound");
    fields->timer=.01f;
    CHECK(bh_entities_step(&pool) && panel.mode_identity==0x00442950UL &&
        fields->timer==7.0f && fields->flash==1.8f && fields->scale==1.0f,
        "entry step reaches typing with seven-second sole-message hold");
    CHECK(bh_message_enqueue(&messages,"1B") && fields->timer==3.0f &&
        messages.arena_used==8,
        "new message during real typing mode clamps live timer to3");
    CHECK(bh_entities_refresh(&pool) && draw.count==13 &&
        draw.commands[9].kind==BH_DRAW_SCREEN_QUAD &&
        draw.commands[10].kind==BH_DRAW_RECT &&
        messages.display_ticks==63,
        "typing refresh emits television quad, flash, old ring icon and new alert");
    CHECK(draw.commands[9].resource==104 &&
        word_float(draw.commands[9].words)>67.19f &&
        word_float(draw.commands[9].words)<67.21f &&
        word_float(draw.commands[9].words+1)==108.0f &&
        word_float(draw.commands[9].words+2)==128.0f &&
        draw.commands[10].resource==150 &&
        word_float(draw.commands[10].words+10)>1.009f &&
        word_float(draw.commands[10].words+10)<1.011f &&
        draw.commands[11].resource==104 && draw.commands[12].resource==151,
        "panel and ring records preserve verified image, position, size and depth");
    CHECK(bh_draw_queue_begin(&draw),"source render queue can begin next frame");
    messages.display_ticks=1;messages.display_phase=1;
    CHECK(bh_entities_refresh(&pool) && messages.display_ticks==0 &&
        messages.display_phase==0 && messages.last_index==-1 &&
        messages.arena_used==11 && fields->timer==.5f,
        "deferred repeat appends current ring text and clamps live typing timer");
    CHECK(bh_draw_queue_begin(&draw),"source draw records reset after repeat");
    CHECK(bh_entities_step(&pool) && fields->cursor==messages.arena+2 &&
        panel.text_commands==1 && panel.sound_requests==2 &&
        bh_text_queue_count(&text)==2 &&
        !strcmp((char*)text.commands[1].text,"H"),
        "typing consumes one character, queues plain text and audible key");
    CHECK(bh_entities_step(&pool) && fields->cursor==messages.arena+3 &&
        panel.text_commands==2 && panel.sound_requests==3 &&
        !strcmp((char*)text.commands[2].text,"i"),
        "next visible character repeats real text/sound owners");
    CHECK(bh_entities_step(&pool) && fields->cursor==messages.arena+4 &&
        panel.text_commands==3 && panel.sound_requests==3 &&
        text.commands[3].text[0]=='\n',
        "newline is queued but silent");
    fields->timer=.01f;
    CHECK(bh_entities_step(&pool) && panel.mode_identity==0x004428b0UL &&
        fields->timer==.3f && panel.sound_requests==4,
        "NUL terminator decrements timer then switches to exit with off sound");
    fields->timer=.01f;
    CHECK(bh_entities_step(&pool) && panel.mode_identity==0x00442bf0UL &&
        fields->timer==.5f && fields->scale==0 && messages.arena_used==6 &&
        !strcmp(messages.arena,"1B"),
        "exit removes first complete arena message and retains next");
    messages.display_ticks=1;messages.display_phase=1;messages.last_index=1;
    CHECK(bh_entities_refresh(&pool) && messages.display_ticks==0 &&
        messages.display_phase==0 && messages.last_index==-1 &&
        messages.arena_used==9 && !strcmp(messages.arena+6,"1B"),
        "refresh expiry appends last ring message and retires displayed index");
    packet=3;CHECK(bh_entities_query(&pool,0,0,NULL,0,&packet) &&
        pool.used==1,"nonrelease panel query event preserves owner");
    packet=4;CHECK(bh_entities_query(&pool,0,0,NULL,0,&packet) &&
        pool.used==0 && !messages.panel_entity && !messages.panel_step_identity,
        "event4 query releases actual panel allocation and unbinds live identity");
    CHECK(!bh_message_panel_advance(&panel) && panel.error==BH_PANEL_STALE,
        "released panel generation cannot advance");
    CHECK(bh_message_panel_close(&panel),"panel sidecar closes after query release");
done:
    bh_draw_queue_dispose(&draw);
    bh_sound_queue_dispose(&sound);if(device)IDirectSound_Release(device);
    CHECK(bh_entities_dispose(&pool),"real entity pool disposed");
    CHECK(bh_math_environment_supported(),"masked PC53 profile retained");
    bh_math_environment_leave(&fp);
    if(count)*count=checks;
    printf("{\"scenario\":\"message-panel\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_MESSAGE_PANEL_STANDALONE
int main(void){unsigned long count;return bh_message_panel_checkpoint(&count);}
#endif

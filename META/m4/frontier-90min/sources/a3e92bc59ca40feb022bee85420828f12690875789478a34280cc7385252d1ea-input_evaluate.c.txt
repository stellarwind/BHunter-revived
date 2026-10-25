#include "../src/bh_input_evaluate.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL input-evaluate: %s\n",s);}} while(0)
static float decoded(BHPlayerInputState *state,unsigned long index) {float v;memcpy(&v,state->values+index,4);return v;}
int bh_input_evaluate_checkpoint(unsigned long *count) {
    BHPlayerInputList list,saved;BHInputSnapshot snapshot,before;BHPlayerInputState state;
    unsigned long i;float expected;
    checks=failures=0;memset(&list,0,sizeof(list));memset(&snapshot,0,sizeof(snapshot));
    CHECK(bh_input_evaluate(&list,&snapshot,0) && list.state[0]==10 && list.state[1]==2 &&
        list.state[2]==0 && list.state[6]==1,"empty list emits both padded Boolean batches");
    bh_player_input_bind_key(&list,1,"",0,0,20);snapshot.current[20]=1;memset(&state,0,sizeof(state));
    CHECK(bh_input_evaluate(&list,&snapshot,0) && bh_player_input_apply_packet_checked(&state,list.state,14) &&
        state.values[1]==1,"edge down packet roundtrip through actual decoder");
    snapshot.previous[20]=1;state.values[1]=99;
    CHECK(bh_input_evaluate(&list,&snapshot,0) && bh_player_input_apply_packet_checked(&state,list.state,14) &&
        state.values[1]==99,"held key emits no edge");
    snapshot.current[20]=0;
    CHECK(bh_input_evaluate(&list,&snapshot,0) && bh_player_input_apply_packet_checked(&state,list.state,14) &&
        state.values[1]==0,"edge up packet roundtrip");
    for(i=0;i<8;++i)snapshot.axes[i]=(float)(i+1)/16;
    for(i=1;i<=16;++i) {
        memset(&list,0,sizeof(list));bh_player_input_bind_axis(&list,0,"",0,0,0,0,(unsigned short)i,0);
        expected=snapshot.axes[(i-1)/2];if(!(i&1))expected=-expected;
        CHECK(bh_input_evaluate(&list,&snapshot,0) && bh_player_input_apply_packet_checked(&state,list.state,16) &&
            decoded(&state,0)==expected,"all joystick selectors preserve actual interleaved positive/negative table");
    }
    snapshot.mouse_axes[0]=.25f;snapshot.mouse_axes[1]=.5f;
    for(i=1;i<=4;++i) {
        list.bindings[0].key[2]=0;list.bindings[0].key[3]=(unsigned short)i;
        expected=snapshot.mouse_axes[(i-1)/2];if(!(i&1))expected=-expected;
        CHECK(bh_input_evaluate(&list,&snapshot,0) && bh_player_input_apply_packet_checked(&state,list.state,16) &&
            decoded(&state,0)==expected,"all mouse analog selectors roundtrip");
    }
    bh_player_input_bind_axis(&list,0,"",0,0,21,20,1,1);snapshot.current[20]=snapshot.current[21]=1;
    CHECK(bh_input_evaluate(&list,&snapshot,0) && bh_player_input_apply_packet_checked(&state,list.state,16) &&
        decoded(&state,0)==-1,"negative digital override wins when both keys are down");
    list.bindings[0].modifiers=30;snapshot.current[30]=0;snapshot.axes[0]=.75f;
    CHECK(bh_input_evaluate(&list,&snapshot,0) && bh_player_input_apply_packet_checked(&state,list.state,16) &&
        decoded(&state,0)==1,"analog sum clamps while missing modifier disables digital overrides");
    memset(&list,0,sizeof(list));list.count=2;list.bindings[0].kind=3;list.bindings[1].kind=4;
    snapshot.pair3[0]=0x1234;snapshot.pair3[1]=0xabcd;snapshot.pair4[0]=0x5678;snapshot.pair4[1]=0xef01;
    CHECK(bh_input_evaluate(&list,&snapshot,0) && bh_player_input_apply_packet_checked(&state,list.state,22) &&
        state.values[0]==0xabcd1234u && state.values[1]==0xef015678u,"both packed pair kinds roundtrip with exact16bit halves");
    memset(&list,0,sizeof(list));bh_player_input_bind_string(&list,0,"abc");snapshot.history_head=1;
    snapshot.history[30]='a';snapshot.history[31]='b';snapshot.history[0]='c';
    CHECK(bh_input_evaluate(&list,&snapshot,0) && snapshot.history_head==2 && snapshot.history[1]==0 &&
        bh_player_input_apply_packet_checked(&state,list.state,10) && state.values[0]==1,"string history matches across wrap and consumes one zero marker");
    snapshot.history[0]='z';before=snapshot;
    CHECK(bh_input_evaluate(&list,&snapshot,0) && !memcmp(&before,&snapshot,sizeof(before)),"nonmatching history preserved");
    memset(&list,0,sizeof(list));memset(&snapshot,0,sizeof(snapshot));
    bh_player_input_bind_string(&list,0,"");bh_player_input_bind_key(&list,1,"",0,0,255);
    snapshot.history[0]=1;memset(&state,0,sizeof(state));state.values[1]=77;
    CHECK(bh_input_evaluate(&list,&snapshot,0) && bh_player_input_apply_packet_checked(&state,list.state,10) &&
        snapshot.history[0]==0 && state.values[1]==77,
        "kind5 mutation changes overlapping previous key255 before following binding evaluation");
    memset(&list,0,sizeof(list));bh_player_input_bind_key(&list,0,"",0,30,255);snapshot.previous[0]=1;
    state.values[0]=77;
    CHECK(bh_input_evaluate(&list,&snapshot,1) && bh_player_input_apply_packet_checked(&state,list.state,10) &&
        state.values[0]==0,"full-state missing modifier emits up even when current255 aliases previous0 down");
    snapshot.current[30]=1;
    CHECK(bh_input_evaluate(&list,&snapshot,1) && bh_player_input_apply_packet_checked(&state,list.state,10) &&
        state.values[0]==1,"current255 aliases previous0 with enabled modifier");
    memset(&list,0,sizeof(list));bh_player_input_bind_axis(&list,0,"",0,0,0,0,1,0);
    {unsigned long bits=0x7fc12345;memcpy(snapshot.axes,&bits,4);}
    CHECK(bh_input_evaluate(&list,&snapshot,0) && bh_player_input_apply_packet_checked(&state,list.state,16) &&
        decoded(&state,0)==-1,"unordered analog sum selects original lower clamp");
    snapshot.axes[0]=.25f;bh_player_input_bind_key(&list,1,"",0,0,10);bh_player_input_bind_key(&list,2,"",0,0,11);
    snapshot.current[10]=1;snapshot.current[11]=0;
    CHECK(bh_input_evaluate(&list,&snapshot,1) && bh_player_input_apply_packet_checked(&state,list.state,16) &&
        decoded(&state,0)==.25f && state.values[1]==1 && state.values[2]==0,
        "analog record followed by both nonempty padded Boolean batches decodes in order");
    memset(&list,0,sizeof(list));bh_player_input_bind_key(&list,0,"",0,0,20);snapshot.current[20]=1;
    CHECK(bh_input_set_enabled(&list,&snapshot,0) && !list.flags &&
        bh_player_input_apply_packet_checked(&state,list.state,10) && state.values[0]==1,"wrapper zero mode evaluates full held state");
    saved=list;
    CHECK(bh_input_set_enabled(&list,NULL,7) && list.flags==2 && !memcmp(list.state,saved.state,sizeof(list.state)),
        "wrapper nonzero mode only sets flags2 without consuming snapshot");
    memset(&list,0,sizeof(list));list.count=47;for(i=0;i<47;++i)list.bindings[i].kind=4;
    saved=list;before=snapshot;
    CHECK(!bh_input_evaluate(&list,&snapshot,0) && !memcmp(&saved,&list,sizeof(list)) &&
        !memcmp(&before,&snapshot,sizeof(before)),"oversized original packet output explicitly rejected without partial writes");
    if(count)*count=checks;
    printf("{\"scenario\":\"input-evaluate\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_INPUT_EVALUATE_STANDALONE
int main(void) {return bh_input_evaluate_checkpoint(NULL);}
#endif

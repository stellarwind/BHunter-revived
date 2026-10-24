/* Source-only numerical fixtures for the diagnostic aim estimate, not physics. */
#define main m3_debugger_main_not_executed
#include "../tools/m3_startup_probe.c"
#undef main
int main(void) {
    static const float a[3]={0,0,0},b[3]={1,0,0},c[3]={0,1,0},d[3]={0,0,1};
    static const float t[6]={-0.5f,0,0.25f,0.5f,1,1.5f};
    static const float expected[6][3]={{-0.5f,0,0},{0,0,0},{0.2109375f,0.1171875f,0.0390625f},{0.25f,0.25f,0.25f},{0,0,1},{0,-0.5f,1.5f}};
    static const float basis[9]={1,0,0,0,1,0,0,0,1},aim[3]={0,0,1},velocity[3]={0,0,12};
    float out[4];int i,j,wanted[6]={0,0,0,0,0,0};
    for(i=0;i<6;i++) {
        combat_curve(t[i],a,b,c,d,out);
        for(j=0;j<3;j++)if(out[j]!=expected[i][j])return 1;
    }
    combat_frame(basis,aim,velocity,1,out);
    if(out[0]!=0 || out[1]!=0 || out[2]!=1 || out[3]!=12)return 2;
    flight_yaw_sign=-1;flight_pitch_sign=-1;combat_turn_keys(0.5f,0.5f,wanted);
    if(wanted[3]!=1 || wanted[1]!=1 || wanted[2] || wanted[0])return 3;
    printf("{\"passed\":true,\"curve_coordinate_checks\":18,\"forward_projection_checks\":4,\"turn_direction_checks\":4,\"game_launches\":0,\"original_code_executed\":false}\n");return 0;
}

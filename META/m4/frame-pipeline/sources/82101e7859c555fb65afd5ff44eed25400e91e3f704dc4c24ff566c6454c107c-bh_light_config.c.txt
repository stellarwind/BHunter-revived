#include "bh_light_config.h"
int __cdecl bh_light_configure(BHLightConfig *config,long enabled,float x,float y,float z,
    float directional_strength,float ambient_strength,
    unsigned long directional_color,unsigned long ambient_color) {
    if(!config)return 0;
    config->enabled=enabled;if(!enabled)return 1;
    config->direction.x=-x;config->direction.y=-y;config->direction.z=-z;
    bh_vec3_normalize(&config->direction);
    config->directional[0]=(float)(((directional_color>>16)&255)*directional_strength);
    config->directional[1]=(float)(((directional_color>>8)&255)*directional_strength);
    config->directional[2]=(float)((directional_color&255)*directional_strength);
    config->ambient[0]=(float)(((ambient_color>>16)&255)*ambient_strength);
    config->ambient[1]=(float)(((ambient_color>>8)&255)*ambient_strength);
    config->ambient[2]=(float)((ambient_color&255)*ambient_strength);
    return 1;
}

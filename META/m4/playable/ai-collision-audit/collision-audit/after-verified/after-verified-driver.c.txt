#include <stdio.h>
int __cdecl bh_ballistic_projectile_checkpoint(unsigned long*);
int __cdecl bh_homing_projectile_checkpoint(unsigned long*);
int __cdecl bh_area_projectile_checkpoint(unsigned long*);
int __cdecl bh_effect_orbit_tick_checkpoint(unsigned long*);
int main(void){int r=0,a;unsigned long n=0;r|=bh_ballistic_projectile_checkpoint(&n);r|=bh_homing_projectile_checkpoint(&n);n=0;a=bh_area_projectile_checkpoint(&n);printf("area-projectile: %lu checks, passed=%d\n",n,a);r|=!a;r|=bh_effect_orbit_tick_checkpoint(&n);return r;}

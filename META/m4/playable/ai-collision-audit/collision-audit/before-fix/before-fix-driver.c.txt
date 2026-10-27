int __cdecl bh_ballistic_projectile_checkpoint(unsigned long*);
int __cdecl bh_homing_projectile_checkpoint(unsigned long*);
int __cdecl bh_area_projectile_checkpoint(unsigned long*);
int __cdecl bh_effect_orbit_tick_checkpoint(unsigned long*);
int main(void){int r=0;unsigned long n;r|=bh_ballistic_projectile_checkpoint(&n);r|=bh_homing_projectile_checkpoint(&n);r|=bh_area_projectile_checkpoint(&n);r|=bh_effect_orbit_tick_checkpoint(&n);return r;}

int bh_sound_startup_checkpoint(unsigned long *count);
int main(void){unsigned long count;return bh_sound_startup_checkpoint(&count)?1:0;}

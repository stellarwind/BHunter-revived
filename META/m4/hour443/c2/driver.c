int bh_input_mode_checkpoint(const char *,unsigned long *);
int main(int argc,char **argv){return bh_input_mode_checkpoint(argc>1?argv[1]:0,0);}

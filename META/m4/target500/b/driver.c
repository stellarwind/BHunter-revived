int bh_target_construct_checkpoint(const char *,unsigned long *);
int main(int argc,char **argv){return bh_target_construct_checkpoint(argc>1?argv[1]:0,0);}

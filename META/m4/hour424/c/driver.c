int bh_atlas_wide_checkpoint(const char *,unsigned long *);int main(int argc,char **argv){unsigned long n;return bh_atlas_wide_checkpoint(argc>1?argv[1]:0,&n);}

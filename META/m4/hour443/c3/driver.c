int bh_menu_profile_checkpoint(const char *,unsigned long *);
int main(int argc,char **argv){return bh_menu_profile_checkpoint(argc>1?argv[1]:0,0);}

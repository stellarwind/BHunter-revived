int bh_menu_row_checkpoint(const char*,unsigned long*);int main(int argc,char **argv){unsigned long n;return bh_menu_row_checkpoint(argc>1?argv[1]:0,&n);}

#include <windows.h>
#include <initguid.h>
#include <dplay.h>
int bh_provider_catalog_checkpoint(unsigned long *);
int main(void){unsigned long count;return bh_provider_catalog_checkpoint(&count)?1:0;}

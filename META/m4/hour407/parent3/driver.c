#include <windows.h>
#include <initguid.h>
#include <dplay.h>
#include <dplobby.h>
int bh_provider_startup_checkpoint(unsigned long *);
int main(void){unsigned long count;return bh_provider_startup_checkpoint(&count)?1:0;}

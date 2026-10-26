#include "bh_display_enum.h"
#include <stdio.h>
#include <string.h>
static int label_fits(const char *description,const char *driver){
 size_t a,b;if(!description||!driver)return 0;a=strlen(description);b=strlen(driver);
 return a<128&&b<128&&a+b+3<128;
}
/*00472270: native NULL-GUID primary remains separate from bounded secondary
   count, and seven secondary writes cancel actual native enumeration. */
BOOL CALLBACK bh_display_catalog_callback(GUID *guid,LPSTR description,LPSTR driver,LPVOID context){
 BHDisplayCatalog *catalog=(BHDisplayCatalog*)context;long index;
 if(!catalog||!label_fits(description,driver)||catalog->secondary_count<0||catalog->secondary_count>7)return FALSE;
 if(guid){if(catalog->secondary_count==7)return FALSE;index=catalog->secondary_count+1;}
 else index=0;
 sprintf(catalog->label[index],"%s (%s)",description,driver);strcpy(catalog->description[index],description);
 if(guid){++catalog->secondary_count;if(catalog->secondary_count==7)return FALSE;}
 return TRUE;
}
/*00472330: preserve exact case-sensitive concatenated-name comparison;
   cancellation is independent of DirectDrawCreate HRESULT. */
BOOL CALLBACK bh_display_select_callback(GUID *guid,LPSTR description,LPSTR driver,LPVOID context){
 BHDisplaySelection *selection=(BHDisplaySelection*)context;char label[128];
 if(!selection||!selection->requested||selection->draw||!label_fits(description,driver))return TRUE;
 sprintf(label,"%s (%s)",description,driver);
 if(strcmp(label,selection->requested)!=0)return TRUE;
 ++selection->attempts;selection->result=DirectDrawCreate(guid,&selection->draw,NULL);return FALSE;
}

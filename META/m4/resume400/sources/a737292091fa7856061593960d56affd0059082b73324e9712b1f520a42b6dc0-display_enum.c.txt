#include "../src/bh_display_enum.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL display-enum %d: %s\n",__LINE__,s);}}while(0)
static void memory_cases(void){
 BHDisplayCatalog c,saved;BHDisplaySelection selection;GUID guid;char description[130];int i;BOOL result;
 memset(&c,0,sizeof(c));memset(&guid,0x42,sizeof(guid));
 CHECK(bh_display_catalog_callback(NULL,"Primary","driver",&c),"primary callback continues");
 CHECK(c.secondary_count==0&&!strcmp(c.description[0],"Primary")&&!strcmp(c.label[0],"Primary (driver)"),"primary separate slot and exact format");
 CHECK(bh_display_catalog_callback(NULL,"New Primary","new",&c)&&c.secondary_count==0&&!strcmp(c.label[0],"New Primary (new)"),"repeat primary replaces without increasing secondary count");
 for(i=1;i<=7;++i){sprintf(description,"Device%d",i);result=bh_display_catalog_callback(&guid,description,"drv",&c);CHECK(result==(i==7?FALSE:TRUE),"seventh secondary cancels");CHECK(c.secondary_count==i&&!strcmp(c.description[i],description),"secondary slot and count");sprintf(description,"Device%d (drv)",i);CHECK(!strcmp(c.label[i],description),"whole label stored per128-byte stride");}
 saved=c;CHECK(!bh_display_catalog_callback(&guid,"Overflow","drv",&c)&&!memcmp(&c,&saved,sizeof(c)),"source capacity guard without write");
 CHECK(bh_display_catalog_callback(NULL,"Late primary","late",&c)&&c.secondary_count==7&&!strcmp(c.label[0],"Late primary (late)"),"primary still separate at full secondary capacity");
 memset(&c,0,sizeof(c));memset(description,'a',123);description[123]=0;CHECK(bh_display_catalog_callback(NULL,description,"b",&c)&&strlen(c.label[0])==127,"maximum valid original label length");
 description[123]='a';description[124]=0;saved=c;CHECK(!bh_display_catalog_callback(NULL,description,"b",&c)&&!memcmp(&c,&saved,sizeof(c)),"source overlong-label guard");
 CHECK(!bh_display_catalog_callback(NULL,NULL,"driver",&c)&&!bh_display_catalog_callback(NULL,"desc",NULL,&c)&&!bh_display_catalog_callback(NULL,"desc","driver",NULL),"source missing arguments rejected");
 c.secondary_count=-1;CHECK(!bh_display_catalog_callback(&guid,"desc","driver",&c),"source invalid secondary count guard");c.secondary_count=8;CHECK(!bh_display_catalog_callback(&guid,"desc","driver",&c),"source invalid secondary count guard");
 memset(&selection,0,sizeof(selection));selection.requested="Primary (Driver)";selection.result=123;
 CHECK(bh_display_select_callback(NULL,"Primary","driver",&selection)&&!selection.attempts&&!selection.draw&&selection.result==123,"case-sensitive mismatch never creates or changes result");
 CHECK(bh_display_select_callback(NULL,NULL,"driver",&selection)&&!selection.attempts,"source malformed input guard");
 selection.requested=NULL;CHECK(bh_display_select_callback(NULL,"Primary","driver",&selection)&&!selection.attempts,"source missing requested-name guard");
}
static void native_cases(void){
 BHDisplayCatalog c;BHDisplaySelection s,missing;HRESULT result;DDCAPS caps;GUID invalid;LPDDENUMCALLBACKA collect=bh_display_catalog_callback,select=bh_display_select_callback;ULONG remaining;
 memset(&c,0,sizeof(c));result=DirectDrawEnumerateA(collect,&c);CHECK(result==DD_OK,"actual native DirectDraw enumeration invokes exact WINAPI callback");
 CHECK(c.label[0][0]&&c.description[0][0]&&c.secondary_count>=0&&c.secondary_count<=7,"native primary catalog supplies startup selected identity");if(result!=DD_OK||!c.label[0][0])return;
 memset(&missing,0,sizeof(missing));missing.requested="BHunter absent device 8d1bf";missing.result=123;
 result=DirectDrawEnumerateA(select,&missing);CHECK(result==DD_OK&&!missing.attempts&&!missing.draw&&missing.result==123,"native no-match walk creates no device");
 memset(&s,0,sizeof(s));s.requested=c.label[0];s.result=123;result=DirectDrawEnumerateA(select,&s);
 CHECK(result==DD_OK&&s.attempts==1&&s.result==DD_OK&&s.draw,"actual selected primary root created by whole recovered callback");
 if(s.draw){
  memset(&caps,0,sizeof(caps));caps.dwSize=sizeof(caps);result=IDirectDraw_GetCaps(s.draw,&caps,NULL);CHECK(result==DD_OK&&caps.dwSize==sizeof(caps),"real created COM owner answers device caps for subsequent renderer initialization");
  CHECK(bh_display_select_callback(NULL,c.description[0],"ignored",&s)&&s.attempts==1,"source occupied-output guard preserves actual owned reference");
  remaining=IDirectDraw_Release(s.draw);s.draw=NULL;CHECK(remaining==0,"native root lifetime released once");
 }
 memset(&invalid,0xa7,sizeof(invalid));memset(&s,0,sizeof(s));s.requested="Invalid (private)";
 CHECK(!select(&invalid,"Invalid","private",&s)&&s.attempts==1&&FAILED(s.result)&&!s.draw,"matched callback cancels even after actual invalid-GUID DirectDrawCreate failure");if(s.draw)IDirectDraw_Release(s.draw);
}
int bh_display_enum_checkpoint(unsigned long *count){checks=failures=0;memory_cases();native_cases();if(count)*count=checks;printf("{\"scenario\":\"display-enum\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;}

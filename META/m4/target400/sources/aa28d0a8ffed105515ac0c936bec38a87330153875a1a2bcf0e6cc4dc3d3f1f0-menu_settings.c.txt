#include "../src/bh_menu_settings.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<12)printf("FAIL menu-settings %lu: %s\n",checks,s);}}while(0)
typedef void (__cdecl *Assign)(BHMenuSettings *,BHMenuSettingsMenu *,const unsigned char *,long);
/* Isolated source binding adapter: invoke an actual recovered whole selection
   owner. This is a real observable menu mutation, not a label-render stub and
   not an implementation claim for the original row-label callbacks. */
static void __cdecl refresh_selection(BHMenuSettingsMenu *menu,long index) {
    BHMenuSettings *s=(BHMenuSettings*)menu->user;
    s->current_index=index;
    bh_menu_submenu_first(s);
}
static void key_assignments(void) {
    BHMenuSettings s;BHInputProfile profile,before;BHMenuSettingsMenu menu,sub;
    BHMenuSettingsRow row,*rows[12];Assign owners[4];unsigned long offsets[4]={0x1c,0x28,0x31,0x3a};
    long lengths[4]={6,9,9,10};unsigned char input[2];unsigned long owner,value,k;long i;
    memset(&s,0,sizeof(s));memset(&menu,0,sizeof(menu));memset(&sub,0,sizeof(sub));
    owners[0]=bh_menu_keys_motion;owners[1]=bh_menu_keys_weapons;owners[2]=bh_menu_keys_camera;owners[3]=bh_menu_keys_extra;
    row.refresh=refresh_selection;for(k=0;k<12;++k)rows[k]=&row;
    menu.rows=rows;menu.row_count=12;menu.user=&s;s.input_profile=&profile;s.submenus[0]=&sub;
    for(owner=0;owner<4;++owner)for(i=0;i<12;++i)for(value=0;value<256;++value){
        memset(&profile,0xa5,sizeof(profile));before=profile;input[0]=(unsigned char)value;input[1]=0x33;
        s.current_index=99;s.action=-999;sub.selection=99;sub.display_selection=99;
        owners[owner](&s,&menu,input,i);
        for(k=0;k<sizeof(profile);++k){unsigned char expected=((const unsigned char*)&before)[k];
            if(i>=1&&i<=lengths[owner]&&k==offsets[owner]+i-1)expected=input[0];
            CHECK(((unsigned char*)&profile)[k]==expected,"exact profile byte footprint; all untouched bytes preserved");
        }
        CHECK(sub.selection==i&&sub.display_selection==(float)i&&s.action==i+14,"actual row refresh calls recovered submenu owner after valid/inert assignment");
    }
    row.refresh=NULL;memset(&profile,0x3c,sizeof(profile));input[0]=201;
    bh_menu_keys_motion(&s,&menu,input,2);CHECK(((unsigned char*)&profile)[0x1d]==201,"NULL refresh still assigns first byte");
    before=profile;bh_menu_keys_motion(&s,&menu,NULL,0);CHECK(!memcmp(&before,&profile,sizeof(profile)),"out-of-assignment range does not read NULL input");
    bh_menu_keys_motion(&s,&menu,input,-1);bh_menu_keys_weapons(&s,&menu,input,12);
    CHECK(!memcmp(&before,&profile,sizeof(profile)),"invalid row binding rejected");
}
static void selections(void) {
    BHMenuSettings s;BHMenuSettingsMenu menus[6];BHMenuNetworkRecord records[4];long indexes[6]={-22,-1,0,1,1000000,LONG_MAX};unsigned long i;
    memset(&s,0,sizeof(s));memset(menus,0,sizeof(menus));memset(records,0x45,sizeof(records));
    s.network_menu=menus;s.submenus[0]=menus+1;s.submenus[1]=menus+2;
    for(i=0;i<3;++i)s.network_choices[i]=menus+3+i;
    s.network_records=records;s.network_record_count=4;s.selected_network=records;
    for(i=0;i<6;++i){volatile float expected=(float)indexes[i];
        s.last_nonzero=indexes[i];s.launch_state=78;bh_menu_network_open(&s,17);
        CHECK(s.current_index==17&&menus[0].selection==indexes[i]&&menus[0].display_selection==expected&&s.launch_state==0,"network open restores retained choice and resets launch state");
        s.current_index=indexes[i];bh_menu_submenu_first(&s);bh_menu_submenu_second(&s);
        CHECK(menus[1].selection==indexes[i]&&menus[2].selection==indexes[i]&&menus[1].display_selection==expected&&menus[2].display_selection==expected,"both whole submenu methods preserve signed integer conversion");
        CHECK((unsigned long)s.action==(unsigned long)indexes[i]+14UL,"action arithmetic wraps at original32 bits");
    }
    s.last_nonzero=3;
    for(i=0;i<4;++i){bh_menu_network_select(&s,(long)i);
        CHECK(s.selected_network==records+i&&s.last_nonzero==(i?(long)i:3),"selection uses188-byte record stride and only positive indices replace retained choice");
        CHECK(menus[3].selection==3&&menus[3].display_selection==3&&menus[4].selection==6&&menus[4].display_selection==6&&menus[5].selection==2&&menus[5].display_selection==2,"three independent menus reset to3/6/2");
    }
    bh_menu_network_select(&s,-1);bh_menu_network_select(&s,4);CHECK(s.selected_network==records+3,"invalid record index rejected");
}
static void strings(void) {
    BHMenuSettings s;BHMenuSettingsMenu menu,sub;BHMenuSettingsRow row,*rows[2];BHMenuNetworkRecord selected,before;
    char input[128];unsigned long n,k,offset,size,which;
    memset(&s,0,sizeof(s));memset(&menu,0,sizeof(menu));memset(&sub,0,sizeof(sub));
    row.refresh=refresh_selection;rows[0]=rows[1]=&row;menu.rows=rows;menu.row_count=2;menu.user=&s;
    s.submenus[0]=&sub;s.selected_network=&selected;
    for(which=0;which<2;++which){offset=which?0x1c:0x7c;size=which?96:64;
        for(n=0;n<size;++n){memset(&selected,0xa5,sizeof(selected));before=selected;memset(input,'A'+n%26,n);input[n]=0;s.action=-8;
            if(which)bh_menu_network_phone(&s,&menu,input,1);else bh_menu_network_address(&s,&menu,input,1);
            for(k=0;k<sizeof(selected);++k){unsigned char expected=before.bytes[k];if(k>=offset&&k<=offset+n)expected=(unsigned char)input[k-offset];CHECK(selected.bytes[k]==expected,"string copies exact bytes through terminating NUL and leaves tail/neighbor intact");}
            CHECK(sub.selection==1&&s.action==15,"string owner dispatches real optional refresh");
        }
        before=selected;memset(input,'Z',sizeof(input));input[size]=0;s.action=-8;
        if(which)bh_menu_network_phone(&s,&menu,input,1);else bh_menu_network_address(&s,&menu,input,1);
        CHECK(!memcmp(&selected,&before,sizeof(selected))&&s.action==-8,"overlong original-overflow input rejected without mutation/callback");
    }
    row.refresh=NULL;bh_menu_network_address(&s,&menu,"",0);CHECK(selected.bytes[0x7c]==0,"empty string with absent refresh");
}
int __cdecl bh_menu_settings_checkpoint(unsigned long *count) {
    checks=failures=0;key_assignments();selections();strings();if(count)*count=checks;
    printf("{\"scenario\":\"menu-settings\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_MENU_SETTINGS_STANDALONE
int main(void){return bh_menu_settings_checkpoint(NULL);}
#endif

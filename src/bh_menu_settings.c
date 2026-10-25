#include "bh_menu_settings.h"
#include <stddef.h>
#include <string.h>
typedef char profile_width[sizeof(BHInputProfile)==76?1:-1];
typedef char record_width[sizeof(BHMenuNetworkRecord)==188?1:-1];

static void selection(BHMenuSettingsMenu *menu,long index) {
    menu->selection=index;
    menu->display_selection=(float)index;
}
static int row_valid(BHMenuSettingsMenu *menu,long index) {
    return menu && index>=0 && (unsigned long)index<menu->row_count &&
        menu->rows && menu->rows[index];
}
static void refresh(BHMenuSettingsMenu *menu,long index) {
    BHMenuSettingsRefresh method=menu->rows[index]->refresh;
    if(method)method(menu,index);
}
void __cdecl bh_menu_network_open(BHMenuSettings *s,long index) {
    if(!s||!s->network_menu)return;
    s->current_index=index;
    selection(s->network_menu,s->last_nonzero);
    s->launch_state=0;
}
void __cdecl bh_menu_submenu_first(BHMenuSettings *s) {
    if(!s||!s->submenus[0])return;
    selection(s->submenus[0],s->current_index);
    s->action=(long)((unsigned long)s->current_index+14UL);
}
void __cdecl bh_menu_submenu_second(BHMenuSettings *s) {
    if(!s||!s->submenus[1])return;
    selection(s->submenus[1],s->current_index);
    s->action=(long)((unsigned long)s->current_index+14UL);
}
void __cdecl bh_menu_network_select(BHMenuSettings *s,long index) {
    if(!s||!s->network_records||index<0||(unsigned long)index>=s->network_record_count||
       !s->network_choices[0]||!s->network_choices[1]||!s->network_choices[2])return;
    s->selected_network=s->network_records+index;
    if(index>0)s->last_nonzero=index;
    selection(s->network_choices[0],3);
    selection(s->network_choices[1],6);
    selection(s->network_choices[2],2);
}
static void assign(BHMenuSettings *s,BHMenuSettingsMenu *menu,
                   const unsigned char *input,long index,unsigned long offset,long count) {
    if(!s||!s->input_profile||!row_valid(menu,index))return;
    if(index>=1&&index<=count){
        if(!input)return;
        ((unsigned char*)s->input_profile)[offset+(unsigned long)index-1]=*input;
    }
    refresh(menu,index);
}
void __cdecl bh_menu_keys_motion(BHMenuSettings *s,BHMenuSettingsMenu *m,const unsigned char *p,long i) {
    assign(s,m,p,i,0x1c,6);
}
void __cdecl bh_menu_keys_weapons(BHMenuSettings *s,BHMenuSettingsMenu *m,const unsigned char *p,long i) {
    assign(s,m,p,i,0x28,9);
}
void __cdecl bh_menu_keys_camera(BHMenuSettings *s,BHMenuSettingsMenu *m,const unsigned char *p,long i) {
    assign(s,m,p,i,0x31,9);
}
void __cdecl bh_menu_keys_extra(BHMenuSettings *s,BHMenuSettingsMenu *m,const unsigned char *p,long i) {
    assign(s,m,p,i,0x3a,10);
}
static void text(BHMenuSettings *s,BHMenuSettingsMenu *m,const char *value,long i,unsigned long offset,unsigned long size) {
    const char *end;
    if(!s||!s->selected_network||!value||!row_valid(m,i))return;
    end=(const char*)memchr(value,0,size);
    if(!end)return;
    memcpy(s->selected_network->bytes+offset,value,(unsigned long)(end-value)+1);
    refresh(m,i);
}
void __cdecl bh_menu_network_address(BHMenuSettings *s,BHMenuSettingsMenu *m,const char *p,long i) {
    text(s,m,p,i,0x7c,64);
}
void __cdecl bh_menu_network_phone(BHMenuSettings *s,BHMenuSettingsMenu *m,const char *p,long i) {
    text(s,m,p,i,0x1c,96);
}

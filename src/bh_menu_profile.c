#include "bh_menu_profile.h"
#include <string.h>
static void refresh(BHMenuSettingsMenu *menu,long index){BHMenuSettingsRefresh callback;
 if(!menu||index<0||(unsigned long)index>=menu->row_count||!menu->rows||!menu->rows[index])return;
 callback=menu->rows[index]->refresh;if(callback)callback(menu,index);
}
int __cdecl bh_menu_profile_commit(BHMenuProfileContext *c,BHMenuSettingsMenu *menu,const char *name,long index){
 BHInputProfile *current,*p,*found=NULL;unsigned long i,n;int okay=1;
 if(!c||!c->config||!c->current||!c->pending_selection||!menu||!name)return 0;
 c->error=0;
 if(!memchr(name,0,16)){c->error=1;return 0;}n=(unsigned long)strlen(name);
 if(n){
  if(!bh_config_valid(c->config)||!(*c->current)){c->error=1;return 0;}
  current=*c->current;p=current;
  /* Require the actual current pointer to belong to the real config ring. */
  found=c->config->profiles;for(i=0;i<c->config->profile_count&&found!=current;++i)found=found->next;
  if(i==c->config->profile_count){c->error=1;return 0;}found=NULL;
  do{if(!_strcmpi(name,(const char*)p->payload)){found=p;break;}p=p->next;}while(p!=current);
  if(found){*c->current=found;refresh(menu,0);}
  else{
   found=bh_config_profile_clone_after(c->config,current);
   if(found){memcpy(found->payload,name,n+1);found->flags=1;*c->current=found;refresh(menu,0);}
   else {c->error=1;okay=0;}
  }
  *c->pending_selection=0;
 }
 refresh(menu,index);return okay;
}

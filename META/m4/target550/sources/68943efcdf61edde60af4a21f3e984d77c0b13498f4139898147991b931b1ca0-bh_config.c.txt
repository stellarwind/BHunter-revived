#include "bh_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

typedef char profile_size[sizeof(BHInputProfile)==76 ? 1 : -1];
typedef char profile_links[offsetof(BHInputProfile,next)==4 && offsetof(BHInputProfile,previous)==8 ? 1 : -1];
typedef char profile_baseline[offsetof(BHInputProfile,baseline)==72 ? 1 : -1];
#include "bh_config_defaults.h"

typedef struct ProfileNode {
    BHInputProfile profile;
    BHInputProfile *owned_baseline;
    struct ProfileNode *allocation_next;
} ProfileNode;

static void *__cdecl allocate(unsigned long bytes,void *user) { return malloc(bytes); }
static void __cdecl release(void *pointer,void *user) { free(pointer); }
static int fail(BHConfig *config,BHConfigError error) { if(config) config->error=error;return 0; }
static int terminated(const unsigned char *bytes,unsigned long count) { return memchr(bytes,0,count)!=NULL; }

int __cdecl bh_config_init(BHConfig *config,const BHConfigAllocator *allocator) {
    if (!config) return 0;
    if (allocator && (!allocator->allocate || !allocator->release)) return 0;
    memset(config,0,sizeof(*config));
    if (allocator) config->allocator=*allocator;
    else {config->allocator.allocate=allocate;config->allocator.release=release;}
    return 1;
}

void __cdecl bh_config_dispose(BHConfig *config) {
    ProfileNode *node,*next;
    if (!config) return;
    node=(ProfileNode*)config->owned_nodes;
    while(node) {
        next=node->allocation_next;
        if (node->owned_baseline) config->allocator.release(node->owned_baseline,config->allocator.user);
        config->allocator.release(node,config->allocator.user);
        node=next;
    }
    config->owned_nodes=NULL;config->profiles=NULL;config->profile_count=0;
}

static int append(BHConfig *config,const unsigned char *record,int template_index) {
    ProfileNode *node;
    BHInputProfile *profile,*tail;
    node=(ProfileNode*)config->allocator.allocate(sizeof(*node),config->allocator.user);
    if (!node) return fail(config,BH_CONFIG_MEMORY);
    memset(node,0,sizeof(*node));
    memcpy(&node->profile,record,76);
    profile=&node->profile;
    /* Pointer words in the legacy file are address-space artifacts, not links. */
    profile->next=profile->previous=NULL;profile->baseline=NULL;
    node->allocation_next=(ProfileNode*)config->owned_nodes;config->owned_nodes=node;
    if (template_index>=0) profile->baseline=&default_profiles[template_index];
    else if (!profile->flags) {
        node->owned_baseline=(BHInputProfile*)config->allocator.allocate(76,config->allocator.user);
        if (!node->owned_baseline) return fail(config,BH_CONFIG_MEMORY);
        *node->owned_baseline=*profile;
        profile->baseline=node->owned_baseline;
    }
    if (!config->profiles) {
        profile->next=profile->previous=profile;config->profiles=profile;
    } else {
        tail=config->profiles->previous;
        profile->next=config->profiles;profile->previous=tail;
        tail->next=profile;config->profiles->previous=profile;
    }
    ++config->profile_count;
    return 1;
}

static int defaults(BHConfig *config) {
    int i;
    for(i=0;i<3;i++) if(!append(config,(const unsigned char*)&default_profiles[i],i)) return 0;
    return 1;
}

static int member(const BHConfig *config,const BHInputProfile *profile) {
    const ProfileNode *node=(const ProfileNode*)config->owned_nodes;
    while(node) {if(&node->profile==profile)return 1;node=node->allocation_next;}
    return 0;
}

int __cdecl bh_config_valid(const BHConfig *config) {
    const BHInputProfile *profile;
    const ProfileNode *node;
    unsigned long i,allocations=0;
    if (!config || !config->profiles || !config->profile_count || config->profile_count>4096 ||
        !terminated(config->settings+12,128)) return 0;
    node=(const ProfileNode*)config->owned_nodes;
    while(node) {++allocations;node=node->allocation_next;}
    if (allocations!=config->profile_count) return 0;
    profile=config->profiles;
    for(i=0;i<config->profile_count;i++) {
        if(!member(config,profile) || !member(config,profile->next) || !member(config,profile->previous)) return 0;
        if(profile->next->previous!=profile || profile->previous->next!=profile || !terminated(profile->payload,16)) return 0;
        profile=profile->next;
        if(profile==config->profiles && i+1!=config->profile_count) return 0;
    }
    return profile==config->profiles;
}

BHInputProfile *__cdecl bh_config_profile_clone_after(BHConfig *config,BHInputProfile *current) {
    ProfileNode *node;
    BHInputProfile *profile,*following;
    if(!config || !current || !config->allocator.allocate ||
       !bh_config_valid(config) || !member(config,current)) {
        fail(config,BH_CONFIG_ARGUMENT);return NULL;
    }
    if(config->profile_count>=4096) {fail(config,BH_CONFIG_LIMIT);return NULL;}
    node=(ProfileNode*)config->allocator.allocate(sizeof(*node),config->allocator.user);
    if(!node) {fail(config,BH_CONFIG_MEMORY);return NULL;}
    memset(node,0,sizeof(*node));node->profile=*current;
    profile=&node->profile;following=current->next;
    profile->previous=current;profile->next=following;
    node->allocation_next=(ProfileNode*)config->owned_nodes;
    config->owned_nodes=node;current->next=profile;following->previous=profile;
    ++config->profile_count;config->error=BH_CONFIG_OK;
    return profile;
}

BHInputProfile *__cdecl bh_config_profile_remove(BHConfig *config,BHInputProfile *current) {
    ProfileNode *node,*previous=NULL,*scan;BHInputProfile *selected;
    if(!config||!current||!bh_config_valid(config)||!member(config,current)||!current->flags||config->profile_count<=1){fail(config,BH_CONFIG_ARGUMENT);return NULL;}
    node=(ProfileNode*)config->owned_nodes;
    while(node && &node->profile!=current){previous=node;node=node->allocation_next;}
    if(!node){fail(config,BH_CONFIG_CONTAINER);return NULL;}
    if(node->owned_baseline)for(scan=(ProfileNode*)config->owned_nodes;scan;scan=scan->allocation_next)
        if(scan!=node&&scan->profile.baseline==node->owned_baseline){fail(config,BH_CONFIG_CONTAINER);return NULL;}
    selected=current->previous;current->previous->next=current->next;current->next->previous=current->previous;
    if(config->profiles==current)config->profiles=selected;
    if(previous)previous->allocation_next=node->allocation_next;else config->owned_nodes=node->allocation_next;
    if(node->owned_baseline)config->allocator.release(node->owned_baseline,config->allocator.user);
    config->allocator.release(node,config->allocator.user);--config->profile_count;config->error=BH_CONFIG_OK;return selected;
}

int __cdecl bh_config_load(BHConfig *config,const char *path) {
    BHConfig next;
    FILE *file;
    long length;
    unsigned long count,i;
    unsigned char record[76];
    BHConfigError error;
    if(!config || !config->allocator.allocate || !path || !*path) return fail(config,BH_CONFIG_ARGUMENT);
    bh_config_init(&next,&config->allocator);
    errno=0;file=fopen(path,"rb");
    if(!file) {
        if(errno!=ENOENT) return fail(config,BH_CONFIG_IO);
        memcpy(next.settings,default_settings,416);next.loaded_defaults=1;
        if(!defaults(&next)) goto rejected;
    } else {
        if(fseek(file,0,SEEK_END) || (length=ftell(file))<0 || fseek(file,0,SEEK_SET)) {next.error=BH_CONFIG_IO;goto close_rejected;}
        if(length<416 || (length-416)%76) {next.error=BH_CONFIG_FORMAT;goto close_rejected;}
        count=(unsigned long)(length-416)/76;
        if(count>4096) {next.error=BH_CONFIG_LIMIT;goto close_rejected;}
        if(fread(next.settings,1,416,file)!=416) {next.error=BH_CONFIG_IO;goto close_rejected;}
        if(!terminated(next.settings+12,128)) {next.error=BH_CONFIG_FORMAT;goto close_rejected;}
        for(i=0;i<count;i++) {
            if(fread(record,1,76,file)!=76) {next.error=BH_CONFIG_IO;goto close_rejected;}
            if(!terminated(record+12,16)) {next.error=BH_CONFIG_FORMAT;goto close_rejected;}
            if(!append(&next,record,-1)) goto close_rejected;
        }
        if(fclose(file)) {next.error=BH_CONFIG_IO;goto rejected;}
        if(!count && !defaults(&next)) goto rejected;
    }
    bh_config_dispose(config);*config=next;return 1;
close_rejected:
    fclose(file);
rejected:
    error=next.error;bh_config_dispose(&next);return fail(config,error);
}

int __cdecl bh_config_save(BHConfig *config,const char *path) {
    FILE *file;
    BHInputProfile *profile;
    unsigned long i;
    int ok;
    if(!config || !path || !*path) return fail(config,BH_CONFIG_ARGUMENT);
    if(!bh_config_valid(config)) return fail(config,BH_CONFIG_CONTAINER);
    file=fopen(path,"wb");if(!file) return fail(config,BH_CONFIG_IO);
    ok=fwrite(config->settings,1,416,file)==416;profile=config->profiles;
    for(i=0;ok && i<config->profile_count;i++) {
        ok=fwrite(profile,1,76,file)==76;profile=profile->next;
    }
    if(fclose(file)) ok=0;
    if(!ok) return fail(config,BH_CONFIG_IO);
    config->error=BH_CONFIG_OK;return 1;
}

int __cdecl bh_config_bootstrap(BHConfig *config,const char *path,
                              const BHDeviceName *devices,unsigned long count) {
    unsigned long i,selected=0;
    unsigned short hardware;
    int found=0,ok;
    if(!config || !devices || !count || count>4096) return fail(config,BH_CONFIG_ARGUMENT);
    for(i=0;i<count;i++) if(!terminated((const unsigned char*)devices[i].name,128)) return fail(config,BH_CONFIG_ARGUMENT);
    if(!bh_config_load(config,path)) return 0;
    for(i=0;i<count;i++) if(!_stricmp(devices[i].name,(char*)config->settings+12)) {selected=i;found=1;}
    if(!found) {config->settings[0]=0;config->settings[1]=0;}
    strcpy((char*)config->settings+12,devices[selected].name);
    memcpy(&hardware,config->settings,2);
    config->settings[0]=0;config->settings[1]=0;
    ok=bh_config_save(config,path);
    memcpy(config->settings,&hardware,2);
    return ok;
}

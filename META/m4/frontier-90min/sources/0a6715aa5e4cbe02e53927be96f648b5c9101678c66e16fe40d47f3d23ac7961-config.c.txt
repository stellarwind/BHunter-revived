/* Source-only config/container/path scenarios. Files stay in the build cwd. */
#include "../src/bh_config.h"
#include "../src/bh_paths.h"
#include "../src/bh_vfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <direct.h>

static unsigned long checks;
static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *name,int line) {
    ++checks;if(!ok) {++failures;printf("FAIL config line %d: %s\n",line,name);}
}
static void put32(unsigned char *p,unsigned long x) {
    p[0]=(unsigned char)x;p[1]=(unsigned char)(x>>8);p[2]=(unsigned char)(x>>16);p[3]=(unsigned char)(x>>24);
}
static unsigned long word(const unsigned char *p) {
    return (unsigned long)p[0]|((unsigned long)p[1]<<8)|((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24);
}
static int write_file(const char *path,const void *bytes,unsigned long count) {
    FILE *file=fopen(path,"wb");int ok;
    if(!file) return 0;
    ok=fwrite(bytes,1,count,file)==count;return fclose(file)==0 && ok;
}
static long read_file(const char *path,unsigned char *bytes,unsigned long capacity) {
    FILE *file=fopen(path,"rb");long count;
    if(!file) return -1;
    count=(long)fread(bytes,1,capacity,file);
    if(ferror(file)) count=-1;
    fclose(file);return count;
}

typedef struct Memory {unsigned long calls,fail_at,live;} Memory;
static void *__cdecl test_allocate(unsigned long bytes,void *user) {
    Memory *memory=(Memory*)user;void *pointer;
    ++memory->calls;
    if(memory->fail_at && memory->calls==memory->fail_at) return NULL;
    pointer=malloc(bytes);if(pointer) ++memory->live;return pointer;
}
static void __cdecl test_release(void *pointer,void *user) {
    Memory *memory=(Memory*)user;
    CHECK(pointer!=NULL && memory->live>0,"release owned live allocation exactly once");
    if(pointer) {--memory->live;free(pointer);}
}
static BHConfigAllocator allocator(Memory *memory) {
    BHConfigAllocator result;
    result.allocate=test_allocate;result.release=test_release;result.user=memory;return result;
}

static unsigned char fixture[416+3*76],saved[sizeof(fixture)+1];
static void make_fixture(void) {
    unsigned long i,j;unsigned char *record;
    for(i=0;i<416;i++) fixture[i]=(unsigned char)(i*17+3);
    memset(fixture+12,0,128);strcpy((char*)fixture+12,"Test Renderer");
    fixture[0]=7;fixture[1]=0;
    for(i=0;i<3;i++) {
        record=fixture+416+76*i;
        for(j=0;j<76;j++) record[j]=(unsigned char)(i*23+j*3);
        put32(record,i==1 ? 1 : 0);
        put32(record+4,0xdead0000UL+i);put32(record+8,0xbeef0000UL+i);
        put32(record+72,0xfeed0000UL+i);
        memset(record+12,0,16);sprintf((char*)record+12,"Profile %lu",i);
    }
}

static void paths(void) {
    BHPaths value,before;
    char output[256],directory[257],windows_directory[256],current_directory[256];
    unsigned long n,total;
    memset(&value,0,sizeof(value));
    CHECK(bh_paths_init(&value,"C:\\Windows","D:\\Games\\BHunter")==2,"path success return 2");
    CHECK(!strcmp(value.ini,"C:\\Windows\\BHUNTER.INI") && !strcmp(value.archive,"D:\\Games\\BHunter\\BHUNTER.BIN"),"observed INI and decoded archive names");
    CHECK(bh_paths_init(&value,"C:\\Windows\\","D:\\")==2 && !strcmp(value.archive,"D:\\BHUNTER.BIN"),"root and trailing backslash avoid duplicate");
    CHECK(!strcmp(value.ini,"C:\\Windows\\BHUNTER.INI"),"Windows trailing separator");
    CHECK(bh_paths_init(&value,"C:/Windows/","relative")==2 && !strcmp(value.ini,"C:/Windows/\\BHUNTER.INI") && !strcmp(value.archive,"relative\\BHUNTER.BIN"),"preserve backslash-only rule and relative path");
    before=value;
    CHECK(bh_paths_init(&value,NULL,".")==2 && !strcmp(value.ini,before.ini),"Windows API failure preserves INI");
    before=value;
    CHECK(!bh_paths_init(&value,"C:\\Windows",NULL) && !memcmp(&value,&before,sizeof(value)),"cwd failure is bounded and transactional");
    CHECK(!bh_paths_init(NULL,"C:\\Windows","."),"null output rejected");
    CHECK(!bh_paths_init(&value,"",".") && !memcmp(&value,&before,sizeof(value)),"empty directory rejected");
    for(n=1;n<=256;n++) {
        memset(directory,'x',n);directory[n]=0;value=before;
        total=n+1+11;
        CHECK((bh_paths_init(&value,directory,directory)==2)==(total<256),"every path length boundary");
        if(total>=256) CHECK(!memcmp(&value,&before,sizeof(value)),"oversize preserves both paths");
    }
    strcpy(output,"sentinel");
    CHECK(bh_paths_config(output,sizeof(output),NULL) && !strcmp(output,"BHUNTER.CFG"),"CFG fallback literal");
    CHECK(bh_paths_config(output,sizeof(output),"C:\\Windows\\") && !strcmp(output,"C:\\Windows\\BHUNTER.CFG"),"CFG trailing slash");
    strcpy(output,"sentinel");
    CHECK(!bh_paths_config(output,11,NULL) && !strcmp(output,"sentinel"),"CFG output capacity includes terminator");
    CHECK(bh_paths_config(output,12,NULL) && !strcmp(output,"BHUNTER.CFG"),"CFG exact capacity");
    CHECK(!bh_paths_config(NULL,256,NULL),"CFG null output");
    CHECK(GetWindowsDirectoryA(windows_directory,255)>0 && GetCurrentDirectoryA(255,current_directory)>0,"real directory APIs available");
    memset(&value,0,sizeof(value));memset(&before,0,sizeof(before));
    CHECK(bh_paths_discover(&value)==2 && bh_paths_init(&before,windows_directory,current_directory)==2,"native directory discovery executes");
    CHECK(!memcmp(&value,&before,sizeof(value)),"native discovery agrees with captured real directory inputs");
}

static void defaults_and_container(void) {
    BHConfig config;
    BHInputProfile *p,*previous,*next;
    Memory memory={0,0,0};BHConfigAllocator hooks=allocator(&memory);
    unsigned long i;
    remove("bhcfg_missing.cfg");
    CHECK(bh_config_init(&config,&hooks),"initialize real ownership context");
    CHECK(bh_config_load(&config,"bhcfg_missing.cfg") && config.loaded_defaults,"missing config loads defaults");
    CHECK(bh_config_valid(&config) && config.profile_count==3 && memory.live==3,"three default nodes with borrowed templates");
    CHECK(word(config.settings)==0 && word(config.settings+8)==1 && word(config.settings+400)==56000 && word(config.settings+412)==1,"captured default settings fields");
    p=config.profiles;
    CHECK(!strcmp((char*)p->payload,"<Standard>") && !strcmp((char*)p->next->payload,"<MouseView>") && !strcmp((char*)p->previous->payload,"<Joy 1 View>"),"default profile order and names");
    for(i=0;i<3;i++) {
        CHECK(p->baseline && !p->baseline->next && !p->baseline->previous && !p->baseline->baseline,"default baseline is immutable detached template");
        CHECK(!memcmp(p->payload,p->baseline->payload,60),"default bindings copied");
        CHECK(p->next->previous==p && p->previous->next==p,"forward/backward reciprocity");p=p->next;
    }
    CHECK(p==config.profiles,"full cycle returns to head");
    p=config.profiles;next=p->next;previous=p->previous;
    p->next=(BHInputProfile*)0x12345678;
    CHECK(!bh_config_valid(&config) && !bh_config_save(&config,"bhcfg_bad_save.cfg") && config.error==BH_CONFIG_CONTAINER,"foreign next rejected before dereference/save");
    p->next=next;p->previous=next;
    CHECK(!bh_config_valid(&config),"broken backward edge rejected");p->previous=previous;
    ++config.profile_count;CHECK(!bh_config_valid(&config),"wrong count rejected");--config.profile_count;
    CHECK(bh_config_valid(&config),"restored container valid");
    for(i=0;i<40;i++) CHECK(bh_config_load(&config,"bhcfg_missing.cfg") && memory.live==3 && bh_config_valid(&config),"repeated default reload releases prior ring");
    bh_config_dispose(&config);CHECK(memory.live==0 && !config.profiles && !config.profile_count,"dispose releases owning nodes");
    bh_config_dispose(&config);CHECK(memory.live==0,"repeated dispose safe");
}

static void load_save_and_failure(void) {
    BHConfig config;
    BHInputProfile *p,*head;
    Memory memory={0,0,0};BHConfigAllocator hooks=allocator(&memory);
    unsigned long i,n,before,step;
    unsigned char header[416];
    make_fixture();CHECK(write_file("bhcfg_valid.cfg",fixture,sizeof(fixture)),"write independent legacy fixture");
    CHECK(bh_config_init(&config,&hooks),"initialize custom context");
    CHECK(bh_config_load(&config,"bhcfg_valid.cfg") && !config.loaded_defaults && bh_config_valid(&config),"load valid legacy settings/profiles");
    CHECK(!memcmp(config.settings,fixture,416) && config.profile_count==3 && memory.live==5,"opaque header exact and two owned snapshots");
    p=config.profiles;
    for(i=0;i<3;i++) {
        CHECK(!memcmp(p->payload,fixture+416+i*76+12,60),"custom profile opaque payload exact");
        CHECK(p->flags==(i==1 ? 1UL:0UL),"profile flag word retained");
        if(i!=1) CHECK(p->baseline && !p->baseline->next && !p->baseline->previous && !p->baseline->baseline && !memcmp(p->payload,p->baseline->payload,60),"flag-zero profile owns detached snapshot");
        else CHECK(!p->baseline,"stale nonzero-flag baseline pointer discarded");
        p=p->next;
    }
    config.profiles->payload[20]^=0x55;
    CHECK(config.profiles->payload[20]!=config.profiles->baseline->payload[20],"editing current bindings does not mutate baseline");
    CHECK(bh_config_save(&config,"bhcfg_saved.cfg"),"save profile ring");
    CHECK(read_file("bhcfg_saved.cfg",saved,sizeof(saved))==sizeof(fixture),"exact saved length");
    CHECK(!memcmp(saved,config.settings,416),"save opaque settings exact");
    p=config.profiles;
    for(i=0;i<3;i++) {
        CHECK(!memcmp(saved+416+i*76,p,76),"save exact legacy 76-byte record in ring order including live pointer words");p=p->next;
    }
    CHECK(bh_config_load(&config,"bhcfg_saved.cfg") && bh_config_valid(&config) && memory.live==5,"reload ignores persisted process pointers");
    CHECK(config.profiles->payload[20]==(unsigned char)(fixture[416+12+20]^0x55),"edited bindings survive disk roundtrip");
    head=config.profiles;memcpy(header,config.settings,416);before=memory.live;
    for(n=0;n<sizeof(fixture);n++) {
        if(n>=416 && (n-416)%76==0) continue;
        CHECK(write_file("bhcfg_invalid.cfg",fixture,n),"write truncation fixture");
        CHECK(!bh_config_load(&config,"bhcfg_invalid.cfg") && config.error==BH_CONFIG_FORMAT,"reject every partial header/record length");
        CHECK(config.profiles==head && memory.live==before && !memcmp(config.settings,header,416),"rejected reload preserves prior state and ownership");
    }
    memset(fixture+12,'X',128);CHECK(write_file("bhcfg_invalid.cfg",fixture,sizeof(fixture)),"write unterminated device");
    CHECK(!bh_config_load(&config,"bhcfg_invalid.cfg") && config.error==BH_CONFIG_FORMAT,"unterminated device rejected");
    make_fixture();memset(fixture+416+12,'X',16);CHECK(write_file("bhcfg_invalid.cfg",fixture,sizeof(fixture)),"write unterminated profile");
    CHECK(!bh_config_load(&config,"bhcfg_invalid.cfg") && config.error==BH_CONFIG_FORMAT && memory.live==before,"unterminated profile rejected without leaked allocations");
    make_fixture();memset(fixture+416+2*76+12,'X',16);CHECK(write_file("bhcfg_invalid.cfg",fixture,sizeof(fixture)),"write late unterminated profile");
    CHECK(!bh_config_load(&config,"bhcfg_invalid.cfg") && config.error==BH_CONFIG_FORMAT && memory.live==before && config.profiles==head,"late parse rejection frees constructed nodes and snapshots");
    make_fixture();
    for(step=1;step<=5;step++) {
        memory.fail_at=memory.calls+step;
        CHECK(!bh_config_load(&config,"bhcfg_valid.cfg") && config.error==BH_CONFIG_MEMORY,"reject each node/snapshot allocation failure");
        CHECK(config.profiles==head && memory.live==before && bh_config_valid(&config),"allocation rollback keeps old ring valid");
        memory.fail_at=0;
    }
    for(step=1;step<=3;step++) {
        memory.fail_at=memory.calls+step;
        CHECK(!bh_config_load(&config,"bhcfg_missing.cfg") && config.error==BH_CONFIG_MEMORY,"default allocation failures rejected");
        CHECK(config.profiles==head && memory.live==before,"default rollback releases partial construction");memory.fail_at=0;
    }
    CHECK(!bh_config_load(&config,"") && config.error==BH_CONFIG_ARGUMENT,"empty load path rejected");
    CHECK(!bh_config_save(&config,"bhcfg_absent_dir/output.cfg") && config.error==BH_CONFIG_IO,"save I/O failure reported");
    CHECK(write_file("bhcfg_header.cfg",fixture,416),"header-only legacy file");
    CHECK(bh_config_load(&config,"bhcfg_header.cfg") && !memcmp(config.settings,fixture,416) && config.profile_count==3 && memory.live==3,"header-only keeps settings and supplies defaults");
    CHECK(write_file("bhcfg_single.cfg",fixture,416+76),"one profile fixture");
    CHECK(bh_config_load(&config,"bhcfg_single.cfg") && config.profile_count==1 && config.profiles->next==config.profiles && config.profiles->previous==config.profiles && memory.live==2,"singleton ring and snapshot ownership");
    /* Disposal follows allocation ownership even after a mutable ring is broken. */
    config.profiles->next=NULL;config.profiles->flags=1;
    bh_config_dispose(&config);CHECK(memory.live==0,"dispose independent of mutable links/flags");
}

static void device_selection(void) {
    BHConfig config;
    BHDeviceName devices[3];
    unsigned long attributes;
    make_fixture();memset(devices,0,sizeof(devices));
    strcpy(devices[0].name,"Software");strcpy(devices[1].name,"test renderer");strcpy(devices[2].name,"TEST RENDERER");
    CHECK(bh_config_init(&config,NULL),"initialize device config");
    CHECK(write_file("bhcfg_devices.cfg",fixture,sizeof(fixture)),"write device selection fixture");
    CHECK(bh_config_bootstrap(&config,"bhcfg_devices.cfg",devices,3),"device selection loads and saves");
    CHECK(!strcmp((char*)config.settings+12,"TEST RENDERER") && config.settings[0]==7,"last case-insensitive match wins; runtime flag restored");
    CHECK(read_file("bhcfg_devices.cfg",saved,sizeof(saved))==sizeof(fixture) && saved[0]==0 && saved[1]==0 && !strcmp((char*)saved+12,"TEST RENDERER"),"bootstrap persisted hardware word zero and chosen name");
    CHECK(write_file("bhcfg_devices.cfg",fixture,sizeof(fixture)),"reset device fixture");
    CHECK(bh_config_bootstrap(&config,"bhcfg_devices.cfg",devices,1) && config.settings[0]==0 && !strcmp((char*)config.settings+12,"Software"),"unmatched device selects first and disables hardware");
    CHECK(!bh_config_bootstrap(&config,"bhcfg_devices.cfg",NULL,0) && config.error==BH_CONFIG_ARGUMENT,"no enumerated device diagnosed");
    memset(devices[0].name,'X',128);
    CHECK(!bh_config_bootstrap(&config,"bhcfg_devices.cfg",devices,1) && config.error==BH_CONFIG_ARGUMENT,"unterminated enumerated device rejected");
    strcpy(devices[0].name,"Test Renderer");
    CHECK(write_file("bhcfg_readonly.cfg",fixture,sizeof(fixture)),"write private readonly fixture");
    attributes=GetFileAttributesA("bhcfg_readonly.cfg");
    CHECK(attributes!=(unsigned long)-1 && SetFileAttributesA("bhcfg_readonly.cfg",attributes|FILE_ATTRIBUTE_READONLY),"set own fixture readonly");
    CHECK(!bh_config_bootstrap(&config,"bhcfg_readonly.cfg",devices,1) && config.error==BH_CONFIG_IO && config.settings[0]==7,"failed persistence still restores runtime hardware word");
    CHECK(SetFileAttributesA("bhcfg_readonly.cfg",attributes),"restore own fixture attributes");
    bh_config_dispose(&config);
}

static void bounded_count(void) {
    BHConfig config;FILE *file;unsigned long i;
    make_fixture();CHECK(bh_config_init(&config,NULL),"initialize bounded config");
    file=fopen("bhcfg_count.cfg","wb");CHECK(file!=NULL,"open count-limit fixture");
    if(file) {
        CHECK(fwrite(fixture,1,416,file)==416,"count-limit header");
        for(i=0;i<4097;i++) if(fwrite(fixture+416,1,76,file)!=76) break;
        CHECK(i==4097 && fclose(file)==0,"count-limit records");
        CHECK(!bh_config_load(&config,"bhcfg_count.cfg") && config.error==BH_CONFIG_LIMIT && !config.profiles,"excess record count bounded before allocation");
    }
    file=fopen("bhcfg_count.cfg","wb");CHECK(file!=NULL,"open exact count-limit fixture");
    if(file) {
        CHECK(fwrite(fixture,1,416,file)==416,"exact count-limit header");
        for(i=0;i<4096;i++) if(fwrite(fixture+416,1,76,file)!=76) break;
        CHECK(i==4096 && fclose(file)==0,"exact count-limit records");
        CHECK(bh_config_load(&config,"bhcfg_count.cfg") && config.profile_count==4096 && bh_config_valid(&config),"maximum bounded ring admitted and traversable");
        CHECK(bh_config_save(&config,"bhcfg_count.cfg"),"maximum bounded ring serializes");
    }
    bh_config_dispose(&config);
}

static void integrated_paths(void) {
    BHPaths paths;
    BHConfig config;
    BHVFile *file;
    unsigned char archive[264],payload[4]={0};
    char cwd[256],directory[256],cfg[256];
    CHECK(GetCurrentDirectoryA(255,cwd)>0,"integration build cwd");
    if(strlen(cwd)+17>=sizeof(directory)) {CHECK(0,"fixture directory fits path contract");return;}
    sprintf(directory,"%s\\bhfound_fixture",cwd);_mkdir(directory);
    memset(&paths,0,sizeof(paths));
    CHECK(bh_paths_init(&paths,directory,directory)==2 && bh_paths_config(cfg,sizeof(cfg),directory),"construct isolated startup paths");
    memset(archive,0,sizeof(archive));put32(archive,1);strcpy((char*)archive+4,"foundation.dat");
    put32(archive+4+248,260);put32(archive+4+252,264);memcpy(archive+260,"M2OK",4);
    CHECK(write_file(paths.archive,archive,sizeof(archive)),"write synthetic archive at decoded startup path");
    CHECK(bh_vfile_mount(paths.archive),"path-driven source archive mount");
    file=bh_vfile_open("foundation.dat","rb");CHECK(file!=NULL,"path-driven archive opens asset");
    if(file) {CHECK(bh_vfile_read(payload,1,4,file)==4 && !memcmp(payload,"M2OK",4),"path/VFILE composed payload read");CHECK(bh_vfile_close(file)==0,"close composed handle");}
    bh_vfile_shutdown();
    CHECK(bh_config_init(&config,NULL),"initialize composed config");
    remove(cfg);
    CHECK(bh_config_load(&config,cfg) && config.loaded_defaults && bh_config_save(&config,cfg),"path-driven config defaults and persistence");
    CHECK(bh_config_load(&config,cfg) && !config.loaded_defaults && bh_config_valid(&config),"path-driven config/container reload");
    bh_config_dispose(&config);remove(cfg);remove(paths.archive);_rmdir(directory);
}

int bh_config_checkpoint(unsigned long *total) {
    const char *files[]={"bhcfg_missing.cfg","bhcfg_bad_save.cfg","bhcfg_valid.cfg","bhcfg_saved.cfg","bhcfg_invalid.cfg","bhcfg_header.cfg","bhcfg_single.cfg","bhcfg_devices.cfg","bhcfg_readonly.cfg","bhcfg_count.cfg"};
    unsigned long i;
    checks=0;failures=0;
    paths();defaults_and_container();load_save_and_failure();device_selection();bounded_count();integrated_paths();
    for(i=0;i<sizeof(files)/sizeof(files[0]);i++) remove(files[i]);
    *total=checks;
    printf("{\"scenario\":\"config-container-path\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures ? 1:0;
}

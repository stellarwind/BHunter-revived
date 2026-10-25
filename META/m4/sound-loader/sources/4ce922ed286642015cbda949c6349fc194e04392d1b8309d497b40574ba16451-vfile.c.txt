/* Isolated source-only VFILE fixtures. Never load/map/execute BHUNTER.EXE. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../src/bh_vfile.h"

static unsigned long checks;
static int failures, notifications;
static float progress_last;
#define CHECK(value,label) check((value)!=0,label,__LINE__)
static void check(int ok, const char *name, int line) {
    ++checks;
    if (!ok) { ++failures;printf("FAIL VFILE line %d: %s\n",line,name); }
}
static void __cdecl progress(float fraction) {
    ++notifications;progress_last=fraction;
}
static int callback_mutations_rejected;
static void __cdecl mutation_progress(float fraction) {
    int mount_blocked, detach_blocked;
    mount_blocked=!bh_vfile_mount("vfile_valid.bin") && bh_vfile_error()==BH_VFILE_BUSY;
    detach_blocked=!bh_vfile_detach_archive() && bh_vfile_error()==BH_VFILE_BUSY;
    bh_vfile_shutdown();
    callback_mutations_rejected=mount_blocked && detach_blocked && bh_vfile_error()==BH_VFILE_BUSY;
    progress(fraction);
}
static void u32(unsigned char *p, unsigned long value) {
    p[0]=(unsigned char)value;p[1]=(unsigned char)(value>>8);
    p[2]=(unsigned char)(value>>16);p[3]=(unsigned char)(value>>24);
}
static unsigned long from_u32(const unsigned char *p) {
    return (unsigned long)p[0] | ((unsigned long)p[1]<<8) |
           ((unsigned long)p[2]<<16) | ((unsigned long)p[3]<<24);
}

static const char *names[]={"bhvf_alpha.dat","bhvf_lines.dat","bhvf_empty.dat","bhvf_four.dat"};
static const char *payloads[]={"abcde","A\r\nB\n\rC\rD\nTail","","WXYZ"};
static unsigned char fixture[2048];
static unsigned long fixture_length, starts[4], ends[4];

static int write_bytes(const char *name, const void *bytes, unsigned long count) {
    FILE *file=fopen(name,"wb");int ok;
    if (!file) return 0;
    ok=fwrite(bytes,1,count,file)==count;
    return fclose(file)==0 && ok;
}

static void make_fixture(void) {
    unsigned long i, position=4+4*256;
    memset(fixture,0,sizeof(fixture));u32(fixture,4);
    for (i=0;i<4;i++) {
        strcpy((char*)fixture+4+i*256,names[i]);
        starts[i]=position;
        memcpy(fixture+position,payloads[i],strlen(payloads[i]));
        position+=(unsigned long)strlen(payloads[i]);ends[i]=position;
        u32(fixture+4+i*256+248,starts[i]);u32(fixture+4+i*256+252,ends[i]);
    }
    fixture_length=position;
}

static void core_tests(void) {
    BHVFile *a=NULL,*b=NULL,*reused;
    unsigned char buffer[32], bad[2048];
    unsigned long i;
    bh_vfile_shutdown();remove(names[0]);
    make_fixture();
    CHECK(write_bytes("vfile_valid.bin",fixture,fixture_length),"write synthetic archive");
    CHECK(bh_vfile_mount("vfile_valid.bin"),"mount valid archive");
    CHECK(bh_vfile_entry_count()==4,"validated entry count");
    bh_vfile_set_progress(progress);notifications=0;progress_last=0;
    a=bh_vfile_open("BHVF_ALPHA.DAT","rb");
    CHECK(a!=NULL,"case-insensitive embedded open");if (!a) goto done;
    CHECK(notifications==1 && progress_last==0.25f,"first access callback");
    b=bh_vfile_open(names[0],"rb");CHECK(b && a!=b,"distinct embedded wrappers");
    CHECK(notifications==1,"repeat access does not notify");
    if (b) {CHECK(bh_vfile_close(b)==0,"close second archive wrapper");b=NULL;}
    CHECK(!bh_vfile_mount("vfile_valid.bin") && bh_vfile_error()==BH_VFILE_BUSY,"remount with live archive handle rejected");
    CHECK(!bh_vfile_detach_archive() && bh_vfile_error()==BH_VFILE_BUSY,"detach with live archive handle rejected");
    memset(buffer,0xa5,sizeof(buffer));
    CHECK(bh_vfile_read(buffer+1,2,3,a)==2,"partial final element count");
    CHECK(!memcmp(buffer+1,"abcde",5) && buffer[0]==0xa5 && buffer[6]==0xa5,"partial bytes consumed with guards");
    CHECK(bh_vfile_eof(a)==1 && bh_vfile_tell(a)==5,"read past end sets EOF");
    CHECK(bh_vfile_seek(a,0,SEEK_SET)==0 && !bh_vfile_eof(a),"seek clears EOF");
    CHECK(bh_vfile_read(buffer,1,5,a)==5 && !bh_vfile_eof(a),"exact end does not set wrapper EOF");
    CHECK(bh_vfile_read(buffer,1,1,a)==0 && bh_vfile_eof(a)==1,"next read sets EOF");
    CHECK(bh_vfile_seek(a,LONG_MAX,SEEK_CUR)==0 && bh_vfile_tell(a)==5,"positive seek clamps without overflow");
    CHECK(bh_vfile_seek(a,LONG_MIN,SEEK_CUR)==0 && bh_vfile_tell(a)==0,"negative seek clamps without overflow");
    CHECK(bh_vfile_seek(a,-2,SEEK_END)==0 && bh_vfile_tell(a)==3,"seek relative to end");
    CHECK(bh_vfile_seek(a,1,77)==0 && bh_vfile_tell(a)==3,"unknown seek origin preserves clamped position");
    CHECK(bh_vfile_read(buffer,0,1,a)==0 && bh_vfile_tell(a)==3,"zero-size read is safe no-op");
    CHECK(bh_vfile_read(NULL,1,0,a)==0 && bh_vfile_tell(a)==3,"zero-count read is safe no-op");
    CHECK(bh_vfile_read(buffer,ULONG_MAX,2,a)==0 && bh_vfile_error()==BH_VFILE_OVERFLOW,"overflow read rejected");
    CHECK(bh_vfile_read(NULL,1,1,a)==0 && bh_vfile_error()==BH_VFILE_ARGUMENT,"null destination rejected");
    CHECK(bh_vfile_seek(a,0,SEEK_SET)==0,"restore first wrapper");
    b=bh_vfile_open(names[3],"rb");CHECK(b!=NULL,"second entry open");
    if (b) {
        CHECK(bh_vfile_tell(b)==(long)starts[0]-(long)starts[3],"open invalid-origin seek leaves shared FILE unchanged");
        CHECK(bh_vfile_seek(b,0,SEEK_SET)==0 && bh_vfile_read(buffer,1,1,b)==1 && buffer[0]=='W',"second wrapper read seeks its own cursor");
        CHECK(bh_vfile_tell(a)==(long)starts[3]+1-(long)starts[0],"archive tell follows shared FILE position");
        CHECK(bh_vfile_seek(a,0,SEEK_SET)==0 && bh_vfile_read(buffer,1,2,a)==2 && !memcmp(buffer,"ab",2),"restore cursor after shared tell");
        CHECK(bh_vfile_read(buffer,1,2,b)==2 && !memcmp(buffer,"XY",2),"interleaved wrapper read independent of shared FILE");
        CHECK(bh_vfile_close(b)==0,"close other entry");b=NULL;
    }
    reused=a;CHECK(bh_vfile_close(a)==0,"close first wrapper");a=NULL;
    CHECK(bh_vfile_close(reused)==EOF && bh_vfile_error()==BH_VFILE_ARGUMENT,"double close rejected");
    a=bh_vfile_open(names[2],"rb");CHECK(a==reused,"wrapper recycle list reused");
    if (a) {
        CHECK(bh_vfile_read(buffer,1,1,a)==0 && bh_vfile_eof(a)==1,"empty embedded payload");
        bh_vfile_close(a);a=NULL;
    }
    CHECK(bh_vfile_open("bhvf_does_not_exist","rb")==NULL && bh_vfile_error()==BH_VFILE_NOT_FOUND,"missing file diagnosed");
    CHECK(bh_vfile_open(names[0],"q")==NULL,"non-read mode does not fall back to archive");
    CHECK(write_bytes(names[0],"LOOSE",5),"create loose override");
    a=bh_vfile_open(names[0],"rb");CHECK(a && !a->archive,"loose file precedence");
    if (a) {
        CHECK(bh_vfile_read(buffer,1,5,a)==5 && !memcmp(buffer,"LOOSE",5),"read loose override");
        CHECK(bh_vfile_read(buffer,1,1,a)==0 && bh_vfile_eof(a)==0x10,"loose EOF preserves CRT flag value");
        CHECK(bh_vfile_seek(a,0,SEEK_SET)==0 && !bh_vfile_eof(a),"loose seek resets EOF");
        CHECK(bh_vfile_tell(a)==0,"loose tell");bh_vfile_close(a);a=NULL;
    }
    remove(names[0]);
    /* A loose matching file still participates in first-access bookkeeping. */
    bh_vfile_shutdown();CHECK(bh_vfile_mount("vfile_valid.bin"),"fresh mount for loose callback");
    notifications=0;bh_vfile_set_progress(progress);
    CHECK(write_bytes(names[0],"x",1),"create first-access loose override");
    a=bh_vfile_open(names[0],"rb");CHECK(a && notifications==1 && progress_last==0.25f,"loose first-access callback");
    if (a) {bh_vfile_close(a);a=NULL;}remove(names[0]);
    /* Failed mounts are transactional; every malformed case leaves valid state. */
    for (i=0;i<9;i++) {
        memcpy(bad,fixture,fixture_length);
        switch(i) {
        case 0:u32(bad,ULONG_MAX);break;
        case 1:memset(bad+4,'x',248);break;
        case 2:bad[4]=0;break;
        case 3:u32(bad+4+248,0);break;
        case 4:u32(bad+4+252,starts[0]-1);break;
        case 5:u32(bad+4+252,fixture_length+1);break;
        case 6:u32(bad+4+248,0x80000000UL);break;
        case 7:u32(bad,5);break;
        case 8:u32(bad+4+252,ULONG_MAX);break;
        }
        CHECK(write_bytes("vfile_bad.bin",bad,fixture_length),"write malformed directory");
        CHECK(!bh_vfile_mount("vfile_bad.bin") && bh_vfile_error()==BH_VFILE_FORMAT,"malformed directory rejected");
        a=bh_vfile_open(names[0],"rb");CHECK(a!=NULL,"failed mount preserves old archive");
        if (a) {CHECK(bh_vfile_read(buffer,1,1,a)==1 && buffer[0]=='a',"old archive bytes intact");bh_vfile_close(a);a=NULL;}
    }
    for (i=0;i<4;i++) {
        CHECK(write_bytes("vfile_bad.bin",fixture,i),"write truncated count");
        CHECK(!bh_vfile_mount("vfile_bad.bin"),"truncated count rejected");
    }
    CHECK(write_bytes("vfile_bad.bin",fixture,1027),"write truncated directory");
    CHECK(!bh_vfile_mount("vfile_bad.bin"),"truncated directory rejected");
    CHECK(!bh_vfile_mount("vfile_missing.bin") && bh_vfile_error()==BH_VFILE_IO,"missing archive diagnosed");
    CHECK(!bh_vfile_mount(NULL) && bh_vfile_error()==BH_VFILE_ARGUMENT,"null archive rejected");
    CHECK(!bh_vfile_open(NULL,"rb") && bh_vfile_error()==BH_VFILE_ARGUMENT,"null name rejected");
    CHECK(!bh_vfile_open(names[0],NULL) && bh_vfile_error()==BH_VFILE_ARGUMENT,"null mode rejected");
    CHECK(bh_vfile_mount("vfile_valid.bin"),"reload releases old directory state");
    notifications=0;a=bh_vfile_open(names[3],"rb");
    CHECK(a && notifications==1 && progress_last==0.5f,"mount retains original global first-access counter");
    if (a) {bh_vfile_close(a);a=NULL;}
    CHECK(bh_vfile_detach_archive(),"detach backing FILE after closed handles");
    CHECK(!bh_vfile_open(names[0],"rb"),"uncached embedded open after detach fails safely");
    for (i=0;i<25;i++) {
        CHECK(bh_vfile_mount("vfile_valid.bin"),"repeat mount");
        a=bh_vfile_open(names[3],"rb");
        CHECK(a && bh_vfile_read(buffer,1,4,a)==4 && !memcmp(buffer,"WXYZ",4),"repeat read");
        /* Shutdown deliberately owns even still-open wrappers. */
        bh_vfile_shutdown();a=NULL;
    }
    memset(bad,0,4);CHECK(write_bytes("vfile_bad.bin",bad,4),"write empty archive");
    CHECK(bh_vfile_mount("vfile_bad.bin") && !bh_vfile_entry_count(),"zero-entry archive has valid lifecycle");
done:
    bh_vfile_shutdown();remove(names[0]);
}

static void line_tests(void) {
    const char *expected[]={"A\n","B\n","C\n","D\n","Tail"};
    long positions[]={3,6,8,10,14};
    BHVFile *file=NULL;
    char buffer[40];
    int backend,i;
    bh_vfile_shutdown();CHECK(bh_vfile_mount("vfile_valid.bin"),"mount for line checks");
    for (backend=0;backend<2;backend++) {
        if (backend) CHECK(bh_vfile_preload(names[1],"rb"),"preload line payload");
        file=bh_vfile_open(names[1],"rb");CHECK(file!=NULL,"open line backend");if (!file) continue;
        for (i=0;i<5;i++) {
            memset(buffer,0xa5,sizeof(buffer));
            CHECK(bh_vfile_gets(buffer+1,32,file)==buffer+1 && !strcmp(buffer+1,expected[i]),"CRLF LFCR CR LF and unterminated final line");
            CHECK((unsigned char)buffer[0]==0xa5 && (unsigned char)buffer[34]==0xa5,"line window guard bytes");
            CHECK(bh_vfile_tell(file)==positions[i],"line read-ahead rollback position");
        }
        CHECK(bh_vfile_eof(file)==1,"unterminated final bounded line sets EOF");
        memset(buffer,0xa5,sizeof(buffer));
        CHECK(!bh_vfile_gets(buffer+1,32,file) && (unsigned char)buffer[1]==0xa5,"EOF line leaves output untouched");
        CHECK(bh_vfile_seek(file,0,SEEK_SET)==0 && !bh_vfile_eof(file),"line seek resets EOF");
        CHECK(bh_vfile_gets(buffer,2,file) && !strcmp(buffer,"A") && bh_vfile_tell(file)==2,"newline at window end omitted, not paired beyond window");
        CHECK(bh_vfile_gets(buffer,2,file) && !strcmp(buffer,"\n") && bh_vfile_tell(file)==3,"split newline pair follows original cursor window");
        CHECK(bh_vfile_seek(file,0,SEEK_SET)==0,"seek for one-byte line window");
        memset(buffer,0xa5,sizeof(buffer));
        CHECK(bh_vfile_gets(buffer+1,1,file) && !strcmp(buffer+1,"A") && (unsigned char)buffer[3]==0xa5,"full one-byte window safely terminated in extra byte");
        CHECK(bh_vfile_gets(buffer,1,file) && !strcmp(buffer,"") && bh_vfile_tell(file)==2,"one-byte newline window consumes delimiter");
        CHECK(bh_vfile_gets(buffer,0,file)==NULL && bh_vfile_error()==BH_VFILE_ARGUMENT,"zero line window rejected");
        CHECK(bh_vfile_gets(buffer,ULONG_MAX,file)==NULL && bh_vfile_error()==BH_VFILE_OVERFLOW,"overflow line window rejected");
        CHECK(bh_vfile_gets(NULL,4,file)==NULL && bh_vfile_error()==BH_VFILE_ARGUMENT,"null line destination rejected");
        CHECK(bh_vfile_seek(file,-4,SEEK_END)==0,"seek last line");
        memset(buffer,0xa5,sizeof(buffer));
        CHECK(bh_vfile_gets(buffer+1,4,file) && !strcmp(buffer+1,"Tail") && (unsigned char)buffer[6]==0xa5,"exact full window retains all characters with extra terminator");
        CHECK(!bh_vfile_eof(file) && !bh_vfile_gets(buffer,4,file) && bh_vfile_eof(file)==1,"exact-end line sets EOF on next request");
        CHECK(bh_vfile_close(file)==0,"line backend close");file=NULL;
    }
    CHECK(write_bytes("bhvf_loose.txt","A\r\nB",4),"create loose line fixture");
    file=bh_vfile_open("bhvf_loose.txt","rb");CHECK(file!=NULL,"loose binary line open");
    if (file) {
        CHECK(bh_vfile_gets(buffer,8,file) && !strcmp(buffer,"A\r\n"),"loose binary mode delegates CRLF to CRT");
        CHECK(bh_vfile_gets(buffer,8,file) && !strcmp(buffer,"B"),"loose final line");
        CHECK(!bh_vfile_gets(buffer,8,file) && bh_vfile_eof(file)==0x10,"loose line EOF");
        bh_vfile_close(file);
    }
    file=bh_vfile_open("bhvf_loose.txt","r");CHECK(file!=NULL,"loose text line open");
    if (file) {
        CHECK(bh_vfile_gets(buffer,8,file) && !strcmp(buffer,"A\n"),"loose text mode retains CRT CRLF translation");
        bh_vfile_close(file);
    }
    bh_vfile_shutdown();remove("bhvf_loose.txt");
}

static void preload_tests(void) {
    BHVFile *a=NULL,*b=NULL;
    unsigned char buffer[20];
    char long_name[250];
    unsigned long i;
    bh_vfile_shutdown();CHECK(bh_vfile_mount("vfile_valid.bin"),"mount for preload checks");
    notifications=0;bh_vfile_set_progress(progress);
    CHECK(bh_vfile_preload(names[0],"rb") && notifications==1,"preload reads and records first access");
    a=bh_vfile_open(names[0],"rb");CHECK(a && a->memory && a->begin==0 && a->end==5,"cached wrapper memory layout");
    if (!a) goto done;
    CHECK(bh_vfile_read(buffer,1,2,a)==2 && !memcmp(buffer,"ab",2),"cached read");
    b=bh_vfile_open("BHVF_ALPHA.DAT","wb");
    CHECK(a==b && bh_vfile_tell(a)==0,"cached open shares wrapper and resets cursor regardless of mode");
    CHECK(bh_vfile_read(buffer,2,3,b)==2 && !memcmp(buffer,"abcde",5) && bh_vfile_eof(a)==1,"cached partial elements and shared EOF");
    CHECK(bh_vfile_close(a)==0 && bh_vfile_close(b)==0,"cached close preserves shared handle");
    CHECK(bh_vfile_preload(names[0],"rb") && notifications==1 && bh_vfile_tell(a)==0,"preloading cached entry resets cursor without duplicate allocation/access");
    CHECK(write_bytes(names[0],"LOOSE",5),"create override after caching");
    b=bh_vfile_open(names[0],"rb");
    CHECK(a==b && bh_vfile_read(buffer,1,5,b)==5 && !memcmp(buffer,"abcde",5),"cache takes precedence over new loose file");
    remove(names[0]);
    CHECK(bh_vfile_preload(names[2],"rb"),"preload zero-byte payload with owned nonnull memory");
    b=bh_vfile_open(names[2],"rb");CHECK(b && b->memory && bh_vfile_read(buffer,1,1,b)==0 && bh_vfile_eof(b)==1,"cached zero-byte EOF");
    CHECK(bh_vfile_detach_archive(),"detach retains cached storage");
    b=bh_vfile_open(names[0],"rb");
    CHECK(a==b && bh_vfile_read(buffer,1,5,b)==5 && !memcmp(buffer,"abcde",5),"cached payload readable after archive FILE close");
    CHECK(!bh_vfile_open(names[3],"rb"),"uncached payload unavailable after detach");
    CHECK(bh_vfile_mount("vfile_valid.bin"),"remount with persistent cached wrappers");
    b=bh_vfile_open(names[0],"rb");CHECK(a==b && notifications==2,"remount does not discard cache or renotify it");
    CHECK(write_bytes("bhvf_loose.txt","loose bytes",11),"write loose preload fixture");
    CHECK(bh_vfile_preload("bhvf_loose.txt","rb"),"preload loose backend");
    CHECK(remove("bhvf_loose.txt")==0,"preload closed underlying loose FILE");
    b=bh_vfile_open("bhvf_loose.txt","r");
    CHECK(b && b->memory && bh_vfile_read(buffer,1,11,b)==11 && !memcmp(buffer,"loose bytes",11),"loose cache survives source removal");
    CHECK(!bh_vfile_preload("bhvf_missing_preload","rb") && bh_vfile_error()==BH_VFILE_NOT_FOUND,"missing preload rejected");
    memset(long_name,'x',249);long_name[249]=0;
    CHECK(!bh_vfile_preload(long_name,"rb") && bh_vfile_error()==BH_VFILE_ARGUMENT,"overlong cached name rejected");
    CHECK(!bh_vfile_preload(NULL,"rb") && bh_vfile_error()==BH_VFILE_ARGUMENT,"null cached name rejected");
    for (i=0;i<20;i++) {
        bh_vfile_shutdown();
        CHECK(bh_vfile_mount("vfile_valid.bin") && bh_vfile_preload(names[0],"rb"),"repeated preload ownership cycle");
        a=bh_vfile_open(names[0],"rb");
        CHECK(a && bh_vfile_read(buffer,1,5,a)==5,"repeated memory read");
    }
done:
    bh_vfile_shutdown();remove(names[0]);remove("bhvf_loose.txt");
}

static void edge_tests(void) {
    BHVFile *file;
    unsigned char changed[2048],buffer[12];
    char long_name[248];
    int i;
    bh_vfile_shutdown();
    CHECK(bh_vfile_mount("vfile_valid.bin"),"mount for callback mutation checks");
    bh_vfile_set_progress(mutation_progress);callback_mutations_rejected=0;
    file=bh_vfile_open(names[0],"rb");
    CHECK(file && callback_mutations_rejected && bh_vfile_error()==BH_VFILE_OK,"callback cannot invalidate owning archive state");
    if (file) {CHECK(bh_vfile_read(buffer,1,5,file)==5,"callback rejection leaves payload usable");bh_vfile_close(file);}
    bh_vfile_set_progress(NULL);bh_vfile_set_debug(7);
    CHECK(write_bytes(names[0],"loose",5),"diagnostic loose fixture");
    file=bh_vfile_open(names[0],"rb");CHECK(file!=NULL,"external diagnostic path");if (file) bh_vfile_close(file);
    remove(names[0]);
    CHECK(bh_vfile_preload(names[0],"rb"),"embedded diagnostic path");
    file=bh_vfile_open(names[0],"rb");CHECK(file!=NULL,"cached diagnostic path");if (file) bh_vfile_close(file);
    bh_vfile_set_debug(0);bh_vfile_shutdown();
    memcpy(changed,fixture,fixture_length);
    memset(long_name,'X',247);long_name[247]=0;memcpy(changed+4,long_name,248);
    CHECK(write_bytes("vfile_bad.bin",changed,fixture_length),"write maximum-length name fixture");
    CHECK(bh_vfile_mount("vfile_bad.bin") && bh_vfile_preload(long_name,"rb"),"247-character name fits disk and cache node");
    file=bh_vfile_open(long_name,"rb");CHECK(file && bh_vfile_read(buffer,1,5,file)==5 && !memcmp(buffer,"abcde",5),"maximum-length cached lookup");
    bh_vfile_shutdown();
    /* Allocation-boundary directory errors at every truncation of one record. */
    memcpy(changed,fixture,fixture_length);u32(changed,1);
    for (i=4;i<260;i++) {
        CHECK(write_bytes("vfile_bad.bin",changed,(unsigned long)i),"write one-record truncation");
        CHECK(!bh_vfile_mount("vfile_bad.bin") && bh_vfile_error()==BH_VFILE_FORMAT,"each truncated directory rejected");
    }
    CHECK(bh_vfile_mount("vfile_valid.bin"),"mount after all rejected truncations");
    file=bh_vfile_open(names[0],"rb");CHECK(file && bh_vfile_read(buffer,1,5,file)==5,"reader still valid after malformed input sequence");
    bh_vfile_shutdown();
}

static unsigned long real_archive(const char *path) {
    FILE *raw=NULL;
    unsigned char bytes[256],expected[4096],actual[4098];
    unsigned long count,i,begin,end,remaining,want,entries=0,total=0,classes[6]={0},preload_bytes=0;
    const char *extensions[]={".BSP",".PCX",".TXT",".WAV",".LEV",".PTH"};
    const char *extension;
    char name[248],raw_line[258],line[258];
    BHVFile *file;
    int k;
    bh_vfile_shutdown();
    raw=fopen(path,"rb");CHECK(raw!=NULL,"independent raw archive open");if (!raw) goto done;
    if (fread(bytes,1,4,raw)!=4) {CHECK(0,"raw count read");goto done;}count=from_u32(bytes);
    CHECK(count==1229,"pinned real archive count");if (count!=1229) goto done;
    if (!bh_vfile_mount(path)) {CHECK(0,"mount real directory");goto done;}
    CHECK(bh_vfile_entry_count()==count,"mount real directory count");
    notifications=0;bh_vfile_set_progress(progress);
    for (i=0;i<count;i++) {
        CHECK(fseek(raw,4+(long)i*256,SEEK_SET)==0 && fread(bytes,1,256,raw)==256,"independent directory record");
        memcpy(name,bytes,248);name[247]=0;begin=from_u32(bytes+248);end=from_u32(bytes+252);
        file=bh_vfile_open(name,"rb");CHECK(file!=NULL,"real VFILE open");if (!file) continue;
        CHECK(fseek(raw,(long)begin,SEEK_SET)==0,"raw payload seek");remaining=end-begin;
        while (remaining) {
            want=remaining>4096?4096:remaining;memset(actual,0xa5,sizeof(actual));
            CHECK(fread(expected,1,want,raw)==want,"raw payload read");
            CHECK(bh_vfile_read(actual+1,1,want,file)==want,"real VFILE payload read");
            CHECK(!memcmp(expected,actual+1,want) && actual[0]==0xa5 && actual[want+1]==0xa5,"payload equality and guard bytes");
            total+=want;remaining-=want;
        }
        CHECK(!bh_vfile_eof(file) && bh_vfile_tell(file)==(long)(end-begin),"real exact-end state");
        CHECK(bh_vfile_read(actual,1,1,file)==0 && bh_vfile_eof(file)==1,"real EOF clamp");
        CHECK(bh_vfile_close(file)==0,"real wrapper close");entries++;
        extension=strrchr(name,'.');
        for (k=0;k<6;k++) if (extension && !_stricmp(extension,extensions[k])) {
            if (!classes[k]) {
                CHECK(bh_vfile_preload(name,"rb"),"representative real asset preload");
                file=bh_vfile_open(name,"rb");CHECK(file && file->memory,"representative asset memory backend");
                if (file) {
                    remaining=end-begin;CHECK(fseek(raw,(long)begin,SEEK_SET)==0,"raw representative seek");
                    while (remaining) {
                        want=remaining>4096?4096:remaining;
                        CHECK(fread(expected,1,want,raw)==want,"raw representative read");
                        CHECK(bh_vfile_read(actual,1,want,file)==want && !memcmp(actual,expected,want),"representative memory payload equality");
                        remaining-=want;preload_bytes+=want;
                    }
                    if (k==2) {
                        CHECK(fseek(raw,(long)begin,SEEK_SET)==0 && fgets(raw_line,257,raw)!=NULL,"independent real script first line");
                        want=(unsigned long)strcspn(raw_line,"\r\n");
                        if (raw_line[want]) {raw_line[want]='\n';raw_line[want+1]=0;}
                        CHECK(bh_vfile_seek(file,0,SEEK_SET)==0 && bh_vfile_gets(line,256,file) && !strcmp(line,raw_line),"real cached script normalized line");
                        /* Exercise the same real text through the archive backend
                           before its cache was populated: use another TXT below. */
                    }
                    bh_vfile_close(file);
                }
            } else if (k==2 && classes[k]==1) {
                CHECK(fseek(raw,(long)begin,SEEK_SET)==0 && fgets(raw_line,257,raw)!=NULL,"independent second script line");
                want=(unsigned long)strcspn(raw_line,"\r\n");
                if (raw_line[want]) {raw_line[want]='\n';raw_line[want+1]=0;}
                file=bh_vfile_open(name,"rb");CHECK(file!=NULL,"second real script archive open");
                if (file) {CHECK(bh_vfile_gets(line,256,file) && !strcmp(line,raw_line),"real embedded script normalized line");bh_vfile_close(file);}
            }
            classes[k]++;
        }
    }
    CHECK(notifications==(int)count && progress_last==1.0f,"real first-access notification coverage");
    for (k=0;k<6;k++) CHECK(classes[k]>0,"all six asset classes exercised");
    printf("{\"scenario\":\"vfile-real-assets\",\"entries\":%lu,\"payload_bytes\":%lu,\"preload_payload_bytes\":%lu,\"classes\":[%lu,%lu,%lu,%lu,%lu,%lu]}\n",
           entries,total,preload_bytes,classes[0],classes[1],classes[2],classes[3],classes[4],classes[5]);
done:
    if (raw) fclose(raw);
    bh_vfile_shutdown();return entries;
}

int bh_vfile_checkpoint(const char *archive, unsigned long *total_checks, unsigned long *entries) {
    checks=0;failures=0;core_tests();line_tests();preload_tests();edge_tests();
    *entries=archive ? real_archive(archive) : 0;
    bh_vfile_shutdown();
    remove("vfile_valid.bin");remove("vfile_bad.bin");
    *total_checks=checks;
    printf("{\"scenario\":\"vfile\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures ? 1 : 0;
}

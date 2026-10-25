#include "../src/bh_player_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL player-input: %s\n",s);}} while(0)

typedef struct Memory {unsigned long calls,live,fail_at;} Memory;
static void *__cdecl allocate(unsigned long bytes,void *user) {
    Memory *memory=(Memory*)user;
    void *p;
    ++memory->calls;
    if(memory->calls==memory->fail_at)return NULL;
    p=malloc(bytes);
    if(p) {memset(p,0xa5,bytes);++memory->live;}
    return p;
}
static void __cdecl release(void *p,void *user) {
    Memory *memory=(Memory*)user;
    if(p) {--memory->live;free(p);}
}
static void ownership(void) {
    Memory memory={0,0,0};
    BHSceneAllocator allocator={allocate,release,&memory};
    BHPlayerInputOwner owner;
    BHPlayerInputList *a,*b,expected;
    unsigned long i;
    CHECK(bh_player_input_owner_init(&owner,&allocator),"explicit owner init");
    a=bh_player_input_create(&owner,0);
    CHECK(a!=NULL,"real allocation");
    if(!a)return;
    memset(&expected,0xa5,sizeof(expected));expected.previous=expected.next=NULL;
    expected.flags=0;expected.count=0;
    for(i=0;i<256;++i)expected.bindings[i].kind=0;
    expected.state[0]=expected.state[1]=0;
    CHECK(!memcmp(a,&expected,sizeof(expected)),"constructor exact selective byte writes");
    b=bh_player_input_create(&owner,-7);
    CHECK(b && owner.first==b && b->previous==NULL && b->next==a &&
        a->previous==b && a->next==NULL && b->flags==2,"prepend and nonzero mode");
    memory.fail_at=memory.calls+1;
    CHECK(!bh_player_input_create(&owner,1) && owner.first==b &&
        b->next==a && a->previous==b && memory.live==2,"allocation failure preserves list");
    bh_player_input_owner_dispose(&owner);
    CHECK(!owner.first && memory.live==0,"dispose actual allocations");
    bh_player_input_owner_dispose(&owner);
    CHECK(memory.live==0,"empty dispose");
    CHECK(!bh_player_input_owner_init(NULL,NULL),"reject absent owner");
    allocator.release=NULL;
    CHECK(!bh_player_input_owner_init(&owner,&allocator),"reject unpaired allocator");
    CHECK(bh_player_input_owner_init(&owner,NULL),"default CRT allocator");
    a=bh_player_input_create(&owner,1);
    CHECK(a && a->flags==2 && a->count==0,"default CRT allocate and constructor");
    bh_player_input_owner_dispose(&owner);
}
static void writers(void) {
    BHPlayerInputList list,expected;
    BHInputBinding *e;
    memset(&list,0xa5,sizeof(list));list.count=0;expected=list;e=&expected.bindings[255];
    memcpy(e->name,"nineteen-characters",19);e->name[19]=0;
    e->kind=2;e->flags=0xd3;e->modifiers=0xabcd;
    e->key[0]=0x2345;e->key[1]=0x1234;e->key[2]=0x3456;e->key[3]=0x4567;
    expected.count=256;
    bh_player_input_bind_axis(&list,255,"nineteen-characters-and-more",0xd3,0xabcd,
                              0x1234,0x2345,0x3456,0x4567);
    CHECK(!memcmp(&list,&expected,sizeof(list)),"axis order, high index, truncation and exact byte scope");
    e=&expected.bindings[0];e->name[0]='x';e->name[1]=0;
    e->kind=1;e->flags=0x81;e->modifiers=0xfedc;e->key[0]=0x9876;
    bh_player_input_bind_key(&list,0,"x",0x81,0xfedc,0x9876);
    CHECK(!memcmp(&list,&expected,sizeof(list)),"key preserves axis bytes/name tail and count maximum");
    e=&expected.bindings[255];e->name[0]=0;e->kind=1;e->flags=0;
    e->modifiers=0;e->key[0]=0;
    bh_player_input_bind_key(&list,255,"",0,0,0);
    CHECK(!memcmp(&list,&expected,sizeof(list)),"axis-to-key rebinding preserves key[1..3]");
    e=&expected.bindings[128];memcpy(e->name,"cheat",6);e->kind=5;
    bh_player_input_bind_string(&list,128,"cheat");
    CHECK(!memcmp(&list,&expected,sizeof(list)),"string binding retains keys flags and modifier words");
    e=&expected.bindings[0];memcpy(e->name,"1234567890123456789",20);e->kind=5;
    bh_player_input_bind_string(&list,0,"123456789012345678901234");
    CHECK(!memcmp(&list,&expected,sizeof(list)),"string binding truncates to19 with high count retained");
}
static void setup_profile(void) {
    /* Golden by binding index, independently ordered from the call table.
       0 denotes an untouched slot; 1 is a single key; 2 is an axis. */
    static const unsigned short golden[36][5]={
        {2,0x1f,0x1e,0x23,0x26},{2,0x1d,0x1c,0x22,0x25},
        {2,0x20,0x21,0x24,0x27},{1,0x31},{1,0x33},{1,0x32},
        {1,0x3a},{1,0x3b},{1,0x3c},{1,0x3d},{1,0x3e},{1,0x3f},
        {1,0x2c},{1,0x2d},{1,0x2e},{1,0x2f},{1,0x30},
        {1,0x35},{1,0x36},{1,0x37},{1,0x38},{1,0x39},
        {1,0x28},{1,0x2a},{1,0x29},
        {2,0x41,0x40,0x45,0x47},{2,0x43,0x42,0x44,0x46},
        {1,0x2b},{1,0x34},{0},{0},{0},
        {1,9},{1,0x1b},{1,0x59},{1,0x4e}
    };
    BHPlayerInputList list,expected;
    BHInputProfile profile,unchanged;
    unsigned char *bytes=(unsigned char*)&profile;
    unsigned long i,j,offset;
    memset(&list,0xa5,sizeof(list));list.count=0;
    for(i=0;i<256;++i)list.bindings[i].kind=0;
    for(i=0;i<sizeof(profile);++i)bytes[i]=(unsigned char)(i+0x80);
    unchanged=profile;expected=list;expected.count=36;
    for(i=0;i<36;++i)if(golden[i][0]) {
        expected.bindings[i].kind=(unsigned char)golden[i][0];
        expected.bindings[i].flags=0;expected.bindings[i].modifiers=0;
        expected.bindings[i].name[0]=0;
        for(j=0;j<(golden[i][0]==2 ? 4u : 1u);++j) {
            offset=golden[i][j+1];
            expected.bindings[i].key[j]=(unsigned short)(i<29 ? bytes[offset] : offset);
        }
    }
    bh_player_input_setup(&list,&profile);
    CHECK(!memcmp(&list,&expected,sizeof(list)),"all 33 setup bindings, unsigned profile bytes, fixed keys and untouched holes");
    CHECK(!memcmp(&profile,&unchanged,sizeof(profile)),"profile borrowed unchanged");
    list.count=256;expected=list;
    bh_player_input_setup(&list,&profile);
    CHECK(!memcmp(&list,&expected,sizeof(list)),"repeat setup preserves unrelated high count");
}
static void packets(void) {
    static const unsigned char mixed[]={
        0xaa,7,
        0,3,0,1,255,0xcc,0xcc,0xcc,
        1,1,1,0xcc,
        2,2,0x78,0x56,0x34,0x12,
        3,3,1,0x80,0xff,0x7f,
        4,4,0xab,0xcd,0xef,1,
        0xee,
        0,0,0xcc,0xcc
    };
    static const unsigned char padded_lengths[]={4,4,4,8,8,8,8,12,12};
    BHPlayerInputState state,expected,original;
    unsigned char buffer[300];
    unsigned long i,n,len;
    memset(&original,0xa5,sizeof(original));expected=original;
    expected.values[0]=1;expected.values[1]=0;expected.values[255]=1;
    expected.values[2]=0x12345678;expected.values[3]=0x7fff8001;
    expected.values[4]=0x01efcdab;
    state=original;bh_player_input_apply_packet(&state,mixed);
    CHECK(!memcmp(&state,&expected,sizeof(state)),"mixed packet raw decoder all five tags, padding, unknown and empty batch");
    state=original;
    CHECK(bh_player_input_apply_packet_checked(&state,mixed,sizeof(mixed)) &&
        !memcmp(&state,&expected,sizeof(state)),"checked packet same valid behavior");
    for(i=0;i<sizeof(mixed);++i) {
        state=original;
        CHECK(!bh_player_input_apply_packet_checked(&state,mixed,i) &&
            !memcmp(&state,&original,sizeof(state)),"every truncated packet length rejects atomically");
    }
    CHECK(!bh_player_input_apply_packet_checked(NULL,mixed,sizeof(mixed)) &&
        !bh_player_input_apply_packet_checked(&state,NULL,2),"packet absent argument rejection");
    for(n=0;n<9;++n) {
        memset(buffer,0xcc,sizeof(buffer));buffer[0]=0xfe;buffer[1]=2;
        buffer[2]=0;buffer[3]=(unsigned char)n;
        for(i=0;i<n;++i)buffer[4+i]=(unsigned char)i;
        len=2+padded_lengths[n];buffer[len]=2;buffer[len+1]=254;
        buffer[len+2]=0x11;buffer[len+3]=0x22;buffer[len+4]=0x33;buffer[len+5]=0x44;
        state=original;expected=original;
        for(i=0;i<n;++i)expected.values[i]=1;
        expected.values[254]=0x44332211;
        CHECK(bh_player_input_apply_packet_checked(&state,buffer,len+6) &&
            !memcmp(&state,&expected,sizeof(state)),"batch record padding places next record exactly");
    }
    memset(buffer,0xff,sizeof(buffer));buffer[1]=255;state=original;
    CHECK(bh_player_input_apply_packet_checked(&state,buffer,257) &&
        !memcmp(&state,&original,sizeof(state)),"unsigned255 record count and unknown tags");
    buffer[1]=0;state=original;
    CHECK(bh_player_input_apply_packet_checked(&state,buffer,2) &&
        !memcmp(&state,&original,sizeof(state)),"zero records ignore first header byte");
}
static void transitions(void) {
    static const unsigned long before[]={0,1,2,0xffffffff,0xfffffffe,0x7fffffff,0x80000000};
    static const unsigned long after[]={0xffffffff,2,3,0xfffffffe,0xfffffffd,0x80000000,0x7fffffff};
    static const unsigned char result[]={1,0,2,3,3,2,3};
    BHPlayerInputState state,expected;
    unsigned long i;
    for(i=0;i<sizeof(before)/sizeof(before[0]);++i) {
        memset(&state,0x55,sizeof(state));state.values[255]=before[i];
        expected=state;expected.values[255]=after[i];
        CHECK(bh_player_input_transition(&state,255)==result[i] &&
            !memcmp(&state,&expected,sizeof(state)),"signed state transition result and x86 wrap at high index");
    }
}
int bh_player_input_tests(void) {
    checks=failures=0;ownership();writers();setup_profile();packets();transitions();
    printf("player-input: %lu checks, %lu failures\n",checks,failures);
    return failures==0;
}
int bh_player_input_checkpoint(unsigned long *count) {
    bh_player_input_tests();if(count)*count=checks;
    printf("{\"scenario\":\"player-input\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_PLAYER_INPUT_STANDALONE
int main(void) {return bh_player_input_checkpoint(NULL);}
#endif

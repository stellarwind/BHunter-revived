#include "../src/bh_source_files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int bh_source_files_checkpoint(unsigned long *count){
    BHSourceFiles c;HKEY key;DWORD disposition;char subkey[128],directory[512],path[512],source[512];
    unsigned char *bytes;FILE *file;unsigned long checks=0,failures=0,i,j,n,xorword,expected;
    const unsigned long sizes[]={0,1,2,3,4,7,65535,65536,65537,65540,131075};
#define CHECK(x) do{++checks;if(!(x)){++failures;printf("FAIL source files %d\n",__LINE__);}}while(0)
    bh_source_files_init(&c);CHECK(c.root==HKEY_LOCAL_MACHINE&&c.expected_checksum==0x6b405d08UL);
    sprintf(subkey,"Software\\BHunterRestorationTests-source-%lu",GetCurrentProcessId());
    CHECK(GetTempPathA(sizeof(directory),directory)>0);
    sprintf(path,"%sBHunterRestoration-source-%lu",directory,GetCurrentProcessId());
    CHECK(CreateDirectoryA(path,NULL));sprintf(source,"%s\\copyright.txt",path);
    CHECK(RegCreateKeyExA(HKEY_CURRENT_USER,subkey,0,NULL,0,KEY_ALL_ACCESS,NULL,&key,&disposition)==ERROR_SUCCESS&&disposition==REG_CREATED_NEW_KEY);
    c.root=HKEY_CURRENT_USER;c.subkey=subkey;
    CHECK(!bh_source_files_check(&c)&&c.error==1);
    CHECK(RegSetValueExA(key,"SourceDir",0,REG_SZ,(BYTE*)path,strlen(path)+1)==ERROR_SUCCESS);
    CHECK(!bh_source_files_check(&c)&&c.error==4);
    bytes=(unsigned char*)malloc(131075);CHECK(bytes!=NULL);if(!bytes)return 1;
    for(i=0;i<131075;i++)bytes[i]=(unsigned char)(i*31+i/251);
    for(j=0;j<sizeof(sizes)/sizeof(sizes[0]);j++){
        n=sizes[j];file=fopen(source,"wb");CHECK(file!=NULL);if(!file)break;
        CHECK(fwrite(bytes,1,n,file)==n);CHECK(fclose(file)==0);
        expected=0;for(i=0;i<n/4;i++){memcpy(&xorword,bytes+i*4,4);expected^=xorword;}
        c.expected_checksum=expected;CHECK(bh_source_files_check(&c)&&!c.error&&c.checksum==expected&&c.bytes==n&&c.blocks==(n+65535)/65536);
        c.expected_checksum^=1;CHECK(!bh_source_files_check(&c)&&!c.error);
    }
    strcat(path,"\\");CHECK(RegSetValueExA(key,"SourceDir",0,REG_SZ,(BYTE*)path,strlen(path)+1)==ERROR_SUCCESS);
    c.expected_checksum=expected;CHECK(bh_source_files_check(&c));
    CHECK(RegSetValueExA(key,"SourceDir",0,REG_BINARY,(BYTE*)"abc",3)==ERROR_SUCCESS);
    CHECK(!bh_source_files_check(&c)&&c.error==1);
    CHECK(RegCloseKey(key)==ERROR_SUCCESS);CHECK(RegDeleteKeyA(HKEY_CURRENT_USER,subkey)==ERROR_SUCCESS);
    CHECK(DeleteFileA(source));path[strlen(path)-1]=0;CHECK(RemoveDirectoryA(path));free(bytes);
    if(count)*count=checks;printf("{\"scenario\":\"source-files\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
#undef CHECK
}

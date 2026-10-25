#include "../../../../src/bh_pcx_reader.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL pcx-close-reuse: %s\n",s);}}while(0)
int main(void) {
 unsigned char bytes[901],saved[768];BHPcxReader reader;HANDLE handle;FILE *file;unsigned long i;
 memset(bytes,0,sizeof(bytes));bytes[0]=10;bytes[2]=1;bytes[3]=8;bytes[8]=1;bytes[10]=1;
 bytes[128]=1;bytes[129]=2;bytes[130]=3;bytes[131]=4;bytes[132]=12;
 for(i=0;i<768;++i)bytes[133+i]=(unsigned char)(i*13+3);
 CHECK(!bh_pcx_reader_close(NULL),"NULL source guard");bh_pcx_reader_init(&reader);
 CHECK(bh_pcx_reader_close(&reader)&&!reader.file,"fresh empty owner closes safely");
 CHECK(!bh_pcx_reader_open(&reader,"pcx-close-reuse-missing-dir/missing.pcx")&&!reader.file,"missing file owns no stream");
 CHECK(bh_pcx_reader_close(&reader)&&!reader.file&&reader.error==BH_PCX_OK,"failed-open cleanup clears explicit reader state");
 file=fopen("pcx-close-reuse-private.pcx","wb");CHECK(file!=NULL,"new private PCX fixture");if(!file)goto done;
 CHECK(fwrite(bytes,sizeof(bytes),1,file)==1&&fclose(file)==0,"complete bounded private fixture written");
 CHECK(bh_pcx_reader_open(&reader,"pcx-close-reuse-private.pcx")&&reader.file&&reader.width==2&&reader.height==2,"actual source reader owns valid loose VFILE");if(!reader.file)goto done;
 memcpy(saved,reader.palette,sizeof(saved));
 handle=CreateFile("pcx-close-reuse-private.pcx",GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
 CHECK(handle==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_SHARING_VIOLATION,"positive control proves original FILE handle remains open before close");if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);
 CHECK(bh_pcx_reader_close(&reader),"existing actual close implementation releases owned VFILE");
 CHECK(!reader.file&&!reader.width&&!reader.height&&!reader.palette[0]&&!reader.map[1],"explicit state extension clears entire scoped reader");
 CHECK(!memcmp(saved,bytes+133,768),"caller-retained palette survives scoped reader clearing");
 handle=CreateFile("pcx-close-reuse-private.pcx",GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
 CHECK(handle!=INVALID_HANDLE_VALUE,"exclusive native open proves actual prior FILE handle released");if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);
 CHECK(bh_pcx_reader_close(&reader)&&!reader.file,"repeated close is safe and owns no second release");
 CHECK(bh_pcx_reader_open(&reader,"pcx-close-reuse-private.pcx")&&reader.file,"owner reopens with a fresh actual VFILE after close");
 CHECK(bh_pcx_reader_close(&reader),"reopened owned stream closes");
done:
 bh_pcx_reader_close(&reader);CHECK(DeleteFile("pcx-close-reuse-private.pcx"),"private fixture deletable after all closes");
 printf("{\"scenario\":\"pcx-close-reuse\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);return failures!=0;
}

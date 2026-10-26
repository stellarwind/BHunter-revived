#include "../src/bh_editor_numeric.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;if(failures<10)printf("failure %d\n",__LINE__);}}while(0)
static short value(BHEntity *e){short v;memcpy(&v,(char*)e+0x33a,2);return v;}
static short signedword(long v){unsigned short u=(unsigned short)v;return u<32768?(short)u:(short)((long)u-65536);}
int bh_editor_numeric_checkpoint(unsigned long *count){
 BHEditorNumericContext c;BHEntity e,before;long state,input,k,adjust,result,mode;short v;unsigned long bits;static const long keys[3]={-1,0,9};
 checks=failures=0;memset(&e,0xa5,sizeof(e));c.selected=3;c.entry_state=0;CHECK(bh_editor_numeric_key(&c,&e,1)&&value(&e)==1&&c.entry_state==1);CHECK(bh_editor_numeric_key(&c,&e,2)&&value(&e)==12);CHECK(bh_editor_numeric_key(&c,&e,3)&&value(&e)==123);CHECK(bh_editor_numeric_key(&c,&e,4)&&value(&e)==1234);CHECK(bh_editor_numeric_key(&c,&e,5)&&value(&e)==2345);CHECK(bh_editor_numeric_key(&c,&e,-1)&&value(&e)==-2345&&c.entry_state==2);CHECK(bh_editor_numeric_key(&c,&e,6)&&value(&e)==-3456);CHECK(bh_editor_numeric_key(&c,&e,-1)&&value(&e)==3456&&c.entry_state==1);
 c.entry_state=0;CHECK(bh_editor_numeric_key(&c,&e,-1)&&value(&e)==0&&c.entry_state==2);CHECK(bh_editor_numeric_key(&c,&e,7)&&value(&e)==-7);before=e;c.selected=-1;CHECK(bh_editor_numeric_key(&c,0,9)&&!memcmp(&e,&before,sizeof(e)));c.selected=8;CHECK(!bh_editor_numeric_key(&c,&e,9));c.selected=3;c.entry_state=77;CHECK(bh_editor_numeric_key(&c,&e,9)&&!memcmp(&e,&before,sizeof(e)));
 /* Exhaustive signed16 parameter domain with independent closed-form decimal
    suffix oracle; original stores are verified modulo65536, including -32768. */
 for(state=1;state<=2;++state)for(bits=0;bits<65536;++bits)for(k=0;k<3;++k){input=keys[k];v=signedword((long)bits);memcpy((char*)&e+0x33a,&v,2);c.entry_state=state;adjust=v;mode=state;
  if(input<0){result=-(long)v;mode=3-state;}else if(state==1){if(v>=1000)adjust=v%1000;result=adjust*10+input;}else{if(v<=-1000)adjust=-((-v)%1000);result=adjust*10-input;}
  CHECK(bh_editor_numeric_key(&c,&e,input)&&value(&e)==signedword(result)&&c.entry_state==mode);
 }
 c.entry_state=0;CHECK(bh_editor_numeric_key(&c,&e,65535)&&value(&e)==-1);c.entry_state=0;CHECK(bh_editor_numeric_key(&c,&e,32768)&&value(&e)==-32768);CHECK(!bh_editor_numeric_key(0,&e,2));
 if(count)*count=checks;printf("{\"scenario\":\"editor-numeric\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_EDITOR_NUMERIC_STANDALONE
int main(void){return bh_editor_numeric_checkpoint(0);}
#endif

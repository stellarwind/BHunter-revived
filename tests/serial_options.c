#include "../src/bh_serial_options.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
static unsigned long checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;printf("FAIL serial-options line%d\n",__LINE__);}}while(0)
int bh_serial_options_tests(unsigned long *count){
 static const int rates[12]={1200,2400,4800,9600,14400,19200,38400,56000,57600,115200,128000,256000};
 static const char *const parity[4]={"None","Odd","Even","Mark"};
 static const char *const flow[5]={"None","Software Xon/Xoff","Hardware RTS","Hardware DTR","Hardware RTS/DTR"};
 static const unsigned char id[16]={0x00,0xc2,0xdc,0xf6,0xfe,0xa2,0xd0,0x11,0x9c,0x4f,0x00,0xa0,0xc9,0x05,0x42,0x5e};
 GUID guid,other;BHModemNames names;char expected[40],too_long[130];int i,j,known,next;BOOL (CALLBACK *callback)(const GUID*,DWORD,const void*,void*);
 checks=failures=0;memcpy(&guid,id,16);memset(&other,0,sizeof(other));callback=bh_modem_address_element;
 CHECK(sizeof(GUID)==16&&sizeof(names.names[0])==128&&sizeof(names)==1028);
 for(i=0;i<12;++i){sprintf(expected,"%d bits/sec",rates[i]);CHECK(strcmp(bh_serial_baud_label(rates[i]),expected)==0);CHECK(bh_serial_baud_next(rates[i])==rates[(i+1)%12]);}
 next=rates[0];for(i=0;i<120;++i){CHECK(next==rates[i%12]);next=bh_serial_baud_next(next);}
 for(i=-4096;i<=270000;i+=17){known=0;for(j=0;j<12;++j)if(i==rates[j])known=1;if(!known){CHECK(bh_serial_baud_label(i)==NULL);CHECK(bh_serial_baud_next(i)==9600);}}
 CHECK(!bh_serial_baud_label(INT_MIN)&&!bh_serial_baud_label(INT_MAX));CHECK(bh_serial_baud_next(INT_MIN)==9600&&bh_serial_baud_next(INT_MAX)==9600);
 for(i=-20;i<20;++i){const char *label=bh_serial_stop_label(i);CHECK((i==0||i==2)?label!=NULL:label==NULL);CHECK(bh_serial_stop_next(i)==(i==0?2:0));
 CHECK(i>=0&&i<4?strcmp(bh_serial_parity_label(i),parity[i])==0:bh_serial_parity_label(i)==NULL);CHECK(bh_serial_parity_next(i)==(i>=0&&i<3?i+1:0));
 CHECK(i>=0&&i<5?strcmp(bh_serial_flow_label(i),flow[i])==0:bh_serial_flow_label(i)==NULL);CHECK(bh_serial_flow_next(i)==(i>=1&&i<=4?(i+1)%5:1));}
 CHECK(strcmp(bh_serial_stop_label(0),"One")==0&&strcmp(bh_serial_stop_label(2),"Two")==0);
 memset(&names,0,sizeof(names));CHECK(bh_modem_name_find(&names,"Test")==-1);
 CHECK(callback(&other,0,"Ignore\0",&names)&&names.count==0);
 CHECK(callback(&guid,0,"Modem One\0mODEM tWO\0Modem One\0",&names));CHECK(names.count==3);
 CHECK(bh_modem_name_find(&names,"MODEM ONE")==0);CHECK(bh_modem_name_find(&names,"modem two")==1);CHECK(bh_modem_name_find(&names,"absent")==-1);
 CHECK(callback(&guid,999,"\0",&names)&&names.count==3);
 CHECK(callback(&guid,0,"Three\0Four\0Five\0Six\0Seven\0Ignored\0",&names)&&names.count==8);
 CHECK(strcmp(names.names[7],"Seven")==0);CHECK(bh_modem_name_find(&names,"Ignored")==-1);
 CHECK(callback(&guid,0,"Over capacity\0",&names)&&names.count==8);
 memset(&names,0,sizeof(names));memset(too_long,'x',127);too_long[127]=0;too_long[128]=0;
 CHECK(callback(&guid,0,too_long,&names)&&names.count==1&&strlen(names.names[0])==127);
 too_long[127]='x';too_long[128]=0;too_long[129]=0;CHECK(callback(&guid,0,too_long,&names)&&names.count==1);
 names.count=-1;CHECK(callback(&guid,0,"bad\0",&names)&&names.count==-1);CHECK(bh_modem_name_find(&names,"bad")==-1);
 names.count=9;CHECK(callback(&guid,0,"bad\0",&names)&&names.count==9);
 CHECK(callback(NULL,0,NULL,NULL));CHECK(callback(&guid,0,NULL,NULL));CHECK(bh_modem_name_find(NULL,"bad")==-1);
 if(count)*count=checks;printf("{\"scenario\":\"serial-options\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}

#include "bh_menu_row.h"
#include "bh_atlas_text.h"
#include <string.h>
#include <stddef.h>
typedef char menu_row_layout[sizeof(BHMenuRow)==28&&offsetof(BHMenuRow,title)==12&&offsetof(BHMenuRow,refresh)==24?1:-1];
static short yadd(short value,int add){unsigned short bits=(unsigned short)value+(unsigned short)add;short out;memcpy(&out,&bits,2);return out;}
static unsigned short convert(unsigned char b){
 if(b==32)return 256;if(b>='A'&&b<='Z')return b+192;if(b>='0'&&b<='9')return b+235;if(b>='a'&&b<='z')return b+196;
 switch(b){case 0xc4:return 319;case 0xd6:return 320;case 0xdc:return 321;case 0xe4:return 322;case 0xf6:return 323;case 0xfc:return 324;}return 0;
}
static int valid_title(const unsigned char *p){unsigned long n=0;if(!p)return 1;while(*p){if(*p==10)n=0;else if(convert(*p)&&++n>255)return 0;++p;}return 1;}
static long centered(long width){return 64-(short)(width/2);}
static void queued(BHMenuRows *owner,int result){if(result<=0){++owner->dropped;owner->error=4;}}
static int paint(BHMenuRows *owner,BHMenuRow *row,unsigned char *body){
 const unsigned char *title=row->title,*p;unsigned short line[256],code;unsigned long at=0;short y;unsigned char *next,*scan;
 queued(owner,bh_text_queue_clear(owner->text,row->page,0));
 if(!title)y=26;else{
  y=body?15:21;for(p=title;*p;++p)if(*p==10)y=yadd(y,-6);
  for(p=title;*p;++p){if(*p==10){line[at]=0;queued(owner,bh_text_queue_wide(owner->text,row->page,line,centered(bh_atlas_page_wide_width(row->page,line)),y));y=yadd(y,18);at=0;}else if((code=convert(*p))!=0)line[at++]=code;}
  line[at]=0;queued(owner,bh_text_queue_wide(owner->text,row->page,line,centered(bh_atlas_page_wide_width(row->page,line)),y));y=yadd(y,21);
 }
 while(body){next=NULL;for(scan=body;*scan;++scan)if(*scan==10){*scan=0;if(scan[1])next=scan+1;break;}
  queued(owner,bh_text_queue_text(owner->text,row->page,(const char*)body,centered(bh_atlas_page_text_width(row->page,body)),y));y=yadd(y,12);body=next;
 }
 return 1;
}
int __cdecl bh_menu_rows_init(BHMenuRows *owner,BHAtlasContext *atlas,BHTextQueue *text,BHMenuRow **rows,unsigned long capacity){
 if(!owner||!atlas||!atlas->textures||!atlas->textures->allocator.allocate||!atlas->textures->allocator.release||!text||!rows||!capacity)return 0;
 memset(owner,0,sizeof(*owner));memset(rows,0,capacity*sizeof(*rows));owner->atlas=atlas;owner->text=text;owner->rows=rows;owner->capacity=capacity;return 1;
}
BHMenuRow *__cdecl bh_menu_row_create(BHMenuRows *owner,unsigned long identity,const unsigned char *title,unsigned char *body,unsigned long first,unsigned long second,BHMenuRowRefresh refresh){
 BHMenuRow *row;BHTextureContext *textures;
 if(!owner||!owner->rows||!owner->atlas||!owner->text||!valid_title(title)||owner->count>=owner->capacity){if(owner)owner->error=1;return NULL;}
 textures=owner->atlas->textures;row=(BHMenuRow*)textures->allocator.allocate(sizeof(*row),textures->allocator.user);if(!row){owner->error=2;return NULL;}
 memset(row,0,sizeof(*row));row->page=bh_atlas_page_create(owner->atlas,"textures\\tvfont.pcx",128);
 if(!row->page){textures->allocator.release(row,textures->allocator.user);owner->error=3;return NULL;}
 row->identity=identity;row->image=bh_atlas_page_image(row->page);row->title=title;row->first=first;row->second=second;row->refresh=refresh;
 owner->rows[owner->count]=row;owner->error=0;if(!refresh)paint(owner,row,body);++owner->count;return row;
}
int __cdecl bh_menu_row_refresh(BHMenuRows *owner,unsigned long index,const unsigned char *title,unsigned char *body){BHMenuRow *row;
 if(!owner||!owner->rows||index>=owner->count||!(row=owner->rows[index])||!valid_title(title?title:row->title)){if(owner)owner->error=1;return 0;}
 if(title)row->title=title;owner->error=0;return paint(owner,row,body);
}
int __cdecl bh_menu_rows_dispose(BHMenuRows *owner){unsigned long i;BHTextureContext *textures;
 if(!owner||!owner->atlas||!owner->atlas->textures||!owner->rows||!owner->text||bh_text_queue_count(owner->text))return 0;
 textures=owner->atlas->textures;for(i=0;i<owner->count;++i)if(owner->rows[i]){if(!bh_atlas_page_dispose(owner->atlas,owner->rows[i]->page))return 0;textures->allocator.release(owner->rows[i],textures->allocator.user);owner->rows[i]=NULL;}
 memset(owner,0,sizeof(*owner));return 1;
}

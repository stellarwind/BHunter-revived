#include "bh_atlas_wide.h"
#include <string.h>
#include <stddef.h>
typedef char wide_unit_size[sizeof(unsigned short)==2?1:-1];
typedef char wide_record_layout[sizeof(BHTextCommand)==84&&offsetof(BHTextCommand,text)==12&&offsetof(BHTextCommand,next)==80?1:-1];
static long wrapped_add(long a,long b){unsigned long bits=(unsigned long)a+(unsigned long)b;long value;memcpy(&value,&bits,4);return value;}
static long metric(unsigned short bits){short value;memcpy(&value,&bits,2);return value;}
long __cdecl bh_atlas_page_wide_text(BHAtlasPage *page,const unsigned short *text){
 const BHAtlasRectangle *glyph;RECT source,destination;long width,height,total=0;unsigned long code,index;
 if(!page||!text)return 0;
 if(!page->atlas||!page->image||!page->surface||!page->atlas->surface||page->atlas->rectangle_count>512)return 0;
 while((code=*text++)!=0){
  if(code==10){page->cursor_x=page->left;page->cursor_y=wrapped_add(page->cursor_y,page->line_height);}
  else if(code==32){
   if(page->cursor_x==page->left)page->line_height=0;
   height=metric(page->atlas->rectangles[0].height);if(page->line_height<height)page->line_height=height;
   width=metric(page->atlas->rectangles[0].width);total=wrapped_add(total,width);page->cursor_x=wrapped_add(page->cursor_x,width);
  }else if(code>=32&&(index=code-32)<page->atlas->rectangle_count){
   glyph=page->atlas->rectangles+index;width=metric(glyph->width);height=metric(glyph->height);
   if((long)page->size<wrapped_add(page->cursor_x,width)){page->cursor_x=page->left;page->cursor_y=wrapped_add(page->cursor_y,page->line_height);}
   if(page->cursor_x==page->left)page->line_height=0;
   source.left=metric(glyph->x);source.top=metric(glyph->y);source.right=wrapped_add(source.left,width);source.bottom=wrapped_add(source.top,height);
   destination.left=page->cursor_x;destination.top=page->cursor_y;destination.right=wrapped_add(destination.left,width);destination.bottom=wrapped_add(destination.top,height);
   IDirectDrawSurface_Blt(page->surface,&destination,page->atlas->surface,&source,DDBLT_WAIT,NULL);
   if(page->line_height<height)page->line_height=height;
   total=wrapped_add(total,width);page->cursor_x=wrapped_add(page->cursor_x,width);
  }
 }
 if(page->image->resident)page->image->resident->pixels_dirty=1;
 return total;
}
long __cdecl bh_atlas_page_wide_width(const BHAtlasPage *page,const unsigned short *text){
 unsigned long code,index;long total=0;
 if(!page||!text||!page->atlas||page->atlas->rectangle_count>512)return 0;
 while((code=*text++)!=0)if(code>=32&&(index=code-32)<page->atlas->rectangle_count)total=wrapped_add(total,metric(page->atlas->rectangles[index].width));
 return total;
}
int __cdecl bh_text_queue_wide(BHTextQueue *queue,BHAtlasPage *page,const unsigned short *text,long x,long y){
 unsigned long previous,index,i;unsigned short word;BHTextCommand *command;
 if(!queue)return -1;if(!text){queue->error=BH_TEXT_QUEUE_ARGUMENT;return -1;}
 previous=queue->write;index=previous&255;queue->write=(previous&0xffffff00UL)|((previous+1)&255);
 if((queue->write&255)==(queue->read&255)){queue->write=previous;queue->error=BH_TEXT_QUEUE_FULL;return 0;}
 command=queue->commands+index;command->kind=BH_TEXT_COMMAND_WIDE;
 for(i=0;i<29&&text[i];++i)memcpy(command->text+i*2,text+i,2);
 word=0;memcpy(command->text+i*2,&word,2);command->page=page;
 word=(unsigned short)x;memcpy(&command->position.xy[0],&word,2);word=(unsigned short)y;memcpy(&command->position.xy[1],&word,2);
 queue->error=BH_TEXT_QUEUE_OK;return 1;
}
unsigned long __cdecl bh_text_queue_execute_wide(const BHTextCommand *command){
 bh_atlas_page_cursor(command->page,command->position.xy[0],command->position.xy[1]);
 bh_atlas_page_wide_text(command->page,(const unsigned short*)command->text);return command->next;
}

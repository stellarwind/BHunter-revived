#include "bh_atlas_text.h"
#include <string.h>

static long add(long a,long b) {
    unsigned long bits=(unsigned long)a+(unsigned long)b;long value;
    memcpy(&value,&bits,4);return value;
}
static long signed_metric(unsigned short bits) {
    short value;memcpy(&value,&bits,2);return value;
}
long __cdecl bh_atlas_page_text(BHAtlasPage *page,const unsigned char *text) {
    const BHAtlasRectangle *glyph;RECT source,destination;long width,height,total=0;
    unsigned long code,index;
    if(!page || !text)return 0;
    if(!page->atlas || !page->image || !page->surface || !page->atlas->surface || page->atlas->rectangle_count>512)return 0;
    while((code=*text++)!=0) {
        if(code==10) {
            page->cursor_x=page->left;page->cursor_y=add(page->cursor_y,page->line_height);
        } else if(code==32) {
            if(page->cursor_x==page->left)page->line_height=0;
            height=signed_metric(page->atlas->rectangles[0].height);
            if(page->line_height<height)page->line_height=height;
            width=signed_metric(page->atlas->rectangles[0].width);
            total=add(total,width);page->cursor_x=add(page->cursor_x,width);
        } else if(code>=32 && (index=code-32)<page->atlas->rectangle_count) {
            glyph=page->atlas->rectangles+index;
            width=signed_metric(glyph->width);height=signed_metric(glyph->height);
            if((long)page->size<add(page->cursor_x,width)) {
                page->cursor_x=page->left;page->cursor_y=add(page->cursor_y,page->line_height);
            }
            if(page->cursor_x==page->left)page->line_height=0;
            source.left=signed_metric(glyph->x);source.top=signed_metric(glyph->y);
            source.right=add(source.left,width);source.bottom=add(source.top,height);
            destination.left=page->cursor_x;destination.top=page->cursor_y;
            destination.right=add(destination.left,width);destination.bottom=add(destination.top,height);
            IDirectDrawSurface_Blt(page->surface,&destination,page->atlas->surface,&source,DDBLT_WAIT,NULL);
            if(page->line_height<height)page->line_height=height;
            total=add(total,width);page->cursor_x=add(page->cursor_x,width);
        }
    }
    if(page->image->resident)page->image->resident->pixels_dirty=1;
    return total;
}
long __cdecl bh_atlas_page_text_width(const BHAtlasPage *page,const unsigned char *text) {
    unsigned long code,index;long total=0;
    if(!page || !text || !page->atlas || page->atlas->rectangle_count>512)return 0;
    while((code=*text++)!=0) {
        if(code>=32 && (index=code-32)<page->atlas->rectangle_count)
            total=add(total,signed_metric(page->atlas->rectangles[index].width));
    }
    return total;
}
BHTextureImage *__cdecl bh_atlas_page_image(const BHAtlasPage *page) {
    return page?page->image:NULL;
}

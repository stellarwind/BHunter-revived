#include "bh_native_primitive.h"
#include "bh_texture_draw.h"
#include "bh_round.h"
#include <string.h>
#include <float.h>
static int fail(BHTextureContext *q,BHTextureError error){if(q)q->error=error;return 0;}
int __cdecl bh_native_primitive_emit(BHClipVertex *ring,BHTextureImage *image,unsigned long color,void *user) {
    BHNativePrimitive *native=(BHNativePrimitive *)user;BHTextureContext *q;BHClipVertex *p;
    D3DTLVERTEX *vertices;unsigned long count=0,i;int ok;
    if(!native || !(q=native->textures))return 0;
    if(q->software)return fail(q,BH_TEXTURE_BACKEND);
    if(!ring || !image || !q->allocator.allocate || !q->allocator.release || !_finite(native->near_z) || native->near_z<=0)return fail(q,BH_TEXTURE_ARGUMENT);
    p=ring;do {
        if(!p || !p->next || !p->previous || p->next->previous!=p || p->previous->next!=p || ++count>2048)return fail(q,BH_TEXTURE_TOPOLOGY);
        if(!_finite(p->sx) || !_finite(p->sy) || !_finite(p->point.z) || p->point.z<=0 || !_finite(p->u) || !_finite(p->v))return fail(q,BH_TEXTURE_ARGUMENT);
        p=p->next;
    }while(p!=ring);
    if(count<3)return fail(q,BH_TEXTURE_TOPOLOGY);
    vertices=(D3DTLVERTEX *)q->allocator.allocate(count*sizeof(*vertices),q->allocator.user);
    if(!vertices)return fail(q,BH_TEXTURE_MEMORY);
    memset(vertices,0,count*sizeof(*vertices));p=ring;
    for(i=0;i<count;++i) {
        vertices[i].sx=p->sx;vertices[i].sy=p->sy;
        bh_round_float(&vertices[i].sz,(double)native->near_z/p->point.z);
        bh_round_float(&vertices[i].rhw,1.0/p->point.z);
        vertices[i].color=color;vertices[i].tu=p->u;vertices[i].tv=p->v;p=p->next;
    }
    ok=bh_texture_draw_polygon(q,image,vertices,count);q->allocator.release(vertices,q->allocator.user);
    if(ok){++native->polygons;native->vertices+=count;}return ok;
}

#include "bh_script_blocks.h"
#include "bh_texture_controls.h"
#include "bh_vfile.h"
#include "bh_round.h"
#include "bh_rotation.h"
#include "bh_camera_queue.h"
#include "bh_gdi_text.h"
#include "bh_screen_quad.h"
#include "bh_draw_submit.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
int __cdecl bh_script_blocks_init(BHScriptBlocks *c,BHScriptParse *parser,BHRenderResources *resources,const char *prefix,BHBspSearch search){if(!c||!parser||!resources||!prefix)return 0;memset(c,0,sizeof(*c));c->blocks=(BHScriptBlock*)calloc(600,sizeof(BHScriptBlock));if(!c->blocks)return 0;c->capacity=600;c->parser=parser;c->resources=resources;c->collision_prefix=prefix;c->collision_search=search;return 1;}
void __cdecl bh_script_blocks_dispose(BHScriptBlocks *c){unsigned long i;if(!c)return;for(i=0;i<c->capacity;++i)if(c->blocks[i].collision.bsp.allocator.release)bh_collision_model_dispose(&c->blocks[i].collision);free(c->blocks);memset(c,0,sizeof(*c));}
int __cdecl bh_script_blocks_load(BHScriptBlocks *c,const char *path){BHVFile *file;char line[513],*newline;BHScriptBlock *b;unsigned long length;BHTextureContext *textures;
 if(!c||!c->blocks||!c->capacity||c->capacity>600||!c->parser||!c->parser->vm||!c->resources||!c->resources->models||!path){if(c)c->error=1;return 0;}c->error=0;if(c->count)return 1;
 file=bh_vfile_open(path,"r");if(!file)return 0;c->parser->vm->script_loading=1;
 while(c->count<c->capacity&&bh_vfile_gets(line,512,file)){newline=strchr(line,'\n');if(newline)*newline=0;if(!_stricmp(line,"end"))break;if(line[0]==';')continue;length=(unsigned long)strlen(line);if(length>=16){c->error=1;break;}
  b=c->blocks+c->count;if(b->collision.bsp.allocator.release)bh_collision_model_dispose(&b->collision);memset(b,0,sizeof(*b));memcpy(b->definition.name,line,length+1);b->definition.grid=10;
  if(!bh_script_parse_file(c->parser,&b->definition)){c->error=2;break;}b->resource=bh_render_resources_load_model(c->resources,line);
  if(!bh_collision_model_init(&b->collision,0)){c->error=2;break;}
  b->model.minimum.x=b->model.minimum.y=b->model.minimum.z=9e37f;b->model.maximum.x=b->model.maximum.y=b->model.maximum.z=-9e37f;
  if(bh_collision_model_load(&b->collision,line,c->collision_prefix,c->collision_search)){if(!bh_geometry_model_init(&b->model,&b->collision.geometry,b->definition.grid,b->definition.flags)){c->error=2;break;}b->definition.flags=b->model.flags;}
  if(b->resource)++c->count;
 }
 bh_vfile_close(file);c->parser->vm->script_loading=0;if(c->error)return 0;c->columns=(long)(__int64)(sqrt((double)c->count)*1.15);if(c->columns<5)c->columns=5;c->rows=((long)c->count+c->columns-1)/c->columns;
 textures=c->resources->models->textures;c->water=bh_texture_image_load(textures,"water.gif");if(c->water)bh_texture_image_set_flags(c->water,2);return 1;
}
static float stored(double v){float f;bh_round_float(&f,v);return f;}
int __cdecl bh_script_block_preview(BHScriptBrowser *c,long index,long x,long y){BHScriptBlock *b;BHVec3 angles,center,transformed,position;BHMat34 matrix;char label[32];unsigned long i,length;float sx,sy,radius,xf,yf;double dx,dy,dz,screen_x,screen_y,retained_radius;
 if(!c||!c->library||!c->queue){if(c)c->error=1;return 0;}if(index<0||(unsigned long)index>=c->library->count)return 1;b=c->library->blocks+index;sx=stored((double)x+c->offset_x);sy=stored((double)y+c->offset_y);screen_x=(double)sx*c->height*(double).2f;screen_y=(double)sy*c->height*(double).2f;if(!_finite(screen_x)||!_finite(screen_y)||fabs(screen_x)>=2147483648.0||fabs(screen_y)>=2147483648.0){c->error=1;return 0;}
 strcpy(label,b->definition.name);length=(unsigned long)strlen(label);if(length>4&&label[length-4]=='.')label[length-4]=0;if(bh_gdi_text_queue(c->queue,(short)(__int64)screen_x,(short)(__int64)screen_y,label,b->definition.script?0x806040:0x406080)!=1){c->error=2;return 0;}
 dx=(double)b->model.maximum.x-b->model.minimum.x;dy=(double)b->model.maximum.y-b->model.minimum.y;dz=(double)b->model.maximum.z-b->model.minimum.z;retained_radius=sqrt((dx*stored(dx)+dy*stored(dy))+dz*stored(dz))*5;radius=stored(retained_radius);if(!_finite(radius)||!_finite(b->preview_yaw)||!_finite(b->preview_phase)){c->error=1;return 0;}
 angles.x=b->preview_yaw;angles.y=stored(sin((double)b->preview_phase)*.5);angles.z=0;for(i=0;i<3;++i)((float*)&center)[i]=stored(((double)((float*)&b->model.minimum)[i]+((float*)&b->model.maximum)[i])*.5);
 memset(&matrix,0,sizeof(matrix));matrix.m[0][0]=matrix.m[1][1]=matrix.m[2][2]=1;bh_mat34_rotate_y(angles.x,&matrix);bh_mat34_rotate_x(angles.y,&matrix);bh_mat34_transform_point(&matrix,&center,&transformed);
 xf=stored((((double)sx-(-.5))-2.5)*(2.0/9.0));yf=stored((((double)sy-(-.5))-1.875)*(4.0/17.0));position.x=stored(retained_radius*xf);position.y=stored((double)radius*yf);position.z=radius;
 for(i=0;i<3;++i)((float*)&position)[i]=stored((double)((float*)&position)[i]-((float*)&transformed)[i]);if(bh_draw_submit_resource(c->queue,0,b->resource,&position,&angles,1)!=1){c->error=2;return 0;}++c->cells;return 1;
}
static long signed_word(unsigned long x){long n;memcpy(&n,&x,4);return n;}
static int quad(BHScriptBrowser *c,unsigned long bucket,unsigned long image,float x,float y,float width,float height,float alpha){if(bh_screen_quad_queue(c->queue,bucket,image,x,y,width,height,0,0,alpha)!=1){c->error=2;return 0;}return 1;}
int __cdecl bh_script_block_browser(BHScriptBrowser *c){BHScriptBlocks *library;BHScriptBlock *b;BHVec3 zero={0,0,0};unsigned long r=2401,q,i;long columns,rows,step,x,y,index,first_x,last_x,first_y,last_y,j;double value,change;float px,py;
 if(!c||!c->library||!c->queue||!c->sound||c->height<1||c->height>16384||c->library->count>600||c->library->columns<5||c->library->columns>600||c->library->rows<0||c->library->rows>600||!_finite(c->cursor_x)||!_finite(c->cursor_y)){if(c)c->error=1;return 0;}c->error=0;c->cells=0;library=c->library;columns=library->columns;rows=library->rows;step=c->height/5;
 if(bh_camera_queue_pose(c->queue,&zero,&zero)!=1){c->error=2;return 0;}
 for(i=0;i<library->count;++i){b=library->blocks+i;q=(unsigned long)(signed_word(r*7UL)/64);change=.05-(q&15)*(-1.0/300.0);if(q&16)change=-change;value=(double)b->preview_yaw+change;b->preview_yaw=stored(value);if(value< -3.141592653589793)b->preview_yaw=stored(value-(-6.283185307179586));else if(value>3.141592653589793)b->preview_yaw=stored(value-6.283185307179586);
  r*=49UL;q=(unsigned long)(signed_word(r)/64);b->preview_phase=stored(((double)b->preview_phase-(q&15)*(-1.0/1500.0))-(-.01));value=b->preview_phase;if(value< -3.141592653589793)b->preview_phase=stored(value-(-6.283185307179586));else if(value>3.141592653589793)b->preview_phase=stored(value-6.283185307179586);
 }
 if(c->cursor_x<0)c->cursor_x=0;else if(c->cursor_x>columns)c->cursor_x=(float)columns;if(c->cursor_y<0)c->cursor_y=0;else if(c->cursor_y>rows)c->cursor_y=(float)rows;x=(long)(__int64)c->cursor_x;if(x>=columns)x=columns-1;y=(long)(__int64)c->cursor_y;if(y>=rows)y=rows-1;index=y*columns+x;
 if(index>=0&&(unsigned long)index<library->count&&index!=c->selected){c->selected=index;bh_sound_request(c->sound,c->select_sound,1,1,0,0);}
 c->offset_x=stored(-((double)c->cursor_x-2.5));c->offset_y=stored(-((double)c->cursor_y-1.875));if(c->offset_x>0)c->offset_x=0;if(c->offset_y>0)c->offset_y=0;if(c->offset_x<5-columns)c->offset_x=(float)(5-columns);if(c->offset_y< -((double)rows-3.75))c->offset_y=stored(-((double)rows-3.75));
 for(x=0;x<=columns;++x)if(!quad(c,0,c->grid_image,stored(((double)x+c->offset_x)*step),stored(((double)c->offset_y-rows*(-.5))*step),1,(float)(step*rows),.5f))return 0;
 for(y=0;y<=rows;++y)if(!quad(c,0,c->grid_image,stored(((double)c->offset_x-columns*(-.5))*step),stored(((double)y+c->offset_y)*step),(float)(step*columns),1,.5f))return 0;
 px=(float)(long)(__int64)(((double)c->offset_x+c->cursor_x)*step);py=(float)(long)(__int64)(((double)c->offset_y+c->cursor_y)*step);
 if(!quad(c,3,c->selection_image,px,py,2,(float)(step/4),1)||!quad(c,3,c->selection_image,px,py,(float)(step/4),2,1))return 0;
 x=c->selected%columns;y=c->selected/columns;px=(float)(long)(__int64)(((double)x+c->offset_x-(-.5))*step);py=(float)(long)(__int64)(((double)y+c->offset_y-(-.5))*step);if(!quad(c,3,c->selection_image,px,py,(float)step,(float)step,.25f))return 0;
 x=c->active%columns;y=c->active/columns;px=(float)(long)(__int64)(((double)x+c->offset_x-(-.5))*step);py=(float)(long)(__int64)(((double)y+c->offset_y-(-.5))*step);if(!quad(c,3,c->grid_image,px,py,(float)step,(float)step,.25f))return 0;
 first_x=(long)(__int64)(-(double)c->offset_x);last_x=(long)(__int64)(6.0-c->offset_x);if(last_x>columns)last_x=columns;first_y=(long)(__int64)(-(double)c->offset_y);last_y=(long)(__int64)(4.75-c->offset_y);if(last_y>rows)last_y=rows;
 for(j=first_y;j<last_y;++j)for(x=first_x;x<last_x;++x)if(!bh_script_block_preview(c,j*columns+x,x,j))return 0;return 1;
}

#include "psp/PspRenderer3D.hpp"

#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

#include <cmath>
#include <cstddef>
#include <cstring>

namespace enginewatch {
namespace {
static unsigned int __attribute__((aligned(16))) gu_list[65536];

uint32_t rgba(unsigned r, unsigned g, unsigned b, unsigned a=255) {
    return (a<<24)|(b<<16)|(g<<8)|r;
}
struct ScreenVertex {
    uint32_t color;
    float x,y,z;
};
struct TextVertex {
    float u,v;
    uint32_t color;
    float x,y,z;
};
}

PspRenderer3D::PspRenderer3D() {
    sceGuInit();
    sceGuStart(GU_DIRECT,gu_list);
    // 512-wide hardware stride; two RGBA buffers and a 16-bit depth buffer fit VRAM.
    sceGuDrawBuffer(GU_PSM_8888,(void*)0,512);
    sceGuDispBuffer(480,272,(void*)0x88000,512);
    sceGuDepthBuffer((void*)0x110000,512);
    sceGuOffset(2048-240,2048-136);
    sceGuViewport(2048,2048,480,272);
    sceGuDepthRange(65535,0);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuClearDepth(0);
    sceGuScissor(0,0,480,272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_LIGHTING);
    sceGuDisable(GU_TEXTURE_2D);
    sceGuShadeModel(GU_SMOOTH);
    sceGuFinish();
    sceGuSync(0,0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
    // The geometry and font atlas are persistent RAM buffers read by the GE.
    sceKernelDcacheWritebackAll();
    ready_=true;
}
PspRenderer3D::~PspRenderer3D() {
    if(ready_) { sceGuDisplay(GU_FALSE); sceGuTerm(); }
}

void PspRenderer3D::rectangle(int x,int y,int w,int h,uint32_t c) {
    if(w<=0 || h<=0) return;
    ScreenVertex* v=static_cast<ScreenVertex*>(sceGuGetMemory(sizeof(ScreenVertex)*2));
    v[0]=ScreenVertex{c,static_cast<float>(x),static_cast<float>(y),0.0f};
    v[1]=ScreenVertex{c,static_cast<float>(x+w),static_cast<float>(y+h),0.0f};
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDrawArray(GU_SPRITES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,v);
}

void PspRenderer3D::text(int x,int y,const char* value,uint32_t c,int scale) {
    if(!value || scale<1) return;
    int n=0;
    for(const char* p=value;*p;++p) if(*p!='\n') ++n;
    if(n==0) return;
    TextVertex* v=static_cast<TextVertex*>(sceGuGetMemory(sizeof(TextVertex)*2*n));
    int k=0,px=x,py=y;
    for(const unsigned char* p=reinterpret_cast<const unsigned char*>(value);*p;++p) {
        if(*p=='\n') { px=x;py+=9*scale;continue; }
        int ch=static_cast<int>(*p);
        if(ch>='a'&&ch<='z') ch=ch-'a'+'A';
        if(ch<32 || ch>=96) ch='?';
        const int sx=((ch-32)%16)*8,sy=((ch-32)/16)*8;
        v[k++]=TextVertex{sx/128.0f,sy/32.0f,c,static_cast<float>(px),
                          static_cast<float>(py),0.0f};
        v[k++]=TextVertex{(sx+5)/128.0f,(sy+7)/32.0f,c,static_cast<float>(px+5*scale),
                          static_cast<float>(py+7*scale),0.0f};
        px+=6*scale;
    }
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_8888,0,0,0);
    sceGuTexImage(0,FontAtlas::Width,FontAtlas::Height,FontAtlas::Width,font_.pixels());
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexWrap(GU_CLAMP,GU_CLAMP);
    sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
    sceGuTexScale(1.0f,1.0f);
    sceGuTexOffset(0.0f,0.0f);
    sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_COLOR_8888|
                   GU_VERTEX_32BITF|GU_TRANSFORM_2D,k,0,v);
    sceGuDisable(GU_TEXTURE_2D);
}

void PspRenderer3D::overlay(const Hud3D& h) {
    const uint32_t white=rgba(235,245,244);
    const uint32_t muted=rgba(168,191,195);
    const uint32_t yellow=rgba(249,191,91);
    const uint32_t cyan=rgba(112,239,212);
    const uint32_t red=rgba(255,117,110);
    rectangle(0,0,480,52,rgba(9,23,33,224));
    text(13,6,h.title,white,2);
    text(14,25,h.status,h.alarm?red:cyan);
    rectangle(0,38,480,15,rgba(12,33,44,243));
    text(13,42,h.objective,h.alarm?red:yellow);
    // A small unobtrusive crosshair helps the player face a station.
    rectangle(236,134,9,2,rgba(230,240,229,195));
    rectangle(240,130,2,9,rgba(230,240,229,195));
    rectangle(0,226,480,22,rgba(9,23,33,225));
    text(13,232,h.prompt,white);
    if(!h.panel_visible) return;
    rectangle(53,54,374,169,rgba(7,20,29,238));
    rectangle(53,54,374,2,cyan);
    rectangle(53,221,374,2,cyan);
    rectangle(65,66,350,17,rgba(40,67,79,255));
    text(72,70,h.panel_title,white);
    for(int i=0;i<h.panel_count && i<10;++i) {
        const int yy=92+i*17;
        if(h.selected_row==i) rectangle(67,yy-4,341,15,rgba(65,85,91,230));
        text(74,yy,h.panel_rows[i],h.selected_row==i?yellow:muted);
    }
}

void PspRenderer3D::draw(const World3D& world,const Game3D& game) {
    if(!ready_) return;
    sceGuStart(GU_DIRECT,gu_list);
    sceGuClearColor(rgba(12,24,36));
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDisable(GU_TEXTURE_2D);
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(68.0f,480.0f/272.0f,0.12f,80.0f);
    const Vec3& p=game.camera();
    const float yaw=game.yaw();
    ScePspFVector3 eye={p.x,p.y,p.z};
    const float pitch=game.pitch();
    const float cp=std::cos(pitch);
    ScePspFVector3 center={p.x+std::cos(yaw)*cp,p.y+std::sin(pitch),
                            p.z+std::sin(yaw)*cp};
    ScePspFVector3 up={0.0f,1.0f,0.0f};
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumLookAt(&eye,&center,&up);
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    const std::vector<Vertex3D>& v=world.vertices();
    sceGumDrawArray(GU_TRIANGLES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
                    static_cast<int>(v.size()),0,v.data());

    // HUD uses absolute 2D screen vertices, without changing the 3D camera.
    sceGuDisable(GU_DEPTH_TEST);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
    overlay(game.hud());
    sceGuDisable(GU_BLEND);
    sceGuFinish();
    sceGuSync(0,0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}
} // namespace enginewatch

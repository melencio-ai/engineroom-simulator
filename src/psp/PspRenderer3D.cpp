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
    character_vertices_.reserve(PlayerCharacter::MaxRuntimeVertices);
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
    // PSP GE has a near-plane clipper only when GU_CLIP_PLANES is enabled.
    // Without it, a wall/deck TRIANGLE containing a vertex behind the camera
    // is discarded in its entirety while walking near the surface.
    sceGuEnable(GU_CLIP_PLANES);
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
        v[k++]=TextVertex{static_cast<float>(sx),static_cast<float>(sy),c,static_cast<float>(px),
                          static_cast<float>(py),0.0f};
        v[k++]=TextVertex{static_cast<float>(sx+5),static_cast<float>(sy+7),c,static_cast<float>(px+5*scale),
                          static_cast<float>(py+7*scale),0.0f};
        px+=6*scale;
    }
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_8888,0,0,0);
    sceGuTexImage(0,FontAtlas::Width,FontAtlas::Height,FontAtlas::Width,font_.pixels());
    sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexWrap(GU_CLAMP,GU_CLAMP);
    sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
    // GU_TRANSFORM_2D ignores texture scaling. Its UVs are RAW TEXEL
    // coordinates, unlike the desktop OpenGL normalized UVs. Using 0..1
    // previously sampled one transparent atlas corner and hid HUD text.
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
    // Explicitly restore near-plane clipping for each 3D frame. PSP clips
    // only the near plane in hardware; this prevents faces popping away when
    // the player's viewpoint crosses a large wall/floor triangle boundary.
    sceGuEnable(GU_CLIP_PLANES);
    sceGuDisable(GU_TEXTURE_2D);
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    // Improve 16-bit PSP depth precision for shallow floor viewing angles.
    // The whole engine space fits within 60 m of the player; 0.25 m near
    // plane keeps more precision than the previous 0.12..80 range.
    sceGumPerspective(68.0f,480.0f/272.0f,0.25f,60.0f);
    const CameraView3D camera=game.renderCamera();
    ScePspFVector3 eye={
        camera.eye.x,camera.eye.y,camera.eye.z
    };
    ScePspFVector3 center={
        camera.target.x,camera.target.y,camera.target.z
    };
    ScePspFVector3 up={0.0f,1.0f,0.0f};
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumLookAt(&eye,&center,&up);
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    const std::vector<Vertex3D>& v=world.vertices();
    // PSP GE draw calls have practical per-command vertex limits. The
    // denser v0.5 stair and checker-plate scene must be submitted as small
    // batches; one oversized array may yield a blank PSP framebuffer.
    // 9600 is a multiple of 3, keeping every triangle intact.
    const std::size_t batch=9600u;
    for(std::size_t first=0;first<v.size();first+=batch) {
        const std::size_t left=v.size()-first;
        const int count=static_cast<int>(left<batch?left:batch);
        sceGumDrawArray(GU_TRIANGLES,
                        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
                        count,0,v.data()+first);
    }

    // The animated engineer uses a separate transient PSP GU vertex block;
    // the static ship scene remains at its validated 127,950-vertex budget.
    // The character's low-poly body is shown in third-person; the same
    // animation pose builds gloved arms in first-person.
    game.buildCharacterGeometry(camera,character_vertices_);
    if(!character_vertices_.empty() &&
       character_vertices_.size()<=PlayerCharacter::MaxRuntimeVertices) {
        const std::size_t bytes=character_vertices_.size()*sizeof(Vertex3D);
        Vertex3D* temporary=static_cast<Vertex3D*>(sceGuGetMemory(bytes));
        if(temporary) {
            std::memcpy(temporary,character_vertices_.data(),bytes);
            sceKernelDcacheWritebackRange(temporary,bytes);
            sceGumDrawArray(GU_TRIANGLES,
                           GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
                           static_cast<int>(character_vertices_.size()),0,temporary);
        }
    }

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

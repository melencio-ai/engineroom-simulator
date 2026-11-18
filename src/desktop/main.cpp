#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_opengl.h>

#include "scene/FontAtlas.hpp"
#include "scene/Game3D.hpp"
#include "scene/World3D.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace {
unsigned char channel(uint32_t rgba,int shift) {
    return static_cast<unsigned char>((rgba>>shift)&255u);
}
void color(uint32_t rgba) {
    glColor4ub(channel(rgba,0),channel(rgba,8),channel(rgba,16),channel(rgba,24));
}
uint32_t rgba(unsigned r,unsigned g,unsigned b,unsigned a=255) {
    return (a<<24)|(b<<16)|(g<<8)|r;
}
float stick(Sint16 v) {
    const float f=static_cast<float>(v)/32768.0f;
    return std::fabs(f)<0.17f?0.0f:f;
}
class Renderer {
public:
    Renderer() {
        enginewatch::FontAtlas pixels;
        glGenTextures(1,&font_);
        glBindTexture(GL_TEXTURE_2D,font_);
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,enginewatch::FontAtlas::Width,
                     enginewatch::FontAtlas::Height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.pixels());
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
    }
    ~Renderer() { glDeleteTextures(1,&font_); }

    void rect(int x,int y,int w,int h,uint32_t c) {
        glDisable(GL_TEXTURE_2D);
        color(c);
        glBegin(GL_QUADS);
        glVertex2i(x,y);
        glVertex2i(x+w,y);
        glVertex2i(x+w,y+h);
        glVertex2i(x,y+h);
        glEnd();
    }
    void text(int x,int y,const char* p,uint32_t c,int scale=1) {
        if(!p) return;
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,font_);
        color(c);
        glBegin(GL_QUADS);
        const int sx0=x;
        for(;*p;++p) {
            if(*p=='\n') { x=sx0; y+=9*scale;continue; }
            int ch=static_cast<unsigned char>(*p);
            if(ch>='a'&&ch<='z') ch=ch-'a'+'A';
            if(ch<32 || ch>=96) ch='?';
            const int sx=((ch-32)%16)*8,sy=((ch-32)/16)*8;
            const float u0=sx/128.0f,v0=sy/32.0f;
            const float u1=(sx+5)/128.0f,v1=(sy+7)/32.0f;
            glTexCoord2f(u0,v0);glVertex2i(x,y);
            glTexCoord2f(u1,v0);glVertex2i(x+5*scale,y);
            glTexCoord2f(u1,v1);glVertex2i(x+5*scale,y+7*scale);
            glTexCoord2f(u0,v1);glVertex2i(x,y+7*scale);
            x+=6*scale;
        }
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }
    void draw(const enginewatch::World3D& world,const enginewatch::Game3D& game,int w,int h) {
        glViewport(0,0,w,h);
        glClearColor(12/255.0f,24/255.0f,36/255.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        const float nearPlane=0.12f;
        const float top=std::tan(68.0f*3.141592653589793f/360.0f)*nearPlane;
        const float right=top*static_cast<float>(w)/std::max(h,1);
        glFrustum(-right,right,-top,top,nearPlane,80.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        const enginewatch::Vec3& p=game.camera();
        glRotatef(90.0f+game.yaw()*180.0f/3.141592653589793f,0,1,0);
        glTranslatef(-p.x,-p.y,-p.z);
        glBegin(GL_TRIANGLES);
        const std::vector<enginewatch::Vertex3D>& mesh=world.vertices();
        for(std::size_t i=0;i<mesh.size();++i) {
            color(mesh[i].color);
            glVertex3f(mesh[i].x,mesh[i].y,mesh[i].z);
        }
        glEnd();

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0,480,272,0,-1,1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        const enginewatch::Hud3D hud=game.hud();
        const uint32_t white=rgba(235,245,244),muted=rgba(168,191,195);
        const uint32_t yellow=rgba(249,191,91),cyan=rgba(112,239,212),red=rgba(255,117,110);
        rect(0,0,480,52,rgba(9,23,33,224));
        text(13,6,hud.title,white,2);
        text(14,25,hud.status,hud.alarm?red:cyan);
        rect(0,38,480,15,rgba(12,33,44,243));
        text(13,42,hud.objective,hud.alarm?red:yellow);
        rect(236,134,9,2,rgba(230,240,229,195));
        rect(240,130,2,9,rgba(230,240,229,195));
        rect(0,226,480,22,rgba(9,23,33,225));
        text(13,232,hud.prompt,white);
        if(hud.panel_visible) {
            rect(53,54,374,169,rgba(7,20,29,238));
            rect(53,54,374,2,cyan);
            rect(53,221,374,2,cyan);
            rect(65,66,350,17,rgba(40,67,79));
            text(72,70,hud.panel_title,white);
            for(int i=0;i<hud.panel_count && i<10;++i) {
                const int y=92+i*17;
                if(hud.selected_row==i) rect(67,y-4,341,15,rgba(65,85,91,230));
                text(74,y,hud.panel_rows[i],hud.selected_row==i?yellow:muted);
            }
        }
        glDisable(GL_BLEND);
    }
private:
    GLuint font_=0;
};
}

int main(int,char**) {
    SDL_SetMainReady();
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)!=0) {
        std::fprintf(stderr,"SDL init failed: %s\n",SDL_GetError());
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
    SDL_Window* window=SDL_CreateWindow("Engine Watch 3D v0.3",SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,960,544,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if(!window) { std::fprintf(stderr,"%s\n",SDL_GetError());SDL_Quit();return 1; }
    SDL_GLContext ctx=SDL_GL_CreateContext(window);
    if(!ctx) { std::fprintf(stderr,"%s\n",SDL_GetError());SDL_DestroyWindow(window);SDL_Quit();return 1; }
    SDL_GL_SetSwapInterval(1);
    enginewatch::World3D world;
    enginewatch::Game3D game(world);
    Renderer renderer;
    SDL_GameController* controller=nullptr;
    if(SDL_NumJoysticks()>0 && SDL_IsGameController(0))
        controller=SDL_GameControllerOpen(0);
    bool running=true;
    Uint32 last=SDL_GetTicks();
    while(running) {
        enginewatch::Input3D in;
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type==SDL_QUIT) running=false;
            if(event.type==SDL_KEYDOWN && !event.key.repeat) {
                const SDL_Keycode key=event.key.keysym.sym;
                if(key==SDLK_x||key==SDLK_e||key==SDLK_SPACE||key==SDLK_RETURN) in.interact=true;
                if(key==SDLK_c||key==SDLK_ESCAPE||key==SDLK_BACKSPACE) in.back=true;
                if(key==SDLK_h||key==SDLK_F1) in.help=true;
                if(key==SDLK_p||key==SDLK_TAB) in.pause=true;
                if(key==SDLK_UP) in.up=true;
                if(key==SDLK_DOWN) in.down=true;
            }
            if(event.type==SDL_CONTROLLERBUTTONDOWN) {
                if(event.cbutton.button==SDL_CONTROLLER_BUTTON_A) in.interact=true;
                if(event.cbutton.button==SDL_CONTROLLER_BUTTON_B) in.back=true;
                if(event.cbutton.button==SDL_CONTROLLER_BUTTON_Y) in.help=true;
                if(event.cbutton.button==SDL_CONTROLLER_BUTTON_START) in.pause=true;
                if(event.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP) in.up=true;
                if(event.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN) in.down=true;
            }
        }
        const Uint8* keys=SDL_GetKeyboardState(nullptr);
        if(keys[SDL_SCANCODE_W]||keys[SDL_SCANCODE_UP]) in.forward+=1.0f;
        if(keys[SDL_SCANCODE_S]||keys[SDL_SCANCODE_DOWN]) in.forward-=1.0f;
        if(keys[SDL_SCANCODE_A]) in.strafe-=1.0f;
        if(keys[SDL_SCANCODE_D]) in.strafe+=1.0f;
        if(keys[SDL_SCANCODE_LEFT]||keys[SDL_SCANCODE_Q]) in.turn-=1.0f;
        if(keys[SDL_SCANCODE_RIGHT]) in.turn+=1.0f;
        if(controller) {
            in.forward-=stick(SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTY));
            in.strafe+=stick(SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_LEFTX));
            in.turn+=stick(SDL_GameControllerGetAxis(controller,SDL_CONTROLLER_AXIS_RIGHTX));
            if(SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) in.turn-=1.0f;
            if(SDL_GameControllerGetButton(controller,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) in.turn+=1.0f;
        }
        const Uint32 now=SDL_GetTicks();
        game.update(static_cast<float>(now-last)/1000.0f,in);
        last=now;
        int width=960,height=544;
        SDL_GL_GetDrawableSize(window,&width,&height);
        renderer.draw(world,game,width,height);
        SDL_GL_SwapWindow(window);
        SDL_Delay(1);
    }
    if(controller) SDL_GameControllerClose(controller);
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

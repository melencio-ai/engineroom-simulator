#include "psp/PspRenderer3D.hpp"
#include "scene/Game3D.hpp"
#include "scene/World3D.hpp"

#include <pspctrl.h>
#include <pspkernel.h>

PSP_MODULE_INFO("Engine Watch 3D", 0, 0, 3);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

namespace {
int exitCallback(int, int, void*) {
    sceKernelExitGame();
    return 0;
}
int callbackThread(SceSize, void*) {
    int cb=sceKernelCreateCallback("exit_cb",exitCallback,0);
    if(cb>=0) sceKernelRegisterExitCallback(cb);
    sceKernelSleepThreadCB();
    return 0;
}
void setupCallbacks() {
    const int thread=sceKernelCreateThread("exit_callback",callbackThread,0x11,0xFA0,0,0);
    if(thread>=0) sceKernelStartThread(thread,0,0);
}
float normalizedAxis(unsigned char value) {
    const float v=(static_cast<int>(value)-128)/127.0f;
    return v < 0.16f && v > -0.16f ? 0.0f : (v<-1.0f?-1.0f:(v>1.0f?1.0f:v));
}
}

int main(int, char**) {
    setupCallbacks();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    // World geometry is constructed only once and sent to the PSP GE.
    enginewatch::World3D world;
    enginewatch::Game3D game(world);
    enginewatch::PspRenderer3D renderer;
    unsigned int lastButtons=0;
    unsigned int lastMicros=sceKernelGetSystemTimeLow();

    while(true) {
        SceCtrlData pad{};
        sceCtrlReadBufferPositive(&pad,1);
        const unsigned int now=sceKernelGetSystemTimeLow();
        const float dt=static_cast<float>(now-lastMicros)/1000000.0f;
        lastMicros=now;
        const unsigned int pressed=pad.Buttons & ~lastButtons;
        lastButtons=pad.Buttons;

        enginewatch::Input3D in;
        in.forward=-normalizedAxis(pad.Ly);
        in.strafe=normalizedAxis(pad.Lx);
        if(pad.Buttons & PSP_CTRL_UP) in.forward+=1.0f;
        if(pad.Buttons & PSP_CTRL_DOWN) in.forward-=1.0f;
        if(pad.Buttons & (PSP_CTRL_LEFT|PSP_CTRL_LTRIGGER)) in.turn-=1.0f;
        if(pad.Buttons & (PSP_CTRL_RIGHT|PSP_CTRL_RTRIGGER)) in.turn+=1.0f;
        in.interact=(pressed & PSP_CTRL_CROSS)!=0;
        in.back=(pressed & PSP_CTRL_CIRCLE)!=0;
        in.help=(pressed & PSP_CTRL_TRIANGLE)!=0;
        in.pause=(pressed & PSP_CTRL_START)!=0;
        in.up=(pressed & PSP_CTRL_UP)!=0;
        in.down=(pressed & PSP_CTRL_DOWN)!=0;

        game.update(dt,in);
        renderer.draw(world,game);
    }
    return 0;
}

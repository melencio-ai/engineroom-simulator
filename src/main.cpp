#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cstdio>

#ifdef __PSP__
#include <pspkernel.h>
#include <pspdebug.h>
PSP_MODULE_INFO("Engine Watch", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

static int exitCallback(int, int, void*) {
    sceKernelExitGame();
    return 0;
}
static int callbackThread(SceSize, void*) {
    const int id = sceKernelCreateCallback("exit_cb", exitCallback, nullptr);
    sceKernelRegisterExitCallback(id);
    sceKernelSleepThreadCB();
    return 0;
}
static void setupExitCallback() {
    const int thread = sceKernelCreateThread("callback_thread", callbackThread, 0x11, 0xFA0, 0, 0);
    if (thread >= 0) sceKernelStartThread(thread, 0, nullptr);
}
#endif

#include "ui/GameApp.hpp"

static int failStartup(const char* stage) {
    std::fprintf(stderr, "ENGINE WATCH: %s: %s\n", stage, SDL_GetError());
#ifdef __PSP__
    pspDebugScreenInit();
    pspDebugScreenPrintf("ENGINE WATCH STARTUP ERROR\n\n%s\n\n%s\n", stage, SDL_GetError());
    sceKernelDelayThread(15000000);
#endif
    return 1;
}


int main(int, char**) {
#ifdef __PSP__
    setupExitCallback();
#endif
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
        return failStartup("SDL_Init failed");
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
#ifdef __PSP__
    const int windowWidth = 480, windowHeight = 272;
#else
    const int windowWidth = 960, windowHeight = 544;
#endif
    SDL_Window* window = SDL_CreateWindow("ENGINE WATCH v0.2", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, 0);
    if (!window) {
        failStartup("SDL_CreateWindow failed");
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        failStartup("SDL_CreateRenderer failed");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
#ifndef __PSP__
    SDL_RenderSetLogicalSize(renderer, 480, 272);
#endif
    {
        enginewatch::GameApp app(renderer);
        app.run();
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

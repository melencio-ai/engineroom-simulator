#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cstdio>

#ifdef __PSP__
#include <pspkernel.h>
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

int main(int, char**) {
#ifdef __PSP__
    setupExitCallback();
#endif
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
        std::fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
#ifdef __PSP__
    const int windowWidth = 480, windowHeight = 272;
#else
    const int windowWidth = 960, windowHeight = 544;
#endif
    SDL_Window* window = SDL_CreateWindow("ENGINE WATCH v0.1", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, 0);
    if (!window) {
        std::fprintf(stderr, "Window failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        std::fprintf(stderr, "Renderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, 480, 272);
    {
        enginewatch::GameApp app(renderer);
        app.run();
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

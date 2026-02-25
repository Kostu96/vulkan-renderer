#include "application.hpp"
#include "vlk/vlk.hpp"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <iostream>
#include <print>
#include <stdexcept>

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println(std::cerr, "SDL_Init failed: {}.", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    VkResult result = volkInitialize();
    if (result != VK_SUCCESS) {
        std::println(std::cerr, "Failed to load Vulkan library.");
        return SDL_APP_FAILURE;
    }

    try {
        Application* app = new Application;
        *appstate = app;
    }
    catch (const std::exception& e) {
        std::println(std::cerr, "{}", e.what());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    switch (event->type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    try {
        Application* app = static_cast<Application*>(appstate);
        app->update();
    }
    catch (const std::exception& e) {
        std::println(std::cerr, "{}", e.what());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    Application* app = static_cast<Application*>(appstate);

    delete app;
}

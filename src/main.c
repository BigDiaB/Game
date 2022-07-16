#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

/*#include <DBG/debug.h>
#include <construct/construct.h>*/

#include <SDL2/SDL.h>

const bool ENABLE_VSYNC = true;
const bool ENABLE_SCREEN_RESIZE = true;
const bool ENABLE_SCREEN_REFRESH_DURING_RESIZE = true;
const bool ENABLE_SMOOTH_CAM = true;

const float WINDOW_SCALE = 0.8f;

const unsigned int WINDOW_FLAGS = SDL_WINDOW_METAL | (ENABLE_SCREEN_RESIZE ? SDL_WINDOW_RESIZABLE : 0) | SDL_WINDOW_ALLOW_HIGHDPI;
const unsigned int RENDERER_FLAGS = SDL_RENDERER_ACCELERATED | (ENABLE_VSYNC ? SDL_RENDERER_PRESENTVSYNC : 0) | SDL_RENDERER_TARGETTEXTURE;

const double WORLD_ZOOM = 0.5f;
const double WORLD_SIZE = 1000.0f / WORLD_ZOOM;

const double CAM_SMOOTH = 0.008f * 1000 / WORLD_SIZE;
const unsigned int CAM_SPEED = 5;

#include "util.h"

void render()
{
    SDL_Rect world = get_world_rect();
    SDL_Rect screen = get_screen_rect();

    SDL_Rect ui_rect = {0,0,500,500};
    align_ui_rect(&ui_rect,align_right);

    SDL_Rect obj_rect = {250,250,500,500};

    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    SDL_RenderFillRect(renderer,translate_rect_ui(&ui_rect));
    SDL_RenderFillRect(renderer,translate_rect(&obj_rect));

    SDL_SetRenderDrawColor(renderer,0,255,0,255);
    SDL_RenderDrawRect(renderer,translate_rect_ui(&world));

    SDL_SetRenderDrawColor(renderer,255,0,0,255);
    SDL_RenderDrawRect(renderer,translate_rect_ui(&screen));

    SDL_SetRenderDrawColor(renderer,125,125,125,255);
    SDL_RenderPresent(renderer);
    SDL_RenderClear(renderer);
}

void update(double dt)
{
    accumulator += dt;
    /* Handle input-logic here (every frame) */

    while(accumulator >= 10.0f)
    {
        accumulator -= 10.0f;
        /* Handle game-logic here (every tick, aka 10ms) */
    }
}

bool gameloop()
{
    double dt = tick();
    display_frame_time(dt);
    update_SDL();

    update(dt);

    render();

    return running;
}

int main(__attribute__((unused))int argc, __attribute__((unused))char* argv[])
{   
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Rect win_size;
    int render_width = 0, render_height = 0;
    SDL_GetDisplayBounds(0,&win_size);
    win_width = win_size.w * WINDOW_SCALE;
    win_height = win_size.h * WINDOW_SCALE;
    window = SDL_CreateWindow("I am a v_window, so what?!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_width, win_height, WINDOW_FLAGS);
    renderer = SDL_CreateRenderer(window, 1, RENDERER_FLAGS);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_GetWindowSize(window,&win_width,&win_height);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    if (ENABLE_SCREEN_RESIZE && ENABLE_SCREEN_REFRESH_DURING_RESIZE)
        SDL_SetEventFilter(dynamic_screen_resize, NULL);
    SDL_GetRendererOutputSize(renderer, &render_width, &render_height);
    if(render_width != win_width)
        SDL_RenderSetScale(renderer, (float)render_width / (float) win_width, (float)render_height / (float) win_height);
    tick();
    while(gameloop());
    free(last_keys);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    exit(0);
}

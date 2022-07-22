#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

#include <SDL2/SDL.h>
#include <DBG/debug.h>
#include <construct/construct.h>

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
const double CAM_SPEED = 5.0f;

const double MS_PER_GAMETICK = 10.0f;

#include "util.h"
#include "ECS.h"

buffer rectangle_component;
buffer drawable_component;
unsigned int* entities = NULL;

void drawable_system()
{
    unsigned int size = get_buffer_length(drawable_component),i;
    change_draw_color(255,255,255,255);
    for (i = 0; i < size; i++)
    {
        if (!has_component(get_buffer_fieldui(rectangle_component,i,0),rectangle_component,NULL))
            continue;

        if (get_buffer_fieldui(drawable_component,i,1))
            render_rect(*(rect*)get_buffer_pointerf(rectangle_component,i,1),true,false,align_none);
        if (get_buffer_fieldv(drawable_component,i,2) != NULL)
            render_texture(*(rect*)get_buffer_pointerf(rectangle_component,i,1),false,align_none,get_buffer_fieldv(drawable_component,i,2));
    }
}

void render()
{   
    draw_debug_boundaries();
    drawable_system();
    SDL_SetRenderDrawColor(renderer,125,125,125,255);
    SDL_RenderPresent(renderer);
    SDL_RenderClear(renderer);
}

void update(double dt)
{
    accumulator += dt;
    /* Handle input-logic here (every frame) */

    while(accumulator >= MS_PER_GAMETICK)
    {
        accumulator -= MS_PER_GAMETICK;
        /* Handle game-logic here (every tick, aka 10ms) */
    }
}

int main(__attribute__((unused))int argc, __attribute__((unused))char* argv[])
{   

    SDL_Init(SDL_INIT_VIDEO);
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

    rectangle_component = init_bufferva(0,5,UINT,FLOAT,FLOAT,UINT,UINT);
    drawable_component = init_bufferva(0,3,UINT,UINT,VOID);

    unsigned int entity = create_entity(&entities),idx;
    add_component(entity,rectangle_component);
    has_component(entity,rectangle_component,&idx);

    set_buffer_fieldf(rectangle_component,idx,1,250);
    set_buffer_fieldf(rectangle_component,idx,2,250);
    set_buffer_fieldui(rectangle_component,idx,3,500);
    set_buffer_fieldui(rectangle_component,idx,4,500);

    add_component(entity,drawable_component);
    has_component(entity,drawable_component,&idx);

    set_buffer_fieldui(drawable_component,idx,1,true);
    set_buffer_fieldv(drawable_component,idx,2,SDL_CreateTextureFromSurface(renderer,SDL_LoadBMP("../assets/iso_cube.bmp")));


    tick();
    while(gameloop());
    free(last_keys);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    remove_component(entity,rectangle_component);
    remove_component(entity,drawable_component);
    destroy_entity(entity,&entities);
    
    deinit_buffer(rectangle_component);
    deinit_buffer(drawable_component);
    
    exit(EXIT_SUCCESS);
}

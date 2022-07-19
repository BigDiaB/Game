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

void render()
{   
    rect ui_rect = {0,0,500,500};

    rect obj_rect = {250,250,500,500};

    change_draw_color(255,255,255,255);
    render_rect(&ui_rect,true,true,align_corner);
    render_rect(&obj_rect,true,false,align_none);

    draw_debug_boundaries();
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

bool gameloop()
{
    double dt = tick();
    display_frame_time(dt);
    update_SDL();

    update(dt);

    render();

    return running;
}

bool has_component(unsigned int entity, buffer component, unsigned int* idx)
{
    unsigned int i, size = get_buffer_length(component);

    for (i = 0; i < size; i++)
    {
        if (get_buffer_fieldui(component,i,0) == entity)
        {
            if (idx != NULL)
                *idx = i;
            return true;
        }
    }
    return false;
}

void add_component(unsigned int entity, buffer component)
{
    if (has_component(entity,component,NULL))
        return;
    resize_buffer(component,get_buffer_length(component)+1);
    set_buffer_fieldui(component,get_buffer_length(component)-1,0,entity);
}

void remove_component(unsigned int entity, buffer component)
{
    unsigned int index;
    if (!has_component(entity,component,&index))
        return;

    remove_buffer_at(component,index);
}

unsigned int* entities = NULL;

unsigned int create_entity()
{
    if (entities == NULL)
    {
        entities = malloc(sizeof(unsigned int));
        entities[0] = 0;
    }

    entities[0]++;
    if (entities[0] == 0)
    {
        puts("More than 2^32 entities! Internal buffer overflown!");
        exit(EXIT_FAILURE);
    }
    entities = realloc(entities,entities[0]);

    /* somehow get new id*/
    return entities[entities[0]] = pseudo_random_premutation(entities[0]-1);
}

void destroy_entity(unsigned int entity)
{
    unsigned int i;
    bool found = false;
    for (i = 1; i < entities[0] +1; i++)
    {
        found = entities[i] == entity;
        if (found)
            break;
    }

    if (found)
    {
        if (--entities[0] == 0)
        {
            free(entities);
            entities = NULL;
            return;
        }

        for (; i < entities[0]; i++)
        {
            entities[i] = entities[i + 1];
        }
    }
}

int main(__attribute__((unused))int argc, __attribute__((unused))char* argv[])
{   
    buffer tag_component = init_bufferva(0,2,UINT,VOID);

    unsigned int entity = create_entity();
    add_component(entity,tag_component);

    if (has_component(entity,tag_component,NULL))
        puts("Hello, World!");

    remove_component(entity,tag_component);
    destroy_entity(entity);

    deinit_buffer(tag_component);



    exit(EXIT_SUCCESS);

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
    tick();
    while(gameloop());
    free(last_keys);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    exit(EXIT_SUCCESS);
}

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

/*#include <DBG/debug.h>*/
#include <construct/construct.h>


#include <SDL2/SDL.h>

const float WINDOW_SCALE = 0.8f;
const bool VSYNC = false;

const unsigned int WINDOW_FLAGS = SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
const unsigned int RENDERER_FLAGS = SDL_RENDERER_ACCELERATED | (VSYNC ? SDL_RENDERER_PRESENTVSYNC : 0);

const unsigned int TILE_SIZE = 200;
const unsigned int CHUNK_SIZE = 10;
const float WORLD_ZOOM = 1.0f;
const double WORLD_SIZE = 1000.0f / WORLD_ZOOM;

const double CAM_SMOOTH = 0.008f * 1000 / WORLD_SIZE;
const unsigned int CAM_SPEED = 5;

#include "util.h"

int main(__attribute__((unused))int argc, __attribute__((unused))char* argv[])
{   

    init_buffers();

    float player_x,player_y,player_z;

    resize_buffer(entity_buffer,3);

    while(iterate_over(entity_buffer))
    {
        set_fieldf(0,(get_iterator() + 5) * TILE_SIZE);
        set_fieldf(1,15 * TILE_SIZE);
        set_fieldf(2,2 * TILE_SIZE);
        set_fieldui(3,am_default);
    }

    set_buffer_fieldf(entity_buffer,1,0,5 * TILE_SIZE);
    set_buffer_fieldf(entity_buffer,1,1,5 * TILE_SIZE);
    set_buffer_fieldf(entity_buffer,1,2,1 * TILE_SIZE);
    set_buffer_fieldui(entity_buffer,1,3,am_selected);

    load_chunk("test_chunk.chunk");
    load_chunk("test_chunk2.chunk");
    load_chunk("test_chunk3.chunk");
    load_chunk("test_chunk4.chunk");


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
    SDL_SetEventFilter(dynamic_screen_resize, NULL);
    SDL_GetRendererOutputSize(renderer, &render_width, &render_height);
    if(render_width != win_width)
        SDL_RenderSetScale(renderer, (float)render_width / (float) win_width, (float)render_height / (float) win_height);

    tick();

    assets[am_default] = SDL_CreateTextureFromSurface(renderer,SDL_LoadBMP("../assets/iso_cube.bmp"));
    assets[am_selected] = SDL_CreateTextureFromSurface(renderer,SDL_LoadBMP("../assets/iso_selected.bmp"));
    assets[am_interactable] = SDL_CreateTextureFromSurface(renderer,SDL_LoadBMP("../assets/iso_interactable.bmp"));
    assets[am_debug] = SDL_CreateTextureFromSurface(renderer,SDL_LoadBMP("../assets/iso_debug.bmp"));

    while(running)
    {
        render_draw_buffer();

        player_x = get_buffer_fieldf(entity_buffer,1,0);
        player_y = get_buffer_fieldf(entity_buffer,1,1);
        player_z = get_buffer_fieldf(entity_buffer,1,2);

        unsigned int i;
        for (i = 0; i < get_buffer_length(loaded_world); i++)
        {
            while(iterate_over(get_buffer_fieldv(loaded_world,i,ldm_cubes)))
            {
                add_to_collider_buffer(get_fieldi(cm_x),
                                       get_fieldi(cm_y),
                                       get_fieldi(cm_z),
                                       get_buffer_fieldui(loaded_world,i,ldm_xoff),
                                       get_buffer_fieldui(loaded_world,i,ldm_yoff));
            }
        }

        while(iterate_over(entity_buffer))
        {
            if (get_iterator() == 1)
                continue;
            add_to_collider_buffer(get_fieldf(ebm_x),
                                   get_fieldf(ebm_y),
                                   get_fieldf(ebm_z),
                                   0,
                                   0);
        }

        update_SDL();
        
        double tick_time = tick();

        display_frame_time(tick_time);

        update_actions();

        accumulator += tick_time;
        while(accumulator >= 10.0f)
        {
            accumulator -= 10.0f;

            if (camera_follow)
            {   
                float player_cart_x = player_x - player_y;
                float player_cart_y = (player_x + player_y) / 2 - player_z;
                cam_x -= (cam_x - (WORLD_SIZE * 16.0f / 9.0f / 2 - player_cart_x / 2 - TILE_SIZE / 2)) * CAM_SMOOTH;
                cam_y -= (cam_y - (WORLD_SIZE / 2 - player_cart_y / 2 - TILE_SIZE / 2)) * CAM_SMOOTH;
            }
            else
            {
                if (current_actions[act_cam_move_right])
                    cam_x -= CAM_SPEED;

                if (current_actions[act_cam_move_left])
                    cam_x += CAM_SPEED;

                if (current_actions[act_cam_move_down])
                    cam_y -= CAM_SPEED;

                if (current_actions[act_cam_move_up])
                    cam_y += CAM_SPEED;
            }

            float last_x = player_x, last_y = player_y, last_z = player_z;
            
            unsigned int speed = 5;

            if (current_actions[act_move_right])
                player_x += speed;
            if (current_actions[act_move_left])
                player_x -= speed;

            if (check_collider_buffer(player_x,player_y,player_z))
            {
                player_x = last_x;
            }


            if (current_actions[act_move_down])
                player_y += speed;
            if (current_actions[act_move_up])
                player_y -= speed;

            if (check_collider_buffer(player_x,player_y,player_z))
            {
                player_y = last_y;
            }


            if (current_actions[act_float_up])
                player_z += speed;
            if (current_actions[act_float_down])
                player_z -= speed;

            if (check_collider_buffer(player_x,player_y,player_z))
            {
                player_z = last_z;
            }
        }

        set_buffer_fieldf(entity_buffer,1,0,player_x);
        set_buffer_fieldf(entity_buffer,1,1,player_y);
        set_buffer_fieldf(entity_buffer,1,2,player_z);

        if (get_buffer_length(collider_buffer) != 0)
            resize_buffer(collider_buffer,0);

        SDL_SetRenderDrawColor(renderer,0,175,200,255);
        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);
    }

    safe_world();

    deinit_buffers();
    free(last_keys);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    exit(0);
}

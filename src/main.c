#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

#include <construct/construct.h>
#include <DBG/debug.h>

#include <SDL2/SDL.h>

const uint TILE_SIZE = 200;
const uint WORLD_SIZE = 1000;
const bool vsync = false;

buffer draw_buffer, loaded_world, entity_buffer;
int cam_x = 4 * TILE_SIZE, cam_y = 0 * TILE_SIZE;

enum draw_buffer_mask
{
    dbm_x,dbm_y,dbm_z,dbm_tex
};

enum test_chunk_mask
{
    tcm_x,tcm_y,tcm_z,tcm_type
};

enum asset_mask
{
    am_default,am_selected,am_interactable,num_assets
};

enum action_mask
{
    act_move_up,act_move_down,act_move_left,act_move_right,act_float_up,act_float_down,num_actions
};

SDL_Texture* assets[num_assets];
bool current_actions[num_actions];
bool last_actions[num_actions];

#include "util.h"

int main(__attribute__((unused))int argc, __attribute__((unused))char* argv[])
{   
    /*
    0: X Position   float
    1: Y Position   float
    2: Z Position   float
    3: Tex-index    uint
    */

    push_type(FLOAT);
    push_type(FLOAT);
    push_type(FLOAT);
    push_type(UINT);

    draw_buffer = init_buffer(0);

    /*
    0: X Offset     uint
    1: Y Offset     uint
    2: Cube Buffer  void*
    3: Filename     void*
    */

    push_type(UINT);
    push_type(UINT);
    push_type(VOID);
    push_type(VOID);

    loaded_world = init_buffer(0);

     /*
    0: X Position   float
    1: Y Position   float
    2: Z Position   float
    3: Tex-index    uint
    */

    push_type(FLOAT);
    push_type(FLOAT);
    push_type(FLOAT);
    push_type(UINT);

    entity_buffer = init_buffer(1);

    float player_x,player_y,player_z;
    uint player_tex;

    set_buffer_fieldf(entity_buffer,0,0,5 * TILE_SIZE);
    set_buffer_fieldf(entity_buffer,0,1,5 * TILE_SIZE);
    set_buffer_fieldf(entity_buffer,0,2,1 * TILE_SIZE);
    set_buffer_fieldui(entity_buffer,0,3,am_selected);

    load_chunk("test_chunk.chunk");

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Rect win_size;

    SDL_GetDisplayBounds(0,&win_size);
    const float window_scale = 0.8f;
    win_width = win_size.w * window_scale;
    win_height = win_size.h * window_scale;

    window = SDL_CreateWindow("I am a v_window, so what?!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_width, win_height, SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    renderer = SDL_CreateRenderer(window, 1, SDL_RENDERER_ACCELERATED | (vsync ? SDL_RENDERER_PRESENTVSYNC : 0) );
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_GetWindowSize(window,&win_width,&win_height);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    tick();

    {
        int render_width = 0, render_height = 0;
        SDL_GetRendererOutputSize(renderer, &render_width, &render_height);
        if(render_width != win_width)
            SDL_RenderSetScale(renderer, (float)render_width / (float) win_width, (float)render_height / (float) win_height);
    }

    assets[am_default] = SDL_CreateTextureFromSurface(renderer,SDL_LoadBMP("../assets/iso_cube.bmp"));
    assets[am_selected] = SDL_CreateTextureFromSurface(renderer,SDL_LoadBMP("../assets/iso_selected.bmp"));
    assets[am_interactable] = SDL_CreateTextureFromSurface(renderer,SDL_LoadBMP("../assets/iso_interactable.bmp"));

    while(running)
    {

        player_x = get_buffer_fieldf(entity_buffer,0,0);
        player_y = get_buffer_fieldf(entity_buffer,0,1);
        player_z = get_buffer_fieldf(entity_buffer,0,2);
        player_tex = get_buffer_fieldui(entity_buffer,0,3);

        update_SDL();
        
        double tick_time = tick();

        display_frame_time(tick_time);

        update_actions();

        accumulator += tick_time;
        while(accumulator >= 10.0f)
        {
            accumulator -= 10.0f;
            /* Physics and other time-dependent stuff */

            float last_x = player_x, last_y = player_y, last_z = player_z;

            const float speed = 5.0f;

            uint i;
            for (i = 0; i < 3; i++)
            {
                switch(i)
                {
                    case 0:
                    {
                        if (current_actions[act_move_right])
                            player_x += speed;
                        if (current_actions[act_move_left])
                            player_x -= speed;
                    }
                    break;
                    case 1:
                    {
                        if (current_actions[act_move_down])
                            player_y += speed;

                        if (current_actions[act_move_up])
                            player_y -= speed;
                    }
                    break;
                    case 2:
                    {
                        if (current_actions[act_float_up])
                            player_z += speed;

                        if (current_actions[act_float_down])
                            player_z -= speed;
                    }
                    break;
                    default:
                    break;
                }

                uint j;
                for (j = 0; j < get_buffer_length(loaded_world); j++)
                {
                    while(iterate_over(get_buffer_fieldv(loaded_world,j,2)))
                    {
                        if ((player_x < (get_fieldui(0) + get_buffer_fieldui(loaded_world,0,0)) * TILE_SIZE + TILE_SIZE && player_x + TILE_SIZE > (get_fieldui(0) + get_buffer_fieldui(loaded_world,0,0)) * TILE_SIZE) &&
                           (player_y < (get_fieldui(1) + get_buffer_fieldui(loaded_world,0,1)) * TILE_SIZE + TILE_SIZE && player_y + TILE_SIZE > (get_fieldui(1) + get_buffer_fieldui(loaded_world,0,1)) * TILE_SIZE) &&
                           (player_z < get_fieldui(2) * TILE_SIZE + TILE_SIZE && player_z + TILE_SIZE > get_fieldui(2) * TILE_SIZE))
                        {
                            switch(i)
                            {
                                case 0:
                                player_x = last_x;
                                break;
                                case 1:
                                player_y = last_y;
                                break;
                                case 2:
                                player_z = last_z;
                                break;
                                default:
                                break;
                            }
                        }
                    }
                }
            }            
        }

        set_buffer_fieldf(entity_buffer,0,0,player_x);
        set_buffer_fieldf(entity_buffer,0,1,player_y);
        set_buffer_fieldf(entity_buffer,0,2,player_z);
        set_buffer_fieldui(entity_buffer,0,3,player_tex);


        uint i;
        for (i = 0; i < get_buffer_length(loaded_world); i++)
        {
            while(iterate_over(get_buffer_fieldv(loaded_world,i,2)))
            {
                add_to_draw_buffer((get_fieldui(tcm_x) + get_buffer_fieldui(loaded_world,i,0)) * TILE_SIZE,(get_fieldui(tcm_y) + get_buffer_fieldui(loaded_world,i,1)) * TILE_SIZE,get_fieldui(tcm_z) * TILE_SIZE,get_fieldui(tcm_type));
            }
        }

        while(iterate_over(entity_buffer))
            add_to_draw_buffer(get_fieldf(0),get_fieldf(1),get_fieldf(2),get_fieldui(3));

        render_draw_buffer();

        SDL_SetRenderDrawColor(renderer,125,125,125,255);
        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);
    }

    safe_world();

    uint i;
    for (i = 0; i < get_buffer_length(loaded_world); i++)
        deinit_buffer(get_buffer_fieldv(loaded_world,i,2));
    deinit_buffer(loaded_world);
    deinit_buffer(draw_buffer);
    deinit_buffer(entity_buffer);
    free(last_keys);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    exit(0);
}

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

#include <construct/construct.h>
#include <DBG/debug.h>

#include <SDL2/SDL.h>

const unsigned int TILE_SIZE = 200;
const unsigned int WORLD_SIZE = 1000;
const unsigned int CHUNK_SIZE = 10;
const bool vsync = false;

buffer draw_buffer, loaded_world, collider_buffer, entity_buffer;
float cam_x = 4 * TILE_SIZE, cam_y = 0 * TILE_SIZE;

enum loaded_world_mask
{
    ldm_xoff,ldm_yoff,ldm_drawflag,ldm_cubes,ldm_entities,ldm_filename
};

enum cube_mask
{
    cm_x,cm_y,cm_z,cm_tex
};

enum draw_buffer_ent
{
    dbe_entity, dbe_world
};

enum collider_buffer_mask
{
    cbm_x,cbm_y,cbm_z
};

enum draw_buffer_mask
{
    dbm_x,dbm_y,dbm_z,dbm_tex
};

enum entity_buffer_mask
{
    ebm_x,ebm_y,ebm_z,ebm_type
};

enum entity_type
{
    et_player, et_debug
};

enum asset_mask
{
    am_default,am_selected,am_interactable,am_debug,num_assets
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
    0: X Position  int
    1: Y Position  int
    2: Z Position  int
    */

    push_type(INT);
    push_type(INT);
    push_type(INT);

    collider_buffer = init_buffer(0);

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
    2: Draw Flag    uint
    3: Cube Buffer  void*
    4: Entities     void*
    5: Filename     void*
    */

    push_type(UINT);
    push_type(UINT);
    push_type(UINT);
    push_type(VOID);
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
    unsigned int player_tex;

    set_buffer_fieldf(entity_buffer,0,0,5 * TILE_SIZE);
    set_buffer_fieldf(entity_buffer,0,1,5 * TILE_SIZE);
    set_buffer_fieldf(entity_buffer,0,2,1 * TILE_SIZE);
    set_buffer_fieldui(entity_buffer,0,3,am_selected);

    load_chunk("test_chunk3.chunk");
    load_chunk("test_chunk2.chunk");
    load_chunk("test_chunk.chunk");

    SDL_Init(SDL_INIT_VIDEO);

    const float window_scale = 0.8f;
    SDL_Rect win_size;
    int render_width = 0, render_height = 0;
    SDL_GetDisplayBounds(0,&win_size);
    win_width = win_size.w * window_scale;
    win_height = win_size.h * window_scale;

    window = SDL_CreateWindow("I am a v_window, so what?!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_width, win_height, SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    renderer = SDL_CreateRenderer(window, 1, SDL_RENDERER_ACCELERATED | (vsync ? SDL_RENDERER_PRESENTVSYNC : 0) );
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_GetWindowSize(window,&win_width,&win_height);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
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
        player_x = get_buffer_fieldf(entity_buffer,0,0);
        player_y = get_buffer_fieldf(entity_buffer,0,1);
        player_z = get_buffer_fieldf(entity_buffer,0,2);
        player_tex = get_buffer_fieldui(entity_buffer,0,3);

        update_SDL();
        
        double tick_time = tick();

        display_frame_time(tick_time);

        update_actions();

        
        unsigned int i;
        for (i = 0; i < get_buffer_length(loaded_world); i++)
        {
            while(iterate_over(get_buffer_fieldv(loaded_world,i,ldm_cubes)))
            {
                add_to_collider_buffer(get_fieldi(cm_x) + get_buffer_fieldui(loaded_world,i,ldm_xoff) * CHUNK_SIZE * TILE_SIZE,
                                       get_fieldi(cm_y) + get_buffer_fieldui(loaded_world,i,ldm_yoff) * CHUNK_SIZE * TILE_SIZE,
                                       get_fieldi(cm_z));
            }
        }

        accumulator += tick_time;
        while(accumulator >= 10.0f)
        {
            accumulator -= 10.0f;

            const double smooth = 0.008f;

            float player_cart_x = player_x - player_y;
            float player_cart_y = (player_x + player_y) / 2 - player_z;

            cam_x -= (cam_x - (WORLD_SIZE * 16.0f / 9.0f / 2 - player_cart_x / 2 - TILE_SIZE / 2)) * smooth;
            cam_y -= (cam_y - (WORLD_SIZE / 2 - player_cart_y / 2 - TILE_SIZE / 2)) * smooth;

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

        set_buffer_fieldf(entity_buffer,0,0,player_x);
        set_buffer_fieldf(entity_buffer,0,1,player_y);
        set_buffer_fieldf(entity_buffer,0,2,player_z);
        set_buffer_fieldui(entity_buffer,0,3,player_tex);

        render_draw_buffer();

        SDL_SetRenderDrawColor(renderer,0,175,200,255);
        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);

        if (get_buffer_length(collider_buffer) != 0)
            resize_buffer(collider_buffer,0);
    }

    unsigned int i;

    safe_world();

    for (i = 0; i < get_buffer_length(loaded_world); i++)
        deinit_buffer(get_buffer_fieldv(loaded_world,i,ldm_cubes));
    deinit_buffer(loaded_world);
    deinit_buffer(draw_buffer);
    deinit_buffer(collider_buffer);
    deinit_buffer(entity_buffer);
    free(last_keys);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    exit(0);
}

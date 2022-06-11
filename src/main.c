#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

#include <construct/construct.h>
#include <DBG/debug.h>

#include <SDL2/SDL.h>


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
    am_default,am_selected
};

buffer draw_buffer, loaded_world, entity_buffer;
SDL_Texture* assets[2];
const uint TILE_SIZE = 200;
int cam_x = 4 * TILE_SIZE, cam_y = 0 * TILE_SIZE;

double accumulator;


SDL_Window* window;
SDL_Renderer* renderer;

const uint WORLD_SIZE = 1000;

const bool vsync = true;
int win_width = 1280, win_height = 720;

#include "util.h"


void load_world(char* filename)
{
    resize_buffer(loaded_world,get_buffer_length(loaded_world) + 1);
    FILE *file;
    file = fopen(filename,"rb");

    fseek(file, 0, SEEK_END);
    uint size = ftell(file);
    fseek(file, 0, SEEK_SET);

    void* bin_data = malloc(size);
    fread(bin_data,1,size,file);

    push_type(UINT);
    push_type(UINT);
    push_type(UINT);
    push_type(UINT);

    uint num_elements = *((uint*)bin_data);

    set_buffer_fieldui(loaded_world,get_buffer_length(loaded_world) - 1,0,*((uint*)(bin_data + sizeof(uint))));
    set_buffer_fieldui(loaded_world,get_buffer_length(loaded_world) - 1,1,*((uint*)(bin_data + sizeof(uint) * 2)));
    set_buffer_fieldv(loaded_world,get_buffer_length(loaded_world) - 1,2,init_buffer(num_elements));

    load_buffer_binary(get_buffer_fieldv(loaded_world,get_buffer_length(loaded_world) - 1,2),bin_data + sizeof(uint) * 3,size);

    free(bin_data);
}

void safe_world(char* filename)
{
    FILE* file = fopen(filename,"wb");
    uint size;
    void* bin_data = dump_buffer_binary(get_buffer_fieldv(loaded_world,get_buffer_length(loaded_world) - 1,2),&size);
    uint num_elements = get_buffer_length(get_buffer_fieldv(loaded_world,get_buffer_length(loaded_world) - 1,2));
    uint x_off = get_buffer_fieldui(loaded_world,get_buffer_length(loaded_world) - 1,0), y_off = get_buffer_fieldui(loaded_world,get_buffer_length(loaded_world) - 1,1);

    fwrite(&num_elements,1,sizeof(uint),file);
    fwrite(&x_off,1,sizeof(uint),file);
    fwrite(&y_off,1,sizeof(uint),file);
    fwrite(bin_data,1,get_buffer_size(get_buffer_fieldv(loaded_world,get_buffer_length(loaded_world) - 1,2)),file);
    free(bin_data);
}

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

    push_type(UINT);
    push_type(UINT);
    push_type(VOID);

    loaded_world = init_buffer(0);

    push_type(FLOAT);
    push_type(FLOAT);
    push_type(FLOAT);
    push_type(UINT);

    entity_buffer = init_buffer(1);

    float player_x,player_y,player_z;
    uint player_tex;

    set_buffer_fieldf(entity_buffer,0,0,-200);
    set_buffer_fieldf(entity_buffer,0,1,-200);
    set_buffer_fieldf(entity_buffer,0,2,200);
    set_buffer_fieldui(entity_buffer,0,3,am_selected);

    load_world("test_chunk.world");

    buffer temp = copy_buffer(get_buffer_fieldv(loaded_world,0,2));

    while(iterate_over(temp))
    {
        set_fieldui(tcm_z,get_fieldui(tcm_z) + TILE_SIZE);
    }

    deinit_buffer(temp);
    

    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("I am a v_window, so what?!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, win_width, win_height, SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE );
    renderer = SDL_CreateRenderer(window, 1, SDL_RENDERER_ACCELERATED | (vsync ? SDL_RENDERER_PRESENTVSYNC : 0) );
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_GetWindowSize(window,&win_width,&win_height);
    tick();

    assets[am_default] = SDL_CreateTextureFromSurface(renderer,SDL_LoadBMP("../assets/iso_cube.bmp"));
    assets[am_selected] = SDL_CreateTextureFromSurface(renderer,SDL_LoadBMP("../assets/iso_selected.bmp"));


    


    while(running)
    {

        player_x = get_buffer_fieldf(entity_buffer,0,0);
        player_y = get_buffer_fieldf(entity_buffer,0,1);
        player_z = get_buffer_fieldf(entity_buffer,0,2);
        player_tex = get_buffer_fieldui(entity_buffer,0,3);

        update_SDL();
        
        double tick_time = tick();

        accumulator += tick_time;
        while(accumulator >= 1.0f)
        {
            /* Physics and other time-dependent stuff */

            float last_x = player_x, last_y = player_y, last_z = player_z;

            const float speed = 0.5f;

            if (current_keys[SDL_SCANCODE_RIGHT])
                player_x += speed;

            if (current_keys[SDL_SCANCODE_LEFT])
                player_x -= speed;

            #define COLL_CHECK
            #ifdef COLL_CHECK
            while(iterate_over(get_buffer_fieldv(loaded_world,0,2)))
            {
                if ((player_x < get_fieldui(0) * TILE_SIZE + TILE_SIZE && player_x + TILE_SIZE  > get_fieldui(0) * TILE_SIZE) &&
                   (player_y < get_fieldui(1) * TILE_SIZE + TILE_SIZE && player_y + TILE_SIZE  > get_fieldui(1) * TILE_SIZE) &&
                   (player_z < get_fieldui(2) * TILE_SIZE + TILE_SIZE && player_z + TILE_SIZE  > get_fieldui(2) * TILE_SIZE))
                {
                    player_x = last_x;
                }
            }
            #endif

             if (current_keys[SDL_SCANCODE_DOWN])
                player_y += speed;

            if (current_keys[SDL_SCANCODE_UP])
                player_y -= speed;

            #ifdef COLL_CHECK
            while(iterate_over(get_buffer_fieldv(loaded_world,0,2)))
            {
                if ((player_x < get_fieldui(0) * TILE_SIZE + TILE_SIZE && player_x + TILE_SIZE  > get_fieldui(0) * TILE_SIZE) &&
                   (player_y < get_fieldui(1) * TILE_SIZE + TILE_SIZE && player_y + TILE_SIZE  > get_fieldui(1) * TILE_SIZE) &&
                   (player_z < get_fieldui(2) * TILE_SIZE + TILE_SIZE && player_z + TILE_SIZE  > get_fieldui(2) * TILE_SIZE))
                {
                    player_y = last_y;
                }
            }
            #endif

            if (current_keys[SDL_SCANCODE_W])
                player_z += speed;

            if (current_keys[SDL_SCANCODE_S])
                player_z -= speed;

            #ifdef COLL_CHECK
            while(iterate_over(get_buffer_fieldv(loaded_world,0,2)))
            {
                if ((player_x < get_fieldui(0) * TILE_SIZE + TILE_SIZE && player_x + TILE_SIZE > get_fieldui(0) * TILE_SIZE) &&
                   (player_y < get_fieldui(1) * TILE_SIZE + TILE_SIZE && player_y + TILE_SIZE > get_fieldui(1) * TILE_SIZE) &&
                   (player_z < get_fieldui(2) * TILE_SIZE + TILE_SIZE && player_z + TILE_SIZE > get_fieldui(2) * TILE_SIZE))
                {
                    player_z = last_z;
                }
            }
            #endif

            

            accumulator -= 1.0f;
        }


        uint i;
        for (i = 0; i < get_buffer_length(loaded_world); i++)
        {
            while(iterate_over(get_buffer_fieldv(loaded_world,i,2)))
            {
                add_to_draw_buffer((get_fieldui(tcm_x) + get_buffer_fieldui(loaded_world,i,0)) * TILE_SIZE / 2,(get_fieldui(tcm_y) + get_buffer_fieldui(loaded_world,i,1)) * TILE_SIZE / 2,get_fieldui(tcm_z) * TILE_SIZE,get_fieldui(tcm_type));
            }
        }

        add_to_draw_buffer(get_buffer_fieldf(entity_buffer,0,0) / 2,get_buffer_fieldf(entity_buffer,0,1) / 2,get_buffer_fieldf(entity_buffer,0,2),get_buffer_fieldui(entity_buffer,0,3));

        render_draw_buffer();

        set_buffer_fieldf(entity_buffer,0,0,player_x);
        set_buffer_fieldf(entity_buffer,0,1,player_y);
        set_buffer_fieldf(entity_buffer,0,2,player_z);
        set_buffer_fieldui(entity_buffer,0,3,player_tex);
    }

    safe_world("test_chunk.world");

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


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

#include <construct/construct.h>
#include <DBG/debug.h>

#include <SDL2/SDL.h>

float map_num(float num, float min1, float max1, float min2, float max2)
{
    return (num - min1) * (max2 - min2) / (max1 - min1) + min2;
}

double tick()
{
    static struct timeval t1;
    struct timeval t2;
    gettimeofday(&t2, NULL);
    double elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
    gettimeofday(&t1, NULL);

    return elapsedTime;
}

double accumulator;

const Uint8* current_keys;
Uint8* last_keys = NULL;
int keysize;

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Event event;
const uint WORLD_SIZE = 1000;
const bool vsync = false; 
bool running = true;
int win_width = 1280, win_height = 720;
const int tick_precision = 700;
double ticks[tick_precision] = {0};
uint used_ticks = 0;

bool pressed(uint key)
{
    return !last_keys[key] && current_keys[key];
}

bool released(uint key)
{
    return last_keys[key] && !current_keys[key];
}

bool down(uint key)
{
    return current_keys[key];
}


SDL_Rect* translate_rect(SDL_Rect* r)
{
    float stretch = (float)win_width / (float)win_height;
    r->x = map_num(r->x,0,WORLD_SIZE * stretch,0,win_width);
    r->w = map_num(r->w,0,WORLD_SIZE * stretch,0,win_width);
    r->y = map_num(r->y,0,WORLD_SIZE,0,win_height);
    r->h = map_num(r->h,0,WORLD_SIZE,0,win_height);

    r->x += (win_width - win_height / 9 * 16) / 2;

    return r;
}

SDL_Texture* cube;
enum object_mask
{
    om_x,om_y,om_z,om_tex
};

void quicksort(uint first_index, uint last_index, buffer sort_buffer)
{
    if (first_index < last_index)
    {
        uint pivot = get_buffer_fieldf(sort_buffer,last_index,om_x) + get_buffer_fieldf(sort_buffer,last_index,om_y) + get_buffer_fieldf(sort_buffer,last_index,om_z);
        uint i = (first_index - 1),j;
        for (j = first_index; j < last_index; j++)
        {
            if (get_buffer_fieldf(sort_buffer,j,om_x) + get_buffer_fieldf(sort_buffer,j,om_y) + get_buffer_fieldf(sort_buffer,j,om_z) <= pivot)
            {
                i++;
                swap_buffer_at(sort_buffer,i,j);
            }
        }

        swap_buffer_at(sort_buffer,i + 1,last_index);
        quicksort(first_index, i,sort_buffer);
        quicksort(i + 2, last_index,sort_buffer);
    }
}

int main(__attribute__((unused))int argc, __attribute__((unused))char* argv[])
{   
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("I am a v_window, so what?!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, win_width, win_height, SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE );
    renderer = SDL_CreateRenderer(window, 1, SDL_RENDERER_ACCELERATED | (vsync ? SDL_RENDERER_PRESENTVSYNC : 0) );
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_GetWindowSize(window,&win_width,&win_height);
    tick();

    cube = SDL_CreateTextureFromSurface(renderer,SDL_LoadBMP("../assets/iso_cube.bmp"));

    push_type(FLOAT);
    push_type(FLOAT);
    push_type(FLOAT);
    push_type(VOID);

    /*
    0: X Position   float
    1: Y Position   float
    2: Z Position   float
    3: Tex          void*
    */

    const uint TILE_SIZE = 200;

    float pos_x = 4 * TILE_SIZE, pos_y = 4 * TILE_SIZE;
    int cam_x = 4 * TILE_SIZE, cam_y = -3 * TILE_SIZE;

    buffer objects = init_buffer(256);

    srand(216);
    while(iterate_over(objects))
    {
        set_fieldf(om_x,(get_iterator() % 16) * TILE_SIZE / 2);
        set_fieldf(om_y,(get_iterator() / 16) * TILE_SIZE / 2);
        set_fieldf(om_z,((rand() % 2)) * TILE_SIZE);
        set_fieldv(om_tex,cube);
    }

    uint i,len = get_buffer_length(objects);

    set_buffer_fieldf(objects,len - 1,om_z,2 * TILE_SIZE);
    set_buffer_fieldv(objects,len - 1,om_tex,cube);


    while(running)
    {
        if (last_keys == NULL)
        {
            current_keys = SDL_GetKeyboardState(&keysize);
            last_keys = malloc(keysize * sizeof(Uint8));
        }
        memcpy(last_keys,current_keys,keysize * sizeof(Uint8));
        current_keys = SDL_GetKeyboardState(NULL);
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT:
                running = false;
                break;

                case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    SDL_GetWindowSize(window,&win_width,&win_height);
                }
                default:
                break;
            }
        }
        double tick_time = tick();

        accumulator += tick_time;
        while(accumulator >= 1.0f)
        {
            const float speed = 0.5f;
            if (down(SDL_SCANCODE_W))
                pos_y -= speed;
            if (down(SDL_SCANCODE_S))
                pos_y += speed;
            if (down(SDL_SCANCODE_A))
                pos_x -= speed;
            if (down(SDL_SCANCODE_D))
                pos_x += speed;
        

            /* Physics and other time-dependent stuff */
            accumulator -= 1.0f;
        }

        ticks[used_ticks] = tick_time;
        used_ticks++;

        if (used_ticks == tick_precision)
        {   
            double avrg = 0.0f;
            for (i = 0; i < used_ticks; i++)
            {
                avrg += ticks[i];
            }
            avrg /= used_ticks;
            used_ticks = 0;
            char title[128] = {0};
            sprintf(title,"FPS: %fms %f",avrg,1000.0f / avrg);
            SDL_SetWindowTitle(window,title);
        }

        set_buffer_fieldf(objects,len - 1,om_x,pos_x);
        set_buffer_fieldf(objects,len - 1,om_y,pos_y);

        buffer draw_buffer = copy_buffer(objects);

        quicksort(0,get_buffer_length(draw_buffer) - 1,draw_buffer);

        for (i = 0; i < get_buffer_length(draw_buffer); i++)
        {
            SDL_Rect r = {(get_buffer_fieldf(draw_buffer,i,om_x) - get_buffer_fieldf(draw_buffer,i,om_y) + cam_x), ((get_buffer_fieldf(draw_buffer,i,om_y) + get_buffer_fieldf(draw_buffer,i,om_x) - get_buffer_fieldf(draw_buffer,i,om_z) + cam_y) / 2), TILE_SIZE,TILE_SIZE};
            SDL_RenderCopy(renderer,get_buffer_fieldv(draw_buffer,i,om_tex),NULL,translate_rect(&r));
        }
        
        SDL_SetRenderDrawColor(renderer,125,125,125,255);
        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);

        deinit_buffer(draw_buffer);

        
    }
    deinit_buffer(objects);
    free(last_keys);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    exit(0);
}

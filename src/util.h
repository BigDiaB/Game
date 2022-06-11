#ifndef UTIL_H
#define UTIL_H

SDL_Event event;
const Uint8* current_keys;
Uint8* last_keys = NULL;
int keysize;
bool running = true;

void quicksort(uint first_index, uint last_index, buffer sort_buffer)
{
    if (first_index < last_index)
    {
        float pivot_x = get_buffer_fieldf(sort_buffer,last_index,dbm_x);
        float pivot_y = get_buffer_fieldf(sort_buffer,last_index,dbm_y);
        float pivot_z = get_buffer_fieldf(sort_buffer,last_index,dbm_z);

        int i = (first_index - 1);
        uint j;
 
        for (j = first_index; j < last_index; j++)
        {
            float point_x = get_buffer_fieldf(sort_buffer,j,dbm_x);
            float point_y = get_buffer_fieldf(sort_buffer,j,dbm_y);
            float point_z = get_buffer_fieldf(sort_buffer,j,dbm_z);

            if (pivot_x + pivot_y > point_x + point_y || (pivot_x < point_x + TILE_SIZE / 2 && point_x < pivot_x + TILE_SIZE / 2 && pivot_y < point_y + TILE_SIZE / 2 && point_y < pivot_y + TILE_SIZE / 2 && pivot_z > point_z))
            {
                i++;
                swap_buffer_at(sort_buffer,i,j);
            }
        }

        swap_buffer_at(sort_buffer,i + 1,last_index);
        quicksort(first_index, i < 0 ? 0 : i,sort_buffer);
        quicksort(i + 2, last_index,sort_buffer);
    }
}

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

    if ((float)win_width / (float)win_height >= 16.0f / 9.0f)
    {
        float stretch = (float)win_width / (float)win_height;
        r->x = map_num(r->x,0,WORLD_SIZE * stretch,0,win_width);
        r->w = map_num(r->w,0,WORLD_SIZE * stretch,0,win_width);
        r->y = map_num(r->y,0,WORLD_SIZE,0,win_height);
        r->h = map_num(r->h,0,WORLD_SIZE,0,win_height);

        r->x += (win_width - win_height / 9 * 16) / 2;
    }
    else
    {
        float stretch = (float)win_height / (float)win_width;
        r->x = map_num(r->x,0,WORLD_SIZE * 16.0f / 9.0f,0,win_width);
        r->w = map_num(r->w,0,WORLD_SIZE * 16.0f / 9.0f,0,win_width);
        r->y = map_num(r->y,0,WORLD_SIZE * stretch * 16.0f / 9.0f,0,win_height);
        r->h = map_num(r->h,0,WORLD_SIZE * stretch * 16.0f / 9.0f,0,win_height);

        r->y += (win_height - win_width / 16 * 9) / 2;
        

    }

    

    return r;
}

void update_SDL()
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
}

void add_to_draw_buffer(float x, float y, float z, uint tex)
{
    resize_buffer(draw_buffer,get_buffer_length(draw_buffer) + 1);

    uint len = get_buffer_length(draw_buffer) - 1;
    set_buffer_fieldf(draw_buffer,len,dbm_x,x);
    set_buffer_fieldf(draw_buffer,len,dbm_y,y);
    set_buffer_fieldf(draw_buffer,len,dbm_z,z);
    set_buffer_fieldui(draw_buffer,len,dbm_tex,tex);
}

void render_stuff(uint tex_idx,SDL_Rect* r)
{
    SDL_RenderCopy(renderer,assets[tex_idx],NULL,r);
}

void render_draw_buffer()
{
	uint i;
	if (get_buffer_length(draw_buffer) > 1)
        quicksort(0,get_buffer_length(draw_buffer) - 1,draw_buffer);

    for (i = 0; i < get_buffer_length(draw_buffer); i++)
    {
        SDL_Rect r;
        /*
        (get_buffer_fieldf(draw_buffer,i,dbm_x) - get_buffer_fieldf(draw_buffer,i,dbm_y) + cam_x) * (TILE_SIZE / 2),
        (get_buffer_fieldf(draw_buffer,i,dbm_x) + get_buffer_fieldf(draw_buffer,i,dbm_y) + can_y) * (TILE_SIZE / 2)*/

        r.x = get_buffer_fieldf(draw_buffer,i,dbm_x) - get_buffer_fieldf(draw_buffer,i,dbm_y) + cam_x;
        r.y = (get_buffer_fieldf(draw_buffer,i,dbm_y) + get_buffer_fieldf(draw_buffer,i,dbm_x) - get_buffer_fieldf(draw_buffer,i,dbm_z) + cam_y) / 2;
        r.w = TILE_SIZE;
        r.h = TILE_SIZE;
        SDL_RenderCopy(renderer,assets[get_buffer_fieldui(draw_buffer,i,dbm_tex)],NULL,translate_rect(&r));
    }

    SDL_SetRenderDrawColor(renderer,125,125,125,255);
    SDL_RenderPresent(renderer);
    SDL_RenderClear(renderer);
    resize_buffer(draw_buffer,0);
}

#endif /* UTIL_H */
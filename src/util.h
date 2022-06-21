#ifndef UTIL_H
#define UTIL_H

SDL_Event event;
const Uint8* current_keys;
Uint8* last_keys = NULL;
int keysize;
bool running = true;

void bubble_sort(buffer sort_buffer)
{
    int step,size = get_buffer_length(sort_buffer);
    for (step = 0; step < size - 1; ++step)
    {
        int i,swapped = 0;
        for (i = 0; i < size - step - 1; ++i)
        {
            int tilesize = TILE_SIZE;
            float step_x = get_buffer_fieldf(sort_buffer,i,dbm_x);
            float step_y = get_buffer_fieldf(sort_buffer,i,dbm_y);
            float step_z = get_buffer_fieldf(sort_buffer,i,dbm_z);

            float curr_x = get_buffer_fieldf(sort_buffer,i + 1,dbm_x);
            float curr_y = get_buffer_fieldf(sort_buffer,i + 1,dbm_y);
            float curr_z = get_buffer_fieldf(sort_buffer,i + 1,dbm_z);

            #define AABB(x1,x2,y1,y2,size) (x1 < x2 + size && x2 < x1 + size && y1 < y2 + size && y2 < y1 + size)

            if ((AABB(step_x,curr_x,step_y,curr_y,tilesize)))
            {
                if (step_z > curr_z)
                {
                    swap_buffer_at(sort_buffer,i,i + 1);
                    swapped = 1;
                }
            }
            else if ((int)step_z % tilesize == 0 && (int)curr_z % tilesize == 0)
            {
                if (step_x + step_y + 2 * step_z > curr_x + curr_y + 2 * curr_z)
                {
                    swap_buffer_at(sort_buffer,i,i + 1);
                    swapped = 1;
                }
            }
            else
            {
                if (step_x + step_y + 0 * step_z > curr_x + curr_y + 0 * curr_z)
                {
                    swap_buffer_at(sort_buffer,i,i + 1);
                    swapped = 1;
                }
            }

            #undef AABB
        }
        if (swapped == 0)
        {
            break;
        }
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


void render_draw_buffer()
{
	uint i;
	if (get_buffer_length(draw_buffer) > 1)
    {
        bubble_sort(draw_buffer);
    }

    for (i = 0; i < get_buffer_length(draw_buffer); i++)
    {
        SDL_Rect r;
        r.x = (get_buffer_fieldf(draw_buffer,i,dbm_x) - get_buffer_fieldf(draw_buffer,i,dbm_y)) / 2 + cam_x;
        r.y = ((get_buffer_fieldf(draw_buffer,i,dbm_y) + get_buffer_fieldf(draw_buffer,i,dbm_x)) / 2 - get_buffer_fieldf(draw_buffer,i,dbm_z) + cam_y) / 2;
        r.w = TILE_SIZE;
        r.h = TILE_SIZE;
        SDL_RenderCopy(renderer,assets[get_buffer_fieldui(draw_buffer,i,dbm_tex)],NULL,translate_rect(&r));
    }
    resize_buffer(draw_buffer,0);
}

#endif /* UTIL_H */
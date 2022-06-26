#ifndef UTIL_H
#define UTIL_H

#define USE_FRECT

SDL_Event event;
const Uint8* current_keys;
Uint8* last_keys = NULL;
int keysize;

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

bool running = true;

double accumulator;
int win_width, win_height;
SDL_Window* window;
SDL_Renderer* renderer;

const int tick_precision = vsync ? 10 : 100;
double ticks[tick_precision] = {0};
uint used_ticks = 0;

void update_actions()
{
    memcpy(last_actions,current_actions,sizeof(bool) * num_actions);
    memset(current_actions,0,sizeof(bool) * num_actions);

    if (down(SDL_SCANCODE_RIGHT))
        current_actions[act_move_right] = true;

    if (down(SDL_SCANCODE_LEFT))
        current_actions[act_move_left] = true;

    if (down(SDL_SCANCODE_DOWN))
        current_actions[act_move_down] = true;

    if (down(SDL_SCANCODE_UP))
        current_actions[act_move_up] = true;

    if (down(SDL_SCANCODE_W))
        current_actions[act_float_up] = true;

    if (down(SDL_SCANCODE_S))
        current_actions[act_float_down] = true;
}

void load_chunk(char* filename)
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
    set_buffer_fieldv(loaded_world,get_buffer_length(loaded_world) - 1,3,malloc(strlen(filename) + 1));
    strcpy(get_buffer_fieldv(loaded_world,get_buffer_length(loaded_world) - 1,3),filename);

    load_buffer_binary(get_buffer_fieldv(loaded_world,get_buffer_length(loaded_world) - 1,2),bin_data + sizeof(uint) * 3,size);

    free(bin_data);
}

void safe_world()
{
    uint i,size,x_off,y_off,num_elements;
    void* curr_buffer,*bin_data;
    char* filename;
    FILE* file;

    for (i = 0; i < get_buffer_length(loaded_world); i++)
    {
        curr_buffer = get_buffer_fieldv(loaded_world,i,2);
        bin_data = dump_buffer_binary(curr_buffer,&size);
        filename = get_buffer_fieldv(loaded_world,i,3);
        num_elements = get_buffer_length(curr_buffer);

        x_off = get_buffer_fieldui(loaded_world,i,0);
        y_off = get_buffer_fieldui(loaded_world,i,1);

        file = fopen(filename,"wb");

        fwrite(&num_elements,1,sizeof(uint),file);
        fwrite(&x_off,1,sizeof(uint),file);
        fwrite(&y_off,1,sizeof(uint),file);
        fwrite(bin_data,1,get_buffer_size(curr_buffer),file);

        fclose(file);

        free(bin_data);
        free(filename);
    }
}

void display_frame_time(double tick_time)
{
    ticks[used_ticks] = tick_time;
    used_ticks++;

    if (used_ticks == tick_precision)
    {   
        uint i;
        double avrg = 0.0f;
        for (i = 0; i < used_ticks; i++)
            avrg += ticks[i];
        avrg /= used_ticks;
        used_ticks = 0;
        char title[128] = {0};
        sprintf(title,"FPS: %fms %f",avrg,1000.0f / avrg);
        SDL_SetWindowTitle(window,title);
    }
}

void remove_invisible(buffer vis_buffer)
{
    while(iterate_over(vis_buffer))
    {
        int cart_x = (get_fieldf(dbm_x) - get_fieldf(dbm_y)) / 2 + cam_x;
        int cart_y = ((get_fieldf(dbm_y) + get_fieldf(dbm_x)) / 2 - get_fieldf(dbm_z)) / 2 + cam_y;
        int tilesize = TILE_SIZE;
        int worldsize = WORLD_SIZE;
        float stretch = ((float)win_width / (float)win_height);
        #define AABB(x1,x2,y1,y2,sx1,sx2,sy1,sy2) (x1 < x2 + sx2 && x2 < x1 + sx1 && y1 < y2 + sy2 && y2 < y1 + sy1)
        if (!(AABB(0,cart_x,0,cart_y,(float)worldsize * stretch,tilesize,worldsize,tilesize)))
        #undef AABB
            remove_at(get_iterator());
    }
}

void depth_sort(buffer sort_buffer)
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
            else if ((AABB(step_x,curr_x,step_z,curr_z,tilesize)))
            {
                if (step_y > curr_y)
                {
                    swap_buffer_at(sort_buffer,i,i + 1);
                    swapped = 1;
                }
            }
            else if ((AABB(step_y,curr_y,step_z,curr_z,tilesize)))
            #undef AABB
            {
                if (step_x > curr_x )
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
            else if (step_x + step_y > curr_x + curr_y)
            {
                swap_buffer_at(sort_buffer,i,i + 1);
                swapped = 1;
            }
            else if (step_x + step_y + step_z * 2 > curr_x + curr_y + curr_z * 2)
            {
                swap_buffer_at(sort_buffer,i,i + 1);
                swapped = 1;
            }
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
#ifdef USE_FRECT
SDL_FRect* translate_rect(SDL_FRect* r)
#else
SDL_Rect* translate_rect(SDL_Rect* r)
#endif
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

void add_to_collider_buffer(int x, int y, int z)
{
    resize_buffer(collider_buffer,get_buffer_length(collider_buffer) + 1);

    uint len = get_buffer_length(collider_buffer) - 1;
    set_buffer_fieldi(collider_buffer,len,cbm_x,x);
    set_buffer_fieldi(collider_buffer,len,cbm_y,y);
    set_buffer_fieldi(collider_buffer,len,cbm_z,z);
}

bool check_collider_buffer(float x, float y, float z)
{
    int tilesize = TILE_SIZE;
    while(iterate_over(collider_buffer))
        if((x < get_fieldi(0) + tilesize && x + tilesize > get_fieldi(0)) &&
           (y < get_fieldi(1) + tilesize && y + tilesize > get_fieldi(1)) &&
           (z < get_fieldi(2) + tilesize && z + tilesize > get_fieldi(2)))
        {
            set_iterator(0);
            return true;
        }
    return false;
}


void render_draw_buffer()
{
	uint i;
	if (get_buffer_length(draw_buffer) > 1)
    {
        remove_invisible(draw_buffer);
        depth_sort(draw_buffer);
    }

    for (i = 0; i < get_buffer_length(draw_buffer); i++)
    {
        #ifdef USE_FRECT
        SDL_FRect r;
        #else
        SDL_Rect r;
        #endif
        r.x = ceil((get_buffer_fieldf(draw_buffer,i,dbm_x) - get_buffer_fieldf(draw_buffer,i,dbm_y)) / 2 + cam_x);
        r.y = ceil(((get_buffer_fieldf(draw_buffer,i,dbm_y) + get_buffer_fieldf(draw_buffer,i,dbm_x)) / 2 - get_buffer_fieldf(draw_buffer,i,dbm_z)) / 2 + cam_y);
        r.w = TILE_SIZE;
        r.h = TILE_SIZE;
        #ifdef USE_FRECT
        SDL_RenderCopyF(renderer,assets[get_buffer_fieldui(draw_buffer,i,dbm_tex)],NULL,translate_rect(&r));
        #else
        SDL_RenderCopy(renderer,assets[get_buffer_fieldui(draw_buffer,i,dbm_tex)],NULL,translate_rect(&r));
        #endif
    }

    resize_buffer(draw_buffer,0);
}

#endif /* UTIL_H */
#ifndef UTIL_H
#define UTIL_H

#define USE_FRECT

SDL_Event event;
const Uint8* current_keys;
Uint8* last_keys = NULL;
int keysize;

bool pressed(unsigned int key)
{
    return !last_keys[key] && current_keys[key];
}

bool released(unsigned int key)
{
    return last_keys[key] && !current_keys[key];
}

bool down(unsigned int key)
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
unsigned int used_ticks = 0;

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
    unsigned int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    void* bin_data = malloc(size);
    fread(bin_data,1,size,file);

    push_type(INT);
    push_type(INT);
    push_type(INT);
    push_type(UINT);

    unsigned int num_elements = *((unsigned int*)bin_data);
    set_buffer_fieldui(loaded_world,get_buffer_length(loaded_world) - 1,ldm_xoff,*((unsigned int*)(bin_data + sizeof(unsigned int))));
    set_buffer_fieldui(loaded_world,get_buffer_length(loaded_world) - 1,ldm_yoff,*((unsigned int*)(bin_data + sizeof(unsigned int) * 2)));
    set_buffer_fieldui(loaded_world,get_buffer_length(loaded_world) - 1,ldm_drawflag,0);
    set_buffer_fieldv(loaded_world,get_buffer_length(loaded_world) - 1,ldm_cubes,init_buffer(num_elements));
    set_buffer_fieldv(loaded_world,get_buffer_length(loaded_world) - 1,ldm_filename,malloc(strlen(filename) + 1));
    strcpy(get_buffer_fieldv(loaded_world,get_buffer_length(loaded_world) - 1,ldm_filename),filename);

    load_buffer_binary(get_buffer_fieldv(loaded_world,get_buffer_length(loaded_world) - 1,ldm_cubes),bin_data + sizeof(unsigned int) * 3,size);

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
        curr_buffer = get_buffer_fieldv(loaded_world,i,ldm_cubes);
        bin_data = dump_buffer_binary(curr_buffer,&size);
        filename = get_buffer_fieldv(loaded_world,i,ldm_filename);
        num_elements = get_buffer_length(curr_buffer);

        x_off = get_buffer_fieldui(loaded_world,i,ldm_xoff);
        y_off = get_buffer_fieldui(loaded_world,i,ldm_yoff);

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

float map_num(float num, float min1, float max1, float min2, float max2)
{
    return (num - min1) * (max2 - min2) / (max1 - min1) + min2;
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

void remove_invisible(buffer vis_buffer, unsigned int xoff, unsigned int yoff)
{
    while(iterate_over(vis_buffer))
    {
        int iso_x = get_fieldi(cm_x) + 10 * TILE_SIZE * xoff;
        int iso_y = get_fieldi(cm_y) + 10 * TILE_SIZE * yoff;
        int iso_z = get_fieldi(cm_z);

        int cart_x = (iso_x - iso_y) / 2 + cam_x;
        int cart_y = ((iso_y + iso_x) / 2 - iso_z) / 2 + cam_y;
        int tilesize = TILE_SIZE;
        int worldsize = WORLD_SIZE;


        if ((float)win_width / (float)win_height >= 16.0f / 9.0f)
        {
            float stretch = (float)win_width / (float)win_height;
            #define AABB(x1,x2,y1,y2,sx1,sx2,sy1,sy2) (x1 < x2 + sx2 && x2 < x1 + sx1 && y1 < y2 + sy2 && y2 < y1 + sy1)
            if (!(AABB(0,cart_x,0,cart_y,(float)worldsize * stretch,tilesize,worldsize,tilesize)))
            #undef AABB
            {
                remove_at(get_iterator());
                set_iterator(get_iterator());
                
            }
        }
        else
        {
            float stretch = (float)win_height / (float)win_width;
            #define AABB(x1,x2,y1,y2,sx1,sx2,sy1,sy2) (x1 < x2 + sx2 && x2 < x1 + sx1 && y1 < y2 + sy2 && y2 < y1 + sy1)
            if (!(AABB(0,cart_x,0,cart_y,worldsize,tilesize,(float)worldsize * stretch,tilesize)))
            #undef AABB
            {
                remove_at(get_iterator());
                set_iterator(get_iterator());
            }
        }
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
            int step_x = get_buffer_fieldi(sort_buffer,i,cm_x);
            int step_y = get_buffer_fieldi(sort_buffer,i,cm_y);
            int step_z = get_buffer_fieldi(sort_buffer,i,cm_z);

            int curr_x = get_buffer_fieldi(sort_buffer,i + 1,cm_x);
            int curr_y = get_buffer_fieldi(sort_buffer,i + 1,cm_y);
            int curr_z = get_buffer_fieldi(sort_buffer,i + 1,cm_z);

            
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
                if (step_x > curr_x)
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

double tick()
{
    static struct timeval t1;
    struct timeval t2;
    gettimeofday(&t2, NULL);
    double elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
    gettimeofday(&t1, NULL);
    return elapsedTime;
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

void render_draw_buffer()
{
    unsigned int i, num_chunks = get_buffer_length(loaded_world);
    unsigned int offsets[num_chunks][2];
    unsigned int indices[num_chunks];

    for (i = 0; i < num_chunks; i++)
    {
        offsets[i][0] = get_buffer_fieldui(loaded_world,i,ldm_xoff);
        offsets[i][1] = get_buffer_fieldui(loaded_world,i,ldm_yoff);
        indices[i] = i;
    }

    unsigned int appendages[get_buffer_length(entity_buffer)];
    memset(appendages,0,get_buffer_length(entity_buffer) * sizeof(unsigned int));

    int merges[num_chunks];
    memset(merges,-1,num_chunks * sizeof(int));

    for (i = 0; i < num_chunks; i++)
    {
        unsigned xoff = get_buffer_fieldui(loaded_world,indices[i],ldm_xoff);
        unsigned yoff = get_buffer_fieldui(loaded_world,indices[i],ldm_yoff);
        set_buffer_fieldv(loaded_world,indices[i],ldm_entities,copy_buffer(get_buffer_fieldv(loaded_world,indices[i],ldm_cubes)));
        buffer cubes = get_buffer_fieldv(loaded_world,indices[i],ldm_entities);

        while(iterate_over(entity_buffer))
        {
            #define AABB(x1,x2,y1,y2,sx1,sx2,sy1,sy2) (x1 < x2 + sx2 && x2 < x1 + sx1 && y1 < y2 + sy2 && y2 < y1 + sy1)
            if (AABB(get_fieldf(ebm_x),xoff * 10 * TILE_SIZE,
                     get_fieldf(ebm_y),yoff * 10 * TILE_SIZE,
                     TILE_SIZE,10 * TILE_SIZE,
                     TILE_SIZE,10 * TILE_SIZE))
            {
                appendages[get_iterator()]++;
                if (appendages[get_iterator()] > 1)
                {
                    unsigned int j;
                    for (j = 0; j < get_buffer_length(loaded_world); j++)
                    {
                        if (j == i)
                            continue;
                        if (AABB(get_fieldf(ebm_x),(get_buffer_fieldui(loaded_world,indices[j],ldm_xoff)) * 10 * TILE_SIZE,
                                 get_fieldf(ebm_y),(get_buffer_fieldui(loaded_world,indices[j],ldm_yoff)) * 10 * TILE_SIZE,
                                 TILE_SIZE,10 * TILE_SIZE,
                                 TILE_SIZE,10 * TILE_SIZE))
                        {
                            if (get_buffer_fieldv(loaded_world,indices[j],ldm_entities) != NULL && get_buffer_fieldv(loaded_world,indices[i],ldm_entities) != NULL)
                            {
                                unsigned int k;
                                buffer i_cubes = get_buffer_fieldv(loaded_world,indices[i],ldm_entities);
                                buffer j_cubes = get_buffer_fieldv(loaded_world,indices[j],ldm_entities);

                                if (get_buffer_size(j_cubes) > get_buffer_size(i_cubes))
                                {
                                    int x_off = xoff - (int)get_buffer_fieldui(loaded_world,indices[j],ldm_xoff);
                                    int y_off = yoff - (int)get_buffer_fieldui(loaded_world,indices[j],ldm_yoff);

                                    for (k = 0; k < get_buffer_length(i_cubes); k++)
                                    {
                                        set_buffer_fieldi(i_cubes,k,cm_x,get_buffer_fieldi(i_cubes,k,cm_x) + x_off * TILE_SIZE * 10);
                                        set_buffer_fieldi(i_cubes,k,cm_y,get_buffer_fieldi(i_cubes,k,cm_y) + y_off * TILE_SIZE * 10);
                                    }

                                    append_buffer_at(i_cubes,j_cubes);
                                }
                                else
                                {  
                                    for (k = 0; k < get_buffer_length(i_cubes); k++)
                                    {
                                        set_buffer_fieldi(i_cubes,k,cm_x,get_buffer_fieldi(i_cubes,k,cm_x) + xoff * TILE_SIZE * 10);
                                        set_buffer_fieldi(i_cubes,k,cm_y,get_buffer_fieldi(i_cubes,k,cm_y) + yoff * TILE_SIZE * 10);
                                    }

                                    append_buffer_at(j_cubes,i_cubes);

                                    set_buffer_fieldv(loaded_world,indices[i],ldm_entities,j_cubes);
                                    set_buffer_fieldv(loaded_world,indices[j],ldm_entities,i_cubes);
                                }

                                deinit_buffer(get_buffer_fieldv(loaded_world,indices[i],ldm_entities));
                                set_buffer_fieldv(loaded_world,indices[i],ldm_entities,NULL);

                                merges[indices[j]] = indices[i];
                            }
                        }
                    }
                }
                #undef AABB
                else
                {
                    buffer appendage = create_single_buffer_element(cubes);
                    set_buffer_fieldi(appendage,0,cm_x,get_fieldf(ebm_x) - (xoff) * 10 * TILE_SIZE);
                    set_buffer_fieldi(appendage,0,cm_y,get_fieldf(ebm_y) - (yoff) * 10 * TILE_SIZE);
                    set_buffer_fieldi(appendage,0,cm_z,get_fieldf(ebm_z));
                    set_buffer_fieldui(appendage,0,cm_tex,get_fieldui(ebm_type));
                    append_buffer_element_at(appendage,0,cubes);

                    deinit_buffer(appendage);
                }
            }
        }
    }

    unsigned int step;

    for (step = 0; step < num_chunks - 1; ++step)
    {
        unsigned int i,swapped = 0;
        for (i = 0; i < num_chunks - step - 1; ++i)
        {
            unsigned int this_x = offsets[indices[i]][0];
            unsigned int this_y = offsets[indices[i]][1];
            unsigned int this_xmax = offsets[indices[i]][0] + 1;
            unsigned int this_ymax = offsets[indices[i]][1] + 1;

            unsigned int other_x = offsets[indices[i+1]][0];
            unsigned int other_y = offsets[indices[i+1]][1];
            unsigned int other_xmax = offsets[indices[i+1]][0] + 1;
            unsigned int other_ymax = offsets[indices[i+1]][1] + 1;

            if (merges[indices[i]] != -1)
            {
                this_xmax += offsets[merges[indices[i]]][0] - this_x;
                this_ymax += offsets[merges[indices[i]]][1] - this_y;
            }

            if (((this_xmax <= other_x) && (other_ymax <= this_y)) || ((other_xmax <= this_x) && (this_ymax <= other_y)))
            {

            }
            else if ((this_xmax <= other_x) || (this_ymax <= other_y))
            {
                
            }
            else if ((other_xmax <= this_x) || (other_ymax <= this_y))
            {
                unsigned int temp = indices[i];
                indices[i] = indices[i+1];
                indices[i+1] = temp;
            }
        }

        if (swapped == 0)
        {
            break;
        }
    }

    for (i = 0; i < num_chunks; i++)
    {
        if (get_buffer_fieldui(loaded_world,indices[i],ldm_drawflag) || true)
        {
            unsigned xoff = get_buffer_fieldui(loaded_world,indices[i],ldm_xoff);
            unsigned yoff = get_buffer_fieldui(loaded_world,indices[i],ldm_yoff);

            buffer cubes = get_buffer_fieldv(loaded_world,indices[i],ldm_entities);

            if (cubes == NULL)
                continue;

            depth_sort(cubes);

            while(iterate_over(cubes))
            {
                #ifdef USE_FRECT
                SDL_FRect r;
                #else
                SDL_Rect r;
                #endif

                int iso_x = get_fieldi(cm_x) + 10 * TILE_SIZE * xoff;
                int iso_y = get_fieldi(cm_y) + 10 * TILE_SIZE * yoff;
                int iso_z = get_fieldi(cm_z);

                r.x = (iso_x - iso_y) / 2 + cam_x;
                r.y = ((iso_y + iso_x) / 2 - iso_z) / 2 + cam_y;
                r.w = TILE_SIZE;
                r.h = TILE_SIZE;

                #ifdef USE_FRECT
                SDL_RenderCopyF(renderer,assets[get_fieldui(cm_tex)],NULL,translate_rect(&r));
                #else
                SDL_RenderCopy(renderer,assets[get_fieldui(cm_tex)],NULL,translate_rect(&r));
                #endif
            }

            deinit_buffer(get_buffer_fieldv(loaded_world,indices[i],ldm_entities));
        }
    }
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
    {
        if((x < get_fieldi(0) + tilesize && x + tilesize > get_fieldi(0)) &&
           (y < get_fieldi(1) + tilesize && y + tilesize > get_fieldi(1)) &&
           (z < get_fieldi(2) + tilesize && z + tilesize > get_fieldi(2)))
        {
            set_iterator(0);
            return true;
        }
    }
    return false;
}


#endif /* UTIL_H */
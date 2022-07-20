#ifndef UTIL_H
#define UTIL_H

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

const int tick_precision = ENABLE_VSYNC ? 10 : 100;
double ticks[tick_precision] = {0};
unsigned int used_ticks = 0;

int cam_x = 0, cam_y = 0;

void cam_follow(int x, int y)
{
    if (ENABLE_SMOOTH_CAM)
    {
        cam_x -= (cam_x - (WORLD_SIZE * 0.888f - x * WORLD_ZOOM / 2)) * CAM_SMOOTH;
        cam_y -= (cam_y - (WORLD_SIZE / 2 - y * WORLD_ZOOM / 2)) * CAM_SMOOTH;
    }
    else
    {
        cam_x = WORLD_SIZE * 0.888f - x * WORLD_ZOOM / 2;
        cam_y = WORLD_SIZE / 2 - y * WORLD_ZOOM / 2;
    }
}

float map_num(float num, float min1, float max1, float min2, float max2)
{
    return (num - min1) * (max2 - min2) / (max1 - min1) + min2;
}

struct rect
{
    float x,y;
    unsigned int w,h;
};

typedef struct rect rect;

void change_draw_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    SDL_SetRenderDrawColor(renderer,r,g,b,a);
}

enum align_type
{
    align_none, align_right, align_bottom, align_center, align_corner
};

void align_ui_rect(rect* r,enum align_type align)
{
    if ((float)win_width / (float)win_height >= 16.0f / 9.0f)
    {
        float stretch = (float)win_width / (float)win_height;

        switch(align)
        {
            case align_right:
            r->x = WORLD_SIZE * stretch - r->x - r->w;
            break;
            case align_bottom:
            r->y = WORLD_SIZE - r->y - r->h;
            break;
            case align_center:
            r->x = WORLD_SIZE / 2 * stretch - r->x - r->w / 2;
            r->y = WORLD_SIZE / 2 - r->y - r->h / 2;
            break;
            case align_corner:
            r->x = WORLD_SIZE * stretch - r->x - r->w;
            r->y = WORLD_SIZE - r->y - r->h;
            break;
            case align_none:
            default:
            break;
        }
    }
    else
    {
        float stretch = (float)win_height / (float)win_width;

        switch(align)
        {
            case align_right:
            r->x = WORLD_SIZE * 16.0f / 9.0f - r->x - r->w;
            break;
            case align_bottom:
            r->y = WORLD_SIZE * stretch * 16.0f / 9.0f - r->y - r->h;
            break;
            case align_center:
            r->x = WORLD_SIZE / 2 * 16.0f / 9.0f - r->x - r->w / 2;
            r->y = WORLD_SIZE / 2 * stretch * 16.0f / 9.0f - r->y - r->h / 2;
            break;
            case align_corner:
            r->x = WORLD_SIZE * 16.0f / 9.0f - r->x - r->w;
            r->y = WORLD_SIZE * stretch * 16.0f / 9.0f - r->y - r->h;
            break;
            case align_none:
            default:
            break;
        }
    }
}

rect* translate_rect(rect* r)
{

    if ((float)win_width / (float)win_height >= 16.0f / 9.0f)
    {
        const float stretch = (float)win_width / (float)win_height;
        r->x = map_num(r->x,0,WORLD_SIZE * stretch,0,win_width);
        r->w = map_num(r->w,0,WORLD_SIZE * stretch,0,win_width);
        r->y = map_num(r->y,0,WORLD_SIZE,0,win_height);
        r->h = map_num(r->h,0,WORLD_SIZE,0,win_height);

        r->x += (win_width - win_height / 9 * 16) / 2;
    }
    else
    {
        const float stretch = (float)win_height / (float)win_width;
        r->x = map_num(r->x,0,WORLD_SIZE * 16.0f / 9.0f,0,win_width);
        r->w = map_num(r->w,0,WORLD_SIZE * 16.0f / 9.0f,0,win_width);
        r->y = map_num(r->y,0,WORLD_SIZE * stretch * 16.0f / 9.0f,0,win_height);
        r->h = map_num(r->h,0,WORLD_SIZE * stretch * 16.0f / 9.0f,0,win_height);

        r->y += (win_height - win_width / 16 * 9) / 2;
    }

    return r;
}

rect* translate_rect_ui(rect* r)
{

    if ((float)win_width / (float)win_height >= 16.0f / 9.0f)
    {
        const float stretch = (float)win_width / (float)win_height;
        r->x = map_num(r->x,0,WORLD_SIZE * stretch,0,win_width);
        r->w = map_num(r->w,0,WORLD_SIZE * stretch,0,win_width);
        r->y = map_num(r->y,0,WORLD_SIZE,0,win_height);
        r->h = map_num(r->h,0,WORLD_SIZE,0,win_height);
    }
    else
    {
        const float stretch = (float)win_height / (float)win_width;
        r->x = map_num(r->x,0,WORLD_SIZE * 16.0f / 9.0f,0,win_width);
        r->w = map_num(r->w,0,WORLD_SIZE * 16.0f / 9.0f,0,win_width);
        r->y = map_num(r->y,0,WORLD_SIZE * stretch * 16.0f / 9.0f,0,win_height);
        r->h = map_num(r->h,0,WORLD_SIZE * stretch * 16.0f / 9.0f,0,win_height);
    }

    return r;
}

void render_rect(rect* r, bool fill, bool ui, enum align_type align)
{
    if (ui)
    {
        align_ui_rect(r,align);
        translate_rect_ui(r);
    }
    else
    {
        translate_rect(r);
    }

    SDL_FRect rr = {r->x,r->y,r->w,r->h};

    if (fill)
        SDL_RenderFillRectF(renderer,&rr);
    else
        SDL_RenderDrawRectF(renderer,&rr);
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

rect get_screen_rect()
{
    rect screen = {0,0,0,0};
    if ((float)win_width / (float)win_height >= 16.0f / 9.0f)
    {
        const float stretch = (float)win_width / (float)win_height;
        screen.w = WORLD_SIZE * stretch;
        screen.h = WORLD_SIZE;
    }
    else
    {
        const float stretch = (float)win_height / (float)win_width;
        screen.w = WORLD_SIZE * 16.0f / 9.0f;
        screen.h = WORLD_SIZE * stretch * 16.0f / 9.0f;
    }

    return screen;
}

rect get_world_rect()
{
    rect screen = {0,0,WORLD_SIZE * 16.0f / 9.0f,WORLD_SIZE};

    if ((float)win_width / (float)win_height >= 16.0f / 9.0f)
    {
        const float stretch = (float)win_width / (float)win_height;
        screen.x = (WORLD_SIZE * stretch - WORLD_SIZE / 9 * 16) / 2;
    }
    else
    {
        const float stretch = (float)win_height / (float)win_width;
        screen.y = (WORLD_SIZE * stretch * 16.0f / 9.0f - WORLD_SIZE) / 2;
    }

    return screen;
}

void draw_debug_boundaries()
{
    rect world = get_world_rect();
    rect screen = get_screen_rect();

    change_draw_color(255,0,0,255);
    render_rect(&screen,false,true,align_none);

    change_draw_color(0,255,0,255);
    render_rect(&world,false,true,align_none);

}

bool gameloop();

int dynamic_screen_resize(__attribute__((unused))void *userdata, SDL_Event * event)
{
    if (event->type == SDL_WINDOWEVENT)
    {
        if (event->window.event == SDL_WINDOWEVENT_EXPOSED)
        {
            SDL_GetWindowSize(window,&win_width,&win_height);
            gameloop();
            return 0;
        }
    }
    return 1;
}


#endif /* UTIL_H */
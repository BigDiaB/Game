void add_to_draw_buffer(float x, float y, float z, unsigned int cx, unsigned int cy, unsigned int ent,unsigned int tex)
{
    resize_buffer(draw_buffer,get_buffer_length(draw_buffer) + 1);

    uint len = get_buffer_length(draw_buffer) - 1;
    set_buffer_fieldf(draw_buffer,len,dbm_x,x);
    set_buffer_fieldf(draw_buffer,len,dbm_y,y);
    set_buffer_fieldf(draw_buffer,len,dbm_z,z);
    set_buffer_fieldui(draw_buffer,len,dbm_tex,tex);

    set_buffer_fieldui(draw_buffer,len,dbm_cx,cx);
    set_buffer_fieldui(draw_buffer,len,dbm_cy,cy);

    set_buffer_fieldui(draw_buffer,len,dbm_ent,ent);
}

void render_draw_buffer()
{
    unsigned int i;
    for (i = 0; i < get_buffer_length(loaded_world); i++)
    {
        while(iterate_over(get_buffer_fieldv(loaded_world,i,2)))
        {
            add_to_draw_buffer((float)((int)get_fieldui(0) + (int)get_buffer_fieldui(loaded_world,i,0) * CHUNK_SIZE) * TILE_SIZE,(float)((int)get_fieldui(1) + (int)get_buffer_fieldui(loaded_world,i,1) * CHUNK_SIZE) * TILE_SIZE,get_fieldui(2) * TILE_SIZE,get_buffer_fieldui(loaded_world,i,0),get_buffer_fieldui(loaded_world,i,1),dbe_world,get_fieldui(3));

            add_to_collider_buffer((float)((int)get_fieldui(0) + (int)get_buffer_fieldui(loaded_world,i,0) * CHUNK_SIZE) * TILE_SIZE,(float)((int)get_fieldui(1) + (int)get_buffer_fieldui(loaded_world,i,1) * CHUNK_SIZE) * TILE_SIZE,get_fieldui(2) * TILE_SIZE);
        }
    }

    while(iterate_over(entity_buffer))
    {
        switch(get_fieldui(ebm_type))
        {
            case et_player:
            {
                add_to_draw_buffer(get_fieldf(0),get_fieldf(1),get_fieldf(2),0,0,dbe_entity,am_selected);
            }
            break;
            default:
            {
                add_to_draw_buffer(get_fieldf(0),get_fieldf(1),get_fieldf(2),0,0,dbe_entity,am_debug);
            }
            break;
        }
    }

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

